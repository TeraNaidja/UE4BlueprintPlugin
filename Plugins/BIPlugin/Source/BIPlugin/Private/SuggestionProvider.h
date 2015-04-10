#pragma once

#include "FBlueprintSuggestionProviderManager.h"

class SuggestionProvider: public IBlueprintSuggestionProvider
{
public:
	SuggestionProvider();
	~SuggestionProvider();

	virtual void ProvideSuggestions(const FBlueprintSuggestionContext& InContext, TArray<TSharedPtr<FBlueprintSuggestion>>& OutEntries) override;
};
