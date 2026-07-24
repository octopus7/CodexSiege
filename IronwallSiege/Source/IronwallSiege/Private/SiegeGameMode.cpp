#include "SiegeGameMode.h"

#include "EngineUtils.h"
#include "SiegeArcherSpawner.h"
#include "SiegePlayerController.h"
#include "SiegeWorldDirector.h"

ASiegeGameMode::ASiegeGameMode()
{
    DefaultPawnClass = nullptr;
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

    TActorIterator<ASiegeArcherSpawner> ExistingArcherSpawner(GetWorld());
    if (!ExistingArcherSpawner)
    {
        GetWorld()->SpawnActor<ASiegeArcherSpawner>();
    }
}
