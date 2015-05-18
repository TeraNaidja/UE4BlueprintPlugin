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
}

void BIPluginImpl::StartupModule()
{
	UE_LOG(BILog, Warning, TEXT("BIPlugin Startup"));

	m_NodeInformationDatabase = new GraphNodeInformationDatabase();
	m_SuggestionDatabase = new SuggestionDatabasePath();
	m_SuggestionProvider = TSharedPtr<SuggestionProvider>(new SuggestionProvider(*m_SuggestionDatabase, 
		SuggestionProvider::RebuildDatabaseDelegate::CreateRaw(this, &BIPluginImpl::OnRebuildDatabase)));
	FBlueprintSuggestionProviderManager::Get().RegisterBlueprintSuggestionProvider(m_SuggestionProvider);

	m_SuggestionDatabase->SetGraphNodeDatabase(m_NodeInformationDatabase);

	LoadDatabaseFromFile(PLUGIN_DATABASE_PATH);

	IConsoleManager& consoleManager = IConsoleManager::Get();
	m_RebuildCacheCommand = consoleManager.RegisterConsoleCommand(
		TEXT("BIPlugin_RebuildSuggestionCache"),
		TEXT("Flushes and Rebuilds blueprint suggestion cache based on all available blueprints in current project."),
		FConsoleCommandDelegate::CreateRaw(this, &BIPluginImpl::OnRebuildDatabase), 
		ECVF_Default
		);
	m_PerformKFoldCrossValidationCommand = consoleManager.RegisterConsoleCommand(
		TEXT("BIPlugin_PerformKFoldCrossValidation"),
		TEXT("Performs a K-Fold Cross-Validation test to assess the accuracy of the suggestions. Requires 1 argument: number of folds"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &BIPluginImpl::OnPerformKFoldCrossValidation),
		ECVF_Default
		);
}

void BIPluginImpl::ShutdownModule()
{
	SaveDatabaseToFile(PLUGIN_DATABASE_PATH);

	UE_LOG(BILog, Warning, TEXT("BIPlugin Shutdown"));

	IConsoleManager::Get().UnregisterConsoleObject(m_PerformKFoldCrossValidationCommand);
	IConsoleManager::Get().UnregisterConsoleObject(m_RebuildCacheCommand);
	FBlueprintSuggestionProviderManager::Get().DeregisterBlueprintSuggestionProvider(m_SuggestionProvider);
	m_SuggestionProvider.Reset();
	delete m_SuggestionDatabase;
	delete m_NodeInformationDatabase;
}

void BIPluginImpl::OnRebuildDatabase()
{
	UE_LOG(BILog, Warning, TEXT("Rebuilding suggestion database using available blueprints."));

	m_NodeInformationDatabase->FlushDatabase();
	m_NodeInformationDatabase->FillDatabase();

	m_SuggestionDatabase->FlushDatabase();
	m_SuggestionDatabase->FillSuggestionDatabase();
}

void BIPluginImpl::SaveDatabaseToFile(const TCHAR* a_FilePath)
{
	UE_LOG(BILog, Log, TEXT("Serializing BI Plugin database to '%s'"), a_FilePath);
	FArchive* fileArchive = IFileManager::Get().CreateFileWriter(a_FilePath);

	m_SuggestionDatabase->Serialize(*fileArchive);

	fileArchive->Close();
	delete fileArchive;
}

void BIPluginImpl::LoadDatabaseFromFile(const TCHAR* a_FilePath)
{
	UE_LOG(BILog, Log, TEXT("Deserializing BI Plugin database from '%s'"), a_FilePath);
	FArchive* fileArchive = IFileManager::Get().CreateFileReader(a_FilePath);

	if (fileArchive != nullptr)
	{
		m_SuggestionDatabase->Serialize(*fileArchive);

		fileArchive->Close();
		delete fileArchive;
	}
	else
	{
		UE_LOG(BILog, Log, TEXT("Could not find BI Plugin database at '%s'"), a_FilePath);
	}
}

void BIPluginImpl::OnPerformKFoldCrossValidation(const TArray<FString>& a_Arguments)
{
	if (a_Arguments.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Expected 1 argument (num folds) for KFold Cross validation test."));
	}
	else
	{
		int32 numFolds = FCString::Atoi(*a_Arguments[0]);
		if (numFolds > 0)
		{
			m_SuggestionDatabase->PerformKFoldCrossValidationTest(numFolds);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Expected an integer >0 for numfolds. Suggested number is 10."));
		}
	}
}

IMPLEMENT_MODULE(BIPluginImpl, Module)
