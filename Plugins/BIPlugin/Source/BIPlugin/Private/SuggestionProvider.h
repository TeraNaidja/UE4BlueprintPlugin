#pragma once

#include "BlueprintSuggestionProviderManager.h"

class SuggestionDatabaseBase;
class SuggestionProvider: public IBlueprintSuggestionProvider
{
public:
	DECLARE_DELEGATE(RebuildDatabaseDelegate);

	SuggestionProvider(const SuggestionDatabaseBase& a_Database, const RebuildDatabaseDelegate& a_RebuildDatabaseDelegate);
	~SuggestionProvider();

	virtual void ProvideSuggestions(const FBlueprintSuggestionContext& InContext, TArray<TSharedPtr<FBlueprintSuggestion>>& OutEntries) override;
private:
	const SuggestionDatabaseBase& m_SuggestionDatabase;
	RebuildDatabaseDelegate m_RebuildDatabaseDelegate;
};
