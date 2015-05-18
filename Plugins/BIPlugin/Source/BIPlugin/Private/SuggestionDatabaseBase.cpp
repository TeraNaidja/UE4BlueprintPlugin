#include "BIPluginPrivatePCH.h"
#include "SuggestionDatabaseBase.h"

namespace
{
	struct FoldNodeEntry
	{
		FoldNodeEntry(UEdGraph* a_Graph, UK2Node* a_Node)
			: m_Graph(a_Graph)
			, m_Node(a_Node)
		{
		}

		UEdGraph* m_Graph;
		UK2Node* m_Node;
	};

	void ShuffleNodeArray(TArray<FoldNodeEntry>& a_List)
	{
		struct RandomizeSort
		{
			inline bool operator () (const FoldNodeEntry& lhs, const FoldNodeEntry& rhs) const
			{
				return FMath::Rand() < (RAND_MAX / 2);
			}
		};

		a_List.Sort(RandomizeSort());
	}

	TArray<TArray<FoldNodeEntry>> SplitAvailableNodesInKFolds(int32 a_NumFolds)
	{
		TArray<FoldNodeEntry> availableNodes;
		for (TObjectIterator<UBlueprint> blueprintIt; blueprintIt; ++blueprintIt)
		{
			UBlueprint* blueprint = *blueprintIt;
			TArray<UEdGraph*> graphs;
			blueprint->GetAllGraphs(graphs);
			for (UEdGraph* graph : graphs)
			{
				TArray<UK2Node*> nodesInGraph;
				graph->GetNodesOfClass<UK2Node>(nodesInGraph);
				for (UK2Node* node : nodesInGraph)
				{
					availableNodes.Push(FoldNodeEntry(graph, node));
				}
			}
		}

		FMath::RandInit(75623457);
		ShuffleNodeArray(availableNodes);

		UE_LOG(BILog, Log, TEXT("Splitting up %i nodes in %i folds"), availableNodes.Num(), a_NumFolds);

		TArray<TArray<FoldNodeEntry>> result;
		int32 numNodesPerFold = FMath::CeilToInt(static_cast<float>(availableNodes.Num()) / static_cast<float>(a_NumFolds));

		for (int32 i = 0; i < a_NumFolds; ++i)
		{
			TArray<FoldNodeEntry> foldList;
			foldList.Reserve(numNodesPerFold);
			for (int32 j = 0; j < numNodesPerFold; ++j)
			{
				if (availableNodes.Num() > 0)
				{
					foldList.Push(availableNodes.Pop(false));
				}
			}
			result.Push(foldList);
		}
		return result;
	}

	void LogValidationResult(const SuggestionDatabaseBase::CrossValidateResult& a_Result)
	{
		UE_LOG(BILog, Warning, TEXT("Performed %i tests, %i passed precision (%f%), Took %.2f ms (Min: %.2f ms Max: %.2f ms), Rank Percentages: (%.2f %.2f %.2f %.2f %.2f)"),
			a_Result.m_TestsPerformed,
			a_Result.m_PassedPrecision,
			static_cast<float>(a_Result.m_PassedPrecision) / static_cast<float>(a_Result.m_TestsPerformed),
			FPlatformTime::ToMilliseconds(a_Result.m_CyclesTaken),
			FPlatformTime::ToMilliseconds(a_Result.m_MinCyclesTaken),
			FPlatformTime::ToMilliseconds(a_Result.m_MaxCyclesTaken),
			static_cast<float>(a_Result.m_PassedPrecisionEntryRank[0]) / static_cast<float>(a_Result.m_PassedPrecision),
			static_cast<float>(a_Result.m_PassedPrecisionEntryRank[1]) / static_cast<float>(a_Result.m_PassedPrecision),
			static_cast<float>(a_Result.m_PassedPrecisionEntryRank[2]) / static_cast<float>(a_Result.m_PassedPrecision),
			static_cast<float>(a_Result.m_PassedPrecisionEntryRank[3]) / static_cast<float>(a_Result.m_PassedPrecision),
			static_cast<float>(a_Result.m_PassedPrecisionEntryRank[4]) / static_cast<float>(a_Result.m_PassedPrecision)
			);
	}
}

SuggestionDatabaseBase::SuggestionDatabaseBase()
	: m_GraphNodeDatabase(nullptr)
{
}

SuggestionDatabaseBase::~SuggestionDatabaseBase()
{
}

void SuggestionDatabaseBase::FillSuggestionDatabase()
{
	for (TObjectIterator<UBlueprint> BlueprintIt; BlueprintIt; ++BlueprintIt)
	{
		UBlueprint* Blueprint = *BlueprintIt;
		ParseBlueprint(*Blueprint);
	}
}

