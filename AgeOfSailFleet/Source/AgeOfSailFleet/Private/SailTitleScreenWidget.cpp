#include "SailTitleScreenWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"

namespace SailTitleTransition
{
    constexpr float TitleFadeDuration = 0.45f;
    constexpr float BlackFadeInDuration = 0.55f;
    constexpr float BlackFadeOutDuration = 0.70f;
}

void USailTitleScreenWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    ResetTitleTransition();
}

void USailTitleScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (DepartureButton)
    {
        DepartureButton->OnClicked.AddUniqueDynamic(
            this, &USailTitleScreenWidget::HandleDepartureClicked);
    }
    SetSelectedGraphicsMode(ESailGraphicsMode::ThreeDimensional);
    if (GraphicMode3DCheckBox)
    {
        GraphicMode3DCheckBox->OnCheckStateChanged.AddUniqueDynamic(
            this, &USailTitleScreenWidget::Handle3DModeCheckChanged);
    }
    if (GraphicMode2DCheckBox)
    {
        GraphicMode2DCheckBox->OnCheckStateChanged.AddUniqueDynamic(
            this, &USailTitleScreenWidget::Handle2DModeCheckChanged);
    }
}

void USailTitleScreenWidget::NativeDestruct()
{
    if (DepartureButton)
    {
        DepartureButton->OnClicked.RemoveDynamic(
            this, &USailTitleScreenWidget::HandleDepartureClicked);
    }
    if (GraphicMode3DCheckBox)
    {
        GraphicMode3DCheckBox->OnCheckStateChanged.RemoveDynamic(
            this, &USailTitleScreenWidget::Handle3DModeCheckChanged);
    }
    if (GraphicMode2DCheckBox)
    {
        GraphicMode2DCheckBox->OnCheckStateChanged.RemoveDynamic(
            this, &USailTitleScreenWidget::Handle2DModeCheckChanged);
    }

    Super::NativeDestruct();
}

void USailTitleScreenWidget::SetDepartureEnabled(bool bEnabled)
{
    if (DepartureButton)
    {
        DepartureButton->SetIsEnabled(bEnabled);
    }
}

void USailTitleScreenWidget::ResetTitleTransition()
{
    TransitionState = EDepartureTransition::Idle;
    TransitionTime = 0.0f;
    bDepartureBroadcast = false;
    SetVisibility(ESlateVisibility::Visible);
    SetDepartureEnabled(true);

    SetTitleVisualOpacity(1.0f);
    if (FullscreenBlackOverlay)
    {
        FullscreenBlackOverlay->SetRenderOpacity(0.0f);
        FullscreenBlackOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void USailTitleScreenWidget::Handle3DModeCheckChanged(const bool bIsChecked)
{
    if (bUpdatingGraphicModeChecks)
    {
        return;
    }
    if (bIsChecked)
    {
        SetSelectedGraphicsMode(ESailGraphicsMode::ThreeDimensional);
        return;
    }
    SynchronizeGraphicModeChecks();
}

void USailTitleScreenWidget::Handle2DModeCheckChanged(const bool bIsChecked)
{
    if (bUpdatingGraphicModeChecks)
    {
        return;
    }
    if (bIsChecked)
    {
        SetSelectedGraphicsMode(ESailGraphicsMode::TwoDimensional);
        return;
    }
    SynchronizeGraphicModeChecks();
}

void USailTitleScreenWidget::SetSelectedGraphicsMode(
    const ESailGraphicsMode GraphicsMode)
{
    SelectedGraphicsMode = GraphicsMode;
    SynchronizeGraphicModeChecks();
}

void USailTitleScreenWidget::SynchronizeGraphicModeChecks()
{
    TGuardValue<bool> UpdatingGuard(bUpdatingGraphicModeChecks, true);
    if (GraphicMode3DCheckBox)
    {
        GraphicMode3DCheckBox->SetIsChecked(
            SelectedGraphicsMode == ESailGraphicsMode::ThreeDimensional);
    }
    if (GraphicMode2DCheckBox)
    {
        GraphicMode2DCheckBox->SetIsChecked(
            SelectedGraphicsMode == ESailGraphicsMode::TwoDimensional);
    }
}

void USailTitleScreenWidget::SetTitleVisualOpacity(const float Opacity)
{
    const float ClampedOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
    const ESlateVisibility ContentVisibility =
        ClampedOpacity <= UE_KINDA_SMALL_NUMBER
            ? ESlateVisibility::Collapsed
            : ESlateVisibility::Visible;

    if (TitlePanel)
    {
        TitlePanel->SetRenderOpacity(ClampedOpacity);
        TitlePanel->SetVisibility(ContentVisibility);
    }

    // Rounded-box checkbox brushes can retain their last Slate draw element
    // for a frame when only an ancestor's render opacity changes. Drive their
    // opacity explicitly and collapse them at zero so no radio-ring ghost is
    // left over during the subsequent black-screen transition.
    if (GraphicMode3DCheckBox)
    {
        GraphicMode3DCheckBox->SetRenderOpacity(ClampedOpacity);
        GraphicMode3DCheckBox->SetVisibility(ContentVisibility);
    }
    if (GraphicMode2DCheckBox)
    {
        GraphicMode2DCheckBox->SetRenderOpacity(ClampedOpacity);
        GraphicMode2DCheckBox->SetVisibility(ContentVisibility);
    }
}

void USailTitleScreenWidget::HandleDepartureClicked()
{
    if (TransitionState != EDepartureTransition::Idle)
    {
        return;
    }

    // Lock immediately to prevent repeated StartBattle requests.
    SetDepartureEnabled(false);
    TransitionTime = 0.0f;
    TransitionState = EDepartureTransition::FadeTitle;
}

void USailTitleScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (TransitionState == EDepartureTransition::Idle
        || TransitionState == EDepartureTransition::Complete)
    {
        return;
    }

    TransitionTime += InDeltaTime;
    if (TransitionState == EDepartureTransition::FadeTitle)
    {
        const float Alpha = FMath::Clamp(
            TransitionTime / SailTitleTransition::TitleFadeDuration, 0.0f, 1.0f);
        SetTitleVisualOpacity(1.0f - Alpha);
        if (Alpha >= 1.0f)
        {
            TransitionState = EDepartureTransition::FadeToBlack;
            TransitionTime = 0.0f;
        }
        return;
    }

    if (TransitionState == EDepartureTransition::FadeToBlack)
    {
        const float Alpha = FMath::Clamp(
            TransitionTime / SailTitleTransition::BlackFadeInDuration, 0.0f, 1.0f);
        if (FullscreenBlackOverlay)
        {
            FullscreenBlackOverlay->SetRenderOpacity(Alpha);
        }
        if (Alpha >= 1.0f)
        {
            if (!bDepartureBroadcast)
            {
                bDepartureBroadcast = true;
                OnFleetDepartureRequested.Broadcast();
            }
            TransitionState = EDepartureTransition::FadeFromBlack;
            TransitionTime = 0.0f;
        }
        return;
    }

    const float Alpha = FMath::Clamp(
        TransitionTime / SailTitleTransition::BlackFadeOutDuration, 0.0f, 1.0f);
    if (FullscreenBlackOverlay)
    {
        FullscreenBlackOverlay->SetRenderOpacity(1.0f - Alpha);
    }
    if (Alpha >= 1.0f)
    {
        TransitionState = EDepartureTransition::Complete;
        SetVisibility(ESlateVisibility::Collapsed);
    }
}
