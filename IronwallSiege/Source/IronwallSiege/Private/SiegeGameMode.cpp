#include "SiegeGameMode.h"

#include "EngineUtils.h"
#include "SiegeCameraPawn.h"
#include "SiegePlayerController.h"
#include "SiegeWorldDirector.h"

ASiegeGameMode::ASiegeGameMode()
{
    DefaultPawnClass = ASiegeCameraPawn::StaticClass();
    PlayerControllerClass = ASiegePlayerController::StaticClass();
}

void ASiegeGameMode::BeginPlay()
{
    Super::BeginPlay();

    TActorIterator<ASiegeWorldDirector> Existing(GetWorld());
    if (!Existing)
    {
        GetWorld()->SpawnActor<ASiegeWorldDirector>();
    }
}
