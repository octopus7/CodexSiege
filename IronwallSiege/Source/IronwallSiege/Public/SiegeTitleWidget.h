#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "SiegeTitleWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

UCLASS()
class IRONWALLSIEGE_API USiegeTitleWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    void ShowTitle();
    bool IsTitleVisible() const;

private:
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> MainMenu;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> OptionsMenu;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> ResourceSetCombo;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ResourceStatus;

    UButton* AddMenuButton(UVerticalBox* Parent, const FString& Label);
    UTextBlock* AddText(UVerticalBox* Parent, const FString& Text, int32 Size, const FLinearColor& Color);
    void SetGameplayInput(bool bGameplay);

    UFUNCTION()
    void HandleStartClicked();

    UFUNCTION()
    void HandleOptionsClicked();

    UFUNCTION()
    void HandleBackClicked();

    UFUNCTION()
    void HandleQuitClicked();

    UFUNCTION()
    void HandleResourceSelection(FString SelectedItem, ESelectInfo::Type SelectionType);
};
