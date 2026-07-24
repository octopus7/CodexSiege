#include "FleetPlayerController.h"

#include "EngineUtils.h"
#include "FleetCameraPawn.h"
#include "FleetMoveCommandMarker.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "SailShip.h"

namespace FleetTimeScale
{
    constexpr float Default = 1.0f;
    constexpr float Minimum = 0.5f;
    constexpr float Maximum = 8.0f;
    constexpr float DecreaseStep = 0.5f;
    constexpr float IncreaseStep = 1.5f;
}

AFleetPlayerController::AFleetPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AFleetPlayerController::BeginPlay()
{
    Super::BeginPlay();
    ApplyTimeScale(FleetTimeScale::Default);
    ApplyStrategyInputMode();
}

void AFleetPlayerController::ApplyStrategyInputMode()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
}

void AFleetPlayerController::PlayerTick(const float DeltaSeconds)
{
    Super::PlayerTick(DeltaSeconds);
    if (bDragSelecting)
    {
        GetMousePosition(DragCurrent.X, DragCurrent.Y);
    }
}

void AFleetPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("Select"), IE_Pressed, this, &AFleetPlayerController::BeginSelection);
    InputComponent->BindAction(TEXT("Select"), IE_Released, this, &AFleetPlayerController::EndSelection);
    InputComponent->BindAction(TEXT("ContextCommand"), IE_Pressed, this, &AFleetPlayerController::IssueContextCommand);
    InputComponent->BindAction(TEXT("SelectAll"), IE_Pressed, this, &AFleetPlayerController::SelectAllFriendlyShips);
    InputComponent->BindAction(TEXT("ToggleFreeFlight"), IE_Pressed, this, &AFleetPlayerController::ToggleFreeFlightMode);
    InputComponent->BindAction(TEXT("ResetTimeScale"), IE_Pressed, this, &AFleetPlayerController::ResetTimeScale);
    InputComponent->BindAction(TEXT("DecreaseTimeScale"), IE_Pressed, this, &AFleetPlayerController::DecreaseTimeScale);
    InputComponent->BindAction(TEXT("IncreaseTimeScale"), IE_Pressed, this, &AFleetPlayerController::IncreaseTimeScale);
}

void AFleetPlayerController::ClearSelection()
{
    for (const TWeakObjectPtr<ASailShip>& ShipPtr : SelectedShips)
    {
        if (ASailShip* Ship = ShipPtr.Get())
        {
            Ship->SetSelected(false);
        }
    }
    SelectedShips.Reset();
}

void AFleetPlayerController::SelectAllFriendlyShips()
{
    if (IsFreeFlightActive())
    {
        return;
    }
    ClearSelection();
    for (TActorIterator<ASailShip> It(GetWorld()); It; ++It)
    {
        ASailShip* Ship = *It;
        if (Ship && Ship->IsAfloat() && Ship->GetTeam() == 0)
        {
            Ship->SetSelected(true);
            SelectedShips.Add(Ship);
        }
    }
}

void AFleetPlayerController::BeginSelection()
{
    if (IsFreeFlightActive())
    {
        return;
    }
    bDragSelecting = GetMousePosition(DragStart.X, DragStart.Y);
    DragCurrent = DragStart;
}

void AFleetPlayerController::EndSelection()
{
    if (IsFreeFlightActive())
    {
        bDragSelecting = false;
        return;
    }
    if (!bDragSelecting)
    {
        return;
    }
    GetMousePosition(DragCurrent.X, DragCurrent.Y);
    bDragSelecting = false;

    const bool bAdditive =
        IsInputKeyDown(EKeys::LeftShift) ||
        IsInputKeyDown(EKeys::RightShift);
    if (FVector2D::Distance(DragStart, DragCurrent) >= 12.0f)
    {
        SelectShipsInRectangle(bAdditive);
        return;
    }

    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Pawn, true, Hit);
    ASailShip* Ship = Cast<ASailShip>(Hit.GetActor());
    if (Ship && Ship->GetTeam() == 0 && Ship->IsAfloat())
    {
        SelectShip(Ship, bAdditive);
    }
    else if (!bAdditive)
    {
        ClearSelection();
    }
}

