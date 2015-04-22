#pragma once

#include "SuggestionDatabaseBase.h"
#include "EPathDirection.h"
#include "PathNodeEntry.h"
#include "PathPredictionEntry.h"

enum class EDatabasePathSerializeVersion
{
	VERSION_0_1,
	VERSION_LATEST = VERSION_0_1
};

class SuggestionDatabasePath: public SuggestionDatabaseBase
{
public:
	class NodeIndexType
	{
	public:
		NodeIndexType();
		NodeIndexType(const UK2Node& a_Node);
		NodeIndexType(const PathNodeEntry& a_NodeEntry);
		bool operator ==(const NodeIndexType& a_Other) const;
		friend uint32 GetTypeHash(const NodeIndexType& a_Instance);
		friend FArchive& operator << (FArchive& a_Archive, NodeIndexType& a_Value);

		FString m_NodeSignature;
	};

	typedef TMap<NodeIndexType, TArray<PathPredictionEntry>> PredictionDatabase;

	SuggestionDatabasePath();
	~SuggestionDatabasePath();

	virtual void FlushDatabase() override;
	virtual void ProvideSuggestions(const FBlueprintSuggestionContext& a_Context, int32 a_SuggestionCount, TArray<Suggestion>& a_Output) override;
	virtual bool HasSuggestions() const override;
	virtual void Serialize(FArchive& a_Archive) override;

protected:
	virtual void ParseBlueprint(const UBlueprint& a_Blueprint) override;

private:
	void ParseGraph(const UBlueprint& a_Blueprint, const UEdGraph& a_Graph);
	void ParseNode(const UK2Node& a_Node, EPathDirection a_Direction);
	void AddToPredictionDatabase(const PathPredictionEntry& a_Entry, EPathDirection a_PathDirection);

	PredictionDatabase m_ForwardPredictionDatabase;
	PredictionDatabase m_BackwardPredictionDatabase;
};
