#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SiegeTypes.h"
#include "SiegeWorldDirector.generated.h"

class ASiegeAssetProxyActor;

UCLASS()
class IRONWALLSIEGE_API ASiegeWorldDirector : public AActor
{
    GENERATED_BODY()

public:
    ASiegeWorldDirector();
    virtual void BeginPlay() override;

private:
    ASiegeAssetProxyActor* SpawnAsset(
        ESiegeAssetSlot Slot,
        const FVector& Location,
        const FRotator& Rotation,
        const FVector& Scale,
        const FString& Label);

    UFUNCTION()
    void HandleResourceSetChanged(FName ResourceSetId);
};
