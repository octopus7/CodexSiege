#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SiegeGameInstance.generated.h"

class USiegeResourceSet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSiegeResourceSetChanged, FName, ResourceSetId);

UCLASS()
class IRONWALLSIEGE_API USiegeGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    USiegeGameInstance();
    virtual void Init() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Siege|Resources")
    TArray<TSoftObjectPtr<USiegeResourceSet>> AvailableResourceSets;

    UPROPERTY(BlueprintAssignable, Category="Siege|Resources")
    FSiegeResourceSetChanged OnResourceSetChanged;

    UFUNCTION(BlueprintPure, Category="Siege|Resources")
    TArray<FString> GetResourceSetLabels() const;

    UFUNCTION(BlueprintPure, Category="Siege|Resources")
    int32 GetSelectedResourceSetIndex() const;

    UFUNCTION(BlueprintCallable, Category="Siege|Resources")
    void SelectResourceSetByIndex(int32 Index);

    UFUNCTION(BlueprintPure, Category="Siege|Resources")
    USiegeResourceSet* GetActiveResourceSet() const;

private:
    UPROPERTY(Transient)
    TObjectPtr<USiegeResourceSet> NativePrototypeFallback;

    UPROPERTY(Transient)
    TObjectPtr<USiegeResourceSet> NativeBlenderFallback;

    FName SelectedResourceSetId = TEXT("Prototype");

    void BuildNativeFallbacks();
    USiegeResourceSet* ResolveSet(int32 Index) const;
};
