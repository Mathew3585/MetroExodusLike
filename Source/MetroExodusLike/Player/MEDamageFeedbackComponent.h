#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MEDamageFeedbackComponent.generated.h"

class AController;
class AMECharacter;
class UAudioComponent;
class USoundBase;

/**
 * How the player feels their health, without a health bar: everything is on the screen and in the sound.
 *  - hit: red flash at the screen edges, the view jolts, hurt sound;
 *  - low health: vignette pulsing with the heartbeat, colors drain, heartbeat sound;
 *  - toxic air: sickly tint, the view closes in, color fringes;
 *  - death: fade to black.
 * Drives the post-process of the first-person camera.
 */
UCLASS(ClassGroup = (Player), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEDamageFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMEDamageFeedbackComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// --- Hit ---

	/** Flash strength per point of damage (1 = full flash). */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Hit", meta = (ClampMin = "0"))
	float FlashPerDamage = 0.04f;

	/** How fast the hit flash fades (per second). */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Hit", meta = (ClampMin = "0"))
	float FlashFadeSpeed = 2.5f;

	/** Screen tint of a full flash. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Hit")
	FLinearColor FlashTint = FLinearColor(1.f, 0.35f, 0.3f);

	/** Vignette added by a full flash. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Hit", meta = (ClampMin = "0"))
	float FlashVignette = 0.8f;

	/** View jolt per point of damage (camera spring kick, negative = down). */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Hit")
	float KickPerDamage = -1.5f;

	/** Hurt grunt (a random cue, so it never repeats the same take). Optional. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Hit")
	TObjectPtr<USoundBase> HurtSound;

	/** Minimum time between two grunts: a burst of bullets gives one grunt, not ten. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Hit", meta = (ClampMin = "0", Units = "s"))
	float HurtSoundCooldown = 0.6f;

	// --- Low health ---

	/** Below this share of health the low-health effects start, full at 0. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Low Health", meta = (ClampMin = "0", ClampMax = "1"))
	float LowHealthThreshold = 0.5f;

	/** Vignette at the edge of death, at the peak of a heartbeat. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Low Health", meta = (ClampMin = "0"))
	float LowHealthVignette = 1.4f;

	/** Color saturation at the edge of death (1 = unchanged). */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Low Health", meta = (ClampMin = "0", ClampMax = "1"))
	float LowHealthSaturation = 0.25f;

	/** Heartbeats per second, at the threshold and at the edge of death. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Low Health", meta = (ClampMin = "0", Units = "Hz"))
	float CalmHeartRate = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Low Health", meta = (ClampMin = "0", Units = "Hz"))
	float PanicHeartRate = 2.f;

	/** Looping heartbeat, its volume follows the low-health weight. Optional. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Low Health")
	TObjectPtr<USoundBase> HeartbeatSound;

	/** How fast the low-health effects follow the health. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Low Health", meta = (ClampMin = "0"))
	float LowHealthBlendSpeed = 3.f;

	// --- Toxic air ---

	/** Screen tint when fully poisoned. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Toxic")
	FLinearColor ToxicTint = FLinearColor(0.75f, 0.95f, 0.55f);

	/** Vignette added when fully poisoned: the view closes in. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Toxic", meta = (ClampMin = "0"))
	float ToxicVignette = 1.f;

	/** Color fringes at the screen edges when fully poisoned (blurred, watering eyes). */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Toxic", meta = (ClampMin = "0", ClampMax = "5"))
	float ToxicFringe = 4.f;

	/** Color saturation when fully poisoned (1 = unchanged). */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Toxic", meta = (ClampMin = "0", ClampMax = "1"))
	float ToxicSaturation = 0.6f;

	// --- Death ---

	/** Time between death and the start of the fade (the fall is seen). */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Death", meta = (ClampMin = "0", Units = "s"))
	float DeathFadeDelay = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Death", meta = (ClampMin = "0.01", Units = "s"))
	float DeathFadeTime = 1.5f;

	/** Optional. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback|Death")
	TObjectPtr<USoundBase> DeathSound;

private:
	void HandleHealthChanged(float Health, float Delta, AController* Instigator);
	void HandleDeath(AController* Killer);

	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Heartbeat;

	float Flash = 0.f;
	float LowHealth = 0.f;
	float HeartPhase = 0.f;
	float DeathTime = -1.f;
	float LastHurtSoundTime = -1000.f;
};
