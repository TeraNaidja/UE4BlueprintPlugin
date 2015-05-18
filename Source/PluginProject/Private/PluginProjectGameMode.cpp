// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PluginProject.h"
#include "PluginProjectGameMode.h"
#include "PluginProjectCharacter.h"

APluginProjectGameMode::APluginProjectGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/SideScroller/Blueprints/SideScrollerCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
