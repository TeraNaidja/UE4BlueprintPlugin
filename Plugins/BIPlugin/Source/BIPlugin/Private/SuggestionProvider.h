#pragma once

#include "FBlueprintSuggestionProviderManager.h"

class SuggestionDatabaseBase;
class SuggestionProvider: public IBlueprintSuggestionProvider
{
public:
	SuggestionProvider(const SuggestionDatabaseBase& a_Database);
	~SuggestionProvider();

	virtual void ProvideSuggestions(const FBlueprintSuggestionContext& InContext, TArray<TSharedPtr<FBlueprintSuggestion>>& OutEntries) override;
private:
	const SuggestionDatabaseBase& m_SuggestionDatabase;
};
