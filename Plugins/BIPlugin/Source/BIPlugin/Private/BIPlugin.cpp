#include "BIPluginPrivatePCH.h"

#include "BIPlugin.h"
#include "FBlueprintSuggestionProviderManager.h"
#include "SuggestionProvider.h"
#include "ISuggestionDatabase.h"
#include "SuggestionDatabaseTrie.h"

void BIPluginImpl::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("BIPlugin Startup"));

	m_SuggestionDatabase = TSharedPtr<ISuggestionDatabase>(new SuggestionDatabaseTrie());
	m_SuggestionProvider = TSharedPtr<IBlueprintSuggestionProvider>(new SuggestionProvider());
	FBlueprintSuggestionProviderManager::Get().RegisterBlueprintSuggestionProvider(m_SuggestionProvider);

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
	m_SuggestionDatabase.Reset();
}

void BIPluginImpl::OnRebuildDatabase()
{
	UE_LOG(LogTemp, Warning, TEXT("Rebuilding suggestion database using available blueprints."));

	for (TObjectIterator<UBlueprint> BlueprintIt; BlueprintIt; ++BlueprintIt)
	{
		UBlueprint* Blueprint = *BlueprintIt;
		m_SuggestionDatabase->ParseBlueprint(*Blueprint);
	}
}

IMPLEMENT_MODULE(BIPluginImpl, Module)