void AFleetPlayerController::IssueContextCommand()
{
    if (IsFreeFlightActive())
    {
        return;
    }
    SelectedShips.RemoveAll([](const TWeakObjectPtr<ASailShip>& ShipPtr)
    {
        return !ShipPtr.IsValid() || !ShipPtr->IsAfloat();
    });
    if (SelectedShips.IsEmpty())
    {
        return;
    }

    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Pawn, true, Hit);
    if (ASailShip* TargetShip = Cast<ASailShip>(Hit.GetActor()))
    {
        if (TargetShip->GetTeam() != 0 && TargetShip->IsAfloat())
        {
            for (const TWeakObjectPtr<ASailShip>& ShipPtr : SelectedShips)
            {
                if (ASailShip* Ship = ShipPtr.Get())
                {
                    Ship->SetAttackTarget(TargetShip);
                }
            }
            return;
        }
    }

    FVector RayOrigin;
    FVector RayDirection;
    if (!DeprojectMousePositionToWorld(RayOrigin, RayDirection))
    {
        return;
    }
    const FVector PlanePoint(0.0f, 0.0f, 0.0f);
    const FVector PlaneNormal = FVector::UpVector;
    const FVector CommandPoint = FMath::LinePlaneIntersection(
        RayOrigin,
        RayOrigin + RayDirection * 100000.0f,
        PlanePoint,
        PlaneNormal);

    const int32 ColumnCount = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(SelectedShips.Num())));
    bool bIssuedMoveCommand = false;
    for (int32 Index = 0; Index < SelectedShips.Num(); ++Index)
    {
        ASailShip* Ship = SelectedShips[Index].Get();
        if (!Ship)
        {
            continue;
        }
        const int32 Row = Index / ColumnCount;
        const int32 Column = Index % ColumnCount;
        const FVector FormationOffset(
            static_cast<float>(Row) * -5200.0f,
            (static_cast<float>(Column) - (ColumnCount - 1) * 0.5f) * 3400.0f,
            0.0f);
        Ship->SetMoveCommand(CommandPoint + FormationOffset);
        bIssuedMoveCommand = true;
    }

    if (bIssuedMoveCommand)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        GetWorld()->SpawnActor<AFleetMoveCommandMarker>(
            AFleetMoveCommandMarker::StaticClass(),
            FVector(CommandPoint.X, CommandPoint.Y, 0.0f),
            FRotator::ZeroRotator,
            SpawnParameters);
    }
}

void AFleetPlayerController::ToggleFreeFlightMode()
{
    AFleetCameraPawn* CameraPawn = Cast<AFleetCameraPawn>(GetPawn());
    if (!CameraPawn)
    {
        return;
    }

    CameraPawn->ToggleFreeFlight();
    bDragSelecting = false;
    if (CameraPawn->IsFreeFlightEnabled())
    {
        bShowMouseCursor = false;
        bEnableClickEvents = false;
        bEnableMouseOverEvents = false;
        FInputModeGameOnly InputMode;
        SetInputMode(InputMode);
    }
    else
    {
        ApplyStrategyInputMode();
    }
}

void AFleetPlayerController::ResetTimeScale()
{
    ApplyTimeScale(FleetTimeScale::Default);
}

void AFleetPlayerController::DecreaseTimeScale()
{
    ApplyTimeScale(CurrentTimeScale - FleetTimeScale::DecreaseStep);
}

void AFleetPlayerController::IncreaseTimeScale()
{
    ApplyTimeScale(CurrentTimeScale + FleetTimeScale::IncreaseStep);
}

void AFleetPlayerController::ApplyTimeScale(const float NewTimeScale)
{
    CurrentTimeScale = FMath::Clamp(
        NewTimeScale,
        FleetTimeScale::Minimum,
        FleetTimeScale::Maximum);
    UGameplayStatics::SetGlobalTimeDilation(this, CurrentTimeScale);
}

bool AFleetPlayerController::IsFreeFlightActive() const
{
    const AFleetCameraPawn* CameraPawn = Cast<AFleetCameraPawn>(GetPawn());
    return CameraPawn && CameraPawn->IsFreeFlightEnabled();
}

void AFleetPlayerController::SelectShip(ASailShip* Ship, const bool bAdditive)
{
    if (!Ship)
    {
        return;
    }
    if (!bAdditive)
    {
        ClearSelection();
    }
    if (!SelectedShips.Contains(Ship))
    {
        Ship->SetSelected(true);
        SelectedShips.Add(Ship);
    }
}

void AFleetPlayerController::SelectShipsInRectangle(const bool bAdditive)
{
    if (!bAdditive)
    {
        ClearSelection();
    }
    const FVector2D Minimum(
        FMath::Min(DragStart.X, DragCurrent.X),
        FMath::Min(DragStart.Y, DragCurrent.Y));
    const FVector2D Maximum(
        FMath::Max(DragStart.X, DragCurrent.X),
        FMath::Max(DragStart.Y, DragCurrent.Y));

    for (TActorIterator<ASailShip> It(GetWorld()); It; ++It)
    {
        ASailShip* Ship = *It;
        if (!Ship || !Ship->IsAfloat() || Ship->GetTeam() != 0)
        {
            continue;
        }
        FVector2D ScreenPosition;
        if (ProjectWorldLocationToScreen(Ship->GetActorLocation(), ScreenPosition, true) &&
            ScreenPosition.X >= Minimum.X &&
            ScreenPosition.X <= Maximum.X &&
            ScreenPosition.Y >= Minimum.Y &&
            ScreenPosition.Y <= Maximum.Y)
        {
            SelectShip(Ship, true);
        }
    }
}
