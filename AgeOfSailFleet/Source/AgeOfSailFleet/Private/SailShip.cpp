#include "SailShip.h"
#include "ShipWakeActor.h"

#include "AgeOfSailFleet.h"
#include "Camera/CameraComponent.h"
#include "CannonballActor.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "FleetBattleDirector.h"
#include "FlipbookEffectActor.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace SailShipTuning
{
    constexpr float MaxForwardSpeed = 620.0f;
    constexpr float Acceleration = 105.0f;
    constexpr float TurnRate = 17.0f;
    constexpr float BroadsideRange = 4300.0f;
    constexpr float PreferredBroadsideRange = 3550.0f;
    constexpr float EmergencySeparationRange = 2450.0f;
    constexpr float ShipAvoidanceRange = 3600.0f;
    constexpr float WorldHalfExtent = 30000.0f;
    constexpr float OrderHalfExtent = 28200.0f;
}

ASailShip::ASailShip()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    SetRootComponent(CollisionBox);
    CollisionBox->SetBoxExtent(FVector(930.0f, 420.0f, 250.0f));
    CollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
    CollisionBox->SetCollisionObjectType(ECC_Pawn);
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Block);

    VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
    VisualRoot->SetupAttachment(CollisionBox);
    VisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -70.0f));

    SelectionRingRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SelectionRingRoot"));
    SelectionRingRoot->SetupAttachment(CollisionBox);
    SelectionRingRoot->SetRelativeLocation(FVector(0.0f, 0.0f, 18.0f));

    HullMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HullMesh"));
    HullMesh->SetupAttachment(VisualRoot);
    HullMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ShipSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("ShipSprite"));
    ShipSprite->SetupAttachment(VisualRoot);
    ShipSprite->SetRelativeLocation(FVector(0.0f, 0.0f, 470.0f));
    ShipSprite->SetRelativeScale3D(FVector(5.2f));
    ShipSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ShipSprite->bIsScreenSizeScaled = false;
    ShipSprite->SetHiddenInGame(true);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(CollisionBox);
    SpringArm->TargetArmLength = 1900.0f;
    SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 500.0f));
    SpringArm->SetRelativeRotation(FRotator(-15.0f, -155.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    CubeMesh = CubeFinder.Object;
    CylinderMesh = CylinderFinder.Object;
    SphereMesh = SphereFinder.Object;
}

void ASailShip::BeginPlay()
{
    Super::BeginPlay();
    BuildVisuals();
    UpdateCamera();
    if (AShipWakeActor* NewWake = GetWorld()->SpawnActor<AShipWakeActor>(
        AShipWakeActor::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator))
    {
        NewWake->FollowShip(this);
        WakeActor = NewWake;
    }
}

void ASailShip::ConfigureShip(
    const int32 InTeam,
    const bool bInPlayerFlagship,
    const FLinearColor& InTrimColor,
    const int32 InFleetIndex)
{
    Team = InTeam;
    bPlayerFlagship = bInPlayerFlagship;
    TrimColor = InTrimColor;
    ShipRate = bPlayerFlagship ? 1 : (InFleetIndex % 3 == 1 ? 2 : 3);
    GunCount = ShipRate == 1 ? 104 : (ShipRate == 2 ? 90 : 44);
    ShipClassName =
        ShipRate == 1 ? TEXT("First-rate Ship of the Line") :
        ShipRate == 2 ? TEXT("Second-rate Ship of the Line") :
        TEXT("Heavy Frigate");
    MaxHealth = ShipRate == 1 ? 1650.0f : (ShipRate == 2 ? 1380.0f : 1120.0f);
    Health = MaxHealth;

    static const TCHAR* BlueCaptains[] =
    {
        TEXT("Admiral Elias Ward"),
        TEXT("Captain Anne Mercer"),
        TEXT("Captain Tobias Reed"),
        TEXT("Captain Silas Drake")
    };
    static const TCHAR* RedCaptains[] =
    {
        TEXT("Admiral Lucien Voss"),
        TEXT("Captain Isolde Marat"),
        TEXT("Captain Hector Vale"),
        TEXT("Captain Rafael Cruz")
    };
    static const TCHAR* BlueShips[] =
    {
        TEXT("HMS Sovereign Wind"),
        TEXT("HMS Resolute"),
        TEXT("HMS Azure Crown"),
        TEXT("HMS Sea Lark")
    };
    static const TCHAR* RedShips[] =
    {
        TEXT("RNS Imperieuse"),
        TEXT("RNS Red Crown"),
        TEXT("RNS Vengeance"),
        TEXT("RNS Cerberus")
    };
    const int32 NameIndex = bPlayerFlagship ? 0 : FMath::Clamp(InFleetIndex, 1, 3);
    CaptainName = Team == 0 ? BlueCaptains[NameIndex] : RedCaptains[NameIndex];
    ShipName = Team == 0 ? BlueShips[NameIndex] : RedShips[NameIndex];

    if (bVisualsBuilt)
    {
        BuildVisuals();
    }
}

void ASailShip::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    BattleAge += DeltaSeconds;
    PortReload = FMath::Max(0.0f, PortReload - DeltaSeconds);
    StarboardReload = FMath::Max(0.0f, StarboardReload - DeltaSeconds);

    if (!IsAfloat())
    {
        TickSinking(DeltaSeconds);
        return;
    }

    TickAI(DeltaSeconds);
    TickMovement(DeltaSeconds);
    UpdateSelectionRing(DeltaSeconds);

    const float Bob = FMath::Sin(BattleAge * 0.82f + GetActorLocation().X * 0.0011f) * 14.0f;
    const float Pitch = FMath::Sin(BattleAge * 0.57f + GetActorLocation().Y * 0.0008f) * 1.2f;
    const float Roll = FMath::Sin(BattleAge * 0.91f + GetActorLocation().X * 0.0005f) * 2.1f;
    VisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, Bob - 70.0f));
    VisualRoot->SetRelativeRotation(FRotator(Pitch, 0.0f, Roll));
    Update2DSprite();
}

