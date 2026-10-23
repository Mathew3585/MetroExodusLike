#include "Combat/MEImpactData.h"
#include "Components/DecalComponent.h"
#include "Engine/HitResult.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

const FMEImpactEffect& UMEImpactData::FindEffect(const UPhysicalMaterial* Surface) const
{
	const FMEImpactEffect* Found = Surface ? Surfaces.Find(Surface) : nullptr;
	return Found ? *Found : Default;
}

void UMEImpactData::SpawnImpact(const UObject* WorldContext, const FHitResult& Hit) const
{
	const FMEImpactEffect& Impact = FindEffect(Hit.PhysMaterial.Get());
	const FVector Normal = Hit.ImpactNormal;

	if (Impact.Decal && Hit.GetComponent())
	{
		// Decals project along X: into the surface, random turn so repeated holes don't look stamped
		FRotator Rotation = (-Normal).Rotation();
		Rotation.Roll = FMath::FRandRange(-180.f, 180.f);
		const float Size = Impact.DecalSize * (1.f + FMath::FRandRange(-Impact.DecalSizeVariation, Impact.DecalSizeVariation));

		// Attached: stays on moving things (falling targets, doors)
		if (UDecalComponent* Decal = UGameplayStatics::SpawnDecalAttached(Impact.Decal, FVector(Size), Hit.GetComponent(), NAME_None,
			Hit.ImpactPoint, Rotation, EAttachLocation::KeepWorldPosition, 0.f))
		{
			Decal->SetFadeScreenSize(DecalFadeScreenSize);
			Decal->SetFadeOut(DecalLifetime, DecalFadeTime, true);
		}
	}

	if (Impact.Effect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContext, Impact.Effect, Hit.ImpactPoint, Normal.Rotation());
	}

	if (Impact.Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContext, Impact.Sound, Hit.ImpactPoint);
	}
}
