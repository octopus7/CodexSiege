#include "SiegeAssetProxyActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "SiegeGameInstance.h"
#include "SiegeResourceSet.h"

namespace SiegeGeometry
{
    struct FMeshData
    {
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FLinearColor> Colors;
        TArray<FProcMeshTangent> Tangents;

        void AddVertex(
            const FVector& Position,
            const FVector& Normal,
            const FVector2D& UV,
            const FLinearColor& Color,
            const FVector& Tangent)
        {
            Vertices.Add(Position);
            Normals.Add(Normal);
            UVs.Add(UV);
            Colors.Add(Color);
            Tangents.Add(FProcMeshTangent(Tangent, false));
        }

        void AddQuad(
            const FVector& A,
            const FVector& B,
            const FVector& C,
            const FVector& D,
            const FVector& Normal,
            const FVector& Tangent,
            const FLinearColor& Color)
        {
            const int32 Base = Vertices.Num();
            AddVertex(A, Normal, FVector2D(0.0f, 1.0f), Color, Tangent);
            AddVertex(B, Normal, FVector2D(1.0f, 1.0f), Color, Tangent);
            AddVertex(C, Normal, FVector2D(1.0f, 0.0f), Color, Tangent);
            AddVertex(D, Normal, FVector2D(0.0f, 0.0f), Color, Tangent);
            Triangles.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
        }

        void AddBox(const FTransform& Transform, const FVector& Extent, const FLinearColor& Color)
        {
            const FVector P[8] =
            {
                FVector(-Extent.X, -Extent.Y, -Extent.Z),
                FVector( Extent.X, -Extent.Y, -Extent.Z),
                FVector( Extent.X,  Extent.Y, -Extent.Z),
                FVector(-Extent.X,  Extent.Y, -Extent.Z),
                FVector(-Extent.X, -Extent.Y,  Extent.Z),
                FVector( Extent.X, -Extent.Y,  Extent.Z),
                FVector( Extent.X,  Extent.Y,  Extent.Z),
                FVector(-Extent.X,  Extent.Y,  Extent.Z)
            };

            auto WP = [&Transform, &P](const int32 Index) { return Transform.TransformPosition(P[Index]); };
            auto WN = [&Transform](const FVector& N) { return Transform.TransformVectorNoScale(N).GetSafeNormal(); };

            AddQuad(WP(0), WP(3), WP(2), WP(1), WN(FVector::DownVector), WN(FVector::RightVector), Color);
            AddQuad(WP(4), WP(5), WP(6), WP(7), WN(FVector::UpVector), WN(FVector::ForwardVector), Color);
            AddQuad(WP(0), WP(1), WP(5), WP(4), WN(FVector(0, -1, 0)), WN(FVector::ForwardVector), Color);
            AddQuad(WP(1), WP(2), WP(6), WP(5), WN(FVector(1, 0, 0)), WN(FVector::RightVector), Color);
            AddQuad(WP(2), WP(3), WP(7), WP(6), WN(FVector(0, 1, 0)), WN(FVector(-1, 0, 0)), Color);
            AddQuad(WP(3), WP(0), WP(4), WP(7), WN(FVector(-1, 0, 0)), WN(FVector(0, -1, 0)), Color);
        }

        void AddBox(
            const FVector& Center,
            const FVector& Extent,
            const FLinearColor& Color,
            const FRotator& Rotation = FRotator::ZeroRotator)
        {
            AddBox(FTransform(Rotation, Center), Extent, Color);
        }

        void AddBeam(
            const FVector& Start,
            const FVector& End,
            const float HalfThickness,
            const FLinearColor& Color)
        {
            const FVector Direction = End - Start;
            const FVector Midpoint = (Start + End) * 0.5f;
            const FRotator Rotation = UKismetMathLibrary::MakeRotFromX(Direction);
            AddBox(Midpoint, FVector(Direction.Size() * 0.5f, HalfThickness, HalfThickness), Color, Rotation);
        }

