#include "SiegeTitleWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SiegeGameInstance.h"
#include "SiegeTextureLoader.h"

namespace
{
    void FillCanvas(UWidget* Widget, const int32 ZOrder)
    {
        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
        {
            Slot->SetAnchors(FAnchors(0, 0, 1, 1));
            Slot->SetOffsets(FMargin(0));
            Slot->SetZOrder(ZOrder);
        }
    }

    void SetTextSize(UTextBlock* Text, const int32 Size)
    {
        FSlateFontInfo Font = Text->GetFont();
        Font.Size = Size;
        Text->SetFont(Font);
    }
}

void USiegeTitleWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (WidgetTree->RootWidget)
    {
        ShowTitle();
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    WidgetTree->RootWidget = Canvas;

    UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TitleBackdrop"));
    if (UTexture2D* Texture = FSiegeTextureLoader::LoadPNGFromContent(TEXT("Raw/UI/TitleBackdrop.png"), this))
    {
        Background->SetBrushFromTexture(Texture, true);
    }
    Background->SetColorAndOpacity(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f));
    Canvas->AddChild(Background);
    FillCanvas(Background, 0);

    UBorder* Shade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CinematicShade"));
    Shade->SetBrushColor(FLinearColor(0.005f, 0.008f, 0.012f, 0.48f));
    Canvas->AddChild(Shade);
    FillCanvas(Shade, 1);

    MainMenu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainMenu"));
    Canvas->AddChild(MainMenu);
    if (UCanvasPanelSlot* MainMenuSlot = Cast<UCanvasPanelSlot>(MainMenu->Slot))
    {
        MainMenuSlot->SetAnchors(FAnchors(0.08f, 0.21f));
        MainMenuSlot->SetAlignment(FVector2D(0, 0));
        MainMenuSlot->SetSize(FVector2D(540, 620));
        MainMenuSlot->SetZOrder(2);
    }

    UTextBlock* Title = AddText(MainMenu, TEXT("IRONWALL SIEGE"), 54, FLinearColor(0.92f, 0.83f, 0.61f));
    Title->SetShadowOffset(FVector2D(3, 3));
    Title->SetShadowColorAndOpacity(FLinearColor::Black);
    AddText(MainMenu, TEXT("A DATA-DRIVEN MEDIEVAL SIEGE PROTOTYPE"), 15, FLinearColor(0.72f, 0.75f, 0.78f));

    USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>();
    Spacer->SetSize(FVector2D(1, 70));
    MainMenu->AddChildToVerticalBox(Spacer);

    UButton* StartButton = AddMenuButton(MainMenu, TEXT("BEGIN SIEGE"));
    StartButton->OnClicked.AddDynamic(this, &USiegeTitleWidget::HandleStartClicked);
    UButton* OptionsButton = AddMenuButton(MainMenu, TEXT("OPTIONS"));
    OptionsButton->OnClicked.AddDynamic(this, &USiegeTitleWidget::HandleOptionsClicked);
    UButton* QuitButton = AddMenuButton(MainMenu, TEXT("QUIT"));
    QuitButton->OnClicked.AddDynamic(this, &USiegeTitleWidget::HandleQuitClicked);

    OptionsMenu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OptionsMenu"));
    Canvas->AddChild(OptionsMenu);
    if (UCanvasPanelSlot* OptionsMenuSlot = Cast<UCanvasPanelSlot>(OptionsMenu->Slot))
    {
        OptionsMenuSlot->SetAnchors(FAnchors(0.08f, 0.22f));
        OptionsMenuSlot->SetSize(FVector2D(620, 590));
        OptionsMenuSlot->SetZOrder(3);
    }

    AddText(OptionsMenu, TEXT("OPTIONS"), 44, FLinearColor(0.92f, 0.83f, 0.61f));
    AddText(OptionsMenu, TEXT("VISUAL RESOURCE SET"), 17, FLinearColor(0.74f, 0.78f, 0.82f));

    ResourceSetCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("ResourceSetCombo"));
    ResourceSetCombo->SetContentPadding(FMargin(18, 12));
    OptionsMenu->AddChildToVerticalBox(ResourceSetCombo);
    ResourceSetCombo->OnSelectionChanged.AddDynamic(this, &USiegeTitleWidget::HandleResourceSelection);

    if (USiegeGameInstance* GameInstance = Cast<USiegeGameInstance>(GetGameInstance()))
    {
        const TArray<FString> Labels = GameInstance->GetResourceSetLabels();
        for (const FString& Label : Labels)
        {
            ResourceSetCombo->AddOption(Label);
        }

        const int32 SelectedIndex = GameInstance->GetSelectedResourceSetIndex();
        if (Labels.IsValidIndex(SelectedIndex))
        {
            ResourceSetCombo->SetSelectedOption(Labels[SelectedIndex]);
        }
    }

    ResourceStatus = AddText(
        OptionsMenu,
        TEXT("Production assets use procedural geometry until matching Blender meshes are imported."),
        14,
        FLinearColor(0.65f, 0.68f, 0.71f));
    ResourceStatus->SetAutoWrapText(true);

    UButton* BackButton = AddMenuButton(OptionsMenu, TEXT("BACK"));
    BackButton->OnClicked.AddDynamic(this, &USiegeTitleWidget::HandleBackClicked);
    OptionsMenu->SetVisibility(ESlateVisibility::Collapsed);

    ShowTitle();
}

