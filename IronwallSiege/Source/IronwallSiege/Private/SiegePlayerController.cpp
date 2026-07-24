#include "SiegePlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "SiegeTitleWidget.h"

void ASiegePlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController() && !FParse::Param(FCommandLine::Get(), TEXT("SkipTitle")))
    {
        TitleWidget = CreateWidget<USiegeTitleWidget>(this, USiegeTitleWidget::StaticClass());
        if (TitleWidget)
        {
            TitleWidget->AddToViewport(100);
            TitleWidget->ShowTitle();
        }
    }
}

void ASiegePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("ToggleMenu"), IE_Pressed, this, &ASiegePlayerController::ToggleMenu);
}

void ASiegePlayerController::ToggleMenu()
{
    if (!TitleWidget)
    {
        return;
    }

    if (TitleWidget->IsTitleVisible())
    {
        TitleWidget->SetVisibility(ESlateVisibility::Collapsed);
        bShowMouseCursor = false;
        SetInputMode(FInputModeGameOnly());
    }
    else
    {
        TitleWidget->ShowTitle();
    }
}
