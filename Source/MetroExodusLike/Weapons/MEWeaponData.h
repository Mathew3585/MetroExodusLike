#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Weapons/MEWeaponTypes.h"
#include "MEWeaponData.generated.h"

class UAnimSequenceBase;
class USoundBase;
class UMEItemData;
class UMECrosshairData;
class UMEImpactData;

/**
 * Everything that defines how a weapon behaves: stats, fire modes, handling, animations, sounds.
 * One asset per weapon (DA_Weapon_AR15...), set on the weapon Blueprint (BP_Weapon_AR15), which holds the visuals.
 */
UCLASS(BlueprintType)
class METROEXODUSLIKE_API UMEWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FText DisplayName;

	// --- Stats ---

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0"))
	float Damage = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "1", Units = "RPM"))
	float RoundsPerMinute = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "1"))
	int32 MagazineSize = 30;

	/** Rounds this weapon takes: the reserve is the player's inventory count of this item. None = unlimited reserve. */
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TObjectPtr<UMEItemData> AmmoType;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0", Units = "cm"))
	float Range = 10000.f;

	// --- Fire modes ---

	/** Modes the selector goes through, in order. The first one is the mode the weapon starts in. One entry = no switch. */
	UPROPERTY(EditDefaultsOnly, Category = "Fire Modes")
	TArray<EMEFireMode> FireModes = { EMEFireMode::Auto, EMEFireMode::Single };

	/** Shots per trigger pull in burst mode. */
	UPROPERTY(EditDefaultsOnly, Category = "Fire Modes", meta = (ClampMin = "2"))
	int32 BurstCount = 3;

	/** Selector animation + sound when switching TO each mode. */
	UPROPERTY(EditDefaultsOnly, Category = "Fire Modes")
	TMap<EMEFireMode, FMEWeaponAnimation> FireModeSwitch;

	// --- Handling ---

	UPROPERTY(EditDefaultsOnly, Category = "Handling")
	FMERecoilSettings Recoil;

	UPROPERTY(EditDefaultsOnly, Category = "Handling")
	FMESpreadSettings Spread;

	UPROPERTY(EditDefaultsOnly, Category = "Handling")
	FMESwaySettings Sway;

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

	// --- Effects (where they come from: the weapon Blueprint's components) ---

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	FMEShellEjectSettings ShellEject;

	/** Time for the muzzle light to go out after a shot. Its color, strength and reach: the Blueprint's MuzzleLight. */
	UPROPERTY(EditDefaultsOnly, Category = "Effects", meta = (ClampMin = "0.01", Units = "s"))
	float MuzzleLightDuration = 0.06f;

	/** Bullet holes, dust, sparks and sounds per surface hit. */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TObjectPtr<UMEImpactData> Impacts;

	// --- Sounds ---

	/** Played on every shot, at the weapon. */
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TObjectPtr<USoundBase> FireSound;

	/** How far enemies hear a shot (share of their hearing range, above 1 = beyond it). */
	UPROPERTY(EditDefaultsOnly, Category = "Sounds", meta = (ClampMin = "0"))
	float ShotNoise = 2.f;

	// --- UI ---

	/** Hip-fire crosshair for this weapon only. None = the player's default crosshair. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UMECrosshairData> Crosshair;

	// --- First-person arms ---

	/** Bone of the first-person arms holding the weapon: the gun bone the arm animations were authored with. */
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	FName ArmsGripBone = TEXT("ik_hand_gun");

	/** Weapon placement relative to ArmsGripBone (identity when the animations come with the weapon). */
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	FTransform ArmsGripOffset;

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

	/** Taking the weapon out (after a gesture with the weapon put away...). */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation Draw;

	/** First time the weapon is taken out (longer, the weapon is shown off). Optional: Draw is used without it. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation FirstDraw;

	/** Putting the weapon away, out of view: its last frame is held until the Draw. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation Holster;

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
};
