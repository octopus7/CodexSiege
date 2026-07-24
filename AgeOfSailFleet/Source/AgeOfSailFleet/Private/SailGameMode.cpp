#include "SailGameMode.h"

#include "FleetBattleDirector.h"
#include "FleetCameraPawn.h"
#include "FleetPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "SailFleetHUDWidget.h"
#include "SailShip.h"
#include "SailTitleScreenWidget.h"
#include "Engine/Texture2D.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ASailGameMode::ASailGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = AFleetCameraPawn::StaticClass();
    PlayerControllerClass = AFleetPlayerController::StaticClass();
}

void ASailGameMode::BeginPlay()
{
    Super::BeginPlay();
    BattleDirector = GetWorld()->SpawnActor<AFleetBattleDirector>(
        AFleetBattleDirector::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator);

    if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
    {
        if (UClass* FleetWidgetClass = LoadClass<USailFleetHUDWidget>(
            nullptr,
            TEXT("/Game/UI/WBP_SailFleetHUD.WBP_SailFleetHUD_C")))
        {
            FleetHUDWidget = CreateWidget<USailFleetHUDWidget>(Controller, FleetWidgetClass);
            if (FleetHUDWidget)
            {
                FleetHUDWidget->AddToViewport(10);
                FleetHUDWidget->SetBattleHUDVisible(false);
                const auto LoadGlyph = [](const TCHAR* Path)
                {
                    return LoadObject<UTexture2D>(nullptr, Path);
                };
                FleetHUDWidget->SetDateGlyphTextures(
                    LoadGlyph(TEXT("/Game/UI/DateGlyphs/T_Date_Weekday_FRI.T_Date_Weekday_FRI")),
                    LoadGlyph(TEXT("/Game/UI/DateGlyphs/T_Date_Punctuation_Dot.T_Date_Punctuation_Dot")),
                    LoadGlyph(TEXT("/Game/UI/DateGlyphs/T_Date_Digit_2.T_Date_Digit_2")),
                    LoadGlyph(TEXT("/Game/UI/DateGlyphs/T_Date_Digit_4.T_Date_Digit_4")),
                    LoadGlyph(TEXT("/Game/UI/DateGlyphs/T_Date_Month_July.T_Date_Month_July")),
                    LoadGlyph(TEXT("/Game/UI/DateGlyphs/T_Date_Digit_1.T_Date_Digit_1")),
                    LoadGlyph(TEXT("/Game/UI/DateGlyphs/T_Date_Digit_7.T_Date_Digit_7")),
                    LoadGlyph(TEXT("/Game/UI/DateGlyphs/T_Date_Digit_1.T_Date_Digit_1")),
                    LoadGlyph(TEXT("/Game/UI/DateGlyphs/T_Date_Digit_5.T_Date_Digit_5")));
                FleetHUDWidget->SetWindGlyphTextures(
                    LoadGlyph(TEXT("/Game/UI/DateGlyphs/T_Wind_Compass.T_Wind_Compass")),
                    LoadGlyph(TEXT("/Game/UI/DateGlyphs/T_Wind_Arrow.T_Wind_Arrow")));
            }
        }
        if (UClass* TitleClass = LoadClass<USailTitleScreenWidget>(
            nullptr,
            TEXT("/Game/UI/WBP_TitleScreen.WBP_TitleScreen_C")))
        {
            TitleScreenWidget = CreateWidget<USailTitleScreenWidget>(Controller, TitleClass);
            if (TitleScreenWidget)
            {
                TitleScreenWidget->OnFleetDepartureRequested.AddUniqueDynamic(
                    this,
                    &ASailGameMode::HandleFleetDeparture);
                TitleScreenWidget->AddToViewport(100);
            }
        }
    }

    if (FParse::Param(FCommandLine::Get(), TEXT("AutoStartBattle")))
    {
        if (TitleScreenWidget)
        {
            TitleScreenWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
        HandleFleetDeparture();
    }
}

void ASailGameMode::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RefreshFleetHUD();
}