void SuggestionDatabaseBase::PerformKFoldCrossValidationTest(int32 a_Folds)
{
	UE_LOG(BILog, BI_VERBOSE, TEXT("Performing %i fold cross validation"), a_Folds);
	TArray<TArray<FoldNodeEntry>> folds = SplitAvailableNodesInKFolds(a_Folds);

	const uint32 startCycles = FPlatformTime::Cycles();

	CrossValidateResult* mergedResultPerFold = new CrossValidateResult[a_Folds];
	CrossValidateResult mergedAllResults;

	for (int32 validationPass = 0; validationPass < a_Folds; ++validationPass)
	{
		UE_LOG(BILog, BI_VERBOSE, TEXT("Starting pass %i of cross validation"), validationPass);
		FlushDatabase();

		TArray<FoldNodeEntry>& testFold = folds[validationPass];
		for (int32 i = 0; i < a_Folds; ++i)
		{
			TArray<FoldNodeEntry>& trainingFold = folds[i];
			if (&trainingFold != &testFold)
			{
				for (FoldNodeEntry trainingNode : trainingFold)
				{
					ParseNode(*(trainingNode.m_Node), EPathDirection::Forward);
					ParseNode(*(trainingNode.m_Node), EPathDirection::Backward);
				}
			}
		}

		//Test training data.
		for (FoldNodeEntry testNode : testFold)
		{
			CrossValidateResult result = CrossValidateTest(*(testNode.m_Graph), *(testNode.m_Node), KFOLD_NUM_SUGGESTIONS);
			mergedResultPerFold[validationPass].m_TestsPerformed += result.m_TestsPerformed;
			mergedResultPerFold[validationPass].m_PassedPrecision += result.m_PassedPrecision;
			mergedResultPerFold[validationPass].m_CyclesTaken += result.m_CyclesTaken;
			mergedResultPerFold[validationPass].m_MaxCyclesTaken = FMath::Max(mergedResultPerFold[validationPass].
				m_MaxCyclesTaken, result.m_MaxCyclesTaken);
			mergedResultPerFold[validationPass].m_MinCyclesTaken = FMath::Min(mergedResultPerFold[validationPass].
				m_MinCyclesTaken, result.m_MinCyclesTaken);
			for (int32 i = 0; i < KFOLD_NUM_SUGGESTIONS; ++i)
			{
				mergedResultPerFold[validationPass].m_PassedPrecisionEntryRank[i] += result.m_PassedPrecisionEntryRank[i];
			}
		}

		mergedAllResults.m_TestsPerformed+= mergedResultPerFold[validationPass].m_TestsPerformed;
		mergedAllResults.m_PassedPrecision += mergedResultPerFold[validationPass].m_PassedPrecision;
		mergedAllResults.m_CyclesTaken += mergedResultPerFold[validationPass].m_CyclesTaken;
		mergedAllResults.m_MaxCyclesTaken = FMath::Max(mergedAllResults.m_MaxCyclesTaken, 
			mergedResultPerFold[validationPass].m_MaxCyclesTaken);
		mergedAllResults.m_MinCyclesTaken = FMath::Min(mergedAllResults.m_MinCyclesTaken, 
			mergedResultPerFold[validationPass].m_MinCyclesTaken);
		for (int32 i = 0; i < KFOLD_NUM_SUGGESTIONS; ++i)
		{
			mergedAllResults.m_PassedPrecisionEntryRank[i] += mergedResultPerFold[validationPass].m_PassedPrecisionEntryRank[i];
		}
	}
	const uint32 endCycles = FPlatformTime::Cycles();

	LogValidationResult(mergedAllResults);
	for (int i = 0; i < a_Folds; ++i)
	{
		LogValidationResult(mergedResultPerFold[i]);
	}

	const float timeSpentForSuggestions = FPlatformTime::ToMilliseconds(mergedAllResults.m_CyclesTaken);
	UE_LOG(BILog, Warning, TEXT("Took %.2f ms of which %.2f on generating suggestions (avg %.2f ms per query)"), 
		FPlatformTime::ToMilliseconds(endCycles - startCycles), timeSpentForSuggestions, timeSpentForSuggestions / 
		static_cast<float>(mergedAllResults.m_TestsPerformed));
}

void SuggestionDatabaseBase::SetGraphNodeDatabase(GraphNodeInformationDatabase* a_Database)
{
	m_GraphNodeDatabase = a_Database;
}

void SuggestionDatabaseBase::ParseBlueprint(const UBlueprint& a_Blueprint)
{
	//const FString onlyParsingBlueprint("SideScrollerExampleMap");
	//FString blueprintName;
	//a_Blueprint.GetName(blueprintName);
	//if (blueprintName != onlyParsingBlueprint)
	//	return;
	//UE_LOG(LogTemp, Warning, TEXT("This is actually a lie. We are only parsing %s"), *onlyParsingBlueprint);

	TArray<UEdGraph*> graphsInBlueprint;
	a_Blueprint.GetAllGraphs(graphsInBlueprint);
	for (auto graph : graphsInBlueprint)
	{
		ParseGraph(*graph);
	}
}

void SuggestionDatabaseBase::ParseGraph(const UEdGraph& a_Graph)
{
	TArray<UK2Node*> nodes;
	a_Graph.GetNodesOfClass(nodes);
	for (UK2Node* node : nodes)
	{
		ParseNode(*node, EPathDirection::Forward);
		ParseNode(*node, EPathDirection::Backward);
	}
}

const GraphNodeInformationDatabase& SuggestionDatabaseBase::GetGraphNodeDatabase() const
{
	verify(m_GraphNodeDatabase != nullptr);
	return *m_GraphNodeDatabase;
}

GraphNodeInformationDatabase& SuggestionDatabaseBase::GetGraphNodeDatabase()
{
	return const_cast<GraphNodeInformationDatabase&>((const_cast<const SuggestionDatabaseBase*>(this))->GetGraphNodeDatabase());
}
