#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SiegePlayerController.generated.h"

class USiegeTitleWidget;

UCLASS()
class IRONWALLSIEGE_API ASiegePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<USiegeTitleWidget> TitleWidget;

    void ToggleMenu();
};
