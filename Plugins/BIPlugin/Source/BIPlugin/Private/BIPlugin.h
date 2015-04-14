#pragma once

#include "ModuleManager.h"

class IBlueprintSuggestionProvider;
class ISuggestionDatabase;
class BIPluginImpl : public IModuleInterface
{
public:
	void StartupModule();
	void ShutdownModule();

	void OnRebuildDatabase();

private:
	TSharedPtr<IBlueprintSuggestionProvider> m_SuggestionProvider;
	TUniquePtr<ISuggestionDatabase> m_SuggestionDatabase;
	IConsoleCommand* m_RebuildCacheCommand;
};
