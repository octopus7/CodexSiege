#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SailFleetHUDWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UImage;
class UProgressBar;
class UTexture2D;
class UTextBlock;

UENUM(BlueprintType)
enum class ESailFleetFaction : uint8
{
    BlueFleet UMETA(DisplayName = "Blue Fleet"),
    RedFleet UMETA(DisplayName = "Red Fleet")
};

USTRUCT(BlueprintType)
struct AGEOFSAILFLEET_API FSailShipHUDEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fleet HUD")
    FText CaptainName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fleet HUD")
    FText ShipName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fleet HUD")
    FText ShipClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fleet HUD")
    TObjectPtr<UTexture2D> CaptainPortrait = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fleet HUD")
    ESailFleetFaction Faction = ESailFleetFaction::BlueFleet;

    /** 1 = bronze, 2 = silver, 3 = admiral's gold. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fleet HUD", meta = (ClampMin = "1", ClampMax = "3"))
    int32 ShipRank = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fleet HUD", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HealthFraction = 1.0f;
};

/**
 * Native presentation model for WBP_SailFleetHUD.
 *
 * This class deliberately never creates or reparents widgets. The complete visual
 * hierarchy is authored by Content/Python/create_ui_assets.py and saved in the WBP.
 * Native code only binds to that existing hierarchy and updates presentation data.
 */
UCLASS(Abstract, Blueprintable)
class AGEOFSAILFLEET_API USailFleetHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    static constexpr int32 MaxVisibleShipCards = 8;

    UFUNCTION(BlueprintCallable, Category = "Fleet HUD")
    void SetSelectedShips(const TArray<FSailShipHUDEntry>& InShips);

    UFUNCTION(BlueprintCallable, Category = "Fleet HUD")
    void ClearSelection();

    UFUNCTION(BlueprintCallable, Category = "Fleet HUD")
    void SetBattleHUDVisible(bool bVisible);

    /** All date characters remain image glyphs; no date TextBlock is used. */
    UFUNCTION(BlueprintCallable, Category = "Fleet HUD|Date")
    void SetDateGlyphTextures(
        UTexture2D* Weekday,
        UTexture2D* Punctuation,
        UTexture2D* DayTens,
        UTexture2D* DayOnes,
        UTexture2D* Month,
        UTexture2D* YearThousands,
        UTexture2D* YearHundreds,
        UTexture2D* YearTens,
        UTexture2D* YearOnes);

    UFUNCTION(BlueprintCallable, Category = "Fleet HUD|Wind")
    void SetWindGlyphTextures(
        UTexture2D* Compass,
        UTexture2D* Arrow);

    UFUNCTION(BlueprintCallable, Category = "Fleet HUD|Wind")
    void SetWindHeadingDegrees(float HeadingDegrees);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UHorizontalBox> ShipCardRow;

private:
    struct FBoundShipCard
    {
        UBorder* Container = nullptr;
        UImage* FactionGlow = nullptr;
        UImage* LocketFrame = nullptr;
        UImage* Portrait = nullptr;
        UTextBlock* CaptainName = nullptr;
        UTextBlock* ShipName = nullptr;
        UTextBlock* ShipClass = nullptr;
        UTextBlock* RankText = nullptr;
        UProgressBar* HealthBar = nullptr;

        bool IsComplete() const;
    };

    TArray<FBoundShipCard> BoundCards;
    TMap<FName, UImage*> BoundGlyphImages;

    UPROPERTY(Transient)
    TArray<FSailShipHUDEntry> CurrentShips;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> BronzeLocketTexture;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> SilverLocketTexture;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> GoldLocketTexture;

    void BindExistingCardWidgets();
    void BindExistingGlyphWidgets();
    void RefreshPresentation();
    void ApplyShipToCard(const FSailShipHUDEntry& Ship, FBoundShipCard& Card);
    void ApplyGlyphTexture(FName SlotName, UTexture2D* Texture);
};
