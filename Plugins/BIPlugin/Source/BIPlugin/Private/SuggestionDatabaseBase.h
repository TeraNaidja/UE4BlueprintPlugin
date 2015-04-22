#pragma once

#include "Suggestion.h"

class GraphNodeInformationDatabase;
struct FBlueprintSuggestionContext;
class SuggestionDatabaseBase
{
public:
	SuggestionDatabaseBase();
	~SuggestionDatabaseBase();

	void FillSuggestionDatabase();
	virtual void FlushDatabase() = 0;
	virtual void ProvideSuggestions(const FBlueprintSuggestionContext& a_Context, int32 a_SuggestionCount, TArray<Suggestion>& a_Output) = 0;
	virtual bool HasSuggestions() const = 0;

	virtual void Serialize(FArchive& a_Archive) = 0;

	void SetGraphNodeDatabase(GraphNodeInformationDatabase* a_Database);
protected:
	virtual void ParseBlueprint(const UBlueprint& a_Blueprint) = 0;
	const GraphNodeInformationDatabase& GetGraphNodeDatabase() const;
	GraphNodeInformationDatabase& GetGraphNodeDatabase();

private:
	GraphNodeInformationDatabase* m_GraphNodeDatabase;
};
