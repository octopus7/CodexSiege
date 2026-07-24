#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "SiegeUserSettings.generated.h"

UCLASS(Config=GameUserSettings, DefaultConfig)
class IRONWALLSIEGE_API USiegeUserSettings : public UGameUserSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, BlueprintReadWrite, Category="Siege|Resources")
    FName SelectedResourceSetId = TEXT("Prototype");
};
