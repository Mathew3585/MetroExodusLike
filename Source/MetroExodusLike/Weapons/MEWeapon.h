#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapons/MEWeaponTypes.h"
#include "MEWeapon.generated.h"

class UArrowComponent;
class UNiagaraComponent;
class UPointLightComponent;
class USkeletalMeshComponent;
class UAnimSequenceBase;
class UMEWeaponData;

/**
 * A weapon: its visuals and its ammo / fire mode state. How it behaves comes from its Data (DA_Weapon_AR15...).
 * One Blueprint child per weapon (BP_Weapon_AR15...): the model with its parts on their bones, the muzzle flash,
 * the muzzle light and the ShellEjectPort arrow, placed in the viewport. No code to add a weapon.
 * Spawned twice by the weapon component: drawn in first person in the arms, and invisible in the world
 * body's hands for the shadow and the AI.
 */
UCLASS(Abstract)
class METROEXODUSLIKE_API AMEWeapon : public AActor
{
	GENERATED_BODY()

public:
	AMEWeapon();

	virtual void Tick(float DeltaSeconds) override;

	/** First person: drawn in the view model. World: invisible, only casts the shadow. Fills the magazine. */
	void Initialize(bool bFirstPerson);

	/** Plays a weapon-side animation (bolt, magazine...). */
	void PlayWeaponAnimation(UAnimSequenceBase* Animation);

	/** Muzzle flash + light flash. Eye: where the view is, the light never goes past a wall between the eye and the muzzle. */
	void PlayMuzzleFlash(const FVector& Eye);

	UMEWeaponData* GetData() const { return Data; }
	USkeletalMeshComponent* GetMesh() const { return Mesh; }
	const UArrowComponent* GetShellEjectPort() const { return ShellEjectPort; }
	/** Barrel direction: the muzzle flash's X axis. */
	FVector GetBarrelDirection() const;
	/** The muzzle (where the flash comes out, X along the barrel): shots and lights start there. */
	USceneComponent* GetMuzzle() const;

	// --- Ammo ---

	int32 GetAmmoInMagazine() const { return AmmoInMagazine; }

	/** Removes one round from the magazine. */
	void ConsumeRound() { AmmoInMagazine = FMath::Max(0, AmmoInMagazine - 1); }

	/** Rounds missing to fill the magazine. */
	int32 GetRoomInMagazine() const;

	/** Puts rounds in the magazine (taken from the reserve by the caller). */
	void LoadRounds(int32 Rounds) { AmmoInMagazine += FMath::Clamp(Rounds, 0, GetRoomInMagazine()); }

	// --- Fire modes ---

	EMEFireMode GetFireMode() const;

	/** Next mode of the data's FireModes. Returns false when there is only one. */
	bool CycleFireMode();

protected:
	/** How this weapon behaves: stats, fire modes, animations, sounds... */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UMEWeaponData> Data;

	/** The weapon model. Static parts (magazine, sights...) go under it, attached to their bone (Parent Socket). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/** Place it at the muzzle, X along the barrel. Pick the effect in its Niagara System Asset. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UNiagaraComponent> MuzzleFlash;

	/** Place it at the muzzle. Its color, intensity and radius are the flash at the moment of the shot. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> MuzzleLight;

	/** Place it on the ejection port, pointing where the casings fly. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UArrowComponent> ShellEjectPort;

private:
	void UpdateMuzzleLightLocation();

	int32 AmmoInMagazine = 0;
	int32 FireModeIndex = 0;

	/** Light strength set in the Blueprint: the peak of each flash. */
	float MuzzleLightPeak = 0.f;
	float MuzzleLightAlpha = 0.f;
	/** Placed in the Blueprint: where the light goes back to before each flash. */
	FTransform MuzzleLightRestTransform;
	FVector MuzzleEye = FVector::ZeroVector;
};
