#include "Player/MEFootstepData.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

USoundBase* UMEFootstepData::FindSound(const UPhysicalMaterial* Surface) const
{
	const TObjectPtr<USoundBase>* Found = Surface ? Surfaces.Find(Surface) : nullptr;
	return Found && *Found ? Found->Get() : Default.Get();
}