void ASailShip::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

float ASailShip::GetPortReloadRatio() const
{
    return 1.0f - FMath::Clamp(PortReload / ReloadDuration, 0.0f, 1.0f);
}

float ASailShip::GetStarboardReloadRatio() const
{
    return 1.0f - FMath::Clamp(StarboardReload / ReloadDuration, 0.0f, 1.0f);
}

void ASailShip::SetMoveCommand(const FVector& Destination)
{
    MoveDestination = Destination;
    MoveDestination.X = FMath::Clamp(
        MoveDestination.X,
        -SailShipTuning::OrderHalfExtent,
        SailShipTuning::OrderHalfExtent);
    MoveDestination.Y = FMath::Clamp(
        MoveDestination.Y,
        -SailShipTuning::OrderHalfExtent,
        SailShipTuning::OrderHalfExtent);
    MoveDestination.Z = GetActorLocation().Z;
    bHasMoveCommand = true;
    AttackTarget.Reset();
    AIBroadsideTarget.Reset();
}

void ASailShip::SetAttackTarget(ASailShip* Target)
{
    if (Target && Target != this && Target->GetTeam() != Team)
    {
        AttackTarget = Target;
        AIBroadsideTarget.Reset();
        bHasMoveCommand = false;
    }
}

void ASailShip::SetSelected(const bool bInSelected)
{
    bSelected = bInSelected && IsAfloat();
    for (UStaticMeshComponent* Marker : SelectionMarkers)
    {
        if (Marker)
        {
            Marker->SetVisibility(bSelected);
        }
    }
}

void ASailShip::SetGraphicsMode(const ESailGraphicsMode InGraphicsMode)
{
    GraphicsMode = InGraphicsMode;
}

void ASailShip::BuildVisuals()
{
    if (GraphicsMode == ESailGraphicsMode::TwoDimensional)
    {
        Build2DSpriteVisuals();
        return;
    }

    if (bVisualsBuilt)
    {
        if (TrimMaterial)
        {
            TrimMaterial->SetVectorParameterValue(TEXT("Color"), TrimColor);
        }
        if (SailMaterial)
        {
            SailMaterial->SetVectorParameterValue(
                TEXT("Color"),
                Team == 0 ? FLinearColor(0.025f, 0.09f, 0.38f) : FLinearColor(0.55f, 0.018f, 0.012f));
        }
        return;
    }
    bVisualsBuilt = true;

    UMaterialInterface* ShapeBase = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    UMaterialInterface* BlueSailAsset = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Game/Art/Materials/M_BlueFleetSail.M_BlueFleetSail"));
    UMaterialInterface* RedSailAsset = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Game/Art/Materials/M_RedFleetSail.M_RedFleetSail"));
    UMaterialInterface* HullPlankAsset = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Game/Art/Materials/M_HullOakPlanks.M_HullOakPlanks"));
    UMaterialInterface* DeckPlankAsset = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Game/Art/Materials/M_DeckOakPlanks.M_DeckOakPlanks"));

    WoodMaterial = UMaterialInstanceDynamic::Create(ShapeBase, this);
    DarkWoodMaterial = UMaterialInstanceDynamic::Create(ShapeBase, this);
    MetalMaterial = UMaterialInstanceDynamic::Create(ShapeBase, this);
    TrimMaterial = UMaterialInstanceDynamic::Create(ShapeBase, this);
    SailMaterial = UMaterialInstanceDynamic::Create(
        Team == 0 && BlueSailAsset ? BlueSailAsset :
        Team == 1 && RedSailAsset ? RedSailAsset : ShapeBase,
        this);

    WoodMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.29f, 0.11f, 0.035f));
    DarkWoodMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.085f, 0.026f, 0.009f));
    MetalMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.018f, 0.022f, 0.025f));
    TrimMaterial->SetVectorParameterValue(TEXT("Color"), TrimColor);
    SailMaterial->SetVectorParameterValue(
        TEXT("Color"),
        Team == 0 ? FLinearColor(0.025f, 0.09f, 0.38f) : FLinearColor(0.55f, 0.018f, 0.012f));

    const TCHAR* RatedShipPath =
        ShipRate == 1
            ? TEXT("/Game/Art/Ships/SM_Warship_FirstRate.SM_Warship_FirstRate")
            : ShipRate == 2
                ? TEXT("/Game/Art/Ships/SM_Warship_SecondRate.SM_Warship_SecondRate")
                : TEXT("/Game/Art/Ships/SM_Warship_Frigate.SM_Warship_Frigate");
    UStaticMesh* AuthoredShip = LoadObject<UStaticMesh>(nullptr, RatedShipPath);
    if (!AuthoredShip)
    {
        AuthoredShip = LoadObject<UStaticMesh>(
            nullptr,
            TEXT("/Game/Art/Ships/SM_AgeOfSailWarship.SM_AgeOfSailWarship"));
    }
    if (AuthoredShip)
    {
        UStaticMeshComponent* AuthoredComponent = NewObject<UStaticMeshComponent>(
            this,
            TEXT("BlenderAuthoredWarship"));
        AuthoredComponent->SetupAttachment(VisualRoot);
        AuthoredComponent->SetStaticMesh(AuthoredShip);
        AuthoredComponent->SetRelativeScale3D(FVector::OneVector);
        AuthoredComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        AuthoredComponent->RegisterComponent();
        DetailMeshes.Add(AuthoredComponent);

        for (const FName SlotName : AuthoredComponent->GetMaterialSlotNames())
        {
            const FString Slot = SlotName.ToString();
            const int32 SlotIndex = AuthoredComponent->GetMaterialIndex(SlotName);
            if (Slot.Contains(TEXT("AgedCanvas")) || Slot.Contains(TEXT("Pennant")))
            {
                AuthoredComponent->SetMaterial(SlotIndex, SailMaterial);
            }
            else if (Slot.Contains(TEXT("OakHull")) && HullPlankAsset)
            {
                AuthoredComponent->SetMaterial(SlotIndex, HullPlankAsset);
            }
            else if (Slot.Contains(TEXT("WeatheredDeck")) && DeckPlankAsset)
            {
                AuthoredComponent->SetMaterial(SlotIndex, DeckPlankAsset);
            }
            else if (Slot.Contains(TEXT("FlagshipGold")))
            {
                AuthoredComponent->SetMaterial(SlotIndex, TrimMaterial);
            }
        }
        CollisionBox->SetBoxExtent(
            ShipRate == 1
                ? FVector(1500.0f, 500.0f, 350.0f)
                : ShipRate == 2
                    ? FVector(1280.0f, 430.0f, 315.0f)
                    : FVector(1080.0f, 350.0f, 275.0f));
        BuildSelectionMarkers();
        return;
    }

    BuildHull();
    BuildDeckAndRailings();
    BuildGunDeck();
    BuildMastsAndRigging();
    BuildSails();
    BuildDecorations();
}

