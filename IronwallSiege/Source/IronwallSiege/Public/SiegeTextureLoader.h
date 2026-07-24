#pragma once

#include "CoreMinimal.h"

class UTexture2D;

class IRONWALLSIEGE_API FSiegeTextureLoader
{
public:
    static UTexture2D* LoadPNGFromContent(const FString& RelativePath, UObject* Outer);
};
