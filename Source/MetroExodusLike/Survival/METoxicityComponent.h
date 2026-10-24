#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "METoxicityComponent.generated.h"

class AMECharacter;
class USoundBase;

/**
 * How much toxic air the player has breathed: builds up in toxic zones, wears off outside.
 * Past a threshold it hurts, a little at a time; the player coughs more and more.
 * The screen effects are drawn by the damage feedback. The gas mask (4.4) makes the player protected.
 */
UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEToxicityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMEToxicityComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Toxic zones the player is in. Overlapping zones: the densest one counts. */
	void AddToxicZone(const UObject* Zone, float Density);
	void RemoveToxicZone(const UObject* Zone);

	/** Breathing filtered air (gas mask): no build-up even inside a zone. */
	void SetProtected(bool bInProtected) { bProtected = bInProtected; }
	bool IsProtected() const { return bProtected; }

	/** 0 = clean air, 1 = fully poisoned. */
	float GetToxicity() const { return Toxicity; }

	/** Densest zone the player is in, 0 outside. */
	float GetExposure() const;

protected:
	/** Time to go from clean to fully poisoned in a zone of density 1. */
	UPROPERTY(EditDefaultsOnly, Category = "Toxicity", meta = (ClampMin = "0.1", Units = "s"))
	float BuildUpTime = 8.f;

	/** Time to go from fully poisoned back to clean, out of the gas. */
	UPROPERTY(EditDefaultsOnly, Category = "Toxicity", meta = (ClampMin = "0.1", Units = "s"))
	float RecoveryTime = 6.f;

	/** Below this toxicity it does not hurt yet: time to react. */
	UPROPERTY(EditDefaultsOnly, Category = "Toxicity|Damage", meta = (ClampMin = "0", ClampMax = "1"))
	float DamageThreshold = 0.4f;

	/** Damage per second when fully poisoned (less just above the threshold). */
	UPROPERTY(EditDefaultsOnly, Category = "Toxicity|Damage", meta = (ClampMin = "0"))
	float MaxDamagePerSecond = 6.f;

	/** Damage comes in chokes, this far apart (not a bit every frame). */
	UPROPERTY(EditDefaultsOnly, Category = "Toxicity|Damage", meta = (ClampMin = "0.1", Units = "s"))
	float DamageInterval = 1.f;

	/** Random cough (a cue). Optional. */
	UPROPERTY(EditDefaultsOnly, Category = "Toxicity|Sound")
	TObjectPtr<USoundBase> CoughSound;

	/** Coughs start above this toxicity. */
	UPROPERTY(EditDefaultsOnly, Category = "Toxicity|Sound", meta = (ClampMin = "0", ClampMax = "1"))
	float CoughThreshold = 0.15f;

	/** Time between coughs, just above the threshold and fully poisoned (with some randomness). */
	UPROPERTY(EditDefaultsOnly, Category = "Toxicity|Sound", meta = (ClampMin = "0", Units = "s"))
	float CoughIntervalMild = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Toxicity|Sound", meta = (ClampMin = "0", Units = "s"))
	float CoughIntervalSevere = 1.6f;

	virtual void BeginPlay() override;

private:
	void UpdateDamage(float DeltaTime);
	void UpdateCoughs(float DeltaTime);

	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;

	TMap<TWeakObjectPtr<const UObject>, float> Zones;
	bool bProtected = false;
	float Toxicity = 0.f;
	float PendingDamage = 0.f;
	float DamageTimer = 0.f;
	float CoughTimer = 0.f;
};