void ASailShip::Build2DSpriteVisuals()
{
    bVisualsBuilt = true;
    HullMesh->SetHiddenInGame(true);
    DirectionalSprites.Reset();

    static const TCHAR* DirectionNames[] =
    {
        TEXT("N"), TEXT("NE"), TEXT("E"), TEXT("SE"),
        TEXT("S"), TEXT("SW"), TEXT("W"), TEXT("NW")
    };
    const TCHAR* Faction = Team == 0 ? TEXT("Blue") : TEXT("Red");
    for (const TCHAR* Direction : DirectionNames)
    {
        const FString AssetPath = FString::Printf(
            TEXT("/Game/Art/Sprites/Ships/T_Ship_%s_%s.T_Ship_%s_%s"),
            Faction,
            Direction,
            Faction,
            Direction);
        DirectionalSprites.Add(LoadObject<UTexture2D>(nullptr, *AssetPath));
    }

    ShipSprite->SetHiddenInGame(false);
    ShipSprite->SetRelativeScale3D(FVector(
        ShipRate == 1 ? 5.8f : ShipRate == 2 ? 5.3f : 4.8f));
    ActiveSpriteDirection = INDEX_NONE;
    Update2DSprite();
    BuildSelectionMarkers();

    if (!DirectionalSprites.IsValidIndex(0) || !DirectionalSprites[0])
    {
        UE_LOG(
            LogAgeOfSail,
            Warning,
            TEXT("2D ship sprites are missing for faction %s; run import_game_assets.py."),
            Faction);
    }
}

void ASailShip::Update2DSprite()
{
    if (GraphicsMode != ESailGraphicsMode::TwoDimensional
        || !ShipSprite
        || DirectionalSprites.Num() != 8)
    {
        return;
    }

    const APlayerController* PlayerController = GetWorld()
        ? GetWorld()->GetFirstPlayerController()
        : nullptr;
    const APlayerCameraManager* CameraManager =
        PlayerController ? PlayerController->PlayerCameraManager : nullptr;
    if (!CameraManager)
    {
        return;
    }

    FVector ToCamera = CameraManager->GetCameraLocation() - GetActorLocation();
    ToCamera.Z = 0.0f;
    if (!ToCamera.Normalize())
    {
        return;
    }

    const float RelativeYaw = FMath::FindDeltaAngleDegrees(
        GetActorRotation().Yaw,
        ToCamera.Rotation().Yaw);
    const int32 DirectionIndex =
        (FMath::RoundToInt(RelativeYaw / 45.0f) + 8) % 8;
    if (DirectionIndex != ActiveSpriteDirection
        && DirectionalSprites.IsValidIndex(DirectionIndex)
        && DirectionalSprites[DirectionIndex])
    {
        ActiveSpriteDirection = DirectionIndex;
        ShipSprite->SetSprite(DirectionalSprites[DirectionIndex]);
    }
}

void ASailShip::BuildHull()
{
    struct FStation
    {
        float X;
        float Width;
        float DeckZ;
    };
    const FStation Stations[] =
    {
        {-1050.0f, 45.0f, 300.0f},
        {-860.0f, 280.0f, 285.0f},
        {-520.0f, 410.0f, 275.0f},
        {-80.0f, 470.0f, 270.0f},
        {390.0f, 455.0f, 290.0f},
        {760.0f, 385.0f, 355.0f},
        {980.0f, 270.0f, 430.0f}
    };

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FLinearColor> Colors;
    TArray<FVector2D> UVs;
    constexpr int32 RingCount = 6;
    for (const FStation& Station : Stations)
    {
        const float W = Station.Width;
        Vertices.Append({
            FVector(Station.X, 0.0f, -235.0f),
            FVector(Station.X, W * 0.72f, -105.0f),
            FVector(Station.X, W, Station.DeckZ - 25.0f),
            FVector(Station.X, -W, Station.DeckZ - 25.0f),
            FVector(Station.X, -W * 0.72f, -105.0f),
            FVector(Station.X, 0.0f, -235.0f)
        });
        for (int32 RingVertex = 0; RingVertex < RingCount; ++RingVertex)
        {
            Colors.Add(RingVertex == 2 || RingVertex == 3
                ? FLinearColor(0.34f, 0.13f, 0.035f)
                : FLinearColor(0.16f, 0.045f, 0.012f));
            UVs.Add(FVector2D(
                (Station.X + 1050.0f) / 2030.0f,
                static_cast<float>(RingVertex) / static_cast<float>(RingCount - 1)));
        }
    }
    for (int32 StationIndex = 0; StationIndex < UE_ARRAY_COUNT(Stations) - 1; ++StationIndex)
    {
        for (int32 RingVertex = 0; RingVertex < RingCount - 1; ++RingVertex)
        {
            const int32 A = StationIndex * RingCount + RingVertex;
            const int32 B = A + 1;
            const int32 C = A + RingCount;
            const int32 D = C + 1;
            Triangles.Append({A, C, B, B, C, D});
        }
    }

    HullMesh->CreateMeshSection_LinearColor(
        0,
        Vertices,
        Triangles,
        {},
        UVs,
        Colors,
        {},
        false);
    HullMesh->SetMaterial(0, WoodMaterial);

    AddBeam(TEXT("Keel"), FVector(-980.0f, 0.0f, -235.0f), FVector(1040.0f, 0.0f, -235.0f), 27.0f, DarkWoodMaterial);
    for (int32 Rib = -760; Rib <= 720; Rib += 185)
    {
        AddBox(
            *FString::Printf(TEXT("Rib_%d"), Rib),
            FVector(static_cast<float>(Rib), 0.0f, 45.0f),
            FVector(24.0f, 790.0f, 28.0f),
            FRotator::ZeroRotator,
            DarkWoodMaterial);
    }
}