UTextBlock* USiegeTitleWidget::AddText(
    UVerticalBox* Parent,
    const FString& Text,
    const int32 Size,
    const FLinearColor& Color)
{
    UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
    Block->SetText(FText::FromString(Text));
    Block->SetColorAndOpacity(FSlateColor(Color));
    SetTextSize(Block, Size);
    UVerticalBoxSlot* TextSlot = Parent->AddChildToVerticalBox(Block);
    TextSlot->SetPadding(FMargin(0, 4, 0, 8));
    return Block;
}

UButton* USiegeTitleWidget::AddMenuButton(UVerticalBox* Parent, const FString& Label)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>();
    Button->SetBackgroundColor(FLinearColor(0.10f, 0.12f, 0.14f, 0.90f));

    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString(Label));
    Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.88f, 0.80f)));
    Text->SetJustification(ETextJustify::Center);
    Text->SetMargin(FMargin(12));
    SetTextSize(Text, 22);
    Button->AddChild(Text);

    UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button);
    ButtonSlot->SetPadding(FMargin(0, 7, 0, 7));
    return Button;
}

void USiegeTitleWidget::ShowTitle()
{
    SetVisibility(ESlateVisibility::Visible);
    if (MainMenu)
    {
        MainMenu->SetVisibility(ESlateVisibility::Visible);
    }
    if (OptionsMenu)
    {
        OptionsMenu->SetVisibility(ESlateVisibility::Collapsed);
    }
    SetGameplayInput(false);
}

bool USiegeTitleWidget::IsTitleVisible() const
{
    return GetVisibility() == ESlateVisibility::Visible;
}

void USiegeTitleWidget::SetGameplayInput(const bool bGameplay)
{
    if (APlayerController* Controller = GetOwningPlayer())
    {
        Controller->bShowMouseCursor = !bGameplay;
        if (bGameplay)
        {
            Controller->SetInputMode(FInputModeGameOnly());
        }
        else
        {
            FInputModeUIOnly InputMode;
            InputMode.SetWidgetToFocus(TakeWidget());
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            Controller->SetInputMode(InputMode);
        }
    }
}

void USiegeTitleWidget::HandleStartClicked()
{
    SetVisibility(ESlateVisibility::Collapsed);
    SetGameplayInput(true);
}

void USiegeTitleWidget::HandleOptionsClicked()
{
    MainMenu->SetVisibility(ESlateVisibility::Collapsed);
    OptionsMenu->SetVisibility(ESlateVisibility::Visible);
}

void USiegeTitleWidget::HandleBackClicked()
{
    OptionsMenu->SetVisibility(ESlateVisibility::Collapsed);
    MainMenu->SetVisibility(ESlateVisibility::Visible);
}

void USiegeTitleWidget::HandleQuitClicked()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void USiegeTitleWidget::HandleResourceSelection(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectionType;
    if (USiegeGameInstance* GameInstance = Cast<USiegeGameInstance>(GetGameInstance()))
    {
        const TArray<FString> Labels = GameInstance->GetResourceSetLabels();
        const int32 Index = Labels.IndexOfByKey(SelectedItem);
        if (Index != INDEX_NONE)
        {
            GameInstance->SelectResourceSetByIndex(Index);
            if (ResourceStatus)
            {
                const bool bPrototype = Index == 0;
                ResourceStatus->SetText(FText::FromString(
                    bPrototype
                        ? TEXT("Procedural prototype geometry is active.")
                        : TEXT("Blender production slots are active. Missing meshes fall back to procedural geometry.")));
            }
        }
    }
}
