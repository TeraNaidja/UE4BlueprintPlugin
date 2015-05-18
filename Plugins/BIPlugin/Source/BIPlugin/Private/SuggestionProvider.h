#pragma once

#include "BlueprintSuggestionProviderManager.h"

class SuggestionDatabaseBase;
class SuggestionProvider: public IBlueprintSuggestionProvider
{
public:
	DECLARE_DELEGATE(RebuildDatabaseDelegate);

	SuggestionProvider(SuggestionDatabaseBase& a_Database, const RebuildDatabaseDelegate& a_RebuildDatabaseDelegate);
	~SuggestionProvider();

	virtual void ProvideSuggestions(const FBlueprintSuggestionContext& InContext, TArray<TSharedPtr<FBlueprintSuggestion>>& OutEntries) override;
private:
	void SubscribeToGraphChanged(UEdGraph* a_Graph);
	void OnGraphChanged(const FEdGraphEditAction& a_Action);
	void OnEnabledConsoleCommand();

	SuggestionDatabaseBase& m_SuggestionDatabase;
	RebuildDatabaseDelegate m_RebuildDatabaseDelegate;

	UEdGraph* m_LastGraphForSuggestions;
	FDelegateHandle m_OnGraphChangedHandle;
	FAutoConsoleCommand m_EnabledConsoleCommand;
	bool m_SuggestionsEnabled;
};
