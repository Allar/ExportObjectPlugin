// Copyright 2015 Michael Allar. All Rights Reserved.

#pragma once
#include "StereoscopicRenderBlueprintLibrary.Generated.h"

// Library of helper functions for UMGEx Widgets
UCLASS()
class EXPORTOBJECTPLUGIN_API UStereoscopicRenderBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:


	UFUNCTION(BlueprintCallable, Category = "Online")
	static bool ExportObjectToPath(class UObject* ObjectToExport, FString SaveFileName, FString FileNameAppendage);


};
