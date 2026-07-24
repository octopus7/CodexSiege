#include "SiegeTextureLoader.h"

#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"

UTexture2D* FSiegeTextureLoader::LoadPNGFromContent(const FString& RelativePath, UObject* Outer)
{
    (void)Outer;
    const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), RelativePath);
    if (!FPaths::FileExists(FullPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Ironwall Siege: title background not found at %s"), *FullPath);
        return nullptr;
    }

    return FImageUtils::ImportFileAsTexture2D(FullPath);
}
