#pragma once

#include "ModuleManager.h"

class GraphNodeInformationDatabase;
class IBlueprintSuggestionProvider;
class SuggestionDatabaseBase;
class BIPluginImpl : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual void PostLoadCallback() override;

	void OnRebuildDatabase();

private:
	TSharedPtr<IBlueprintSuggestionProvider> m_SuggestionProvider;
	SuggestionDatabaseBase* m_SuggestionDatabase;
	GraphNodeInformationDatabase* m_NodeInformationDatabase;

	IConsoleCommand* m_RebuildCacheCommand;
};
