// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlueprintExtensionTest.generated.h"

/**
 * 
 */
UCLASS()
class PLUGINPROJECT_API UBlueprintExtensionTest : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, meta = (FriendlyName = "Test Extension Item", CompactNodeTitle = "Test", Keywords = "Test Extension Blueprint"), Category = Game)
	static FText HelloWorld();
};
