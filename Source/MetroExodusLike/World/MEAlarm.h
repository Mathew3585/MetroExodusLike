#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MEAlarm.generated.h"

class UStaticMeshComponent;
class UAudioComponent;
class USoundBase;

/**
 * A station alarm (bell, siren). An alerted enemy runs to the nearest one; once it is rung,
 * every enemy of the level comes. Stop the runner before he gets there.
 */
UCLASS()
class METROEXODUSLIKE_API AMEAlarm : public AActor
{
	GENERATED_BODY()

public:
	AMEAlarm();

	/** Rung by an enemy: the alarm sounds and the whole level is alerted toward ThreatLocation. */
	void Raise(const FVector& ThreatLocation);
	bool IsRaised() const { return bRaised; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Rings this long (loops the sound). */
	UPROPERTY(EditAnywhere, Category = "Alarm", meta = (ClampMin = "0", Units = "s"))
	float RingTime = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Alarm")
	TObjectPtr<USoundBase> AlarmSound;

private:
	void StopRinging();

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Ringing;

	bool bRaised = false;
	FTimerHandle RingTimer;
};
