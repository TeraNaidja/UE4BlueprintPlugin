#include "BIPluginPrivatePCH.h"
#include "SuggestionDatabasePath.h"

#include "BlueprintSuggestion.h"
#include "BlueprintSuggestionContext.h"
#include "GraphNodeInformationDatabase.h"
#include "GraphNodeInformation.h"

#include "StackTimer.h"

namespace
{
	EEdGraphPinDirection ToPinDirection(EPathDirection a_PathDirection)
	{
		switch (a_PathDirection)
		{
		case EPathDirection::Forward:
			return EEdGraphPinDirection::EGPD_Input;
		case EPathDirection::Backward:
			return EEdGraphPinDirection::EGPD_Output;
		default:
			UE_LOG(BILog, Fatal, TEXT("Unknown EPathDirection while translating to pin direction in ToPinDirection"));
			return EEdGraphPinDirection::EGPD_Input;
		}
	}

	UK2Node* GetNodeFromPin(const UEdGraphPin* a_Pin)
	{
		check(a_Pin->GetOuter()->IsA(UK2Node::StaticClass()));
		return Cast<UK2Node>(a_Pin->GetOuter());
	}

	TArray<UK2Node*> FindNodesInDirection(const UK2Node& a_Node, EPathDirection a_ExploreDirection)
	{
		TArray<UK2Node*> result;
		for (auto childPin : a_Node.Pins)
		{
			if (childPin->Direction == ToPinDirection(a_ExploreDirection) && !childPin->bHidden && 
				!childPin->bNotConnectable)
			{
				for (auto linkedPin : childPin->LinkedTo)
				{
					UK2Node* parentNode = GetNodeFromPin(linkedPin);
					result.Push(parentNode);
				}
			}
		}
		return result;
	}

	void FindPathForNodeRecursive(const UK2Node& a_CurrentNode, EPathDirection a_ExploreDirection, PathPredictionEntry& a_ParentPath, TArray<PathPredictionEntry>& a_Results, int32 a_Depth)
	{
		if (a_Depth < PathContextPath::MAX_CONTEXT_PATH_LENGTH)
		{
			TArray<UK2Node*> children = FindNodesInDirection(a_CurrentNode, a_ExploreDirection);
			for (UK2Node* child : children)
			{
				PathPredictionEntry thisPath = PathPredictionEntry(a_ParentPath);
				thisPath.m_ContextPath.PushNode(PathNodeEntry(*child));
				a_Results.Push(thisPath);
				FindPathForNodeRecursive(*child, a_ExploreDirection, thisPath, a_Results, a_Depth + 1);
			}
		}
	}

	TArray<PathPredictionEntry> CreatePredictionPathsForNode(const UK2Node& a_Node, EPathDirection a_ExploreDirection)
	{
		TArray<PathPredictionEntry> result;
		PathPredictionEntry initialNode;
		initialNode.m_Direction = a_ExploreDirection;
		initialNode.m_PredictionVertex = PathNodeEntry(a_Node);

		for (const auto node : FindNodesInDirection(a_Node, a_ExploreDirection))
		{
			PathPredictionEntry pathEntry = PathPredictionEntry(initialNode);
			pathEntry.m_AnchorVertex = PathNodeEntry(*node);
			pathEntry.m_NumUses = 1;
			result.Push(pathEntry);
			FindPathForNodeRecursive(*node, a_ExploreDirection, pathEntry, result, 0);
		}

		return result;
	}

	void FindAllContextPathsRecursive(const UK2Node& a_Node, EPathDirection a_ExploreDirection, int32 a_Depth, 
		PathContextPath& a_CurrentPath, TArray<PathContextPath>& a_Results)
	{
		if (a_Depth < PathContextPath::MAX_CONTEXT_PATH_LENGTH)
		{
			TArray<UK2Node*> children = FindNodesInDirection(a_Node, a_ExploreDirection);
			if (children.Num() > 0)
			{
				for (UK2Node* child : children)
				{
					PathContextPath childPath = PathContextPath(a_CurrentPath);
					childPath.PushNode(PathNodeEntry(*child));
					FindAllContextPathsRecursive(*child, a_ExploreDirection, a_Depth + 1, childPath, a_Results);
				}
			}
			else
			{
				a_Results.Push(a_CurrentPath);
			}
		}
		else
		{
			a_Results.Push(a_CurrentPath);
		}
	}

