#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SiegeGameMode.generated.h"

UCLASS()
class IRONWALLSIEGE_API ASiegeGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASiegeGameMode();
    virtual void BeginPlay() override;
};
