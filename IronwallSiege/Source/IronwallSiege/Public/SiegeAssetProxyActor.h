#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SiegeTypes.h"
#include "SiegeAssetProxyActor.generated.h"

class UProceduralMeshComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class IRONWALLSIEGE_API ASiegeAssetProxyActor : public AActor
{
    GENERATED_BODY()

public:
    ASiegeAssetProxyActor();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Siege")
    ESiegeAssetSlot AssetSlot = ESiegeAssetSlot::Wall;

    UFUNCTION(BlueprintCallable, Category="Siege")
    void ConfigureAsset(ESiegeAssetSlot NewSlot);

    UFUNCTION(BlueprintCallable, Category="Siege")
    void RefreshVisual();

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UProceduralMeshComponent> ProceduralMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> ReplacementMesh;

    void BuildProceduralMesh();
};