	TArray<PathContextPath> FindAllContextPaths(const UK2Node& a_Node, EPathDirection a_ExploreDirection)
	{
		TArray<PathContextPath> result;
		PathContextPath initialPath;
		FindAllContextPathsRecursive(a_Node, a_ExploreDirection, 0, initialPath, result);		
		return result;
	}

	void FilterSuggestionsUsingContextPaths(const TArray<PathPredictionEntry>& a_AvailableSuggestionPaths, const TArray<PathContextPath>& a_AvailableContextPaths, TArray<Suggestion>& a_Output, int32 a_Flags)
	{
		for (const PathPredictionEntry& entry : a_AvailableSuggestionPaths)
		{
			for (const PathContextPath& context : a_AvailableContextPaths)
			{
				float contextSimilarity = ((a_Flags & ESuggestionFlags::CalculateContext) != 0) ? 
					context.CompareContext(entry.m_ContextPath) : 0.0f;
				
				Suggestion suggest(entry.m_PredictionVertex.m_NodeSignature, contextSimilarity, entry.m_NumUses);
				a_Output.Push(suggest);
			}
		}
	}

	void CombineSuggestions(TArray<Suggestion>& a_InOutSuggestions)
	{
		TArray<Suggestion> collapsedSuggestions;
		for (Suggestion suggested : a_InOutSuggestions)
		{
			Suggestion* containedSuggestion = collapsedSuggestions.FindByPredicate([&](const Suggestion& a_Suggestion) {
				return a_Suggestion.CompareSignatures(suggested);
			}); 
			if (containedSuggestion != nullptr)
			{
				//Combine scores or pick max? For now we will just go with the max approach. This might be an interesting field of research right here. 
				// Do we select nodes based on max context similarity or just on max uses? 
				containedSuggestion->SetSuggestionContextScore(FMath::Max(suggested.GetSuggestionContextScore(), 
					containedSuggestion->GetSuggestionContextScore()));
				containedSuggestion->SetSuggestionUsesScore(containedSuggestion->GetSuggestionUsesScore() + 
					suggested.GetSuggestionUsesScore());
			}
			else
			{
				collapsedSuggestions.Add(suggested);
			}
		}

		a_InOutSuggestions = collapsedSuggestions;
	}

	TArray<PathPredictionEntry> RemoveIncompatibleSuggestionsBasedOnConnectablePinTypes(const TArray<PathPredictionEntry>& a_AvailableSuggestions, const UEdGraphPin& a_ConnectingPin, GraphNodeInformationDatabase& a_NodeInfoDatabase, UEdGraph* a_ContextGraph)
	{
		TArray<PathPredictionEntry> result;
		result.Reserve(a_AvailableSuggestions.Num());

		for (PathPredictionEntry entry : a_AvailableSuggestions)
		{
			const GraphNodeInformation* suggestionNodeInfo = a_NodeInfoDatabase.FindNodeInformation(
				entry.m_PredictionVertex.m_NodeSignatureGuid, a_ContextGraph);
			//PHILTODO: This looks weird. We could not retrieve information about the suggested node. 
			if (suggestionNodeInfo != nullptr)
			{
				EEdGraphPinDirection otherPinDirection = UEdGraphPin::GetComplementaryDirection(
					a_ConnectingPin.Direction);
				if (suggestionNodeInfo->HasPinTypeInDirection(a_ConnectingPin.PinType, otherPinDirection))
				{
					result.Push(entry);
				}
			}
			else
			{
				UE_LOG(BILog, BI_VERBOSE, TEXT("Got no information about suggested node %s in graph %s"), 
					*(entry.m_PredictionVertex.m_NodeTitle.ToString()), *(a_ContextGraph->GetName()));
			}
		}

		return result;
	}

	void SelectTopNSuggestions(TArray<Suggestion>& a_InOutSuggestions, int32 a_MaxSuggestionCount, int32 a_Flags)
	{
		struct SuggestionNodeSortingContextOverUses
		{
			inline bool operator() (const Suggestion& lhs, const Suggestion& rhs) const
			{
				return lhs.GetSuggestionContextScore() > rhs.GetSuggestionContextScore() || 
					(lhs.GetSuggestionContextScore() == rhs.GetSuggestionContextScore() && 
					lhs.GetSuggestionUsesScore() > rhs.GetSuggestionUsesScore());
			}
		};

		struct SuggestionNodeSortingUsesOverContext
		{
			inline bool operator() (const Suggestion& lhs, const Suggestion& rhs) const
			{
				return lhs.GetSuggestionUsesScore() > rhs.GetSuggestionUsesScore() ||
					(lhs.GetSuggestionUsesScore() == rhs.GetSuggestionUsesScore() &&
					lhs.GetSuggestionContextScore() > rhs.GetSuggestionContextScore());
			}
		};

		if ((a_Flags & ESuggestionFlags::SortUsesOverContext) != 0)
		{
			a_InOutSuggestions.Sort(SuggestionNodeSortingUsesOverContext());
		}
		else
		{
			a_InOutSuggestions.Sort(SuggestionNodeSortingContextOverUses());
		}

		if (a_InOutSuggestions.Num() > a_MaxSuggestionCount)
		{
			a_InOutSuggestions.SetNum(a_MaxSuggestionCount, true);
			a_InOutSuggestions.Shrink();
		}
	}

