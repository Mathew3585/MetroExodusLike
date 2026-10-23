#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MECrosshairData.generated.h"

class UTexture2D;

/**
 * A crosshair style: one image, spread apart from its center as the weapon spread grows.
 * One asset per style (DA_Crosshair_Cross...). Chosen on the player controller, a weapon can override it.
 */
UCLASS(BlueprintType)
class METROEXODUSLIKE_API UMECrosshairData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Whole crosshair, centered in the image. */
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	TObjectPtr<UTexture2D> Texture;

	/** Size on screen at rest, in Slate units (pixels at 1080p). */
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair", meta = (ClampMin = "1"))
	float Size = 48.f;

	/** Tint and opacity. */
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair")
	FLinearColor Color = FLinearColor(1.f, 1.f, 1.f, 0.8f);

	/** Middle part that never moves (center dot), as a share of the image radius. 0 = everything spreads. */
	UPROPERTY(EditDefaultsOnly, Category = "Crosshair", meta = (ClampMin = "0", ClampMax = "1"))
	float CenterRadius = 0.f;

	// --- Spread ---

	/** 1 = the parts move apart exactly as much as the bullet cone grows. */
	UPROPERTY(EditDefaultsOnly, Category = "Spread", meta = (ClampMin = "0"))
	float SpreadScale = 1.f;

	/** The parts never move further apart than this, in Slate units. */
	UPROPERTY(EditDefaultsOnly, Category = "Spread", meta = (ClampMin = "0"))
	float MaxSpread = 40.f;

	/** How fast the crosshair follows the spread (higher = snappier). */
	UPROPERTY(EditDefaultsOnly, Category = "Spread", meta = (ClampMin = "0"))
	float SmoothingSpeed = 14.f;
};
