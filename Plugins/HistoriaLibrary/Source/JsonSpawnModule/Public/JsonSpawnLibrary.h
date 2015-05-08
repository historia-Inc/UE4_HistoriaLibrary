#pragma once

#include "JsonSpawnLibrary.generated.h"

UCLASS()
class UJsonSpawnLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="JsonSpawn")
	static JSONSPAWNMODULE_API TArray<AActor*> SpawnActorsFromJson(FString Json);
};