	bool StringToSuggestionFlag(const FString& a_InputString, ESuggestionFlags::Flags& a_OutputFlag)
	{
		bool succes = false;
		if (a_InputString.Compare(TEXT("SortUsesOverContext"), ESearchCase::IgnoreCase) == 0)
		{
			a_OutputFlag = ESuggestionFlags::SortUsesOverContext;
			succes = true;
		}
		else if (a_InputString.Compare(TEXT("CalculateContext"), ESearchCase::IgnoreCase) == 0)
		{
			a_OutputFlag = ESuggestionFlags::CalculateContext;
			succes = true;
		}
		return succes;
	}
}

SuggestionDatabasePath::NodeIndexType::NodeIndexType() :
	m_NodeSignature("DEFAULT_INVALID_SIGNATURE")
{
}

SuggestionDatabasePath::NodeIndexType::NodeIndexType(const UK2Node& a_Node) :
	m_NodeSignature(a_Node.GetSignature().ToString())
{
}

SuggestionDatabasePath::NodeIndexType::NodeIndexType(const PathNodeEntry& a_NodeEntry) :
	m_NodeSignature(a_NodeEntry.m_NodeSignature.ToString())
{
}

bool SuggestionDatabasePath::NodeIndexType::operator ==(const NodeIndexType& a_Other) const
{
	return m_NodeSignature == a_Other.m_NodeSignature;
}

uint32 GetTypeHash(const SuggestionDatabasePath::NodeIndexType& a_Instance)
{
	return FCrc::MemCrc32(*a_Instance.m_NodeSignature, a_Instance.m_NodeSignature.Len() * sizeof(TCHAR));
}

FArchive& operator << (FArchive& a_Archive, SuggestionDatabasePath::NodeIndexType& a_Value)
{
	return a_Archive << a_Value.m_NodeSignature;
}

SuggestionDatabasePath::SuggestionDatabasePath()
	: m_SuggestionFlags(ESuggestionFlags::CalculateContext)
	, m_ToggleFlagCommand(TEXT("BIPlugin_ToggleSelectionFlag"), TEXT("Hue^3"), 
	FConsoleCommandWithArgsDelegate::CreateRaw(this, &SuggestionDatabasePath::ToggleSuggestionFlag))
{
}

SuggestionDatabasePath::~SuggestionDatabasePath()
{
	FlushDatabase();
}

void SuggestionDatabasePath::FlushDatabase()
{
	m_ForwardPredictionDatabase.Empty();
	m_BackwardPredictionDatabase.Empty();
}

//#define DETAILED_SUGGESTION_TIMINGS
#ifdef DETAILED_SUGGESTION_TIMINGS
#define TIMING_START(name, friendlyName) StackTimer name(#friendlyName);
#define TIMING_LOG(name) name.LogElapsedMs();
#else
#define TIMING_START(name, friendlyName) 
#define TIMING_LOG(name) 
#endif

