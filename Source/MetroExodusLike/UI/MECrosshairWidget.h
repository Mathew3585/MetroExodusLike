#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MECrosshairWidget.generated.h"

class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMECrosshairData;

/**
 * Hip-fire crosshair: its parts move apart with the weapon spread (shots, movement, jumps) and it fades out when aiming.
 * Style from the weapon's crosshair, else the default one. Logic only: the layout lives in WBP_Crosshair,
 * which must contain an Image named CrosshairImage, centered.
 */
UCLASS(Abstract)
class METROEXODUSLIKE_API UMECrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Used when the weapon has no crosshair of its own. */
	void SetDefaultCrosshair(UMECrosshairData* InDefaultCrosshair) { DefaultCrosshair = InDefaultCrosshair; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CrosshairImage;

	/** Draws the image spread from its center (M_Crosshair). */
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	TObjectPtr<UMaterialInterface> Material;

private:
	void ApplyStyle(UMECrosshairData* Style);

	UPROPERTY(Transient)
	TObjectPtr<UMECrosshairData> DefaultCrosshair;

	/** Style currently shown. */
	UPROPERTY(Transient)
	TObjectPtr<UMECrosshairData> Current;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;

	float Spread = 0.f;
};