void ASailShip::BuildDeckAndRailings()
{
    AddBox(TEXT("MainDeck"), FVector(-40.0f, 0.0f, 280.0f), FVector(1740.0f, 720.0f, 34.0f), FRotator::ZeroRotator, WoodMaterial);
    AddBox(TEXT("Forecastle"), FVector(-690.0f, 0.0f, 365.0f), FVector(560.0f, 640.0f, 45.0f), FRotator::ZeroRotator, WoodMaterial);
    AddBox(TEXT("Quarterdeck"), FVector(680.0f, 0.0f, 430.0f), FVector(570.0f, 610.0f, 48.0f), FRotator::ZeroRotator, WoodMaterial);
    AddBox(TEXT("SternCabin"), FVector(805.0f, 0.0f, 330.0f), FVector(345.0f, 590.0f, 300.0f), FRotator::ZeroRotator, DarkWoodMaterial);

    for (int32 Side : {-1, 1})
    {
        const float Y = static_cast<float>(Side) * 392.0f;
        AddBeam(
            *FString::Printf(TEXT("Gunwale_%d"), Side),
            FVector(-850.0f, Y, 360.0f),
            FVector(845.0f, Y, 430.0f),
            17.0f,
            TrimMaterial);
        for (int32 X = -780; X <= 800; X += 150)
        {
            AddBeam(
                *FString::Printf(TEXT("RailPost_%d_%d"), Side, X),
                FVector(static_cast<float>(X), Y, 330.0f),
                FVector(static_cast<float>(X), Y, 420.0f),
                8.0f,
                DarkWoodMaterial);
        }
    }
}

void ASailShip::BuildGunDeck()
{
    int32 GunIndex = 0;
    for (int32 Side : {-1, 1})
    {
        for (int32 X = -690; X <= 660; X += 225)
        {
            const float Y = static_cast<float>(Side) * 430.0f;
            AddBox(
                *FString::Printf(TEXT("GunPort_%02d"), GunIndex),
                FVector(static_cast<float>(X), Y, 110.0f),
                FVector(105.0f, 20.0f, 92.0f),
                FRotator::ZeroRotator,
                MetalMaterial);
            AddBeam(
                *FString::Printf(TEXT("Cannon_%02d"), GunIndex),
                FVector(static_cast<float>(X), static_cast<float>(Side) * 330.0f, 108.0f),
                FVector(static_cast<float>(X), static_cast<float>(Side) * 515.0f, 108.0f),
                18.0f,
                MetalMaterial);
            ++GunIndex;
        }
    }
}

void ASailShip::BuildMastsAndRigging()
{
    struct FMast
    {
        float X;
        float Height;
        float YardWidth;
    };
    const FMast Masts[] =
    {
        {-430.0f, 1370.0f, 650.0f},
        {120.0f, 1640.0f, 790.0f},
        {610.0f, 1210.0f, 540.0f}
    };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Masts); ++Index)
    {
        const FMast& Mast = Masts[Index];
        AddBeam(
            *FString::Printf(TEXT("Mast_%d"), Index),
            FVector(Mast.X, 0.0f, 250.0f),
            FVector(Mast.X, 0.0f, Mast.Height),
            Index == 1 ? 27.0f : 23.0f,
            DarkWoodMaterial);
        AddBeam(
            *FString::Printf(TEXT("LowerYard_%d"), Index),
            FVector(Mast.X, -Mast.YardWidth * 0.5f, Mast.Height * 0.68f),
            FVector(Mast.X, Mast.YardWidth * 0.5f, Mast.Height * 0.68f),
            14.0f,
            DarkWoodMaterial);
        AddBeam(
            *FString::Printf(TEXT("UpperYard_%d"), Index),
            FVector(Mast.X, -Mast.YardWidth * 0.38f, Mast.Height * 0.86f),
            FVector(Mast.X, Mast.YardWidth * 0.38f, Mast.Height * 0.86f),
            11.0f,
            DarkWoodMaterial);
    }

    AddBeam(TEXT("Bowsprit"), FVector(-910.0f, 0.0f, 310.0f), FVector(-1420.0f, 0.0f, 570.0f), 24.0f, DarkWoodMaterial);
    AddBeam(TEXT("ForeStay"), FVector(-1390.0f, 0.0f, 560.0f), FVector(-430.0f, 0.0f, 1350.0f), 4.0f, MetalMaterial);
    AddBeam(TEXT("MainStay"), FVector(-430.0f, 0.0f, 1350.0f), FVector(120.0f, 0.0f, 1620.0f), 4.0f, MetalMaterial);
    AddBeam(TEXT("MizzenStay"), FVector(120.0f, 0.0f, 1620.0f), FVector(610.0f, 0.0f, 1190.0f), 4.0f, MetalMaterial);
    for (int32 Side : {-1, 1})
    {
        const float Y = static_cast<float>(Side) * 380.0f;
        AddBeam(*FString::Printf(TEXT("MainShroud_%d"), Side), FVector(120.0f, Y, 330.0f), FVector(120.0f, 0.0f, 1580.0f), 4.0f, MetalMaterial);
        AddBeam(*FString::Printf(TEXT("ForeShroud_%d"), Side), FVector(-430.0f, Y, 325.0f), FVector(-430.0f, 0.0f, 1320.0f), 4.0f, MetalMaterial);
        AddBeam(*FString::Printf(TEXT("MizzenShroud_%d"), Side), FVector(610.0f, Y, 440.0f), FVector(610.0f, 0.0f, 1160.0f), 4.0f, MetalMaterial);
    }
}

