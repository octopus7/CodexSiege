#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SailGameMode.generated.h"

class AFleetBattleDirector;
class USailFleetHUDWidget;
class USailTitleScreenWidget;

UCLASS()
class AGEOFSAILFLEET_API ASailGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASailGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    AFleetBattleDirector* GetBattleDirector() const { return BattleDirector.Get(); }

private:
    UPROPERTY()
    TWeakObjectPtr<AFleetBattleDirector> BattleDirector;

    UPROPERTY()
    TObjectPtr<USailFleetHUDWidget> FleetHUDWidget;

    UPROPERTY()
    TObjectPtr<USailTitleScreenWidget> TitleScreenWidget;

    UFUNCTION()
    void HandleFleetDeparture();

    void RefreshFleetHUD();
};
