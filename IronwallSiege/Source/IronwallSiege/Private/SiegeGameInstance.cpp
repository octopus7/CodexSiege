#include "SiegeGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "SiegeResourceSet.h"
#include "SiegeUserSettings.h"

USiegeGameInstance::USiegeGameInstance()
{
    AvailableResourceSets.Add(TSoftObjectPtr<USiegeResourceSet>(
        FSoftObjectPath(TEXT("/Game/Data/DA_PrototypeSiege.DA_PrototypeSiege"))));
    AvailableResourceSets.Add(TSoftObjectPtr<USiegeResourceSet>(
        FSoftObjectPath(TEXT("/Game/Data/DA_BlenderProduction.DA_BlenderProduction"))));
}

void USiegeGameInstance::Init()
{
    Super::Init();
    BuildNativeFallbacks();

    if (const USiegeUserSettings* Settings = Cast<USiegeUserSettings>(GEngine->GetGameUserSettings()))
    {
        SelectedResourceSetId = Settings->SelectedResourceSetId;
    }

    if (FParse::Param(FCommandLine::Get(), TEXT("BlenderProduction")))
    {
        SelectedResourceSetId = TEXT("BlenderProduction");
    }

    if (GetSelectedResourceSetIndex() == INDEX_NONE)
    {
        SelectedResourceSetId = TEXT("Prototype");
    }
}

void USiegeGameInstance::BuildNativeFallbacks()
{
    NativePrototypeFallback = NewObject<USiegeResourceSet>(this, TEXT("NativePrototypeFallback"));
    NativePrototypeFallback->ResourceSetId = TEXT("Prototype");
    NativePrototypeFallback->DisplayName = FText::FromString(TEXT("Prototype Geometry"));
    NativePrototypeFallback->Flavor = ESiegeResourceFlavor::Prototype;

    NativeBlenderFallback = NewObject<USiegeResourceSet>(this, TEXT("NativeBlenderFallback"));
    NativeBlenderFallback->ResourceSetId = TEXT("BlenderProduction");
    NativeBlenderFallback->DisplayName = FText::FromString(TEXT("Blender Production (procedural fallback)"));
    NativeBlenderFallback->Flavor = ESiegeResourceFlavor::BlenderProduction;
    NativeBlenderFallback->StoneColor = FLinearColor(0.42f, 0.39f, 0.34f);
    NativeBlenderFallback->WoodColor = FLinearColor(0.22f, 0.11f, 0.045f);
    NativeBlenderFallback->ArmyColor = FLinearColor(0.24f, 0.18f, 0.15f);

    const TPair<ESiegeAssetSlot, const TCHAR*> Paths[] =
    {
        { ESiegeAssetSlot::Ground,       TEXT("/Game/Art/Blender/SM_Ground_SiegeField.SM_Ground_SiegeField") },
        { ESiegeAssetSlot::Wall,         TEXT("/Game/Art/Blender/SM_Fortress_Wall_A.SM_Fortress_Wall_A") },
        { ESiegeAssetSlot::Gate,         TEXT("/Game/Art/Blender/SM_Fortress_Gatehouse_A.SM_Fortress_Gatehouse_A") },
        { ESiegeAssetSlot::Tower,        TEXT("/Game/Art/Blender/SM_Fortress_Tower_A.SM_Fortress_Tower_A") },
        { ESiegeAssetSlot::Trebuchet,    TEXT("/Game/Art/Blender/SM_Siege_Trebuchet_A.SM_Siege_Trebuchet_A") },
        { ESiegeAssetSlot::BatteringRam, TEXT("/Game/Art/Blender/SM_Siege_BatteringRam_A.SM_Siege_BatteringRam_A") },
        { ESiegeAssetSlot::Infantry,     TEXT("/Game/Art/Blender/SM_Unit_Infantry_A.SM_Unit_Infantry_A") }
    };

    for (const TPair<ESiegeAssetSlot, const TCHAR*>& Pair : Paths)
    {
        NativeBlenderFallback->Meshes.Add(
            Pair.Key,
            TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(Pair.Value)));
    }
}

USiegeResourceSet* USiegeGameInstance::ResolveSet(const int32 Index) const
{
    if (!AvailableResourceSets.IsValidIndex(Index))
    {
        return nullptr;
    }

    if (USiegeResourceSet* Loaded = AvailableResourceSets[Index].LoadSynchronous())
    {
        return Loaded;
    }

    return Index == 0 ? NativePrototypeFallback.Get() : NativeBlenderFallback.Get();
}

TArray<FString> USiegeGameInstance::GetResourceSetLabels() const
{
    TArray<FString> Labels;
    for (int32 Index = 0; Index < AvailableResourceSets.Num(); ++Index)
    {
        if (const USiegeResourceSet* Set = ResolveSet(Index))
        {
            Labels.Add(Set->DisplayName.ToString());
        }
    }
    return Labels;
}

int32 USiegeGameInstance::GetSelectedResourceSetIndex() const
{
    for (int32 Index = 0; Index < AvailableResourceSets.Num(); ++Index)
    {
        if (const USiegeResourceSet* Set = ResolveSet(Index))
        {
            if (Set->ResourceSetId == SelectedResourceSetId)
            {
                return Index;
            }
        }
    }
    return INDEX_NONE;
}

void USiegeGameInstance::SelectResourceSetByIndex(const int32 Index)
{
    USiegeResourceSet* Set = ResolveSet(Index);
    if (!Set)
    {
        return;
    }

    SelectedResourceSetId = Set->ResourceSetId;

    if (USiegeUserSettings* Settings = Cast<USiegeUserSettings>(GEngine->GetGameUserSettings()))
    {
        Settings->SelectedResourceSetId = SelectedResourceSetId;
        Settings->SaveSettings();
    }

    OnResourceSetChanged.Broadcast(SelectedResourceSetId);
}

USiegeResourceSet* USiegeGameInstance::GetActiveResourceSet() const
{
    const int32 Index = GetSelectedResourceSetIndex();
    return ResolveSet(Index == INDEX_NONE ? 0 : Index);
}
