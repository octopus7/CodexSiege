#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FleetPlayerController.generated.h"

class ASailShip;

UCLASS()
class AGEOFSAILFLEET_API AFleetPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AFleetPlayerController();
    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaSeconds) override;
    virtual void SetupInputComponent() override;

    const TArray<TWeakObjectPtr<ASailShip>>& GetSelectedShips() const { return SelectedShips; }
    bool IsDragSelecting() const { return bDragSelecting; }
    FVector2D GetDragStart() const { return DragStart; }
    FVector2D GetDragCurrent() const { return DragCurrent; }
    void ClearSelection();
    void SelectAllFriendlyShips();

private:
    UPROPERTY()
    TArray<TWeakObjectPtr<ASailShip>> SelectedShips;

    bool bDragSelecting = false;
    FVector2D DragStart = FVector2D::ZeroVector;
    FVector2D DragCurrent = FVector2D::ZeroVector;

    void BeginSelection();
    void EndSelection();
    void IssueContextCommand();
    void SelectShip(ASailShip* Ship, bool bAdditive);
    void SelectShipsInRectangle(bool bAdditive);
};
