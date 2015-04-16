#include "BIPluginPrivatePCH.h"

#include "BIPlugin.h"
#include "FBlueprintSuggestionProviderManager.h"
#include "SuggestionProvider.h"
#include "ISuggestionDatabase.h"
#include "SuggestionDatabasePath.h"

void BIPluginImpl::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("BIPlugin Startup"));

	m_SuggestionDatabase = TUniquePtr<ISuggestionDatabase>(new SuggestionDatabasePath());
	m_SuggestionProvider = TSharedPtr<IBlueprintSuggestionProvider>(new SuggestionProvider(*m_SuggestionDatabase));
	FBlueprintSuggestionProviderManager::Get().RegisterBlueprintSuggestionProvider(m_SuggestionProvider);

	m_RebuildCacheCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("BIPlugin_RebuildSuggestionCache"),
		TEXT("Flushes and Rebuilds blueprint suggestion bache based on all available blueprints in current project."),
		FConsoleCommandDelegate::CreateRaw(this, &BIPluginImpl::OnRebuildDatabase),
		ECVF_Default
		);

	OnRebuildDatabase();
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