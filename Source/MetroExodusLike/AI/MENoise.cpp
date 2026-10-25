#include "AI/MENoise.h"
#include "Perception/AISense_Hearing.h"
#include "GameFramework/Actor.h"

namespace MENoise
{
	const FName Footstep = TEXT("Footstep");
	const FName Landing = TEXT("Landing");
	const FName Gunshot = TEXT("Gunshot");
	const FName Casing = TEXT("Casing");
	const FName Door = TEXT("Door");

	void Report(AActor* Instigator, const FVector& Location, float Loudness, FName Tag)
	{
		if (Instigator && Loudness > 0.f)
		{
			UAISense_Hearing::ReportNoiseEvent(Instigator, Location, Loudness, Instigator, 0.f, Tag);
		}
	}
}
