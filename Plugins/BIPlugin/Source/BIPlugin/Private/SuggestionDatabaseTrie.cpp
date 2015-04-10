#include "BIPluginPrivatePCH.h"
#include "SuggestionDatabaseTrie.h"

SuggestionDatabaseTrie::SuggestionDatabaseTrie()
{
}

SuggestionDatabaseTrie::~SuggestionDatabaseTrie()
{
}

void SuggestionDatabaseTrie::ParseBlueprint(const UBlueprint& a_Blueprint)
{
	FString name;
	a_Blueprint.GetName(name);
	if (name != "SideScrollerExampleMap")
		return;

	TArray<UEdGraph*> graphsInBlueprint;
	a_Blueprint.GetAllGraphs(graphsInBlueprint);
	for (auto graph : graphsInBlueprint)
	{
	}
}

void SuggestionDatabaseTrie::FlushDatabase()
{
}
