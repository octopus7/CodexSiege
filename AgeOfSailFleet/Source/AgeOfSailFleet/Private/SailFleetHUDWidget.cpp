#include "SailFleetHUDWidget.h"

#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

namespace SailFleetHUD
{
    const FLinearColor HealthyBar(0.12f, 0.46f, 0.23f, 1.0f);
    const FLinearColor DamagedBar(0.67f, 0.13f, 0.06f, 1.0f);
}

bool USailFleetHUDWidget::FBoundShipCard::IsComplete() const
{
    return Container && FactionGlow && LocketFrame && Portrait && CaptainName
        && ShipName && ShipClass && RankText && HealthBar;
}

void USailFleetHUDWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BronzeLocketTexture = LoadObject<UTexture2D>(
        nullptr,
        TEXT("/Game/UI/Lockets/T_Locket_Bronze.T_Locket_Bronze"));
    SilverLocketTexture = LoadObject<UTexture2D>(
        nullptr,
        TEXT("/Game/UI/Lockets/T_Locket_Silver.T_Locket_Silver"));
    GoldLocketTexture = LoadObject<UTexture2D>(
        nullptr,
        TEXT("/Game/UI/Lockets/T_Locket_Gold.T_Locket_Gold"));
    BindExistingCardWidgets();
    BindExistingGlyphWidgets();
    RefreshPresentation();
    if (!IsDesignTime())
    {
        SetVisibility(ESlateVisibility::Collapsed);
    }
}

void USailFleetHUDWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    // PreConstruct is presentation-only. It makes the authored WBP readable in
    // Designer without ever constructing or altering its widget hierarchy.
    if (IsDesignTime() && CurrentShips.IsEmpty())
    {
        TArray<FSailShipHUDEntry> PreviewShips;

        FSailShipHUDEntry BluePreview;
        BluePreview.CaptainName = NSLOCTEXT("SailFleetHUD", "PreviewBlueCaptain", "CAPT. E. HARCOURT");
        BluePreview.ShipName = NSLOCTEXT("SailFleetHUD", "PreviewBlueShip", "HMS RESOLUTE");
        BluePreview.ShipClass = NSLOCTEXT("SailFleetHUD", "PreviewBlueClass", "FIRST-RATE | 100 GUNS");
        BluePreview.Faction = ESailFleetFaction::BlueFleet;
        BluePreview.ShipRank = 3;
        PreviewShips.Add(BluePreview);

        FSailShipHUDEntry RedPreview;
        RedPreview.CaptainName = NSLOCTEXT("SailFleetHUD", "PreviewRedCaptain", "CAPT. L. MOREAU");
        RedPreview.ShipName = NSLOCTEXT("SailFleetHUD", "PreviewRedShip", "LE VENGEUR");
        RedPreview.ShipClass = NSLOCTEXT("SailFleetHUD", "PreviewRedClass", "HEAVY FRIGATE | 44 GUNS");
        RedPreview.Faction = ESailFleetFaction::RedFleet;
        RedPreview.ShipRank = 2;
        RedPreview.HealthFraction = 0.68f;
        PreviewShips.Add(RedPreview);

        CurrentShips = MoveTemp(PreviewShips);
    }

    if (BoundCards.IsEmpty())
    {
        BindExistingCardWidgets();
    }
    RefreshPresentation();
}

void USailFleetHUDWidget::SetSelectedShips(const TArray<FSailShipHUDEntry>& InShips)
{
    CurrentShips = InShips;
    if (CurrentShips.Num() > MaxVisibleShipCards)
    {
        CurrentShips.SetNum(MaxVisibleShipCards);
    }
    RefreshPresentation();
}

void USailFleetHUDWidget::ClearSelection()
{
    CurrentShips.Reset();
    RefreshPresentation();
}

