#include "BIPluginPrivatePCH.h"
#include "SuggestionDatabasePath.h"

#include "FBlueprintSuggestionProviderManager.h"

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
			if (childPin->Direction == ToPinDirection(a_ExploreDirection) && !childPin->bHidden && !childPin->bNotConnectable)
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

	const int MAX_CONTEXT_PATH_LENGTH = 3;

	void FindPathForNodeRecursive(const UK2Node& a_CurrentNode, EPathDirection a_ExploreDirection, PathPredictionEntry& a_ParentPath, TArray<PathPredictionEntry>& a_Results, int32 a_Depth)
	{
		if (a_Depth < MAX_CONTEXT_PATH_LENGTH)
		{
			TArray<UK2Node*> children = FindNodesInDirection(a_CurrentNode, a_ExploreDirection);
			for (UK2Node* child : children)
			{
				PathPredictionEntry thisPath = PathPredictionEntry(a_ParentPath);
				thisPath.m_ContextPath.Push(PathNodeEntry(child->GetSignature().ToString()));
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
		initialNode.m_PredictionVertex = PathNodeEntry(a_Node.GetSignature().ToString());

		for (const auto node : FindNodesInDirection(a_Node, a_ExploreDirection))
		{
			PathPredictionEntry pathEntry = PathPredictionEntry(initialNode);
			pathEntry.m_AnchorVertex = PathNodeEntry(node->GetSignature().ToString());
			result.Push(pathEntry);
			FindPathForNodeRecursive(*node, a_ExploreDirection, pathEntry, result, 0);
		}

		return result;
	}

	void FindAllContextPathsRecursive(const UK2Node& a_Node, EPathDirection a_ExploreDirection, int32 a_Depth, 
		TArray<PathNodeEntry>& a_CurrentPath, TArray<TArray<PathNodeEntry>>& a_Results)
	{
		if (a_Depth < MAX_CONTEXT_PATH_LENGTH)
		{
			TArray<UK2Node*> children = FindNodesInDirection(a_Node, a_ExploreDirection);
			if (children.Num() > 0)
			{
				for (UK2Node* child : children)
				{
					TArray<PathNodeEntry> childPath = TArray<PathNodeEntry>(a_CurrentPath);
					childPath.Push(PathNodeEntry(*child));
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

	TArray<TArray<PathNodeEntry>> FindAllContextPaths(const UK2Node& a_Node, EPathDirection a_ExploreDirection)
	{
		TArray<TArray<PathNodeEntry>> result;
		TArray<PathNodeEntry> initialPath;
		FindAllContextPathsRecursive(a_Node, a_ExploreDirection, 0, initialPath, result);		

		return result;
	}
}

SuggestionDatabasePath::NodeIndexType::NodeIndexType() :
	m_NodeSignature("DEFAULT_INVALID_SIGNATURE")
{
}

SuggestionDatabasePath::NodeIndexType::NodeIndexType(const FString& a_NodeSignature) :
	m_NodeSignature(a_NodeSignature)
{
}

SuggestionDatabasePath::NodeIndexType::NodeIndexType(const PathNodeEntry& a_NodeEntry) :
	m_NodeSignature(a_NodeEntry.m_NodeSignature)
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

SuggestionDatabasePath::SuggestionDatabasePath()
{
}

SuggestionDatabasePath::~SuggestionDatabasePath()
{
	FlushDatabase();
}

void SuggestionDatabasePath::ParseBlueprint(const UBlueprint& a_Blueprint)
{
	FString name;
	a_Blueprint.GetName(name);
	if (name != "PuzzleBlockGrid")
		return;

	TArray<UEdGraph*> graphsInBlueprint;
	a_Blueprint.GetAllGraphs(graphsInBlueprint);
	for (auto graph : graphsInBlueprint)
	{
		ParseGraph(a_Blueprint, *graph);
	}
}

void SuggestionDatabasePath::FlushDatabase()
{
	m_ForwardPredictionDatabase.Empty();
	m_BackwardPredictionDatabase.Empty();
}

void SuggestionDatabasePath::ProvideSuggestions(const FBlueprintSuggestionContext& a_Context, int32 a_SuggestionCount, const TArray<Suggestion>& a_Output) const
{
	verify(a_Context.Pins.Num() == 1); //We assume that we are only dealing with one connected pin now.
	EPathDirection direction = (a_Context.Pins[0].Pin->Direction == EEdGraphPinDirection::EGPD_Input)? EPathDirection::Backward : EPathDirection::Forward;
	const UK2Node& ownerNode = *a_Context.Pins[0].OwnerNode;
	NodeIndexType nodeIndex = NodeIndexType(ownerNode.GetSignature().ToString());

	TArray<TArray<PathNodeEntry>> availableContextPaths = FindAllContextPaths(ownerNode, direction);
	//Todo: Filter out and collapse results based on context.

	PredictionDatabase db = (direction == EPathDirection::Forward) ? m_ForwardPredictionDatabase : m_BackwardPredictionDatabase;

	const TArray<PathPredictionEntry>* suggestionPaths = db.Find(nodeIndex);
	if (suggestionPaths != nullptr)
	{
		for (PathPredictionEntry entry : *suggestionPaths)
		{
			FString path;
			for (PathNodeEntry nodeEntry : entry.m_ContextPath)
			{
				path.Append(nodeEntry.m_NodeSignature);
			}
			UE_LOG(LogTemp, Warning, TEXT("Prediction: %s Anchor: %s Path: %s"), *entry.m_PredictionVertex.m_NodeSignature,
				*entry.m_AnchorVertex.m_NodeSignature, *path);
		}
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