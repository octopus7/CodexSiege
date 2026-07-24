#pragma once

#include "CoreMinimal.h"
#include "SailGraphicsMode.generated.h"

/** Rendering presentation selected on the title screen. */
UENUM(BlueprintType)
enum class ESailGraphicsMode : uint8
{
    ThreeDimensional UMETA(DisplayName = "3D"),
    TwoDimensional UMETA(DisplayName = "2D")
};
