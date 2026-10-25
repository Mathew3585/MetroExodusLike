#pragma once

#include "CoreMinimal.h"

class AActor;

/**
 * Noises the enemies can hear. Loudness scales their hearing range (1 = heard at the full range,
 * 0.25 = at a quarter of it). A noise only says where something happened, not who made it.
 */
namespace MENoise
{
	METROEXODUSLIKE_API extern const FName Footstep;
	METROEXODUSLIKE_API extern const FName Landing;
	METROEXODUSLIKE_API extern const FName Gunshot;
	METROEXODUSLIKE_API extern const FName Casing;
	METROEXODUSLIKE_API extern const FName Door;

	/** Reports a noise made by Instigator at Location (Instigator = the pawn responsible). */
	METROEXODUSLIKE_API void Report(AActor* Instigator, const FVector& Location, float Loudness, FName Tag);
}