void ASailShip::BuildSails()
{
    AddBox(TEXT("ForeCourse"), FVector(-438.0f, 0.0f, 780.0f), FVector(12.0f, 610.0f, 360.0f), FRotator(0.0f, 0.0f, 0.0f), SailMaterial);
    AddBox(TEXT("ForeTopsail"), FVector(-438.0f, 0.0f, 1125.0f), FVector(12.0f, 430.0f, 265.0f), FRotator(0.0f, 0.0f, 0.0f), SailMaterial);
    AddBox(TEXT("MainCourse"), FVector(112.0f, 0.0f, 890.0f), FVector(12.0f, 750.0f, 445.0f), FRotator(0.0f, 0.0f, 0.0f), SailMaterial);
    AddBox(TEXT("MainTopsail"), FVector(112.0f, 0.0f, 1310.0f), FVector(12.0f, 520.0f, 295.0f), FRotator(0.0f, 0.0f, 0.0f), SailMaterial);
    AddBox(TEXT("MizzenCourse"), FVector(603.0f, 0.0f, 790.0f), FVector(12.0f, 500.0f, 330.0f), FRotator(0.0f, 0.0f, 0.0f), SailMaterial);
}

void ASailShip::BuildDecorations()
{
    AddSphere(TEXT("Figurehead"), FVector(-1080.0f, 0.0f, 300.0f), 58.0f, TrimMaterial);
    AddBox(TEXT("SternGallery"), FVector(1000.0f, 0.0f, 390.0f), FVector(55.0f, 600.0f, 210.0f), FRotator::ZeroRotator, TrimMaterial);
    AddBeam(TEXT("Rudder"), FVector(1005.0f, 0.0f, 150.0f), FVector(1060.0f, 0.0f, -170.0f), 38.0f, DarkWoodMaterial);

    if (bPlayerFlagship)
    {
        AddBox(TEXT("FlagshipUpperGallery"), FVector(955.0f, 0.0f, 555.0f), FVector(120.0f, 650.0f, 65.0f), FRotator::ZeroRotator, TrimMaterial);
        AddBeam(TEXT("FlagshipCrownRail"), FVector(910.0f, -325.0f, 610.0f), FVector(910.0f, 325.0f, 610.0f), 15.0f, TrimMaterial);
        AddSphere(TEXT("FlagshipFigureheadCrown"), FVector(-1118.0f, 0.0f, 374.0f), 39.0f, TrimMaterial);
        AddBeam(TEXT("FlagshipPennant"), FVector(120.0f, 0.0f, 1620.0f), FVector(120.0f, 0.0f, 1785.0f), 7.0f, TrimMaterial);
        for (int32 Side : {-1, 1})
        {
            for (int32 Ornament = 0; Ornament < 3; ++Ornament)
            {
                AddSphere(
                    *FString::Printf(TEXT("FlagshipGalleryOrnament_%d_%d"), Side, Ornament),
                    FVector(1018.0f, static_cast<float>(Side) * (110.0f + Ornament * 92.0f), 480.0f),
                    26.0f,
                    TrimMaterial);
            }
        }
    }

    BuildSelectionMarkers();
}

void ASailShip::BuildSelectionMarkers()
{
    if (!SelectionMarkers.IsEmpty() || !CubeMesh || !SelectionRingRoot)
    {
        return;
    }

    UMaterialInterface* RingMaterialAsset = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Game/Art/Materials/M_ShipWake.M_ShipWake"));
    if (!RingMaterialAsset)
    {
        RingMaterialAsset = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    }
    SelectionMaterial = UMaterialInstanceDynamic::Create(
        RingMaterialAsset,
        this);
    if (SelectionMaterial)
    {
        SelectionMaterial->SetVectorParameterValue(
            TEXT("Color"),
            FLinearColor(0.12f, 1.4f, 4.0f, 1.0f));
    }

    constexpr int32 DashCount = 24;
    const float Radius =
        ShipRate == 1 ? 1780.0f : (ShipRate == 2 ? 1540.0f : 1320.0f);
    const float DashLength =
        ShipRate == 1 ? 255.0f : (ShipRate == 2 ? 225.0f : 200.0f);
    for (int32 DashIndex = 0; DashIndex < DashCount; ++DashIndex)
    {
        const float AngleDegrees =
            static_cast<float>(DashIndex) * (360.0f / DashCount);
        const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
        UStaticMeshComponent* Marker = NewObject<UStaticMeshComponent>(
            this,
            *FString::Printf(TEXT("SelectionRingDash_%02d"), DashIndex));
        Marker->SetupAttachment(SelectionRingRoot);
        Marker->SetStaticMesh(CubeMesh);
        Marker->SetRelativeLocation(FVector(
            FMath::Cos(AngleRadians) * Radius,
            FMath::Sin(AngleRadians) * Radius,
            0.0f));
        Marker->SetRelativeRotation(FRotator(
            0.0f,
            AngleDegrees + 90.0f,
            0.0f));
        Marker->SetRelativeScale3D(
            FVector(DashLength, 48.0f, 10.0f) / 100.0f);
        Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Marker->SetCastShadow(false);
        Marker->SetBoundsScale(2.0f);
        if (SelectionMaterial)
        {
            Marker->SetMaterial(0, SelectionMaterial);
        }
        Marker->SetVisibility(bSelected && IsAfloat());
        Marker->RegisterComponent();
        DetailMeshes.Add(Marker);
        SelectionMarkers.Add(Marker);
    }
}

