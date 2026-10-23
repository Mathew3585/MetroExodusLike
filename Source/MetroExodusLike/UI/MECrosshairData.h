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

	// --- Hit marker: flashes when a shot hurts something, also while aiming ---

	/** None = no hit marker. */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Marker")
	TObjectPtr<UTexture2D> HitMarkerTexture;

	/** Size on screen, in Slate units. */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Marker", meta = (ClampMin = "1"))
	float HitMarkerSize = 24.f;

	/** Time to fade out. */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Marker", meta = (ClampMin = "0.01", Units = "s"))
	float HitMarkerDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Hit Marker")
	FLinearColor HitColor = FLinearColor(1.f, 1.f, 1.f, 0.9f);

	/** The shot that kills. */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Marker")
	FLinearColor KillColor = FLinearColor(0.85f, 0.1f, 0.05f, 0.95f);

	/** Pops at this size and shrinks back to normal (a kill pops bigger). */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Marker", meta = (ClampMin = "1"))
	float HitMarkerPop = 1.25f;
};