        void AddCylinder(
            const FVector& Center,
            const float Radius,
            const float HalfHeight,
            const int32 Segments,
            const FLinearColor& Color,
            const FRotator& Rotation = FRotator::ZeroRotator)
        {
            const FTransform Transform(Rotation, Center);
            for (int32 Index = 0; Index < Segments; ++Index)
            {
                const float A0 = 2.0f * PI * static_cast<float>(Index) / static_cast<float>(Segments);
                const float A1 = 2.0f * PI * static_cast<float>(Index + 1) / static_cast<float>(Segments);
                const FVector L0(Radius * FMath::Cos(A0), Radius * FMath::Sin(A0), -HalfHeight);
                const FVector L1(Radius * FMath::Cos(A1), Radius * FMath::Sin(A1), -HalfHeight);
                const FVector U1(L1.X, L1.Y, HalfHeight);
                const FVector U0(L0.X, L0.Y, HalfHeight);
                const FVector SideNormal = Transform.TransformVectorNoScale(
                    FVector(FMath::Cos((A0 + A1) * 0.5f), FMath::Sin((A0 + A1) * 0.5f), 0.0f)).GetSafeNormal();
                const FVector SideTangent = Transform.TransformVectorNoScale(FVector(0, 0, 1)).GetSafeNormal();
                AddQuad(
                    Transform.TransformPosition(L0),
                    Transform.TransformPosition(L1),
                    Transform.TransformPosition(U1),
                    Transform.TransformPosition(U0),
                    SideNormal,
                    SideTangent,
                    Color);

                const int32 BottomBase = Vertices.Num();
                const FVector BottomNormal = Transform.TransformVectorNoScale(FVector::DownVector).GetSafeNormal();
                AddVertex(Transform.TransformPosition(FVector(0, 0, -HalfHeight)), BottomNormal, FVector2D(0.5f), Color, FVector::ForwardVector);
                AddVertex(Transform.TransformPosition(L1), BottomNormal, FVector2D(1, 0), Color, FVector::ForwardVector);
                AddVertex(Transform.TransformPosition(L0), BottomNormal, FVector2D(0, 0), Color, FVector::ForwardVector);
                Triangles.Append({ BottomBase, BottomBase + 1, BottomBase + 2 });

                const int32 TopBase = Vertices.Num();
                const FVector TopNormal = Transform.TransformVectorNoScale(FVector::UpVector).GetSafeNormal();
                AddVertex(Transform.TransformPosition(FVector(0, 0, HalfHeight)), TopNormal, FVector2D(0.5f), Color, FVector::ForwardVector);
                AddVertex(Transform.TransformPosition(U0), TopNormal, FVector2D(0, 1), Color, FVector::ForwardVector);
                AddVertex(Transform.TransformPosition(U1), TopNormal, FVector2D(1, 1), Color, FVector::ForwardVector);
                Triangles.Append({ TopBase, TopBase + 1, TopBase + 2 });
            }
        }
    };

    void AddCrenels(
        FMeshData& Mesh,
        const float Span,
        const float Depth,
        const float Height,
        const int32 Count,
        const FLinearColor& Color)
    {
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const float Alpha = Count == 1 ? 0.5f : static_cast<float>(Index) / static_cast<float>(Count - 1);
            const float X = FMath::Lerp(-Span * 0.5f, Span * 0.5f, Alpha);
            Mesh.AddBox(FVector(X, 0, Height), FVector(38, Depth, 45), Color);
        }
    }
}

ASiegeAssetProxyActor::ASiegeAssetProxyActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
    ProceduralMesh->SetupAttachment(Root);
    ProceduralMesh->bUseAsyncCooking = true;
    ProceduralMesh->SetCollisionProfileName(TEXT("BlockAll"));

    ReplacementMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReplacementMesh"));
    ReplacementMesh->SetupAttachment(Root);
    ReplacementMesh->SetCollisionProfileName(TEXT("BlockAll"));

    FactionMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FactionMarker"));
    FactionMarker->SetupAttachment(Root);
    FactionMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FactionMarker->SetCastShadow(false);
    FactionMarker->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MarkerMesh(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (MarkerMesh.Succeeded())
    {
        FactionMarker->SetStaticMesh(MarkerMesh.Object);
    }
}

void ASiegeAssetProxyActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshVisual();
}

void ASiegeAssetProxyActor::BeginPlay()
{
    Super::BeginPlay();
    RefreshVisual();
    RefreshFactionMarker();
}

void ASiegeAssetProxyActor::ConfigureAsset(const ESiegeAssetSlot NewSlot)
{
    AssetSlot = NewSlot;
    bEmitValidationLog = true;
    RefreshVisual();
    bEmitValidationLog = false;
}

void ASiegeAssetProxyActor::RefreshVisual()
{
    USiegeResourceSet* ResourceSet = nullptr;
    if (const UWorld* World = GetWorld())
    {
        ResourceSet = Cast<USiegeGameInstance>(World->GetGameInstance())
            ? Cast<USiegeGameInstance>(World->GetGameInstance())->GetActiveResourceSet()
            : nullptr;
    }

    UStaticMesh* Replacement = ResourceSet ? ResourceSet->LoadMesh(AssetSlot) : nullptr;
    ReplacementMesh->SetStaticMesh(Replacement);
    ReplacementMesh->SetVisibility(Replacement != nullptr);
    ReplacementMesh->SetCollisionEnabled(
        Replacement ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

    ProceduralMesh->SetVisibility(Replacement == nullptr);
    ProceduralMesh->SetCollisionEnabled(
        Replacement ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);

    if (!Replacement)
    {
        BuildProceduralMesh();
    }
    else if (bEmitValidationLog)
    {
        const FVector BoundsExtent = Replacement->GetBounds().BoxExtent;
        UE_LOG(
            LogTemp,
            Display,
            TEXT("IronwallSiegeAsset slot=%s source=StaticMesh asset=%s extent_cm=(%.1f,%.1f,%.1f)"),
            *StaticEnum<ESiegeAssetSlot>()->GetNameStringByValue(static_cast<int64>(AssetSlot)),
            *Replacement->GetPathName(),
            BoundsExtent.X,
            BoundsExtent.Y,
            BoundsExtent.Z);
    }

    RefreshFactionMarker();
}

void ASiegeAssetProxyActor::InitializeCombatant(
    const ESiegeFaction NewFaction,
    const float NewMaxHealth,
    const float NewMoveSpeed,
    const float NewAttackDamage,
    const float NewAttackRange)
{
    Faction = NewFaction;
    MaxHealth = FMath::Max(1.0f, NewMaxHealth);
    CurrentHealth = MaxHealth;
    MoveSpeed = FMath::Max(0.0f, NewMoveSpeed);
    AttackDamage = FMath::Max(0.0f, NewAttackDamage);
    AttackRange = FMath::Max(0.0f, NewAttackRange);
    AttackCooldownRemaining = 0.0f;
    CombatVelocity = FVector::ZeroVector;
    CombatHomeLocation = GetActorLocation();
    bCombatEnabled = true;
    bDefeated = false;
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    RefreshVisual();
    RefreshFactionMarker();
}

bool ASiegeAssetProxyActor::ApplyCombatDamage(const float Damage, AActor* DamageSource)
{
    if (!IsCombatAlive() || Damage <= 0.0f)
    {
        return false;
    }

    CurrentHealth = FMath::Max(0.0f, CurrentHealth - Damage);
    TriggerActionPulse();

    if (AssetSlot == ESiegeAssetSlot::Gate && (Damage >= 100.0f || CurrentHealth <= 0.0f))
    {
        UE_LOG(
            LogTemp,
            Display,
            TEXT("IronwallSiegeCombat gate_health=%.0f/%.0f damage=%.0f source=%s"),
            CurrentHealth,
            MaxHealth,
            Damage,
            DamageSource ? *DamageSource->GetName() : TEXT("Unknown"));
    }

    if (CurrentHealth <= 0.0f)
    {
        HandleDefeat(DamageSource);
        return true;
    }
    return false;
}

float ASiegeAssetProxyActor::GetHealthRatio() const
{
    return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

void ASiegeAssetProxyActor::AdvanceCombatTime(const float DeltaSeconds)
{
    AttackCooldownRemaining = FMath::Max(0.0f, AttackCooldownRemaining - DeltaSeconds);
    ActionPulse = FMath::Max(0.0f, ActionPulse - DeltaSeconds * 2.6f);

    if (FactionMarker && FactionMarker->IsVisible())
    {
        const float PulseScale = 1.0f + ActionPulse * 0.65f;
        FactionMarker->SetRelativeScale3D(MarkerBaseScale * PulseScale);
    }
}

void ASiegeAssetProxyActor::StartAttackCooldown(const float Duration)
{
    AttackCooldownRemaining = FMath::Max(0.0f, Duration);
}

void ASiegeAssetProxyActor::TriggerActionPulse()
{
    ActionPulse = 1.0f;
}

void ASiegeAssetProxyActor::RefreshFactionMarker()
{
    if (!FactionMarker)
    {
        return;
    }

    const bool bShowMarker = bCombatEnabled && !bDefeated && Faction != ESiegeFaction::Neutral;
    FactionMarker->SetVisibility(bShowMarker);
    if (!bShowMarker)
    {
        return;
    }

    float MarkerHeight = 280.0f;
    MarkerBaseScale = FVector(0.10f);
    if (AssetSlot == ESiegeAssetSlot::BatteringRam)
    {
        MarkerHeight = 520.0f;
        MarkerBaseScale = FVector(0.16f);
    }
    else if (AssetSlot == ESiegeAssetSlot::Trebuchet)
    {
        MarkerHeight = 920.0f;
        MarkerBaseScale = FVector(0.18f);
    }
    else if (AssetSlot == ESiegeAssetSlot::Gate)
    {
        MarkerHeight = 840.0f;
        MarkerBaseScale = FVector(0.20f);
    }

    FactionMarker->SetRelativeLocation(FVector(0.0f, 0.0f, MarkerHeight));
    FactionMarker->SetRelativeScale3D(MarkerBaseScale);

    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BaseMaterial && !FactionMarkerMaterial)
    {
        FactionMarkerMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        FactionMarker->SetMaterial(0, FactionMarkerMaterial);
    }

    if (FactionMarkerMaterial)
    {
        const FLinearColor FactionColor =
            Faction == ESiegeFaction::Attackers
                ? FLinearColor(0.80f, 0.035f, 0.015f)
                : FLinearColor(0.025f, 0.16f, 0.85f);
        FactionMarkerMaterial->SetVectorParameterValue(TEXT("Color"), FactionColor);
    }
}

void ASiegeAssetProxyActor::HandleDefeat(AActor* DamageSource)
{
    bDefeated = true;
    CombatVelocity = FVector::ZeroVector;
    SetActorEnableCollision(false);
    RefreshFactionMarker();

    if (AssetSlot == ESiegeAssetSlot::Gate)
    {
        FVector BreachedLocation = GetActorLocation();
        BreachedLocation.Z -= 760.0f;
        SetActorLocation(BreachedLocation);
        UE_LOG(
            LogTemp,
            Display,
            TEXT("IronwallSiegeBattle gate_breached source=%s"),
            DamageSource ? *DamageSource->GetName() : TEXT("Unknown"));
    }
    else
    {
        SetActorHiddenInGame(true);
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("IronwallSiegeCombat defeated=%s faction=%s slot=%s"),
        *GetName(),
        *StaticEnum<ESiegeFaction>()->GetNameStringByValue(static_cast<int64>(Faction)),
        *StaticEnum<ESiegeAssetSlot>()->GetNameStringByValue(static_cast<int64>(AssetSlot)));
}

void ASiegeAssetProxyActor::BuildProceduralMesh()
{
    using namespace SiegeGeometry;

    ProceduralMesh->ClearAllMeshSections();
    FMeshData Mesh;

    FLinearColor Stone(0.32f, 0.34f, 0.36f);
    FLinearColor Wood(0.18f, 0.09f, 0.035f);
    FLinearColor Army(0.18f, 0.22f, 0.28f);
    if (const UWorld* World = GetWorld())
    {
        if (const USiegeGameInstance* GameInstance = Cast<USiegeGameInstance>(World->GetGameInstance()))
        {
            if (const USiegeResourceSet* Set = GameInstance->GetActiveResourceSet())
            {
                Stone = Set->StoneColor;
                Wood = Set->WoodColor;
                Army = Set->ArmyColor;
            }
        }
    }

    if (AssetSlot == ESiegeAssetSlot::Infantry)
    {
        if (Faction == ESiegeFaction::Attackers)
        {
            Army = FLinearColor(0.48f, 0.055f, 0.025f);
        }
        else if (Faction == ESiegeFaction::Defenders)
        {
            Army = FLinearColor(0.035f, 0.12f, 0.52f);
        }
    }

    switch (AssetSlot)
    {
        case ESiegeAssetSlot::Ground:
            Mesh.AddBox(FVector(0, 0, -25), FVector(4500, 4200, 25), FLinearColor(0.08f, 0.065f, 0.045f));
            break;

        case ESiegeAssetSlot::Wall:
            Mesh.AddBox(FVector(0, 0, 300), FVector(300, 80, 300), Stone);
            AddCrenels(Mesh, 520, 90, 645, 7, Stone * 0.9f);
            break;

        case ESiegeAssetSlot::Gate:
            Mesh.AddBox(FVector(-220, 0, 280), FVector(120, 100, 280), Stone);
            Mesh.AddBox(FVector(220, 0, 280), FVector(120, 100, 280), Stone);
            Mesh.AddBox(FVector(0, 0, 560), FVector(340, 100, 95), Stone * 0.92f);
            Mesh.AddBox(FVector(0, -92, 220), FVector(105, 18, 220), Wood);
            AddCrenels(Mesh, 580, 108, 700, 7, Stone * 0.86f);
            break;

        case ESiegeAssetSlot::Tower:
            Mesh.AddCylinder(FVector(0, 0, 390), 245, 390, 16, Stone);
            for (int32 Index = 0; Index < 10; ++Index)
            {
                const float Angle = 2.0f * PI * static_cast<float>(Index) / 10.0f;
                Mesh.AddBox(
                    FVector(FMath::Cos(Angle) * 205, FMath::Sin(Angle) * 205, 825),
                    FVector(42, 42, 55),
                    Stone * 0.88f,
                    FRotator(0, FMath::RadiansToDegrees(Angle), 0));
            }
            break;

        case ESiegeAssetSlot::Trebuchet:
            Mesh.AddBeam(FVector(-230, -110, 95), FVector(230, -110, 95), 28, Wood);
            Mesh.AddBeam(FVector(-230, 110, 95), FVector(230, 110, 95), 28, Wood);
            Mesh.AddBeam(FVector(-160, -100, 100), FVector(0, -80, 560), 24, Wood);
            Mesh.AddBeam(FVector(160, -100, 100), FVector(0, -80, 560), 24, Wood);
            Mesh.AddBeam(FVector(-160, 100, 100), FVector(0, 80, 560), 24, Wood);
            Mesh.AddBeam(FVector(160, 100, 100), FVector(0, 80, 560), 24, Wood);
            Mesh.AddBeam(FVector(-260, 0, 570), FVector(520, 0, 780), 22, Wood * 0.85f);
            Mesh.AddBox(FVector(-300, 0, 555), FVector(75, 90, 95), FLinearColor(0.12f, 0.12f, 0.11f));
            Mesh.AddCylinder(FVector(-180, -130, 80), 85, 30, 12, Wood, FRotator(90, 0, 0));
            Mesh.AddCylinder(FVector(180, -130, 80), 85, 30, 12, Wood, FRotator(90, 0, 0));
            Mesh.AddCylinder(FVector(-180, 130, 80), 85, 30, 12, Wood, FRotator(90, 0, 0));
            Mesh.AddCylinder(FVector(180, 130, 80), 85, 30, 12, Wood, FRotator(90, 0, 0));
            break;

        case ESiegeAssetSlot::BatteringRam:
            Mesh.AddBeam(FVector(-280, -120, 90), FVector(280, -120, 90), 26, Wood);
            Mesh.AddBeam(FVector(-280, 120, 90), FVector(280, 120, 90), 26, Wood);
            Mesh.AddBeam(FVector(-220, -110, 100), FVector(-160, -110, 360), 22, Wood);
            Mesh.AddBeam(FVector(220, -110, 100), FVector(160, -110, 360), 22, Wood);
            Mesh.AddBeam(FVector(-220, 110, 100), FVector(-160, 110, 360), 22, Wood);
            Mesh.AddBeam(FVector(220, 110, 100), FVector(160, 110, 360), 22, Wood);
            Mesh.AddBox(FVector(0, 0, 390), FVector(280, 155, 20), Wood * 0.8f, FRotator(0, 0, 8));
            Mesh.AddCylinder(FVector(0, 0, 210), 55, 330, 10, Wood * 0.75f, FRotator(0, 90, 0));
            Mesh.AddCylinder(FVector(-205, -145, 75), 75, 28, 12, Wood, FRotator(90, 0, 0));
            Mesh.AddCylinder(FVector(205, -145, 75), 75, 28, 12, Wood, FRotator(90, 0, 0));
            Mesh.AddCylinder(FVector(-205, 145, 75), 75, 28, 12, Wood, FRotator(90, 0, 0));
            Mesh.AddCylinder(FVector(205, 145, 75), 75, 28, 12, Wood, FRotator(90, 0, 0));
            break;

        case ESiegeAssetSlot::Infantry:
            Mesh.AddBox(FVector(0, 0, 95), FVector(32, 24, 75), Army);
            Mesh.AddCylinder(FVector(0, 0, 195), 28, 28, 10, FLinearColor(0.38f, 0.31f, 0.25f));
            Mesh.AddBox(FVector(-20, -5, 18), FVector(11, 12, 32), Army * 0.8f);
            Mesh.AddBox(FVector(20, -5, 18), FVector(11, 12, 32), Army * 0.8f);
            Mesh.AddBox(FVector(-48, -18, 115), FVector(12, 48, 62), FLinearColor(0.22f, 0.16f, 0.08f));
            Mesh.AddBeam(FVector(45, 0, 40), FVector(45, 0, 300), 4, Wood);
            break;
    }

    ProceduralMesh->CreateMeshSection_LinearColor(
        0,
        Mesh.Vertices,
        Mesh.Triangles,
        Mesh.Normals,
        Mesh.UVs,
        Mesh.Colors,
        Mesh.Tangents,
        true);

    if (bEmitValidationLog)
    {
        UE_LOG(
            LogTemp,
            Display,
            TEXT("IronwallSiegeAsset slot=%s source=Procedural vertices=%d triangles=%d"),
            *StaticEnum<ESiegeAssetSlot>()->GetNameStringByValue(static_cast<int64>(AssetSlot)),
            Mesh.Vertices.Num(),
            Mesh.Triangles.Num() / 3);
    }

    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BaseMaterial)
    {
        UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        const FLinearColor SlotColor =
            AssetSlot == ESiegeAssetSlot::Ground ? FLinearColor(0.08f, 0.065f, 0.045f) :
            (AssetSlot == ESiegeAssetSlot::Wall || AssetSlot == ESiegeAssetSlot::Gate || AssetSlot == ESiegeAssetSlot::Tower) ? Stone :
            (AssetSlot == ESiegeAssetSlot::Infantry ? Army : Wood);
        DynamicMaterial->SetVectorParameterValue(TEXT("Color"), SlotColor);
        ProceduralMesh->SetMaterial(0, DynamicMaterial);
    }
}
