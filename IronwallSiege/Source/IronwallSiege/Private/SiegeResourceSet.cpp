#include "SiegeResourceSet.h"
#include "Engine/StaticMesh.h"

FPrimaryAssetId USiegeResourceSet::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(TEXT("SiegeResourceSet"), GetFName());
}

UStaticMesh* USiegeResourceSet::LoadMesh(const ESiegeAssetSlot Slot) const
{
    if (const TSoftObjectPtr<UStaticMesh>* Mesh = Meshes.Find(Slot))
    {
        return Mesh->LoadSynchronous();
    }
    return nullptr;
}