void SuggestionDatabasePath::ProvideSuggestions(const FBlueprintSuggestionContext& a_Context, int32 a_SuggestionCount, TArray<Suggestion>& a_Output)
{
	verify(a_Context.Graphs.Num() == 1); //We assume that we are only dealing with a single graph.
	verify(a_Context.Pins.Num() == 1); //We assume that we are only dealing with one connected pin now.

	const uint32 startTime = FPlatformTime::Cycles();

	UE_LOG(BILog, BI_VERBOSE, TEXT("Providing suggestions for context: (Node %s, PinType %s)"),
		*a_Context.Pins[0].OwnerNode->GetNodeTitle(ENodeTitleType::MenuTitle).ToString(), 
		*a_Context.Pins[0].Pin->PinType.PinCategory);

	EPathDirection direction = (a_Context.Pins[0].Pin->Direction == EEdGraphPinDirection::EGPD_Input)? 
		EPathDirection::Backward : EPathDirection::Forward;
	const UK2Node& ownerNode = *a_Context.Pins[0].OwnerNode;
	NodeIndexType nodeIndex = NodeIndexType(ownerNode);

	//TODO: BlueprintGraph.K2Node_VariableGet::GetSignature() Fix to differentiate between fields? 
	TArray<PathContextPath> availableContextPaths = FindAllContextPaths(ownerNode, direction);

	UE_LOG(BILog, BI_VERBOSE, TEXT("Found %i context paths: "), availableContextPaths.Num());
	for (const PathContextPath& contextPath : availableContextPaths)
	{
		UE_LOG(BILog, BI_VERBOSE, TEXT("\t%s >> %s"), 
			*a_Context.Pins[0].OwnerNode->GetNodeTitle(ENodeTitleType::MenuTitle).ToString(), *contextPath.GetPathString());
	}

	PredictionDatabase db = (direction == EPathDirection::Forward) ? m_ForwardPredictionDatabase : m_BackwardPredictionDatabase;

	TIMING_START(suggestionTimer, "FindSuggestionPaths");
	const TArray<PathPredictionEntry>* suggestionPaths = db.Find(nodeIndex);
	TIMING_LOG(suggestionTimer);

	if (suggestionPaths != nullptr)
	{
		TIMING_START(compatibilityTimer, "RemoveIncompatibleSuggestions");
		TArray<PathPredictionEntry> compatibleEntries = RemoveIncompatibleSuggestionsBasedOnConnectablePinTypes(
			*suggestionPaths, *a_Context.Pins[0].Pin, GetGraphNodeDatabase(), a_Context.Graphs[0]);
		TIMING_LOG(compatibilityTimer);

		TIMING_START(filterTimer, "FilterSuggestions");
		FilterSuggestionsUsingContextPaths(compatibleEntries, availableContextPaths, a_Output, m_SuggestionFlags);
		TIMING_LOG(filterTimer);

		TIMING_START(combineTimer, "CombineSuggestions");
		CombineSuggestions(a_Output);
		TIMING_LOG(combineTimer);

		TIMING_START(selectTopTimer, "SelectTopN");
		SelectTopNSuggestions(a_Output, a_SuggestionCount, m_SuggestionFlags);
		TIMING_LOG(selectTopTimer);
	}

	UE_LOG(BILog, BI_VERBOSE, TEXT("Got %i suggestions (%.2f ms): "), a_Output.Num(), FPlatformTime::ToMilliseconds(FPlatformTime::Cycles() - startTime));
	for (const auto& suggestion : a_Output)
	{
		UE_LOG(BILog, BI_VERBOSE, TEXT("\t%s"), *suggestion.GetNodeSignature().ToString());
	}
}

bool SuggestionDatabasePath::HasSuggestions() const
{
	return m_BackwardPredictionDatabase.Num() > 0 || m_ForwardPredictionDatabase.Num() > 0;
}

void SuggestionDatabasePath::GenerateSuggestionForCreatedLink(const UK2Node& a_NodeA, const UK2Node& a_NodeB)
{
	ParseNode(a_NodeA, EPathDirection::Forward, a_NodeB);
	ParseNode(a_NodeA, EPathDirection::Backward, a_NodeB);
	ParseNode(a_NodeB, EPathDirection::Forward, a_NodeA);
	ParseNode(a_NodeB, EPathDirection::Backward, a_NodeA);
}

void SuggestionDatabasePath::Serialize(FArchive& a_Archive)
{
	int32 fileVersion = (int32)EDatabasePathSerializeVersion::VERSION_LATEST;
	a_Archive << fileVersion;
	if (fileVersion == (int32)EDatabasePathSerializeVersion::VERSION_LATEST)
	{
		a_Archive << m_ForwardPredictionDatabase;
		a_Archive << m_BackwardPredictionDatabase;
	}
	else
	{
		UE_LOG(BILog, Warning, TEXT("Could not deserialize suggestion database information. Version difference in \
			file (%i) and code (%i)"), fileVersion, static_cast<int32>(EDatabasePathSerializeVersion::VERSION_LATEST));
	}
}