void USailFleetHUDWidget::SetBattleHUDVisible(bool bVisible)
{
    SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void USailFleetHUDWidget::SetDateGlyphTextures(
    UTexture2D* Weekday,
    UTexture2D* Punctuation,
    UTexture2D* DayTens,
    UTexture2D* DayOnes,
    UTexture2D* Month,
    UTexture2D* YearThousands,
    UTexture2D* YearHundreds,
    UTexture2D* YearTens,
    UTexture2D* YearOnes)
{
    ApplyGlyphTexture(TEXT("DateWeekday"), Weekday);
    ApplyGlyphTexture(TEXT("DatePunctuation"), Punctuation);
    ApplyGlyphTexture(TEXT("DateDayTens"), DayTens);
    ApplyGlyphTexture(TEXT("DateDayOnes"), DayOnes);
    ApplyGlyphTexture(TEXT("DateMonth"), Month);
    ApplyGlyphTexture(TEXT("DateYearThousands"), YearThousands);
    ApplyGlyphTexture(TEXT("DateYearHundreds"), YearHundreds);
    ApplyGlyphTexture(TEXT("DateYearTens"), YearTens);
    ApplyGlyphTexture(TEXT("DateYearOnes"), YearOnes);
}

void USailFleetHUDWidget::SetWindGlyphTextures(
    UTexture2D* Compass,
    UTexture2D* Arrow)
{
    ApplyGlyphTexture(TEXT("WindCompass"), Compass);
    ApplyGlyphTexture(TEXT("WindArrow"), Arrow);
}

void USailFleetHUDWidget::SetWindHeadingDegrees(const float HeadingDegrees)
{
    if (UImage* const* ArrowImage = BoundGlyphImages.Find(TEXT("WindArrow")))
    {
        if (*ArrowImage)
        {
            (*ArrowImage)->SetRenderTransformAngle(HeadingDegrees + 90.0f);
        }
    }
    if (UImage* const* GlowImage = BoundGlyphImages.Find(TEXT("WindArrowGlow")))
    {
        if (*GlowImage)
        {
            (*GlowImage)->SetRenderTransformAngle(HeadingDegrees + 90.0f);
        }
    }
}

void USailFleetHUDWidget::BindExistingCardWidgets()
{
    BoundCards.Reset(MaxVisibleShipCards);

    for (int32 CardIndex = 1; CardIndex <= MaxVisibleShipCards; ++CardIndex)
    {
        const FString Suffix = FString::Printf(TEXT("%02d"), CardIndex);
        const auto WidgetName = [&Suffix](const TCHAR* Prefix)
        {
            return FName(*(FString(Prefix) + Suffix));
        };
        FBoundShipCard Card;
        Card.Container = Cast<UBorder>(GetWidgetFromName(WidgetName(TEXT("CardContainer"))));
        Card.FactionGlow = Cast<UImage>(GetWidgetFromName(WidgetName(TEXT("FactionGlow"))));
        Card.LocketFrame = Cast<UImage>(GetWidgetFromName(WidgetName(TEXT("LocketFrame"))));
        Card.Portrait = Cast<UImage>(GetWidgetFromName(WidgetName(TEXT("Portrait"))));
        Card.CaptainName = Cast<UTextBlock>(GetWidgetFromName(WidgetName(TEXT("CaptainName"))));
        Card.ShipName = Cast<UTextBlock>(GetWidgetFromName(WidgetName(TEXT("ShipName"))));
        Card.ShipClass = Cast<UTextBlock>(GetWidgetFromName(WidgetName(TEXT("ShipClass"))));
        Card.RankText = Cast<UTextBlock>(GetWidgetFromName(WidgetName(TEXT("RankText"))));
        Card.HealthBar = Cast<UProgressBar>(GetWidgetFromName(WidgetName(TEXT("HealthBar"))));

        if (!Card.IsComplete())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("WBP_SailFleetHUD card %s is incomplete. Re-run Content/Python/create_ui_assets.py."),
                *Suffix);
        }
        BoundCards.Add(Card);
    }
}

void USailFleetHUDWidget::BindExistingGlyphWidgets()
{
    BoundGlyphImages.Reset();
    static const FName SlotNames[] =
    {
        TEXT("DateWeekday"),
        TEXT("DatePunctuation"),
        TEXT("DateDayTens"),
        TEXT("DateDayOnes"),
        TEXT("DateMonth"),
        TEXT("DateYearThousands"),
        TEXT("DateYearHundreds"),
        TEXT("DateYearTens"),
        TEXT("DateYearOnes"),
        TEXT("WindCompass"),
        TEXT("WindArrow")
    };

    for (const FName SlotName : SlotNames)
    {
        BoundGlyphImages.Add(
            SlotName,
            Cast<UImage>(GetWidgetFromName(FName(*(SlotName.ToString() + TEXT("Image"))))));
        BoundGlyphImages.Add(
            FName(*(SlotName.ToString() + TEXT("Glow"))),
            Cast<UImage>(GetWidgetFromName(FName(*(SlotName.ToString() + TEXT("Glow"))))));
    }
}

