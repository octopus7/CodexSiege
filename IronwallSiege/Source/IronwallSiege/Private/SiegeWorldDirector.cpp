#include "SiegeWorldDirector.h"

#include "EngineUtils.h"
#include "SiegeAssetProxyActor.h"
#include "SiegeGameInstance.h"

ASiegeWorldDirector::ASiegeWorldDirector()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ASiegeWorldDirector::BeginPlay()
{
    Super::BeginPlay();

    if (USiegeGameInstance* GameInstance = Cast<USiegeGameInstance>(GetGameInstance()))
    {
        GameInstance->OnResourceSetChanged.AddDynamic(this, &ASiegeWorldDirector::HandleResourceSetChanged);
    }

    SpawnAsset(ESiegeAssetSlot::Trebuchet, FVector(-800, -1500, 0), FRotator(0, 8, 0), FVector(1.25f), TEXT("Trebuchet_A"));
    SpawnAsset(ESiegeAssetSlot::BatteringRam, FVector(620, -1000, 0), FRotator::ZeroRotator, FVector(1.15f), TEXT("BatteringRam_A"));

    int32 SoldierIndex = 0;
    for (int32 Row = 0; Row < 3; ++Row)
    {
        for (int32 Column = -4; Column <= 4; ++Column)
        {
            if (Row == 0 && FMath::Abs(Column) <= 1)
            {
                continue;
            }
            const FVector Position(
                static_cast<float>(Column) * 180.0f,
                -420.0f - static_cast<float>(Row) * 250.0f,
                0.0f);
            SpawnAsset(
                ESiegeAssetSlot::Infantry,
                Position,
                FRotator(0, FMath::FRandRange(-8.0f, 8.0f), 0),
                FVector(1.15f),
                FString::Printf(TEXT("Infantry_%02d"), SoldierIndex++));
        }
    }
}

ASiegeAssetProxyActor* ASiegeWorldDirector::SpawnAsset(
    const ESiegeAssetSlot Slot,
    const FVector& Location,
    const FRotator& Rotation,
    const FVector& Scale,
    const FString& Label)
{
    FActorSpawnParameters Parameters;
    Parameters.Owner = this;

    ASiegeAssetProxyActor* Actor = GetWorld()->SpawnActor<ASiegeAssetProxyActor>(
        ASiegeAssetProxyActor::StaticClass(),
        Location,
        Rotation,
        Parameters);
    if (Actor)
    {
        Actor->SetActorScale3D(Scale);
#if WITH_EDITOR
        Actor->SetActorLabel(Label);
#endif
        Actor->ConfigureAsset(Slot);
    }
    return Actor;
}

void ASiegeWorldDirector::HandleResourceSetChanged(FName ResourceSetId)
{
    (void)ResourceSetId;
    for (TActorIterator<ASiegeAssetProxyActor> It(GetWorld()); It; ++It)
    {
        It->RefreshVisual();
    }
}