void ASailShip::UpdateSelectionRing(const float DeltaSeconds)
{
    if (!SelectionRingRoot || !bSelected || !IsAfloat())
    {
        return;
    }

    SelectionRingRoot->AddRelativeRotation(
        FRotator(0.0f, 34.0f * DeltaSeconds, 0.0f));
}

UStaticMeshComponent* ASailShip::AddBox(
    const FName Name,
    const FVector& Location,
    const FVector& Size,
    const FRotator& Rotation,
    UMaterialInstanceDynamic* Material)
{
    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, Name);
    Component->SetupAttachment(VisualRoot);
    Component->SetStaticMesh(CubeMesh);
    Component->SetRelativeLocation(Location);
    Component->SetRelativeRotation(Rotation);
    Component->SetRelativeScale3D(Size / 100.0f);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetMaterial(0, Material);
    Component->RegisterComponent();
    DetailMeshes.Add(Component);
    return Component;
}

UStaticMeshComponent* ASailShip::AddBeam(
    const FName Name,
    const FVector& Start,
    const FVector& End,
    const float Radius,
    UMaterialInstanceDynamic* Material)
{
    const FVector Direction = End - Start;
    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, Name);
    Component->SetupAttachment(VisualRoot);
    Component->SetStaticMesh(CylinderMesh);
    Component->SetRelativeLocation((Start + End) * 0.5f);
    Component->SetRelativeRotation(FQuat::FindBetweenNormals(FVector::UpVector, Direction.GetSafeNormal()).Rotator());
    Component->SetRelativeScale3D(FVector(Radius / 50.0f, Radius / 50.0f, Direction.Size() / 100.0f));
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetMaterial(0, Material);
    Component->RegisterComponent();
    DetailMeshes.Add(Component);
    return Component;
}

UStaticMeshComponent* ASailShip::AddSphere(
    const FName Name,
    const FVector& Location,
    const float Radius,
    UMaterialInstanceDynamic* Material)
{
    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, Name);
    Component->SetupAttachment(VisualRoot);
    Component->SetStaticMesh(SphereMesh);
    Component->SetRelativeLocation(Location);
    Component->SetRelativeScale3D(FVector(Radius / 50.0f));
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetMaterial(0, Material);
    Component->RegisterComponent();
    DetailMeshes.Add(Component);
    return Component;
}

void ASailShip::FireBroadside(const int32 Side)
{
    if (!IsAfloat() || (Side < 0 ? PortReload : StarboardReload) > 0.0f)
    {
        return;
    }

    const FVector LateralDirection = GetActorRightVector() * static_cast<float>(Side);
    const int32 BroadsideGunCount = ShipRate == 1 ? 8 : (ShipRate == 2 ? 7 : 6);
    for (int32 Gun = 0; Gun < BroadsideGunCount; ++Gun)
    {
        const float AlongDeck =
            -660.0f +
            static_cast<float>(Gun) *
            (1320.0f / static_cast<float>(BroadsideGunCount - 1));
        const FVector Start =
            GetActorLocation() +
            GetActorForwardVector() * AlongDeck +
            GetActorRightVector() * static_cast<float>(Side) * 455.0f +
            FVector(0.0f, 0.0f, 150.0f);
        const FRotator Spread(
            FMath::FRandRange(-1.5f, 3.0f),
            FMath::FRandRange(-4.0f, 4.0f),
            0.0f);
        const FVector ShotDirection = Spread.RotateVector(LateralDirection).GetSafeNormal();

        if (ACannonballActor* Ball = GetWorld()->SpawnActor<ACannonballActor>(
            ACannonballActor::StaticClass(),
            Start,
            ShotDirection.Rotation()))
        {
            Ball->Launch(
                this,
                ShotDirection * FMath::FRandRange(2350.0f, 2650.0f) +
                GetActorForwardVector() * CurrentSpeed +
                FVector(0.0f, 0.0f, FMath::FRandRange(300.0f, 430.0f)),
                ShipRate == 1 ? 55.0f : 48.0f);
        }
        if (Gun % 2 == 0)
        {
            if (AFlipbookEffectActor* Effect = GetWorld()->SpawnActor<AFlipbookEffectActor>(
                AFlipbookEffectActor::StaticClass(),
                Start + LateralDirection * 35.0f,
                LateralDirection.Rotation()))
            {
                const float GradeScale = ShipRate == 1 ? 4.2f : (ShipRate == 2 ? 3.7f : 3.25f);
                Effect->PlayEffect(ESailFlipbookEffect::CannonMuzzle, GradeScale, 0.95f);
            }
        }
    }

    if (Side < 0)
    {
        PortReload = ReloadDuration;
    }
    else
    {
        StarboardReload = ReloadDuration;
    }
    UE_LOG(
        LogAgeOfSail,
        Display,
        TEXT("Broadside fired team=%d side=%s guns=%d captain=%s"),
        Team,
        Side < 0 ? TEXT("port") : TEXT("starboard"),
        BroadsideGunCount,
        *CaptainName);
}

