#include "SinkingFoamActor.h"

#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "SailShip.h"

ASinkingFoamActor::ASinkingFoamActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    for (int32 PatchIndex = 0; PatchIndex < PatchCount; ++PatchIndex)
    {
        UProceduralMeshComponent* Patch =
            CreateDefaultSubobject<UProceduralMeshComponent>(
                *FString::Printf(TEXT("FoamPatch_%02d"), PatchIndex));
        Patch->SetupAttachment(Root);
        Patch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Patch->SetCastShadow(false);
        Patch->SetBoundsScale(2.5f);
        FoamPatches.Add(Patch);
    }
}

void ASinkingFoamActor::BeginPlay()
{
    Super::BeginPlay();

    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Game/Art/Materials/M_ShipWake.M_ShipWake"));
    if (!BaseMaterial)
    {
        BaseMaterial = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    }
    FoamMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    if (FoamMaterial)
    {
        FoamMaterial->SetVectorParameterValue(
            TEXT("Color"),
            FLinearColor(0.68f, 0.94f, 1.0f, 1.0f));
    }

    PatchStates.SetNum(PatchCount);
    for (UProceduralMeshComponent* Patch : FoamPatches)
    {
        CreatePatchMesh(Patch);
        if (Patch && FoamMaterial)
        {
            Patch->SetMaterial(0, FoamMaterial);
        }
    }
    SetLifeSpan(TotalDuration + 0.5f);
}

void ASinkingFoamActor::FollowSinkingShip(
    ASailShip* InShip,
    const int32 InShipRate)
{
    Ship = InShip;
    RandomStream.Initialize(
        InShip ? InShip->GetUniqueID() * 7919 : FMath::Rand());

    HullHalfLength =
        InShipRate == 1 ? 1500.0f : (InShipRate == 2 ? 1280.0f : 1080.0f);
    HullHalfWidth =
        InShipRate == 1 ? 500.0f : (InShipRate == 2 ? 430.0f : 350.0f);
    HullHalfHeight =
        InShipRate == 1 ? 350.0f : (InShipRate == 2 ? 315.0f : 275.0f);
    EffectAge = 0.0f;
    NoContactAge = 0.0f;
    ContactFade = 1.0f;
    UpdateSurfaceTransform();

    if (PatchStates.Num() != PatchCount)
    {
        PatchStates.SetNum(PatchCount);
    }
    for (int32 PatchIndex = 0; PatchIndex < PatchCount; ++PatchIndex)
    {
        ResetPatch(PatchIndex, true);
    }
    UpdateContactOutline();
}

void ASinkingFoamActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    EffectAge += DeltaSeconds;
    UpdateSurfaceTransform();
    if (UpdateContactOutline())
    {
        NoContactAge = 0.0f;
        ContactFade = 1.0f;
    }
    else
    {
        NoContactAge += DeltaSeconds;
        ContactFade = 1.0f - FMath::Clamp(NoContactAge / 1.4f, 0.0f, 1.0f);
    }

    for (int32 PatchIndex = 0; PatchIndex < PatchStates.Num(); ++PatchIndex)
    {
        UpdatePatch(PatchIndex, DeltaSeconds);
    }

    if (EffectAge >= TotalDuration || NoContactAge >= 1.4f || !Ship.IsValid())
    {
        Destroy();
    }
}

