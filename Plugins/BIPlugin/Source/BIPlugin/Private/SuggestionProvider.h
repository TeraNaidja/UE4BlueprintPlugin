#pragma once

#include "FBlueprintSuggestionProviderManager.h"

class ISuggestionDatabase;
class SuggestionProvider: public IBlueprintSuggestionProvider
{
public:
	SuggestionProvider(const ISuggestionDatabase& a_Database);
	~SuggestionProvider();

	virtual void ProvideSuggestions(const FBlueprintSuggestionContext& InContext, TArray<TSharedPtr<FBlueprintSuggestion>>& OutEntries) override;
private:
	const ISuggestionDatabase& m_SuggestionDatabase;
};