void ASailShip::ReceiveCannonImpact(
    const float Damage,
    ASailShip* Attacker,
    const FVector& ImpactPoint)
{
    (void)ImpactPoint;
    if (!IsAfloat() || !Attacker || Attacker->GetTeam() == Team)
    {
        return;
    }

    Health = FMath::Max(0.0f, Health - Damage);
    AttackTarget = Attacker;
    if (AFlipbookEffectActor* Effect = GetWorld()->SpawnActor<AFlipbookEffectActor>(
        AFlipbookEffectActor::StaticClass(),
        ImpactPoint,
        FRotator::ZeroRotator))
    {
        Effect->PlayEffect(ESailFlipbookEffect::HullImpact, 4.5f, 1.05f);
    }
    if (Health <= 0.0f)
    {
        bSinking = true;
        bHasMoveCommand = false;
        AttackTarget.Reset();
        AIBroadsideTarget.Reset();
        SailSetting = 0.0f;
        SteeringInput = 0.0f;
        SinkRollDirection = FMath::RandBool() ? 1.0f : -1.0f;
        SinkPitchDirection = FMath::RandBool() ? 1.0f : -1.0f;
        SetSelected(false);
        CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        for (int32 Burst = 0; Burst < 3; ++Burst)
        {
            const FVector BurstLocation =
                GetActorLocation()
                + GetActorForwardVector() * FMath::FRandRange(-650.0f, 650.0f)
                + GetActorRightVector() * FMath::FRandRange(-260.0f, 260.0f)
                + FVector(0.0f, 0.0f, FMath::FRandRange(90.0f, 280.0f));
            if (AFlipbookEffectActor* DestructionEffect =
                GetWorld()->SpawnActor<AFlipbookEffectActor>(
                    AFlipbookEffectActor::StaticClass(),
                    BurstLocation,
                    FRotator::ZeroRotator))
            {
                DestructionEffect->PlayEffect(
                    ESailFlipbookEffect::HullImpact,
                    ShipRate == 1 ? 6.0f : 5.2f,
                    1.35f);
            }
        }
        UE_LOG(LogAgeOfSail, Display, TEXT("Ship sunk team=%d captain=%s"), Team, *CaptainName);
    }
}

void ASailShip::TickMovement(const float DeltaSeconds)
{
    FVector WindDirection = FVector(0.82f, 0.57f, 0.0f).GetSafeNormal();
    float WindStrength = 0.82f;
    for (TActorIterator<AFleetBattleDirector> It(GetWorld()); It; ++It)
    {
        WindDirection = It->GetWindDirection();
        WindStrength = It->GetWindStrength();
        break;
    }
    const float WindEfficiency = FMath::Clamp(
        0.35f + 0.65f * (1.0f - FMath::Abs(FVector::DotProduct(GetActorForwardVector(), -WindDirection))),
        0.32f,
        1.0f);
    const float TargetSpeed =
        SailSetting *
        SailShipTuning::MaxForwardSpeed *
        WindEfficiency *
        WindStrength *
        (ShipRate == 1 ? 0.82f : (ShipRate == 2 ? 0.94f : 1.12f));
    CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, TargetSpeed, DeltaSeconds, SailShipTuning::Acceleration);

    const float SteeringAuthority = FMath::Clamp(CurrentSpeed / 260.0f, 0.18f, 1.0f);
    AddActorWorldRotation(FRotator(
        0.0f,
        SteeringInput *
            SailShipTuning::TurnRate *
            SteeringAuthority *
            (ShipRate == 1 ? 0.78f : (ShipRate == 2 ? 0.92f : 1.22f)) *
            DeltaSeconds,
        0.0f));

    FVector NewLocation = GetActorLocation() + GetActorForwardVector() * CurrentSpeed * DeltaSeconds;
    NewLocation.X = FMath::Clamp(
        NewLocation.X,
        -SailShipTuning::WorldHalfExtent,
        SailShipTuning::WorldHalfExtent);
    NewLocation.Y = FMath::Clamp(
        NewLocation.Y,
        -SailShipTuning::WorldHalfExtent,
        SailShipTuning::WorldHalfExtent);
    NewLocation.Z = 0.0f;
    SetActorLocation(NewLocation, true);
}

void ASailShip::TickAI(const float DeltaSeconds)
{
    AIThinkTime -= DeltaSeconds;

    ASailShip* Target = AttackTarget.Get();
    if (!Target || !Target->IsAfloat() || Target->GetTeam() == Team)
    {
        AttackTarget.Reset();
        Target = nullptr;
    }

    if (!Target && !bHasMoveCommand && AIThinkTime <= 0.0f)
    {
        Target = FindNearestEnemy();
        AttackTarget = Target;
        AIThinkTime = 0.35f;
    }

    if (bHasMoveCommand)
    {
        const FVector Offset = MoveDestination - GetActorLocation();
        if (Offset.Size2D() < 260.0f)
        {
            bHasMoveCommand = false;
            SailSetting = 0.15f;
            SteeringInput = 0.0f;
        }
        else
        {
            SailSetting = FMath::Clamp(Offset.Size2D() / 1600.0f, 0.38f, 1.0f);
            const FVector Separation =
                ComputeSeparationVector(SailShipTuning::ShipAvoidanceRange);
            AimAIAt(
                Offset.GetSafeNormal2D() + Separation * 1.45f,
                DeltaSeconds);
        }
        return;
    }

    if (!Target)
    {
        SteeringInput = 0.0f;
        SailSetting = 0.45f;
        return;
    }

    const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    const float Distance = ToTarget.Size2D();
    const FVector RadialDirection = ToTarget.GetSafeNormal2D();

    if (AIBroadsideTarget.Get() != Target)
    {
        AIBroadsideTarget = Target;
        const FVector ClockwiseTangent =
            FVector::CrossProduct(FVector::UpVector, RadialDirection).GetSafeNormal2D();
        const float ClockwiseAlignment =
            FVector::DotProduct(GetActorForwardVector(), ClockwiseTangent);
        const float CounterAlignment =
            FVector::DotProduct(GetActorForwardVector(), -ClockwiseTangent);
        if (FMath::IsNearlyEqual(ClockwiseAlignment, CounterAlignment, 0.05f))
        {
            AIBroadsideSign = ((GetUniqueID() + Team) & 1) == 0 ? 1.0f : -1.0f;
        }
        else
        {
            AIBroadsideSign = ClockwiseAlignment >= CounterAlignment ? 1.0f : -1.0f;
        }
    }

    const FVector TangentDirection =
        FVector::CrossProduct(FVector::UpVector, RadialDirection).GetSafeNormal2D()
        * AIBroadsideSign;
    const FVector Separation =
        ComputeSeparationVector(SailShipTuning::ShipAvoidanceRange);
    FVector DesiredHeading = TangentDirection;

    if (Distance < SailShipTuning::EmergencySeparationRange)
    {
        const float RetreatWeight = FMath::GetMappedRangeValueClamped(
            FVector2D(SailShipTuning::EmergencySeparationRange, 1200.0f),
            FVector2D(0.9f, 2.2f),
            Distance);
        DesiredHeading =
            TangentDirection * 0.7f
            - RadialDirection * RetreatWeight
            + Separation * 2.6f;
        SailSetting = 0.92f;
    }
    else
    {
        const float RangeCorrection = FMath::Clamp(
            (Distance - SailShipTuning::PreferredBroadsideRange) / 1250.0f,
            -0.82f,
            0.82f);
        DesiredHeading =
            TangentDirection
            + RadialDirection * RangeCorrection
            + Separation * 1.6f;
        SailSetting =
            Distance > SailShipTuning::BroadsideRange ? 0.82f : 0.52f;
    }

    AimAIAt(DesiredHeading, DeltaSeconds);

    const float LocalRight =
        FVector::DotProduct(GetActorRightVector(), RadialDirection);
    if (Distance < SailShipTuning::BroadsideRange && FMath::Abs(LocalRight) > 0.68f)
    {
        FireBroadside(LocalRight >= 0.0f ? 1 : -1);
    }
}

