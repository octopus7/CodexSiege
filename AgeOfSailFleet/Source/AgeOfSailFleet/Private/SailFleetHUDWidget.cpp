#include "SailFleetHUDWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "FleetPlayerController.h"
#include "Rendering/DrawElements.h"

namespace SailFleetHUD
{
    const FLinearColor HealthyBar(0.12f, 0.46f, 0.23f, 1.0f);
    const FLinearColor DamagedBar(0.67f, 0.13f, 0.06f, 1.0f);
}

bool USailFleetHUDWidget::FBoundShipCard::IsComplete() const
{
    return Container && FactionGlow && AccentTop && AccentLeft && AccentRight
        && AccentBottom && AccentDivider && Portrait && CornerTopLeft
        && CornerTopRight && CornerBottomLeft && CornerBottomRight
        && CaptainName && ShipName && ShipClass && RankText && HealthBar;
}

void USailFleetHUDWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
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

int32 USailFleetHUDWidget::NativePaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    const int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    const bool bParentEnabled) const
{
    const int32 BaseLayer = Super::NativePaint(
        Args,
        AllottedGeometry,
        MyCullingRect,
        OutDrawElements,
        LayerId,
        InWidgetStyle,
        bParentEnabled);

    const AFleetPlayerController* FleetController =
        GetOwningPlayer<AFleetPlayerController>();
    if (!FleetController || !FleetController->IsDragSelecting())
    {
        return BaseLayer;
    }

    const float ViewportScale = FMath::Max(
        UWidgetLayoutLibrary::GetViewportScale(this),
        KINDA_SMALL_NUMBER);
    const FVector2D Start = FleetController->GetDragStart() / ViewportScale;
    const FVector2D Current = FleetController->GetDragCurrent() / ViewportScale;
    const FVector2D Minimum(
        FMath::Min(Start.X, Current.X),
        FMath::Min(Start.Y, Current.Y));
    const FVector2D Maximum(
        FMath::Max(Start.X, Current.X),
        FMath::Max(Start.Y, Current.Y));

    if ((Maximum - Minimum).SizeSquared() < 4.0f)
    {
        return BaseLayer;
    }

    TArray<FVector2D> OutlinePoints;
    OutlinePoints.Reserve(5);
    OutlinePoints.Add(Minimum);
    OutlinePoints.Add(FVector2D(Maximum.X, Minimum.Y));
    OutlinePoints.Add(Maximum);
    OutlinePoints.Add(FVector2D(Minimum.X, Maximum.Y));
    OutlinePoints.Add(Minimum);

    const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        BaseLayer + 1,
        PaintGeometry,
        OutlinePoints,
        ESlateDrawEffect::None,
        FLinearColor(0.01f, 0.16f, 0.32f, 0.82f),
        true,
        7.0f);
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        BaseLayer + 2,
        PaintGeometry,
        OutlinePoints,
        ESlateDrawEffect::None,
        FLinearColor(0.62f, 0.94f, 1.0f, 1.0f),
        true,
        2.2f);

    return BaseLayer + 2;
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
    if (DateWindPanel)
    {
        DateWindPanel->SetVisibility(
            bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
        DateWindPanel->SetRenderOpacity(bVisible ? 1.0f : 0.0f);
    }
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
    UTexture2D* /*Compass*/,
    UTexture2D* Arrow)
{
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
        Card.FactionGlow = Cast<UBorder>(GetWidgetFromName(WidgetName(TEXT("FactionWash"))));
        Card.AccentTop = Cast<UBorder>(GetWidgetFromName(WidgetName(TEXT("AccentTop"))));
        Card.AccentLeft = Cast<UBorder>(GetWidgetFromName(WidgetName(TEXT("AccentLeft"))));
        Card.AccentRight = Cast<UBorder>(GetWidgetFromName(WidgetName(TEXT("AccentRight"))));
        Card.AccentBottom = Cast<UBorder>(GetWidgetFromName(WidgetName(TEXT("AccentBottom"))));
        Card.AccentDivider = Cast<UBorder>(GetWidgetFromName(WidgetName(TEXT("AccentDivider"))));
        Card.Portrait = Cast<UImage>(GetWidgetFromName(WidgetName(TEXT("Portrait"))));
        Card.CornerTopLeft = Cast<UTextBlock>(GetWidgetFromName(WidgetName(TEXT("CornerTopLeft"))));
        Card.CornerTopRight = Cast<UTextBlock>(GetWidgetFromName(WidgetName(TEXT("CornerTopRight"))));
        Card.CornerBottomLeft = Cast<UTextBlock>(GetWidgetFromName(WidgetName(TEXT("CornerBottomLeft"))));
        Card.CornerBottomRight = Cast<UTextBlock>(GetWidgetFromName(WidgetName(TEXT("CornerBottomRight"))));
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
        TEXT("WindArrow")
    };

    for (const FName SlotName : SlotNames)
    {
        UImage* MainImage = Cast<UImage>(
            GetWidgetFromName(FName(*(SlotName.ToString() + TEXT("Image")))));
        UImage* GlowImage = Cast<UImage>(
            GetWidgetFromName(FName(*(SlotName.ToString() + TEXT("Glow")))));
        BoundGlyphImages.Add(SlotName, MainImage);
        BoundGlyphImages.Add(
            FName(*(SlotName.ToString() + TEXT("Glow"))),
            GlowImage);

        if (MainImage)
        {
            MainImage->SetVisibility(ESlateVisibility::HitTestInvisible);
            MainImage->SetRenderOpacity(1.0f);
        }
        if (GlowImage)
        {
            GlowImage->SetVisibility(ESlateVisibility::HitTestInvisible);
            GlowImage->SetRenderOpacity(1.0f);
        }
    }
}

