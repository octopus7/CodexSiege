#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SailFleetUIEditorLibrary.generated.h"

class UWidgetTree;
class UWidget;
class UFont;
class UFontFace;

/** Small editor bridge because UE 5.7 does not expose UWidgetBlueprint::WidgetTree to Python. */
UCLASS()
class AGEOFSAILFLEET_API USailFleetUIEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Fleet UI|Editor", meta = (DevelopmentOnly))
    static UWidgetTree* GetWidgetTree(UObject* WidgetBlueprintObject);

    UFUNCTION(BlueprintCallable, Category = "Fleet UI|Editor", meta = (DevelopmentOnly))
    static UWidget* ConstructWidget(UWidgetTree* WidgetTree, TSubclassOf<UWidget> WidgetClass, FName WidgetName);

    UFUNCTION(BlueprintCallable, Category = "Fleet UI|Editor", meta = (DevelopmentOnly))
    static void ClearWidgetTree(UWidgetTree* WidgetTree);

    UFUNCTION(BlueprintCallable, Category = "Fleet UI|Editor", meta = (DevelopmentOnly))
    static void SetRootWidget(UWidgetTree* WidgetTree, UWidget* RootWidget);

    UFUNCTION(BlueprintCallable, Category = "Fleet UI|Editor", meta = (DevelopmentOnly))
    static UWidget* FindWidget(UWidgetTree* WidgetTree, FName WidgetName);

    UFUNCTION(BlueprintCallable, Category = "Fleet UI|Editor", meta = (DevelopmentOnly))
    static void CompileWidgetBlueprint(UObject* WidgetBlueprintObject);

    /** Configure an authored UFont to use an imported FontFace for Slate/UMG. */
    UFUNCTION(BlueprintCallable, Category = "Fleet UI|Editor", meta = (DevelopmentOnly))
    static bool ConfigureRuntimeFont(UFont* FontAsset, UFontFace* FontFaceAsset);
};
