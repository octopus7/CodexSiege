#include "SailFleetUIEditorLibrary.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"

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

bool USailFleetUIEditorLibrary::ConfigureRuntimeFont(
    UFont* FontAsset,
    UFontFace* FontFaceAsset)
{
#if WITH_EDITOR
    if (!FontAsset || !FontFaceAsset)
    {
        return false;
    }

    FontAsset->Modify();
    FontAsset->FontCacheType = EFontCacheType::Runtime;
    FontAsset->RuntimeFontSource = ERuntimeFontSource::Asset;

    FCompositeFont& CompositeFont = FontAsset->GetMutableInternalCompositeFont();
    CompositeFont.DefaultTypeface.Fonts.Reset();
    FTypefaceEntry& RegularFace =
        CompositeFont.DefaultTypeface.Fonts.Emplace_GetRef(FName(TEXT("Regular")));
    RegularFace.Font = FFontData(FontFaceAsset);
    CompositeFont.MakeDirty();

    FontAsset->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}
