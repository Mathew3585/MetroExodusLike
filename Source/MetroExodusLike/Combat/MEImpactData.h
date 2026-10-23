#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MEImpactData.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class UPhysicalMaterial;
class USoundBase;
struct FHitResult;

/** What a bullet leaves on one kind of surface. Every part is optional. */
USTRUCT(BlueprintType)
struct FMEImpactEffect
{
	GENERATED_BODY()

	/** Bullet hole (deferred decal material). */
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInterface> Decal;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "cm"))
	float DecalSize = 5.f;

	/** Random share of the size, either way (0.2 = +/- 20%). */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "1"))
	float DecalSizeVariation = 0.2f;

	/** Dust, sparks, splinters... Spawned facing out of the surface. */
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> Effect;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> Sound;
};

/**
 * Bullet impacts per surface: the hit physical material picks the entry (PM_Concrete, PM_Metal...).
 * One asset per kind of projectile (DA_Impacts_Bullet...), set on the weapon.
 */
UCLASS(BlueprintType)
class METROEXODUSLIKE_API UMEImpactData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Spawns the decal, effect and sound of the hit surface. */
	void SpawnImpact(const UObject* WorldContext, const FHitResult& Hit) const;

	/** Surfaces without a physical material, or not listed below. */
	UPROPERTY(EditDefaultsOnly, Category = "Impacts")
	FMEImpactEffect Default;

	UPROPERTY(EditDefaultsOnly, Category = "Impacts")
	TMap<TObjectPtr<UPhysicalMaterial>, FMEImpactEffect> Surfaces;

	/** Bullet holes stay this long, then fade. */
	UPROPERTY(EditDefaultsOnly, Category = "Decals", meta = (ClampMin = "0", Units = "s"))
	float DecalLifetime = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Decals", meta = (ClampMin = "0", Units = "s"))
	float DecalFadeTime = 2.f;

	/** Bullet holes smaller than this share of the screen are not drawn (far away). */
	UPROPERTY(EditDefaultsOnly, Category = "Decals", meta = (ClampMin = "0", ClampMax = "1"))
	float DecalFadeScreenSize = 0.002f;

private:
	const FMEImpactEffect& FindEffect(const UPhysicalMaterial* Surface) const;
};