void ASailGameMode::HandleFleetDeparture()
{
    if (FleetHUDWidget)
    {
        FleetHUDWidget->SetBattleHUDVisible(true);
    }
    if (AFleetBattleDirector* Director = BattleDirector.Get())
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("Graphics2D")))
        {
            Director->SetGraphicsMode(ESailGraphicsMode::TwoDimensional);
        }
        else if (TitleScreenWidget)
        {
            Director->SetGraphicsMode(TitleScreenWidget->GetSelectedGraphicsMode());
        }
        Director->StartBattle();
    }
}

void ASailGameMode::RefreshFleetHUD()
{
    if (!FleetHUDWidget || !FleetHUDWidget->IsVisible())
    {
        return;
    }
    const AFleetPlayerController* Controller = Cast<AFleetPlayerController>(
        GetWorld()->GetFirstPlayerController());
    if (!Controller)
    {
        return;
    }
    if (const AFleetBattleDirector* Director = BattleDirector.Get())
    {
        FleetHUDWidget->SetWindHeadingDegrees(
            Director->GetWindDirection().Rotation().Yaw);
    }

    TArray<FSailShipHUDEntry> Entries;
    for (const TWeakObjectPtr<ASailShip>& ShipPtr : Controller->GetSelectedShips())
    {
        const ASailShip* Ship = ShipPtr.Get();
        if (!Ship || !Ship->IsAfloat())
        {
            continue;
        }
        FSailShipHUDEntry& Entry = Entries.AddDefaulted_GetRef();
        Entry.CaptainName = FText::FromString(Ship->GetCaptainName());
        Entry.ShipName = FText::FromString(Ship->GetShipName());
        Entry.ShipClass = FText::FromString(FString::Printf(
            TEXT("%s - %d guns"),
            *Ship->GetShipClassName(),
            Ship->GetGunCount()));
        Entry.Faction =
            Ship->GetTeam() == 0
                ? ESailFleetFaction::BlueFleet
                : ESailFleetFaction::RedFleet;
        Entry.ShipRank =
            Ship->GetShipRate() == 1 ? 3 :
            Ship->GetShipRate() == 2 ? 2 : 1;
        Entry.HealthFraction = Ship->GetHealthRatio();
        const TCHAR* PortraitPath =
            Ship->GetCaptainName().Contains(TEXT("Ward"))
                ? TEXT("/Game/UI/Captains/T_Captain_Blue_Admiral_Ward.T_Captain_Blue_Admiral_Ward")
                : Ship->GetCaptainName().Contains(TEXT("Mercer"))
                    ? TEXT("/Game/UI/Captains/T_Captain_Blue_Captain_Mercer.T_Captain_Blue_Captain_Mercer")
                    : Ship->GetCaptainName().Contains(TEXT("Reed"))
                        ? TEXT("/Game/UI/Captains/T_Captain_Blue_Captain_Reed.T_Captain_Blue_Captain_Reed")
                        : Ship->GetCaptainName().Contains(TEXT("Voss"))
                            ? TEXT("/Game/UI/Captains/T_Captain_Red_Admiral_Voss.T_Captain_Red_Admiral_Voss")
                            : Ship->GetCaptainName().Contains(TEXT("Marat"))
                                ? TEXT("/Game/UI/Captains/T_Captain_Red_Captain_Marat.T_Captain_Red_Captain_Marat")
                                : Ship->GetCaptainName().Contains(TEXT("Vale"))
                                    ? TEXT("/Game/UI/Captains/T_Captain_Red_Captain_Vale.T_Captain_Red_Captain_Vale")
                                    : TEXT("/Game/UI/Captains/T_Captain_Red_Captain_Cruz.T_Captain_Red_Captain_Cruz");
        Entry.CaptainPortrait = LoadObject<UTexture2D>(nullptr, PortraitPath);
        if (!Entry.CaptainPortrait)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Fleet captain portrait missing: %s"),
                PortraitPath);
        }
    }
    FleetHUDWidget->SetSelectedShips(Entries);
}