SuggestionDatabaseBase::CrossValidateResult SuggestionDatabasePath::CrossValidateTest(UEdGraph& a_Graph, const UK2Node& a_Node, int32 a_NumSuggestionsToUse)
{
	CrossValidateResult result;

	TArray<Suggestion> suggestResult;
	suggestResult.Reserve(a_NumSuggestionsToUse);
	for (UEdGraphPin* pin : a_Node.Pins)
	{
		if (pin->LinkedTo.Num() > 0)
		{
			result.m_TestsPerformed++;

			FBlueprintSuggestionContext::FPinParentCombo pinInfo;
			pinInfo.OwnerNode = &a_Node;
			pinInfo.Pin = pin;
			FBlueprintSuggestionContext context;
			context.Graphs.Push(&a_Graph);
			context.Pins.Push(pinInfo);

			const uint32 startCycles = FPlatformTime::Cycles();
			ProvideSuggestions(context, a_NumSuggestionsToUse, suggestResult);
			uint32 cyclesTaken = FPlatformTime::Cycles() - startCycles;
			result.m_CyclesTaken += cyclesTaken;
			result.m_MaxCyclesTaken = FMath::Max(result.m_MaxCyclesTaken, cyclesTaken);
			result.m_MinCyclesTaken = FMath::Min(result.m_MinCyclesTaken, cyclesTaken);

			for (UEdGraphPin* otherPin : pin->LinkedTo)
			{
				UK2Node* otherNode = GetNodeFromPin(otherPin);
				FBlueprintNodeSignature otherSignature = otherNode->GetSignature();
				FGuid otherSignatureGuid = otherSignature.AsGuid();
				
				int32 index = 0;
				for (const Suggestion& suggest : suggestResult)
				{
					if (suggest.GetNodeSignatureGuid() == otherSignatureGuid)
					{
						result.m_PassedPrecision++;
						result.m_PassedPrecisionEntryRank[index]++;
						break;
					}
					index++;
				}
			}
		}
	}

	return result;
}

void SuggestionDatabasePath::ParseNode(const UK2Node& a_Node, EPathDirection a_Direction)
{
	TArray<PathPredictionEntry> preditionPaths = CreatePredictionPathsForNode(a_Node, a_Direction);
	for (PathPredictionEntry entry : preditionPaths)
	{
		AddToPredictionDatabase(entry, a_Direction);
	}
}

void SuggestionDatabasePath::ParseNode(const UK2Node& a_Node, EPathDirection a_Direction, const UK2Node& a_AnchorNodeConstraint)
{
	TArray<PathPredictionEntry> preditionPaths = CreatePredictionPathsForNode(a_Node, a_Direction);
	for (PathPredictionEntry entry : preditionPaths)
	{
		if (entry.m_AnchorVertex.m_NodeSignatureGuid == a_AnchorNodeConstraint.GetSignature().AsGuid())
		{
			AddToPredictionDatabase(entry, a_Direction);
		}
	}
}

void SuggestionDatabasePath::AddToPredictionDatabase(const PathPredictionEntry& a_Entry, EPathDirection a_Direction)
{
	PredictionDatabase& outputDatabase = (a_Direction == EPathDirection::Forward)? m_ForwardPredictionDatabase : 
		m_BackwardPredictionDatabase;

	NodeIndexType nodeIndex = NodeIndexType(a_Entry.m_AnchorVertex);
	TArray<PathPredictionEntry>* predictions = outputDatabase.Find(nodeIndex);
	if (predictions == nullptr)
	{
		predictions = &(outputDatabase.Add(nodeIndex));
	}

	PathPredictionEntry* match = predictions->FindByPredicate([&](const PathPredictionEntry& obj) -> bool
	{
		return obj.CompareExcludingUses(a_Entry);
	});

	if (match != nullptr)
	{
		match->m_NumUses++;
	}
	else
	{
		predictions->Push(a_Entry);
	}
}

void SuggestionDatabasePath::ToggleSuggestionFlag(const TArray<FString>& a_Args)
{
	if (a_Args.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Need at least one argument to toggle suggestion flags"));
	}
	else
	{
		const FString& flagString = a_Args[0];
		ESuggestionFlags::Flags flagToToggle; 
		if (StringToSuggestionFlag(flagString, flagToToggle))
		{
			m_SuggestionFlags ^= flagToToggle;
			UE_LOG(BILog, Log, TEXT("Toggled flag '%s' (0x%x). New Flags: 0x%x"), *flagString, (int32)flagToToggle, m_SuggestionFlags);
		}
		else
		{
			UE_LOG(BILog, Warning, TEXT("Could not toggle flag. Invalid flag specified %s"), *flagString);
		}
	}
}
