#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SiegeTypes.h"
#include "SiegeResourceSet.generated.h"

class UMaterialInterface;
class UStaticMesh;

UCLASS(BlueprintType)
class IRONWALLSIEGE_API USiegeResourceSet : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FName ResourceSetId = TEXT("Prototype");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    ESiegeResourceFlavor Flavor = ESiegeResourceFlavor::Prototype;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
    TMap<ESiegeAssetSlot, TSoftObjectPtr<UStaticMesh>> Meshes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
    TSoftObjectPtr<UMaterialInterface> PrimaryMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
    FLinearColor StoneColor = FLinearColor(0.32f, 0.34f, 0.36f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
    FLinearColor WoodColor = FLinearColor(0.18f, 0.09f, 0.035f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
    FLinearColor ArmyColor = FLinearColor(0.18f, 0.22f, 0.28f);

    UStaticMesh* LoadMesh(ESiegeAssetSlot Slot) const;
};
