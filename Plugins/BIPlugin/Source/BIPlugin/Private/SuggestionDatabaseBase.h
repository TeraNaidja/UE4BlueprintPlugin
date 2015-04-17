#pragma once

#include "Suggestion.h"

class GraphNodeInformationDatabase;
struct FBlueprintSuggestionContext;
class SuggestionDatabaseBase
{
public:
	SuggestionDatabaseBase();
	~SuggestionDatabaseBase();

	virtual void ParseBlueprint(const UBlueprint& a_Blueprint) = 0;
	virtual void FlushDatabase() = 0;
	virtual void ProvideSuggestions(const FBlueprintSuggestionContext& a_Context, int32 a_SuggestionCount, TArray<Suggestion>& a_Output) const = 0;

	void SetGraphNodeDatabase(GraphNodeInformationDatabase* a_Database);
protected:
	GraphNodeInformationDatabase& GetGraphNodeDatabase() const;

private:
	GraphNodeInformationDatabase* m_GraphNodeDatabase;
};
