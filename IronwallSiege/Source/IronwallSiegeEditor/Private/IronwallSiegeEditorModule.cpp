#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "SiegeResourceSet.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
    USiegeResourceSet* CreateResourceSet(const FString& AssetName)
    {
        const FString PackageName = FString::Printf(TEXT("/Game/Data/%s"), *AssetName);
        const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);
        if (USiegeResourceSet* Existing = LoadObject<USiegeResourceSet>(nullptr, *ObjectPath))
        {
            return Existing;
        }

        UPackage* Package = CreatePackage(*PackageName);
        Package->FullyLoad();
        USiegeResourceSet* Asset = NewObject<USiegeResourceSet>(
            Package,
            USiegeResourceSet::StaticClass(),
            *AssetName,
            RF_Public | RF_Standalone);
        FAssetRegistryModule::AssetCreated(Asset);
        Package->MarkPackageDirty();
        return Asset;
    }

    bool SaveResourceSet(USiegeResourceSet* Asset)
    {
        if (!Asset)
        {
            return false;
        }

        UPackage* Package = Asset->GetPackage();
        const FString Filename = FPackageName::LongPackageNameToFilename(
            Package->GetName(),
            FPackageName::GetAssetPackageExtension());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.SaveFlags = SAVE_NoError;
        return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
    }

    void AddProductionMesh(
        USiegeResourceSet* ResourceSet,
        const ESiegeAssetSlot Slot,
        const TCHAR* Path)
    {
        ResourceSet->Meshes.Add(
            Slot,
            TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(Path)));
    }

    void BootstrapResourceSets()
    {
        USiegeResourceSet* Prototype = CreateResourceSet(TEXT("DA_PrototypeSiege"));
        if (Prototype && Prototype->DisplayName.IsEmpty())
        {
            Prototype->ResourceSetId = TEXT("Prototype");
            Prototype->DisplayName = FText::FromString(TEXT("Prototype Geometry"));
            Prototype->Flavor = ESiegeResourceFlavor::Prototype;
            Prototype->StoneColor = FLinearColor(0.32f, 0.34f, 0.36f);
            Prototype->WoodColor = FLinearColor(0.18f, 0.09f, 0.035f);
            Prototype->ArmyColor = FLinearColor(0.18f, 0.22f, 0.28f);
            SaveResourceSet(Prototype);
        }

        USiegeResourceSet* Production = CreateResourceSet(TEXT("DA_BlenderProduction"));
        if (Production && Production->DisplayName.IsEmpty())
        {
            Production->ResourceSetId = TEXT("BlenderProduction");
            Production->DisplayName = FText::FromString(TEXT("Blender Production (procedural fallback)"));
            Production->Flavor = ESiegeResourceFlavor::BlenderProduction;
            Production->StoneColor = FLinearColor(0.42f, 0.39f, 0.34f);
            Production->WoodColor = FLinearColor(0.22f, 0.11f, 0.045f);
            Production->ArmyColor = FLinearColor(0.24f, 0.18f, 0.15f);

            AddProductionMesh(Production, ESiegeAssetSlot::Ground,       TEXT("/Game/Art/Blender/SM_Ground_SiegeField.SM_Ground_SiegeField"));
            AddProductionMesh(Production, ESiegeAssetSlot::Wall,         TEXT("/Game/Art/Blender/SM_Fortress_Wall_A.SM_Fortress_Wall_A"));
            AddProductionMesh(Production, ESiegeAssetSlot::Gate,         TEXT("/Game/Art/Blender/SM_Fortress_Gatehouse_A.SM_Fortress_Gatehouse_A"));
            AddProductionMesh(Production, ESiegeAssetSlot::Tower,        TEXT("/Game/Art/Blender/SM_Fortress_Tower_A.SM_Fortress_Tower_A"));
            AddProductionMesh(Production, ESiegeAssetSlot::Trebuchet,    TEXT("/Game/Art/Blender/SM_Siege_Trebuchet_A.SM_Siege_Trebuchet_A"));
            AddProductionMesh(Production, ESiegeAssetSlot::BatteringRam, TEXT("/Game/Art/Blender/SM_Siege_BatteringRam_A.SM_Siege_BatteringRam_A"));
            AddProductionMesh(Production, ESiegeAssetSlot::Infantry,     TEXT("/Game/Art/Blender/SM_Unit_Infantry_A.SM_Unit_Infantry_A"));
            SaveResourceSet(Production);
        }
    }
}

class FIronwallSiegeEditorModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        if (GIsEditor && !IsRunningCommandlet())
        {
            BootstrapResourceSets();
        }
    }
};

IMPLEMENT_MODULE(FIronwallSiegeEditorModule, IronwallSiegeEditor);
