#include "AI/MEEnemyProfile.h"
#include "Sound/SoundBase.h"

USoundBase* UMEEnemyProfile::GetBark(EMEBark Bark) const
{
	const TObjectPtr<USoundBase>* Found = Barks.Find(Bark);
	return Found ? Found->Get() : nullptr;
}
