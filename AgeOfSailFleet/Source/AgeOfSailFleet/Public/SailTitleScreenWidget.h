#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SailGraphicsMode.h"
#include "SailTitleScreenWidget.generated.h"

class UButton;
class UBorder;
class UCheckBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFleetDepartureRequested);

/**
 * Binding-only native layer for WBP_TitleScreen.
 * The complete title-screen tree is authored by create_ui_assets.py.
 */
UCLASS(Abstract, Blueprintable)
class AGEOFSAILFLEET_API USailTitleScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** A director/controller can bind StartBattle directly to this event. */
    UPROPERTY(BlueprintAssignable, Category = "Title Screen")
    FOnFleetDepartureRequested OnFleetDepartureRequested;

    UFUNCTION(BlueprintCallable, Category = "Title Screen")
    void SetDepartureEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Title Screen")
    void ResetTitleTransition();

    UFUNCTION(BlueprintPure, Category = "Title Screen")
    ESailGraphicsMode GetSelectedGraphicsMode() const { return SelectedGraphicsMode; }

protected:
    virtual void NativeOnInitialized() override;
    /** Delegate binding only; this function never constructs the widget tree. */
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UButton> DepartureButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UBorder> TitlePanel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UBorder> FullscreenBlackOverlay;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UCheckBox> GraphicMode3DCheckBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UCheckBox> GraphicMode2DCheckBox;

private:
    enum class EDepartureTransition : uint8
    {
        Idle,
        FadeTitle,
        FadeToBlack,
        FadeFromBlack,
        Complete
    };

    UFUNCTION()
    void HandleDepartureClicked();

    UFUNCTION()
    void Handle3DModeCheckChanged(bool bIsChecked);

    UFUNCTION()
    void Handle2DModeCheckChanged(bool bIsChecked);

    void SetSelectedGraphicsMode(ESailGraphicsMode GraphicsMode);
    void SynchronizeGraphicModeChecks();

    EDepartureTransition TransitionState = EDepartureTransition::Idle;
    ESailGraphicsMode SelectedGraphicsMode = ESailGraphicsMode::ThreeDimensional;
    float TransitionTime = 0.0f;
    bool bDepartureBroadcast = false;
    bool bUpdatingGraphicModeChecks = false;
};
