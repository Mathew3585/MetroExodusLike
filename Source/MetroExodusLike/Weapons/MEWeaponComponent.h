#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MEWeaponComponent.generated.h"

class AMECharacter;
class UAudioComponent;
class AMEWeapon;
class AMEShellCasing;
class UMEWeaponData;
struct FMEWeaponAnimation;

/**
 * The player's weapon handling: spawns the weapon in the first-person arms,
 * and a copy in the world body's hands (shadow, AI).
 * Fires hitscan shots from the view (semi / full auto, dry fire) inside a spread cone, kicks the view
 * and the arms on each shot, aims down sights, reloads (tactical / empty), pulls the weapon aside near walls.
 */
UCLASS(ClassGroup = (Weapons), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMEWeaponComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	AMEWeapon* GetWeapon() const { return Weapon; }
	UMEWeaponData* GetWeaponData() const;

	/** 0 = free, 1 = against a wall: how much the weapon is pulled aside. Smoothed. */
	float GetWallBlockAmount() const { return WallBlockAmount; }

	/** Trigger pressed / released. Full auto keeps firing while held. */
	void StartFire();
	void StopFire();

	/** Tactical reload if a round is chambered, empty reload otherwise. */
	void Reload();

	/** Looks the weapon over. Cut short by firing, aiming, reloading or sprinting. */
	void Inspect();

	/** Drawing or reloading: no firing, no other action. */
	bool IsBusy() const;

	/** Aim button pressed / released. Aiming waits while busy, sprinting or blocked by a wall. */
	void SetWantsToAim(bool bWants) { bWantsToAim = bWants; }
	bool WantsToAim() const { return bWantsToAim; }

	/** 0 = hip, 1 = fully aiming. Smoothed. */
	float GetAimAlpha() const { return AimAlpha; }

	/** Arms + weapon recoil offset, in view space (X forward, Z up). */
	FVector GetArmsKick() const { return ArmsKick; }

	/** Current spread cone half-angle. */
	float GetSpreadAngle() const;

	/** Rounds of the current weapon's ammo type the player carries. */
	int32 GetReserveAmmo() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Weapon the player starts with. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	TObjectPtr<UMEWeaponData> StartingWeapon;

	/** Actor class spawned for the weapon (a Blueprint child can add effects). */
	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	TSubclassOf<AMEWeapon> WeaponClass;

	/** Thickness of the wall check: catches edges the weapon would clip into. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapons|Wall", meta = (ClampMin = "0", Units = "cm"))
	float WallCheckRadius = 6.f;

	/** Blend out when an action is cut short (fire animation when aiming in / out, inspect...). */
	UPROPERTY(EditDefaultsOnly, Category = "Weapons", meta = (ClampMin = "0", Units = "s"))
	float InterruptBlendOutTime = 0.12f;

	/** How fast the weapon goes aside / comes back. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapons|Wall", meta = (ClampMin = "0"))
	float WallBlockBlendSpeed = 8.f;

	/** Above this wall block the weapon is too far aside to fire. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapons|Wall", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxWallBlockToFire = 0.3f;

	/** Draws the impacts and prints the ammo on screen (until there is a HUD / watch). */
	UPROPERTY(EditDefaultsOnly, Category = "Weapons|Debug")
	bool bDebugShots = true;

private:
	void Equip(UMEWeaponData* WeaponData);
	/**
	 * Plays the arms + weapon animations and the sound. Returns the arms animation length.
	 * Interruptible actions (fire, inspect) give way when the stance changes (aim in / out, sprint).
	 */
	float PlayAction(const FMEWeaponAnimation& Action, bool bInterruptible = false);

	/** Cuts an interruptible action still playing: arms back to the loops, weapon back to its rest pose, sound stopped. */
	void InterruptAction();
	bool IsPlayingInterruptibleAction() const;
	void UpdateWallBlock(float DeltaTime);
	void UpdateFiring();
	void UpdateAiming(float DeltaTime);
	void UpdateRecoil(float DeltaTime);
	void AddRecoil();
	void FireShot();
	/** Throws a casing out of the ejection port, where it is seen on screen. */
	void EjectShell();
	void FinishReload();
	void PrintAmmo() const;

	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;

	/** Weapon drawn in first person, in the arms. */
	UPROPERTY(Transient)
	TObjectPtr<AMEWeapon> Weapon;

	/** Copy held by the world body: invisible, casts the shadow. */
	UPROPERTY(Transient)
	TObjectPtr<AMEWeapon> WorldWeapon;

	float WallBlockAmount = 0.f;

	// Firing
	bool bWantsToFire = false;
	bool bFiredThisPress = false;
	double LastShotTime = -1.0;

	// Aiming
	bool bWantsToAim = false;
	bool bAimTarget = false;
	float AimAlpha = 0.f;

	// Current action
	bool bActionInterruptible = false;
	double ActionEndTime = 0.0;
	TWeakObjectPtr<UAudioComponent> ActionSound;

	// Recoil: view kick still to apply, view climb to give back, arms spring, spread bloom
	FRotator PendingRecoil = FRotator::ZeroRotator;
	FRotator RecoveryBudget = FRotator::ZeroRotator;
	FVector ArmsKick = FVector::ZeroVector;
	FVector ArmsKickVelocity = FVector::ZeroVector;
	float SpreadBloom = 0.f;

	/** Casings in the world, reused oldest first past the weapon's limit. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AMEShellCasing>> Casings;
	int32 NextCasing = 0;

	/** Until this time the weapon is drawing / reloading. */
	double BusyUntil = 0.0;
	FTimerHandle ReloadTimer;
};
