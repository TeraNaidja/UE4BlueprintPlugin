#pragma once

#include "SuggestionDatabaseBase.h"
#include "PathNodeEntry.h"
#include "PathPredictionEntry.h"

enum class EDatabasePathSerializeVersion
{
	VERSION_0_1,
	VERSION_LATEST = VERSION_0_1
};

namespace ESuggestionFlags
{
	enum Flags : int32
	{
		SortUsesOverContext = (1 << 0),
		CalculateContext = (1 << 1),
	};
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
	virtual void GenerateSuggestionForCreatedLink(const UK2Node& a_NodeA, const UK2Node& a_NodeB) override;
	virtual void Serialize(FArchive& a_Archive) override;

	virtual CrossValidateResult CrossValidateTest(UEdGraph& a_Graph, const UK2Node& a_Node) override;

protected:
	virtual void ParseNode(const UK2Node& a_Node, EPathDirection a_Direction) override;

private:
	/** Creates suggestions for a node, but has the additional constraint of requiring the first node (Anchor) to match */
	void ParseNode(const UK2Node& a_node, EPathDirection a_Direction, const UK2Node& a_AnchorNodeConstraint);
	void AddToPredictionDatabase(const PathPredictionEntry& a_Entry, EPathDirection a_PathDirection);

	void ToggleSuggestionFlag(const TArray<FString>& a_Args);

	PredictionDatabase m_ForwardPredictionDatabase;
	PredictionDatabase m_BackwardPredictionDatabase;
	int32 m_SuggestionFlags; //ESuggestionFlags
	FAutoConsoleCommand m_ToggleFlagCommand;
};