void USailFleetHUDWidget::ApplyGlyphTexture(FName SlotName, UTexture2D* Texture)
{
    if (!Texture)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Fleet HUD glyph texture missing for slot %s"),
            *SlotName.ToString());
        return;
    }

    if (UImage* const* MainImage = BoundGlyphImages.Find(SlotName))
    {
        if (*MainImage)
        {
            (*MainImage)->SetBrushFromTexture(Texture, true);
            (*MainImage)->SetColorAndOpacity(FLinearColor::White);
            (*MainImage)->SetVisibility(ESlateVisibility::HitTestInvisible);
            (*MainImage)->SetRenderOpacity(1.0f);
        }
    }

    const FName GlowName(*(SlotName.ToString() + TEXT("Glow")));
    if (UImage* const* GlowImage = BoundGlyphImages.Find(GlowName))
    {
        if (*GlowImage)
        {
            (*GlowImage)->SetBrushFromTexture(Texture, true);
            (*GlowImage)->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.86f));
            (*GlowImage)->SetVisibility(ESlateVisibility::HitTestInvisible);
            (*GlowImage)->SetRenderOpacity(1.0f);
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
    const FLinearColor AccentColor =
        Rank == 3 ? FLinearColor(1.0f, 0.48f, 0.025f, 1.0f) :
        Rank == 2 ? FLinearColor(0.08f, 0.58f, 1.0f, 1.0f) :
                    FLinearColor(1.0f, 0.23f, 0.025f, 1.0f);
    const FLinearColor MutedAccent(
        AccentColor.R * 0.62f,
        AccentColor.G * 0.62f,
        AccentColor.B * 0.62f,
        0.82f);
    const FLinearColor FactionColor =
        bBlueFleet
            ? FLinearColor(0.025f, 0.28f, 1.0f, 0.82f)
            : FLinearColor(1.0f, 0.035f, 0.012f, 0.82f);
    Card.FactionGlow->SetBrushColor(FactionColor);
    Card.AccentTop->SetBrushColor(AccentColor);
    Card.AccentLeft->SetBrushColor(MutedAccent);
    Card.AccentRight->SetBrushColor(MutedAccent);
    Card.AccentBottom->SetBrushColor(AccentColor);
    Card.AccentDivider->SetBrushColor(FLinearColor(
        AccentColor.R,
        AccentColor.G,
        AccentColor.B,
        0.72f));
    Card.CornerTopLeft->SetColorAndOpacity(FSlateColor(AccentColor));
    Card.CornerTopRight->SetColorAndOpacity(FSlateColor(AccentColor));
    Card.CornerBottomLeft->SetColorAndOpacity(FSlateColor(MutedAccent));
    Card.CornerBottomRight->SetColorAndOpacity(FSlateColor(MutedAccent));
    Card.CaptainName->SetText(Ship.CaptainName);
    Card.ShipName->SetText(Ship.ShipName);
    // Keep both authored slots in the layout so the card retains its original
    // height and lower whitespace, but display only captain and ship names.
    Card.ShipClass->SetVisibility(ESlateVisibility::Hidden);
    Card.RankText->SetVisibility(ESlateVisibility::Hidden);

    if (Ship.CaptainPortrait)
    {
        Card.Portrait->SetBrushFromTexture(Ship.CaptainPortrait, true);
        Card.Portrait->SetDesiredSizeOverride(FVector2D(166.0f, 154.0f));
        Card.Portrait->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
        Card.Portrait->SetRenderScale(FVector2D(1.0f, 1.0f));
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