void USailFleetHUDWidget::ApplyGlyphTexture(FName SlotName, UTexture2D* Texture)
{
    if (!Texture)
    {
        return;
    }

    if (UImage* const* MainImage = BoundGlyphImages.Find(SlotName))
    {
        if (*MainImage)
        {
            (*MainImage)->SetBrushFromTexture(Texture, true);
            (*MainImage)->SetColorAndOpacity(FLinearColor::White);
        }
    }

    const FName GlowName(*(SlotName.ToString() + TEXT("Glow")));
    if (UImage* const* GlowImage = BoundGlyphImages.Find(GlowName))
    {
        if (*GlowImage)
        {
            (*GlowImage)->SetBrushFromTexture(Texture, true);
            (*GlowImage)->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.86f));
        }
    }
}

void USailFleetHUDWidget::RefreshPresentation()
{
    for (int32 CardIndex = 0; CardIndex < BoundCards.Num(); ++CardIndex)
    {
        FBoundShipCard& Card = BoundCards[CardIndex];
        if (!Card.Container)
        {
            continue;
        }

        const bool bHasShip = CurrentShips.IsValidIndex(CardIndex);
        Card.Container->SetVisibility(
            bHasShip ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
        if (bHasShip)
        {
            ApplyShipToCard(CurrentShips[CardIndex], Card);
        }
    }
}

void USailFleetHUDWidget::ApplyShipToCard(const FSailShipHUDEntry& Ship, FBoundShipCard& Card)
{
    const bool bBlueFleet = Ship.Faction == ESailFleetFaction::BlueFleet;

    const int32 Rank = FMath::Clamp(Ship.ShipRank, 1, 3);
    UTexture2D* LocketTexture =
        Rank == 3 ? GoldLocketTexture.Get() :
        Rank == 2 ? SilverLocketTexture.Get() :
                    BronzeLocketTexture.Get();
    if (LocketTexture)
    {
        Card.FactionGlow->SetBrushFromTexture(LocketTexture, true);
        Card.LocketFrame->SetBrushFromTexture(LocketTexture, true);
    }
    const FVector2D LocketRenderSize(166.0f, 170.0f);
    Card.FactionGlow->SetDesiredSizeOverride(LocketRenderSize);
    Card.LocketFrame->SetDesiredSizeOverride(LocketRenderSize);
    Card.Portrait->SetDesiredSizeOverride(LocketRenderSize);
    const FLinearColor FactionGlowColor =
        bBlueFleet
            ? FLinearColor(0.025f, 0.20f, 1.0f, 0.68f)
            : FLinearColor(1.0f, 0.025f, 0.012f, 0.68f);
    Card.FactionGlow->SetColorAndOpacity(FactionGlowColor);
    Card.LocketFrame->SetColorAndOpacity(FLinearColor::White);
    Card.RankText->SetText(
        Rank == 3 ? NSLOCTEXT("SailFleetHUD", "RankThree", "III  ADMIRAL FLAGSHIP") :
        Rank == 2 ? NSLOCTEXT("SailFleetHUD", "RankTwo", "II  SENIOR CAPTAIN") :
                    NSLOCTEXT("SailFleetHUD", "RankOne", "I  SHIP CAPTAIN"));

    Card.CaptainName->SetText(Ship.CaptainName);
    Card.ShipName->SetText(Ship.ShipName);
    Card.ShipClass->SetText(Ship.ShipClass);

    if (Ship.CaptainPortrait)
    {
        Card.Portrait->SetBrushFromTexture(Ship.CaptainPortrait, true);
        Card.Portrait->SetColorAndOpacity(FLinearColor::White);
    }
    else
    {
        Card.Portrait->SetColorAndOpacity(FLinearColor(0.16f, 0.14f, 0.11f, 1.0f));
    }

    const float Health = FMath::Clamp(Ship.HealthFraction, 0.0f, 1.0f);
    Card.HealthBar->SetPercent(Health);
    Card.HealthBar->SetFillColorAndOpacity(
        FMath::Lerp(SailFleetHUD::DamagedBar, SailFleetHUD::HealthyBar, Health));
}
