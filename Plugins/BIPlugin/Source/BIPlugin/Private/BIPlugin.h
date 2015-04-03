#pragma once

#include "ModuleManager.h"

class BIPluginImpl : public IModuleInterface
{
public:
	void StartupModule();
	void ShutdownModule();
};
