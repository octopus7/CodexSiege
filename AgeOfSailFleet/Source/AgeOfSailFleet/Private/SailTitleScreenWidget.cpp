#include "SailTitleScreenWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"

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
    if (GraphicModeComboBox)
    {
        GraphicModeComboBox->ClearOptions();
        GraphicModeComboBox->AddOption(TEXT("3D"));
        GraphicModeComboBox->AddOption(TEXT("2D"));
        GraphicModeComboBox->SetSelectedOption(
            SelectedGraphicsMode == ESailGraphicsMode::TwoDimensional
                ? TEXT("2D")
                : TEXT("3D"));
        GraphicModeComboBox->OnSelectionChanged.AddUniqueDynamic(
            this, &USailTitleScreenWidget::HandleGraphicModeChanged);
    }
}

void USailTitleScreenWidget::NativeDestruct()
{
    if (DepartureButton)
    {
        DepartureButton->OnClicked.RemoveDynamic(
            this, &USailTitleScreenWidget::HandleDepartureClicked);
    }
    if (GraphicModeComboBox)
    {
        GraphicModeComboBox->OnSelectionChanged.RemoveDynamic(
            this, &USailTitleScreenWidget::HandleGraphicModeChanged);
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

    if (TitlePanel)
    {
        TitlePanel->SetRenderOpacity(1.0f);
    }
    if (FullscreenBlackOverlay)
    {
        FullscreenBlackOverlay->SetRenderOpacity(0.0f);
        FullscreenBlackOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void USailTitleScreenWidget::HandleGraphicModeChanged(
    const FString SelectedItem,
    ESelectInfo::Type SelectionType)
{
    (void)SelectionType;
    SelectedGraphicsMode =
        SelectedItem.Equals(TEXT("2D"), ESearchCase::IgnoreCase)
            ? ESailGraphicsMode::TwoDimensional
            : ESailGraphicsMode::ThreeDimensional;
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
        if (TitlePanel)
        {
            TitlePanel->SetRenderOpacity(1.0f - Alpha);
        }
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
