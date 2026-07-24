#include "SailFleetUIEditorLibrary.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"

#if WITH_EDITOR
#include "Kismet2/KismetEditorUtilities.h"
#include "WidgetBlueprint.h"
#endif

UWidgetTree* USailFleetUIEditorLibrary::GetWidgetTree(UObject* WidgetBlueprintObject)
{
#if WITH_EDITOR
    if (UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(WidgetBlueprintObject))
    {
        return WidgetBlueprint->WidgetTree;
    }
#endif
    return nullptr;
}

UWidget* USailFleetUIEditorLibrary::ConstructWidget(
    UWidgetTree* WidgetTree,
    TSubclassOf<UWidget> WidgetClass,
    FName WidgetName)
{
#if WITH_EDITOR
    if (WidgetTree && WidgetClass)
    {
        UWidget* Widget = WidgetTree->ConstructWidget<UWidget>(WidgetClass, WidgetName);
        if (Widget)
        {
            Widget->bIsVariable = true;
        }
        return Widget;
    }
#endif
    return nullptr;
}

void USailFleetUIEditorLibrary::ClearWidgetTree(UWidgetTree* WidgetTree)
{
#if WITH_EDITOR
    if (WidgetTree && WidgetTree->RootWidget)
    {
        WidgetTree->RemoveWidget(WidgetTree->RootWidget);
        WidgetTree->RootWidget = nullptr;
    }
#endif
}

void USailFleetUIEditorLibrary::SetRootWidget(UWidgetTree* WidgetTree, UWidget* RootWidget)
{
#if WITH_EDITOR
    if (WidgetTree)
    {
        WidgetTree->RootWidget = RootWidget;
    }
#endif
}

UWidget* USailFleetUIEditorLibrary::FindWidget(UWidgetTree* WidgetTree, FName WidgetName)
{
    return WidgetTree ? WidgetTree->FindWidget(WidgetName) : nullptr;
}

void USailFleetUIEditorLibrary::CompileWidgetBlueprint(UObject* WidgetBlueprintObject)
{
#if WITH_EDITOR
    if (UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(WidgetBlueprintObject))
    {
        FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
    }
#endif
}
