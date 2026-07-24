#include "FleetBattleDirector.h"

#include "AgeOfSailFleet.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/World.h"
#include "FleetCameraPawn.h"
#include "FleetPlayerController.h"
#include "SailOceanActor.h"
#include "SailShip.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AFleetBattleDirector::AFleetBattleDirector()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
    SunLight->SetupAttachment(Root);
    SunLight->SetRelativeRotation(FRotator(-37.0f, -28.0f, 0.0f));
    SunLight->SetIntensity(6.5f);
    SunLight->SetLightColor(FLinearColor(1.0f, 0.78f, 0.55f));
    SunLight->bAtmosphereSunLight = true;

    SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
    SkyLight->SetupAttachment(Root);
    SkyLight->SetIntensity(1.1f);
    SkyLight->bRealTimeCapture = true;

    SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
    SkyAtmosphere->SetupAttachment(Root);

    HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
    HeightFog->SetupAttachment(Root);
    HeightFog->SetFogDensity(0.0015f);
    HeightFog->SetFogHeightFalloff(0.03f);
    HeightFog->SetFogMaxOpacity(0.55f);
    HeightFog->SetStartDistance(10000.0f);
    HeightFog->SetFogInscatteringColor(FLinearColor(0.12f, 0.28f, 0.34f));
}

void AFleetBattleDirector::BeginPlay()
{
    Super::BeginPlay();
    GetWorld()->SpawnActor<ASailOceanActor>(
        ASailOceanActor::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator);
}

void AFleetBattleDirector::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (BattleState != EFleetBattleState::InBattle)
    {
        return;
    }
    BattleElapsedTime += DeltaSeconds;
    WindHeadingDegrees =
        35.0f +
        FMath::Sin(BattleElapsedTime * 0.018f) * 22.0f +
        FMath::Sin(BattleElapsedTime * 0.006f) * 9.0f;
    WindStrength = FMath::Clamp(
        0.78f + FMath::Sin(BattleElapsedTime * 0.027f) * 0.14f,
        0.58f,
        1.0f);
    if (BattleElapsedTime >= 4.0f &&
        FParse::Param(FCommandLine::Get(), TEXT("FleetSmokeTest")))
    {
        UE_LOG(
            LogAgeOfSail,
            Display,
            TEXT("Fleet smoke test passed ships_blue=%d ships_red=%d wind=%.2f"),
            CountAfloat(0),
            CountAfloat(1),
            WindStrength);
        FPlatformMisc::RequestExit(false);
        return;
    }
    OutcomeCheckTime += DeltaSeconds;
    if (OutcomeCheckTime < 0.75f)
    {
        return;
    }
    OutcomeCheckTime = 0.0f;

    const int32 BlueAfloat = CountAfloat(0);
    const int32 RedAfloat = CountAfloat(1);
    if (BlueAfloat == 0)
    {
        BattleState = EFleetBattleState::RedVictory;
    }
    else if (RedAfloat == 0)
    {
        BattleState = EFleetBattleState::BlueVictory;
    }
}

FVector AFleetBattleDirector::GetWindDirection() const
{
    return FRotator(0.0f, WindHeadingDegrees, 0.0f).Vector();
}

void AFleetBattleDirector::StartBattle()
{
    if (BattleState != EFleetBattleState::AwaitingOrders)
    {
        return;
    }
    BattleState = EFleetBattleState::InBattle;
    SpawnFleet(0);
    SpawnFleet(1);

    if (AFleetPlayerController* Controller = Cast<AFleetPlayerController>(
        GetWorld()->GetFirstPlayerController()))
    {
        Controller->SelectAllFriendlyShips();
        if (AFleetCameraPawn* CameraPawn = Cast<AFleetCameraPawn>(Controller->GetPawn()))
        {
            // Open on the entire engagement instead of dropping the camera on
            // one fleet. Players can zoom in after reading both formations.
            CameraPawn->FocusLocation(FVector::ZeroVector);
        }
    }
    UE_LOG(LogAgeOfSail, Display, TEXT("Fleet battle started blue=3 red=4"));
}

int32 AFleetBattleDirector::CountAfloat(const int32 Team) const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<ASailShip>& ShipPtr : FleetShips)
    {
        const ASailShip* Ship = ShipPtr.Get();
        if (Ship && Ship->GetTeam() == Team && Ship->IsAfloat())
        {
            ++Count;
        }
    }
    return Count;
}

void AFleetBattleDirector::SpawnFleet(const int32 Team)
{
    if (Team == 0)
    {
        SpawnShip(0, 0, FVector(-16000.0f, 0.0f, 0.0f), FRotator(0.0f, 8.0f, 0.0f), true);
        SpawnShip(0, 1, FVector(-20500.0f, -6000.0f, 0.0f), FRotator(0.0f, 12.0f, 0.0f), false);
        SpawnShip(0, 2, FVector(-20500.0f, 6000.0f, 0.0f), FRotator(0.0f, 3.0f, 0.0f), false);
    }
    else
    {
        SpawnShip(1, 0, FVector(16000.0f, 0.0f, 0.0f), FRotator(0.0f, 185.0f, 0.0f), true);
        SpawnShip(1, 1, FVector(20500.0f, -6000.0f, 0.0f), FRotator(0.0f, 190.0f, 0.0f), false);
        SpawnShip(1, 2, FVector(20500.0f, 6000.0f, 0.0f), FRotator(0.0f, 179.0f, 0.0f), false);
        SpawnShip(1, 3, FVector(25500.0f, 0.0f, 0.0f), FRotator(0.0f, 184.0f, 0.0f), false);
    }
}

ASailShip* AFleetBattleDirector::SpawnShip(
    const int32 Team,
    const int32 FleetIndex,
    const FVector& Location,
    const FRotator& Rotation,
    const bool bFlagship)
{
    const FTransform SpawnTransform(Rotation, Location);
    ASailShip* Ship = GetWorld()->SpawnActorDeferred<ASailShip>(
        ASailShip::StaticClass(),
        SpawnTransform);
    if (Ship)
    {
        const FLinearColor Trim =
            Team == 0
                ? (bFlagship ? FLinearColor(0.92f, 0.62f, 0.08f) : FLinearColor(0.05f, 0.22f, 0.68f))
                : (bFlagship ? FLinearColor(0.95f, 0.68f, 0.12f) : FLinearColor(0.68f, 0.035f, 0.018f));
        Ship->ConfigureShip(Team, bFlagship, Trim, FleetIndex);
        Ship->SetGraphicsMode(GraphicsMode);
        UGameplayStatics::FinishSpawningActor(Ship, SpawnTransform);
        FleetShips.Add(Ship);
    }
    return Ship;
}
