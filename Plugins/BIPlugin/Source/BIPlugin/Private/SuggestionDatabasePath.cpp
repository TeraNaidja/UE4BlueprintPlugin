#include "BIPluginPrivatePCH.h"
#include "SuggestionDatabasePath.h"

#include "BlueprintSuggestion.h"
#include "BlueprintSuggestionContext.h"
#include "GraphNodeInformationDatabase.h"
#include "GraphNodeInformation.h"

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
			UE_LOG(LogTemp, Warning, TEXT("Unknown EPathDirection while translating to pin direction in ToPinDirection"));
			return EEdGraphPinDirection::EGPD_Input;
		}
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
					check(linkedPin->GetOuter()->IsA(UK2Node::StaticClass()));
					UK2Node* parentNode = Cast<UK2Node>(linkedPin->GetOuter());
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

	void FilterSuggestionsUsingContextPaths(const TArray<PathPredictionEntry>& a_AvailableSuggestionPaths, const TArray<PathContextPath>& a_AvailableContextPaths, TArray<Suggestion>& a_Output)
	{
		for (const PathPredictionEntry& entry : a_AvailableSuggestionPaths)
		{
			for (const PathContextPath& context : a_AvailableContextPaths)
			{
				float contextSimilarity = context.CompareContext(entry.m_ContextPath);
				
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

	TArray<PathPredictionEntry> RemoveIncompatibleSuggestionsBasedOnConnectablePinTypes(const TArray<PathPredictionEntry>& a_AvailableSuggestions, const UEdGraphPin& a_ConnectingPin, GraphNodeInformationDatabase& a_NodeInfoDatabase)
	{
		TArray<PathPredictionEntry> result;
		result.Reserve(a_AvailableSuggestions.Num());

		for (PathPredictionEntry entry : a_AvailableSuggestions)
		{
			const GraphNodeInformation* suggestionNodeInfo = a_NodeInfoDatabase.FindNodeInformation(
				entry.m_PredictionVertex.m_NodeSignatureGuid);
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
				UE_LOG(LogTemp, Log, TEXT("Got no information about suggested node %s"), 
					*(entry.m_PredictionVertex.m_NodeTitle.ToString()));
			}
		}

		return result;
	}

	void SelectTopNSuggestions(TArray<Suggestion>& a_InOutSuggestions, int32 a_MaxSuggestionCount)
	{
		struct SuggestionNodeSorting
		{
			inline bool operator() (const Suggestion& lhs, const Suggestion& rhs) const
			{
				return lhs.GetSuggestionContextScore() > rhs.GetSuggestionContextScore();
			}
		};

		a_InOutSuggestions.Sort(SuggestionNodeSorting());
		if (a_InOutSuggestions.Num() > a_MaxSuggestionCount)
		{
			a_InOutSuggestions.SetNum(a_MaxSuggestionCount, true);
			a_InOutSuggestions.Shrink();
		}
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

void SuggestionDatabasePath::ProvideSuggestions(const FBlueprintSuggestionContext& a_Context, int32 a_SuggestionCount, TArray<Suggestion>& a_Output)
{
	verify(a_Context.Pins.Num() == 1); //We assume that we are only dealing with one connected pin now.
	EPathDirection direction = (a_Context.Pins[0].Pin->Direction == EEdGraphPinDirection::EGPD_Input)? 
		EPathDirection::Backward : EPathDirection::Forward;
	const UK2Node& ownerNode = *a_Context.Pins[0].OwnerNode;
	NodeIndexType nodeIndex = NodeIndexType(ownerNode);

	//TODO: BlueprintGraph.K2Node_VariableGet::GetSignature() Fix to differentiate between fields? 
	TArray<PathContextPath> availableContextPaths = FindAllContextPaths(ownerNode, direction);

	PredictionDatabase db = (direction == EPathDirection::Forward) ? m_ForwardPredictionDatabase : m_BackwardPredictionDatabase;

	const TArray<PathPredictionEntry>* suggestionPaths = db.Find(nodeIndex);
	if (suggestionPaths != nullptr)
	{ 
		TArray<PathPredictionEntry> compatibleEntries = RemoveIncompatibleSuggestionsBasedOnConnectablePinTypes(
			*suggestionPaths, *a_Context.Pins[0].Pin, GetGraphNodeDatabase());
		FilterSuggestionsUsingContextPaths(compatibleEntries, availableContextPaths, a_Output);
		CombineSuggestions(a_Output);
		SelectTopNSuggestions(a_Output, a_SuggestionCount);
	}
}

bool SuggestionDatabasePath::HasSuggestions() const
{
	return m_BackwardPredictionDatabase.Num() > 0 || m_ForwardPredictionDatabase.Num() > 0;
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
		UE_LOG(LogTemp, Warning, TEXT("Could not deserialize suggestion database information. Version difference in \
			file (%i) and code (%i)"), fileVersion, (int32)EDatabasePathSerializeVersion::VERSION_LATEST);
	}
}

void SuggestionDatabasePath::ParseBlueprint(const UBlueprint& a_Blueprint)
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
		ParseGraph(a_Blueprint, *graph);
	}
}

void SuggestionDatabasePath::ParseGraph(const UBlueprint& a_Blueprint, const UEdGraph& a_Graph)
{
	TArray<UK2Node*> nodes;
	a_Graph.GetNodesOfClass(nodes);
	for (UK2Node* node : nodes)
	{
		ParseNode(*node, EPathDirection::Forward);
		ParseNode(*node, EPathDirection::Backward);
	}
}

void SuggestionDatabasePath::ParseNode(const UK2Node& a_Node, EPathDirection a_Direction)
{
	TArray<PathPredictionEntry> preditionPaths = CreatePredictionPathsForNode(a_Node, a_Direction);
	for (PathPredictionEntry entry : preditionPaths)
	{
		AddToPredictionDatabase(entry, a_Direction);
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
