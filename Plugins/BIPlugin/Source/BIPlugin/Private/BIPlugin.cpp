#include "BIPluginPrivatePCH.h"

#include "BIPlugin.h"
#include "BlueprintSuggestionProviderManager.h"
#include "SuggestionProvider.h"
#include "SuggestionDatabaseBase.h"
#include "SuggestionDatabasePath.h"
#include "GraphNodeInformationDatabase.h"

void BIPluginImpl::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("BIPlugin Startup"));

	m_NodeInformationDatabase = new GraphNodeInformationDatabase();
	m_SuggestionDatabase = new SuggestionDatabasePath();
	m_SuggestionProvider = TSharedPtr<SuggestionProvider>(new SuggestionProvider(*m_SuggestionDatabase, 
		SuggestionProvider::RebuildDatabaseDelegate::CreateRaw(this, &BIPluginImpl::OnRebuildDatabase)));
	FBlueprintSuggestionProviderManager::Get().RegisterBlueprintSuggestionProvider(m_SuggestionProvider);

	m_SuggestionDatabase->SetGraphNodeDatabase(m_NodeInformationDatabase);

	m_RebuildCacheCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("BIPlugin_RebuildSuggestionCache"),
		TEXT("Flushes and Rebuilds blueprint suggestion bache based on all available blueprints in current project."),
		FConsoleCommandDelegate::CreateRaw(this, &BIPluginImpl::OnRebuildDatabase), 
		ECVF_Default
		);

}

void BIPluginImpl::ShutdownModule()
{
	UE_LOG(LogTemp, Warning, TEXT("BIPlugin Shutdown"));
	IConsoleManager::Get().UnregisterConsoleObject(m_RebuildCacheCommand);
	FBlueprintSuggestionProviderManager::Get().DeregisterBlueprintSuggestionProvider(m_SuggestionProvider);
	m_SuggestionProvider.Reset();
	delete m_SuggestionDatabase;
	delete m_NodeInformationDatabase;
}

void BIPluginImpl::OnRebuildDatabase()
{
	UE_LOG(LogTemp, Warning, TEXT("Rebuilding suggestion database using available blueprints."));

	m_NodeInformationDatabase->FillDatabase();

	m_SuggestionDatabase->FlushDatabase();
	m_SuggestionDatabase->FillSuggestionDatabase();
}

IMPLEMENT_MODULE(BIPluginImpl, Module)