void ASinkingFoamActor::CreatePatchMesh(UProceduralMeshComponent* Patch)
{
    if (!Patch)
    {
        return;
    }

    constexpr int32 EdgeSegments = 16;
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colors;
    Vertices.Reserve(EdgeSegments + 1);
    Triangles.Reserve(EdgeSegments * 3);
    Normals.Reserve(EdgeSegments + 1);
    UVs.Reserve(EdgeSegments + 1);
    Colors.Reserve(EdgeSegments + 1);

    Vertices.Add(FVector::ZeroVector);
    Normals.Add(FVector::UpVector);
    UVs.Add(FVector2D(0.5f, 0.5f));
    Colors.Add(FLinearColor::Transparent);
    for (int32 Segment = 0; Segment < EdgeSegments; ++Segment)
    {
        const float Angle =
            static_cast<float>(Segment) / static_cast<float>(EdgeSegments)
            * UE_TWO_PI;
        const FVector2D Radial(FMath::Cos(Angle), FMath::Sin(Angle));
        Vertices.Add(FVector(Radial.X * 50.0f, Radial.Y * 50.0f, 0.0f));
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 0.5f) + Radial * 0.5f);
        Colors.Add(FLinearColor::Transparent);

        Triangles.Add(0);
        Triangles.Add(Segment + 1);
        Triangles.Add((Segment + 1) % EdgeSegments + 1);
    }
    Patch->CreateMeshSection_LinearColor(
        0,
        Vertices,
        Triangles,
        Normals,
        UVs,
        Colors,
        {},
        false);
}

void ASinkingFoamActor::ResetPatch(
    const int32 PatchIndex,
    const bool bInitialPhase)
{
    if (!PatchStates.IsValidIndex(PatchIndex)
        || !FoamPatches.IsValidIndex(PatchIndex))
    {
        return;
    }

    FFoamPatchState& State = PatchStates[PatchIndex];
    const float EvenAngle =
        static_cast<float>(PatchIndex) / static_cast<float>(PatchCount)
        * UE_TWO_PI;
    const float Angle =
        EvenAngle + RandomStream.FRandRange(-0.14f, 0.14f);
    const float OutlineJitter = RandomStream.FRandRange(0.86f, 1.08f);
    State.OutlineAngle = Angle;
    State.OutlineJitter = OutlineJitter;
    State.LocalCenter = FVector(
        FMath::Cos(Angle) * HullHalfLength * OutlineJitter,
        FMath::Sin(Angle) * HullHalfWidth * OutlineJitter,
        RandomStream.FRandRange(1.0f, 7.0f));
    State.BaseSize = FVector2D(
        RandomStream.FRandRange(260.0f, 520.0f),
        RandomStream.FRandRange(105.0f, 245.0f));
    State.Duration = RandomStream.FRandRange(1.45f, 2.85f);
    State.Age =
        bInitialPhase
            ? RandomStream.FRandRange(0.0f, State.Duration)
            : 0.0f;
    State.StartRotation =
        FMath::RadiansToDegrees(Angle) + 90.0f
        + RandomStream.FRandRange(-24.0f, 24.0f);
    State.RotationSpeed = RandomStream.FRandRange(-28.0f, 28.0f);
    State.Growth = RandomStream.FRandRange(0.55f, 1.25f);

    if (UProceduralMeshComponent* Patch = FoamPatches[PatchIndex])
    {
        Patch->SetVisibility(true);
    }
}

void ASinkingFoamActor::UpdatePatch(
    const int32 PatchIndex,
    const float DeltaSeconds)
{
    if (!PatchStates.IsValidIndex(PatchIndex)
        || !FoamPatches.IsValidIndex(PatchIndex))
    {
        return;
    }

    FFoamPatchState& State = PatchStates[PatchIndex];
    UProceduralMeshComponent* Patch = FoamPatches[PatchIndex];
    if (!Patch)
    {
        return;
    }

    State.Age += DeltaSeconds;
    if (State.Age >= State.Duration)
    {
        if (EffectAge < RepeatDuration)
        {
            ResetPatch(PatchIndex, false);
        }
        else
        {
            Patch->SetVisibility(false);
            return;
        }
    }

    const float NormalizedAge =
        FMath::Clamp(State.Age / State.Duration, 0.0f, 1.0f);
    const float Fade =
        FMath::Sin(NormalizedAge * UE_PI)
        * FMath::Square(1.0f - 0.28f * NormalizedAge);
    const float Expansion = 1.0f + State.Growth * NormalizedAge;

    Patch->SetRelativeLocation(State.LocalCenter);
    Patch->SetRelativeRotation(FRotator(
        0.0f,
        State.StartRotation + State.RotationSpeed * State.Age,
        0.0f));
    Patch->SetRelativeScale3D(FVector(
        State.BaseSize.X / 100.0f * Expansion,
        State.BaseSize.Y / 100.0f * (1.0f + State.Growth * 0.55f * NormalizedAge),
        1.0f));

    const FLinearColor FoamColor(
        0.62f * Fade * ContactFade,
        0.88f * Fade * ContactFade,
        1.0f * Fade * ContactFade,
        0.82f * Fade * ContactFade);
    constexpr int32 EdgeSegments = 16;
    TArray<FLinearColor> Colors;
    Colors.Reserve(EdgeSegments + 1);
    Colors.Add(FoamColor);
    for (int32 Segment = 0; Segment < EdgeSegments; ++Segment)
    {
        Colors.Add(FLinearColor(
            FoamColor.R * 0.2f,
            FoamColor.G * 0.2f,
            FoamColor.B * 0.2f,
            0.0f));
    }
    Patch->UpdateMeshSection_LinearColor(
        0,
        {},
        {},
        {},
        Colors,
        {});
}

