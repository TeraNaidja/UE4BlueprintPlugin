#pragma once

#include "EPathDirection.h"
#include "Suggestion.h"

class GraphNodeInformationDatabase;
struct FBlueprintSuggestionContext;
class SuggestionDatabaseBase
{
	static const int32 KFOLD_NUM_SUGGESTIONS = 5;

public:
	struct CrossValidateResult
	{
		CrossValidateResult()
			: m_TestsPerformed(0)
			, m_PassedPrecision(0)
			, m_CyclesTaken(0)
			, m_MinCyclesTaken(0xffffffff)
			, m_MaxCyclesTaken(0)
		{
			for (int32 i = 0; i < KFOLD_NUM_SUGGESTIONS; ++i)
			{
				m_PassedPrecisionEntryRank[i] = 0;
			}
		}

		int32 m_TestsPerformed;
		int32 m_PassedPrecision;
		int32 m_PassedPrecisionEntryRank[KFOLD_NUM_SUGGESTIONS];
		uint32 m_CyclesTaken;
		uint32 m_MinCyclesTaken;
		uint32 m_MaxCyclesTaken;
	};

	SuggestionDatabaseBase();
	virtual ~SuggestionDatabaseBase();

	void FillSuggestionDatabase();
	virtual void FlushDatabase() = 0;
	virtual void ProvideSuggestions(const FBlueprintSuggestionContext& a_Context, int32 a_SuggestionCount, TArray<Suggestion>& a_Output) = 0;
	virtual bool HasSuggestions() const = 0;
	virtual void GenerateSuggestionForCreatedLink(const UK2Node& a_NodeA, const UK2Node& a_NodeB) = 0;
	virtual CrossValidateResult CrossValidateTest(UEdGraph& a_Graph, const UK2Node& a_Node, int32 a_NumSuggestionsToUse) = 0;

	virtual void Serialize(FArchive& a_Archive) = 0;

	void PerformKFoldCrossValidationTest(int32 a_Folds);
	void SetGraphNodeDatabase(GraphNodeInformationDatabase* a_Database);
protected:
	void ParseBlueprint(const UBlueprint& a_Blueprint);
	void ParseGraph(const UEdGraph& a_Graph);
	virtual void ParseNode(const UK2Node& a_Node, EPathDirection a_Direction) = 0;
	const GraphNodeInformationDatabase& GetGraphNodeDatabase() const;
	GraphNodeInformationDatabase& GetGraphNodeDatabase();

private:
	GraphNodeInformationDatabase* m_GraphNodeDatabase;
};
