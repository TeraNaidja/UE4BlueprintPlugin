#pragma once

#include "ISuggestionDatabase.h"
#include "EPathDirection.h"
#include "PathNodeEntry.h"
#include "PathPredictionEntry.h"

class SuggestionDatabasePath: public ISuggestionDatabase
{
public:
	class NodeIndexType
	{
	public:
		NodeIndexType();
		NodeIndexType(const FString& a_NodeSignature);
		NodeIndexType(const PathNodeEntry& a_NodeEntry);
		bool operator ==(const NodeIndexType& a_Other) const;
		friend uint32 GetTypeHash(const NodeIndexType& a_Instance);

		FString m_NodeSignature;
	};

	typedef TMap<NodeIndexType, TArray<PathPredictionEntry>> PredictionDatabase;

	SuggestionDatabasePath();
	~SuggestionDatabasePath();

	virtual void ParseBlueprint(const UBlueprint& a_Blueprint) override;
	virtual void FlushDatabase() override;
	virtual void ProvideSuggestions(const FBlueprintSuggestionContext& a_Context, int32 a_SuggestionCount, const TArray<Suggestion>& a_Output) const override;
private:
	void ParseGraph(const UBlueprint& a_Blueprint, const UEdGraph& a_Graph);
	void ParseNode(const UK2Node& a_Node, EPathDirection a_Direction);
	void AddToPredictionDatabase(const PathPredictionEntry& a_Entry, EPathDirection a_PathDirection);

	PredictionDatabase m_ForwardPredictionDatabase;
	PredictionDatabase m_BackwardPredictionDatabase;
};
