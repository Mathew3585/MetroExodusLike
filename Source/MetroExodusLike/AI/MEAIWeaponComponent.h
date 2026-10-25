#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MEAIWeaponComponent.generated.h"

class AMEWeapon;

/**
 * An enemy's trigger finger: bursts at a target (or at a place, to suppress), magazine, reload.
 * Shots use the weapon's data (rate, damage, sound, impacts) like the player's, from the muzzle.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEAIWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMEAIWeaponComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetWeapon(AMEWeapon* InWeapon) { Weapon = InWeapon; }

	/**
	 * Fires Rounds shots at Target (followed while firing), or at Location without a target.
	 * Spread in degrees; damage scaled. Does nothing while reloading or empty.
	 */
	void StartBurst(AActor* Target, const FVector& Location, int32 Rounds, float Spread, float DamageScale);
	void StopFiring();

	bool IsFiring() const { return RoundsLeft > 0; }
	bool IsReloading() const { return bReloading; }
	bool IsEmpty() const;

	/** Takes ReloadTime; the magazine is full after (enemies do not count their reserve). */
	void Reload();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "AI Weapon", meta = (ClampMin = "0", Units = "s"))
	float ReloadTime = 2.5f;

	/** Moving targets are harder to hit: extra spread per m/s of their speed. */
	UPROPERTY(EditDefaultsOnly, Category = "AI Weapon", meta = (ClampMin = "0", Units = "Degrees"))
	float MovingTargetSpread = 0.6f;

	/** Where it aims on a target, above its origin (chest). */
	UPROPERTY(EditDefaultsOnly, Category = "AI Weapon", meta = (Units = "cm"))
	float AimHeight = 30.f;

private:
	void FireShot();
	void FinishReload();

	UPROPERTY(Transient)
	TObjectPtr<AMEWeapon> Weapon;

	TWeakObjectPtr<AActor> Target;
	FVector TargetLocation = FVector::ZeroVector;
	int32 RoundsLeft = 0;
	float BurstSpread = 0.f;
	float BurstDamageScale = 1.f;
	float ShotCooldown = 0.f;
	bool bReloading = false;
	FTimerHandle ReloadTimer;
};