void ASailShip::TickSinking(const float DeltaSeconds)
{
    if (!bSinking)
    {
        return;
    }
    SinkTime += DeltaSeconds;
    CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, 0.0f, DeltaSeconds, 72.0f);
    const float SinkSpeed = 42.0f + SinkTime * 20.0f;
    AddActorWorldOffset(
        GetActorForwardVector() * CurrentSpeed * 0.32f * DeltaSeconds
        + FVector(0.0f, 0.0f, -SinkSpeed * DeltaSeconds));
    AddActorWorldRotation(FRotator(
        SinkPitchDirection * (1.5f + SinkTime * 0.16f) * DeltaSeconds,
        0.35f * DeltaSeconds,
        SinkRollDirection * (4.5f + SinkTime * 0.48f) * DeltaSeconds));
    if (SinkTime > 14.0f)
    {
        SetActorHiddenInGame(true);
        SetActorTickEnabled(false);
    }
}

ASailShip* ASailShip::FindNearestEnemy() const
{
    ASailShip* Best = nullptr;
    float BestDistance = BIG_NUMBER;
    for (TActorIterator<ASailShip> It(GetWorld()); It; ++It)
    {
        ASailShip* Candidate = *It;
        if (!Candidate || Candidate == this || !Candidate->IsAfloat() || Candidate->GetTeam() == Team)
        {
            continue;
        }
        const float Distance = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            Best = Candidate;
        }
    }
    return Best;
}

FVector ASailShip::ComputeSeparationVector(const float Radius) const
{
    FVector Separation = FVector::ZeroVector;
    if (Radius <= KINDA_SMALL_NUMBER)
    {
        return Separation;
    }

    for (TActorIterator<ASailShip> It(GetWorld()); It; ++It)
    {
        const ASailShip* Other = *It;
        if (!Other || Other == this || !Other->IsAfloat())
        {
            continue;
        }

        const FVector Away = GetActorLocation() - Other->GetActorLocation();
        const float Distance = Away.Size2D();
        if (Distance <= KINDA_SMALL_NUMBER || Distance >= Radius)
        {
            continue;
        }

        const float Weight = FMath::Square(1.0f - Distance / Radius);
        Separation += Away.GetSafeNormal2D() * Weight;
    }
    return Separation.GetClampedToMaxSize(1.0f);
}

void ASailShip::AimAIAt(const FVector& DesiredDirection, const float DeltaSeconds)
{
    (void)DeltaSeconds;
    if (DesiredDirection.IsNearlyZero())
    {
        SteeringInput = 0.0f;
        return;
    }
    const float DesiredYaw = DesiredDirection.Rotation().Yaw;
    const float DeltaYaw = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, DesiredYaw);
    SteeringInput = FMath::Clamp(DeltaYaw / 35.0f, -1.0f, 1.0f);
}

void ASailShip::ChangeSails(const float Value)
{
    SailSetting = FMath::Clamp(SailSetting + Value * GetWorld()->GetDeltaSeconds() * 0.45f, 0.0f, 1.0f);
}

void ASailShip::Steer(const float Value)
{
    SteeringInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void ASailShip::LookYaw(const float Value)
{
    CameraYaw += Value;
    UpdateCamera();
}

void ASailShip::LookPitch(const float Value)
{
    CameraPitch = FMath::Clamp(CameraPitch + Value, -50.0f, -4.0f);
    UpdateCamera();
}

void ASailShip::ZoomCamera(const float Value)
{
    CameraDistance = FMath::Clamp(CameraDistance - Value * 150.0f, 900.0f, 3400.0f);
    UpdateCamera();
}

void ASailShip::FirePort()
{
    FireBroadside(-1);
}

void ASailShip::FireStarboard()
{
    FireBroadside(1);
}

void ASailShip::SetFullSails()
{
    SailSetting = 1.0f;
}

void ASailShip::ResetCamera()
{
    CameraYaw = 0.0f;
    CameraPitch = -15.0f;
    CameraDistance = 1900.0f;
    UpdateCamera();
}

void ASailShip::UpdateCamera()
{
    SpringArm->TargetArmLength = CameraDistance;
    SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw - 155.0f, 0.0f));
}
