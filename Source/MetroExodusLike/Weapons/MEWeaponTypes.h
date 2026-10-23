#pragma once

#include "CoreMinimal.h"
#include "MEWeaponTypes.generated.h"

class UAnimSequenceBase;
class USoundBase;
class AMEShellCasing;

/** How the weapon fires while the trigger is held. */
UENUM(BlueprintType)
enum class EMEFireMode : uint8
{
	/** One shot per trigger pull. */
	Single,
	/** A few shots per trigger pull (BurstCount). */
	Burst,
	/** Fires as long as the trigger is held. */
	Auto
};

/** Animation authored for the first-person arms and the matching one for the weapon (same length, played together). */
USTRUCT(BlueprintType)
struct FMEWeaponAnimation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimSequenceBase> Arms;

	/** Optional: bolt, magazine, trigger... */
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimSequenceBase> Weapon;

	/** Optional: sound of the whole action, synced with the animations (reload, draw...). */
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> Sound;
};

/** View kick and arms kick on each shot. Hip values, scaled down when aiming. */
USTRUCT(BlueprintType)
struct FMERecoilSettings
{
	GENERATED_BODY()

	/** View climb per shot. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Degrees"))
	float PitchPerShot = 0.9f;

	/** Random sideways view kick per shot (either side). */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Degrees"))
	float YawPerShot = 0.4f;

	/** How fast a kick reaches the view (higher = snappier). */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float KickSpeed = 25.f;

	/** Share of the climb the view gives back once the trigger is released (0 = stays up, 1 = back to the start). */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "1"))
	float Recovery = 0.6f;

	/** How fast the view comes back. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float RecoverySpeed = 6.f;

	/** Arms + weapon pushed back toward the shoulder on each shot (spring impulse). */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "cm/s"))
	float ArmsKickBack = 70.f;

	/** Arms + weapon pushed up on each shot (spring impulse). */
	UPROPERTY(EditAnywhere, meta = (Units = "cm/s"))
	float ArmsKickUp = 20.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float ArmsKickStiffness = 350.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float ArmsKickDamping = 24.f;

	/** Everything above is multiplied by this when aiming. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "1"))
	float AimScale = 0.45f;
};

/** Bullet spread: a cone around the view, grows with each shot and with movement. */
USTRUCT(BlueprintType)
struct FMESpreadSettings
{
	GENERATED_BODY()

	/** Cone half-angle standing still, from the hip. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Degrees"))
	float Hip = 2.f;

	/** Cone half-angle standing still, aiming. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Degrees"))
	float Aim = 0.15f;

	/** Added per shot (from the hip; aiming uses the recoil aim scale). */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Degrees"))
	float PerShot = 0.5f;

	/** Added at walking speed. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Degrees"))
	float Moving = 1.5f;

	/** Added while in the air. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Degrees"))
	float InAir = 4.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Degrees"))
	float HipMax = 7.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Degrees"))
	float AimMax = 1.5f;

	/** How fast the shot bloom goes away. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "DegreesPerSecond"))
	float RecoverySpeed = 6.f;
};

/** Spent casings thrown on each shot. Where from and which way: the weapon's ShellEjectPort arrow. */
USTRUCT(BlueprintType)
struct FMEShellEjectSettings
{
	GENERATED_BODY()

	/** Casing thrown on each shot. None = no casings. */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AMEShellCasing> ShellClass;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "cm/s"))
	float Speed = 300.f;

	/** Random share of the speed, either way (0.2 = +/- 20%). */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "1"))
	float SpeedVariation = 0.2f;

	/** Random cone around the arrow. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "90", Units = "Degrees"))
	float DirectionVariation = 12.f;

	/** Tumble, random axis. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "DegreesPerSecond"))
	float Spin = 1200.f;

	/** Casings kept in the world at once: past this the oldest one is reused. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "1"))
	int32 MaxCasings = 40;
};

