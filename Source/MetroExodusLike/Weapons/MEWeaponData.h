#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MEWeaponData.generated.h"

class UAnimSequenceBase;
class USkeletalMesh;
class UStaticMesh;
class USoundBase;

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

	/** Bone of the first-person arms the weapon is attached to. */
	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	FName ArmsAttachBone = TEXT("ik_hand_gun");

	// --- World body (shadow, what the AI sees) ---

	/** Upper-body pose of the world body holding this weapon. */
	UPROPERTY(EditDefaultsOnly, Category = "World Body")
	TObjectPtr<UAnimSequenceBase> BodyHoldPose;

	/** Bone of the world body the world weapon is attached to. */
	UPROPERTY(EditDefaultsOnly, Category = "World Body")
	FName BodyAttachBone = TEXT("hand_r");

	/** Weapon placement relative to BodyAttachBone in the hold pose. */
	UPROPERTY(EditDefaultsOnly, Category = "World Body")
	FTransform BodyGripOffset;

	// --- First-person arms poses (loops) ---

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Loops")
	TObjectPtr<UAnimSequenceBase> IdleLoop;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Loops")
	TObjectPtr<UAnimSequenceBase> WalkLoop;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Loops")
	TObjectPtr<UAnimSequenceBase> SprintLoop;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Loops")
	TObjectPtr<UAnimSequenceBase> AimLoop;

	// --- One-shots (arms + weapon) ---

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation Draw;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Actions")
	FMEWeaponAnimation Fire;

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

	// --- Sounds ---

	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	TObjectPtr<USoundBase> FireSound;

	// --- Stats ---

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0"))
	float Damage = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "1", Units = "RPM"))
	float RoundsPerMinute = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	bool bFullAuto = true;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "1"))
	int32 MagazineSize = 30;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0"))
	int32 StartingReserveAmmo = 60;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0", Units = "cm"))
	float Range = 10000.f;
};