void ASinkingFoamActor::UpdateSurfaceTransform()
{
    ASailShip* FollowedShip = Ship.Get();
    if (!FollowedShip)
    {
        return;
    }

    const FVector ShipLocation = FollowedShip->GetActorLocation();
    SetActorLocation(FVector(ShipLocation.X, ShipLocation.Y, SurfaceHeight));
    SetActorRotation(FRotator(
        0.0f,
        FollowedShip->GetActorRotation().Yaw,
        0.0f));
}

bool ASinkingFoamActor::UpdateContactOutline()
{
    const ASailShip* FollowedShip = Ship.Get();
    if (!FollowedShip || PatchStates.Num() != PatchCount)
    {
        return false;
    }

    const FQuat ShipRotation = FollowedShip->GetActorQuat();
    const FVector LocalWaterNormal =
        ShipRotation.UnrotateVector(FVector::UpVector);
    const FVector ScaledPlaneNormal(
        LocalWaterNormal.X * HullHalfLength,
        LocalWaterNormal.Y * HullHalfWidth,
        LocalWaterNormal.Z * HullHalfHeight);
    const float NormalSizeSquared = ScaledPlaneNormal.SizeSquared();
    if (NormalSizeSquared <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const float PlaneOffset =
        FollowedShip->GetActorLocation().Z - SurfaceHeight;
    const FVector UnitSphereCenter =
        ScaledPlaneNormal * (-PlaneOffset / NormalSizeSquared);
    const float CenterSizeSquared = UnitSphereCenter.SizeSquared();
    if (CenterSizeSquared >= 1.0f)
    {
        return false;
    }

    const FVector CircleNormal = ScaledPlaneNormal.GetSafeNormal();
    FVector CircleAxisU;
    FVector CircleAxisV;
    CircleNormal.FindBestAxisVectors(CircleAxisU, CircleAxisV);
    const float CircleRadius = FMath::Sqrt(1.0f - CenterSizeSquared);
    const FTransform FoamTransform = GetActorTransform();
    const FVector ShipCenter = FollowedShip->GetActorLocation();

    for (FFoamPatchState& State : PatchStates)
    {
        const FVector UnitSpherePoint =
            UnitSphereCenter
            + (CircleAxisU * FMath::Cos(State.OutlineAngle)
                + CircleAxisV * FMath::Sin(State.OutlineAngle))
                * CircleRadius
                * State.OutlineJitter;
        const FVector ShipLocalPoint(
            UnitSpherePoint.X * HullHalfLength,
            UnitSpherePoint.Y * HullHalfWidth,
            UnitSpherePoint.Z * HullHalfHeight);
        const FVector WorldPoint =
            ShipCenter + ShipRotation.RotateVector(ShipLocalPoint);
        State.LocalCenter = FoamTransform.InverseTransformPosition(WorldPoint);
        State.LocalCenter.Z = FMath::Max(State.LocalCenter.Z, 1.0f);
    }
    return true;
}
