#include "BIPluginPrivatePCH.h"

#include "BIPlugin.h"
#include "BlueprintSuggestionProviderManager.h"
#include "SuggestionProvider.h"
#include "SuggestionDatabaseBase.h"
#include "SuggestionDatabasePath.h"
#include "GraphNodeInformationDatabase.h"

namespace
{
	const TCHAR* PLUGIN_DATABASE_PATH = TEXT("BIPluginData.bin");

	void TestFunction()
	{
	}
}

IConsoleCommand* TestFunctionCommand = nullptr;

void BIPluginImpl::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("BIPlugin Startup"));

	m_NodeInformationDatabase = new GraphNodeInformationDatabase();
	m_SuggestionDatabase = new SuggestionDatabasePath();
	m_SuggestionProvider = TSharedPtr<SuggestionProvider>(new SuggestionProvider(*m_SuggestionDatabase, 
		SuggestionProvider::RebuildDatabaseDelegate::CreateRaw(this, &BIPluginImpl::OnRebuildDatabase)));
	FBlueprintSuggestionProviderManager::Get().RegisterBlueprintSuggestionProvider(m_SuggestionProvider);

	m_SuggestionDatabase->SetGraphNodeDatabase(m_NodeInformationDatabase);

	LoadDatabaseFromFile(PLUGIN_DATABASE_PATH);

	m_RebuildCacheCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("BIPlugin_RebuildSuggestionCache"),
		TEXT("Flushes and Rebuilds blueprint suggestion bache based on all available blueprints in current project."),
		FConsoleCommandDelegate::CreateRaw(this, &BIPluginImpl::OnRebuildDatabase), 
		ECVF_Default
		);
	TestFunctionCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("BIPlugin_TestFunction"),
		TEXT("Your general purpose test function."),
		FConsoleCommandDelegate::CreateRaw(this, &BIPluginImpl::TestFunction),
		ECVF_Default
		);
}

void BIPluginImpl::ShutdownModule()
{
	UE_LOG(LogTemp, Warning, TEXT("BIPlugin Shutdown"));
	IConsoleManager::Get().UnregisterConsoleObject(TestFunctionCommand);
	IConsoleManager::Get().UnregisterConsoleObject(m_RebuildCacheCommand);
	FBlueprintSuggestionProviderManager::Get().DeregisterBlueprintSuggestionProvider(m_SuggestionProvider);
	m_SuggestionProvider.Reset();
	delete m_SuggestionDatabase;
	delete m_NodeInformationDatabase;
}

void BIPluginImpl::OnRebuildDatabase()
{
	UE_LOG(LogTemp, Warning, TEXT("Rebuilding suggestion database using available blueprints."));

	m_NodeInformationDatabase->FlushDatabase();
	m_NodeInformationDatabase->FillDatabase();

	m_SuggestionDatabase->FlushDatabase();
	m_SuggestionDatabase->FillSuggestionDatabase();
}

void BIPluginImpl::SaveDatabaseToFile(const TCHAR* a_FilePath)
{
	FArchive* fileArchive = IFileManager::Get().CreateFileWriter(a_FilePath);

	m_SuggestionDatabase->Serialize(*fileArchive);

	fileArchive->Close();
	delete fileArchive;
}

void BIPluginImpl::LoadDatabaseFromFile(const TCHAR* a_FilePath)
{
	FArchive* fileArchive = IFileManager::Get().CreateFileReader(a_FilePath);

	m_SuggestionDatabase->Serialize(*fileArchive);

	fileArchive->Close();
	delete fileArchive;
}

void BIPluginImpl::TestFunction()
{
	SaveDatabaseToFile(PLUGIN_DATABASE_PATH);
	LoadDatabaseFromFile(PLUGIN_DATABASE_PATH);
}

IMPLEMENT_MODULE(BIPluginImpl, Module)
