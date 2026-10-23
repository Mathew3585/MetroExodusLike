#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MEWeaponData.generated.h"

class UAnimSequenceBase;
class USkeletalMesh;
class UStaticMesh;
class USoundBase;
class UMEItemData;
class UMECrosshairData;
class AMEShellCasing;

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

/** Spent casing thrown out of the ejection port on each shot. Location and directions are in the space of Bone. */
USTRUCT(BlueprintType)
struct FMEShellEjectSettings
{
	GENERATED_BODY()

	/** Casing thrown on each shot. None = no casings. */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AMEShellCasing> ShellClass;

	/** Weapon bone the ejection port moves with. None = the weapon mesh origin. */
	UPROPERTY(EditAnywhere)
	FName Bone;

	/** Ejection port. */
	UPROPERTY(EditAnywhere)
	FVector Location = FVector(-1.5f, 5.5f, 1.7f);

	/** Throw direction (normalized in code). AR-15 mesh: -X is right, +Y forward, +Z up. */
	UPROPERTY(EditAnywhere)
	FVector Direction = FVector(-1.f, -0.3f, 0.6f);

	/** Casing length axis when it leaves the port: along the barrel. */
	UPROPERTY(EditAnywhere)
	FVector BarrelAxis = FVector(0.f, 1.f, 0.f);

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "cm/s"))
	float Speed = 300.f;

	/** Random share of the speed, either way (0.2 = +/- 20%). */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "1"))
	float SpeedVariation = 0.2f;

	/** Random cone around the direction. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "90", Units = "Degrees"))
	float DirectionVariation = 12.f;

	/** Tumble, random axis. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "DegreesPerSecond"))
	float Spin = 1200.f;

	/** Casings kept in the world at once: past this the oldest one is reused. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "1"))
	int32 MaxCasings = 40;
};

/** Separate static part attached to a bone of the weapon (magazine, sights, stock...). */
USTRUCT(BlueprintType)
struct FMEWeaponPart
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere)
	FName Bone;
};

/**
 * Everything that defines a weapon: visuals, animations, sounds and stats.
 * One asset per weapon (DA_Weapon_AR15...), no code needed to add a weapon.
 */
UCLASS(BlueprintType)
class METROEXODUSLIKE_API UMEWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FText DisplayName;

	// --- Visuals ---

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	TObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	TArray<FMEWeaponPart> Parts;

	// --- First-person arms ---

	/** Bone of the first-person arms holding the weapon: the gun bone the arm animations were authored with. */
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	FName ArmsGripBone = TEXT("ik_hand_gun");

	/** Weapon placement relative to ArmsGripBone (identity when the animations come with the weapon). */
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	FTransform ArmsGripOffset;

	// --- Close to a wall: the weapon is pulled aside instead of going through ---

	/** Arms pose with the weapon pulled back (wall getting close). */
	UPROPERTY(EditDefaultsOnly, Category = "First Person|Wall")
	TObjectPtr<UAnimSequenceBase> CloseToWallPose;

	/** Arms pose with the weapon raised out of the way (against the wall). */
	UPROPERTY(EditDefaultsOnly, Category = "First Person|Wall")
	TObjectPtr<UAnimSequenceBase> AgainstWallPose;

	/** Free space needed in front of the eye for the weapon: closer obstacles start pulling it aside. */
	UPROPERTY(EditDefaultsOnly, Category = "First Person|Wall", meta = (ClampMin = "0", Units = "cm"))
	float WallCheckLength = 75.f;

	/** At this distance (and closer) the weapon is fully raised against the wall. */
	UPROPERTY(EditDefaultsOnly, Category = "First Person|Wall", meta = (ClampMin = "0", Units = "cm"))
	float AgainstWallDistance = 30.f;

	// --- World body (shadow, what the AI sees) ---

	/**
	 * Hand bone of the world body holding the weapon.
	 * A hand rather than an IK bone: not every skeleton has IK bones (MetaHumans don't).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "World Body")
	FName GripBone = TEXT("hand_r");

	/** Weapon placement relative to GripBone, measured on the world body's hold pose. */
	UPROPERTY(EditDefaultsOnly, Category = "World Body")
	FTransform GripOffset;

	/** Upper-body pose of the world body holding this weapon. */
	UPROPERTY(EditDefaultsOnly, Category = "World Body")
	TObjectPtr<UAnimSequenceBase> BodyHoldPose;

	// --- First-person arms poses (loops) ---

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Loops")
	TObjectPtr<UAnimSequenceBase> IdleLoop;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Loops")
	TObjectPtr<UAnimSequenceBase> WalkLoop;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Loops")
	TObjectPtr<UAnimSequenceBase> SprintLoop;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Loops")
	TObjectPtr<UAnimSequenceBase> AimLoop;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Loops")
	TObjectPtr<UAnimSequenceBase> AimWalkLoop;

	// --- One-shots (arms + weapon) ---

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation Draw;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation Fire;

	/** Last round of the magazine: the bolt locks back. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation FireLast;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation AimFire;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation DryFire;

	/** Tactical reload: a round is still chambered. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation Reload;

	/** Empty reload: the bolt must be released. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation ReloadEmpty;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation Inspect;

	/** Inspect with an empty magazine (bolt locked back). */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation InspectEmpty;

	/** Weapon pose at rest, restored when an action is cut short (inspect...). */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Weapon Poses")
	TObjectPtr<UAnimSequenceBase> WeaponIdlePose;

	/** Weapon pose with an empty magazine (bolt locked back). */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Weapon Poses")
	TObjectPtr<UAnimSequenceBase> WeaponEmptyPose;

	// --- Sounds ---

	/** Played on every shot, at the weapon. */
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TObjectPtr<USoundBase> FireSound;

	// --- Effects ---

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	FMEShellEjectSettings ShellEject;

	// --- UI ---

	/** Hip-fire crosshair for this weapon only. None = the player's default crosshair. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UMECrosshairData> Crosshair;

	// --- Aiming ---

	/** View field of view when fully aiming. */
	UPROPERTY(EditDefaultsOnly, Category = "Aiming", meta = (ClampMin = "10", ClampMax = "170", Units = "Degrees"))
	float AimFieldOfView = 70.f;

	/** How fast the weapon goes up to the eye / back down. */
	UPROPERTY(EditDefaultsOnly, Category = "Aiming", meta = (ClampMin = "0"))
	float AimSpeed = 10.f;

	/** Walk speed multiplier while aiming. */
	UPROPERTY(EditDefaultsOnly, Category = "Aiming", meta = (ClampMin = "0", ClampMax = "1"))
	float AimMoveSpeedScale = 0.6f;

	// --- Handling ---

	UPROPERTY(EditDefaultsOnly, Category = "Handling")
	FMERecoilSettings Recoil;

	UPROPERTY(EditDefaultsOnly, Category = "Handling")
	FMESpreadSettings Spread;

	// --- Stats ---

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0"))
	float Damage = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "1", Units = "RPM"))
	float RoundsPerMinute = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	bool bFullAuto = true;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "1"))
	int32 MagazineSize = 30;

	/** Rounds this weapon takes: the reserve is the player's inventory count of this item. None = unlimited reserve. */
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TObjectPtr<UMEItemData> AmmoType;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0", Units = "cm"))
	float Range = 10000.f;
};
