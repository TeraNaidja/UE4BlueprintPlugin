#include "BIPluginPrivatePCH.h"

#include "BIPlugin.h"

namespace
{
	void TestFunction()
	{
		UE_LOG(LogTemp, Warning, TEXT("This is a test debug Command"));

		for (TObjectIterator<UBlueprint> blueprintIt; blueprintIt; ++blueprintIt)
		{
			UBlueprint* blueprint = *blueprintIt;
			
			UE_LOG(LogTemp, Warning, TEXT("Friendly Name: %s"), *blueprint->GetFriendlyName());
		}
	} 
}

IConsoleCommand* TestFunctionConsoleCmd;

void BIPluginImpl::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("BIPlugin Startup"));

	TestFunctionConsoleCmd = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("BIPlugin_Test"),
		TEXT("Test Commands yo"),
		FConsoleCommandDelegate::CreateStatic(TestFunction),
		ECVF_Default
		);
}

void BIPluginImpl::ShutdownModule()
{
	UE_LOG(LogTemp, Warning, TEXT("BIPlugin Shutdown"));
	IConsoleManager::Get().UnregisterConsoleObject(TestFunctionConsoleCmd);
}

IMPLEMENT_MODULE(BIPluginImpl, Module)