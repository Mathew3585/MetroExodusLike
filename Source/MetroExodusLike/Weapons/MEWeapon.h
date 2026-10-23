#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MEWeapon.generated.h"

class UMEWeaponData;
class UAnimSequenceBase;

/**
 * A weapon in the world: its mesh, parts and ammo state. Built from a UMEWeaponData.
 */
UCLASS()
class METROEXODUSLIKE_API AMEWeapon : public AActor
{
	GENERATED_BODY()

public:
	AMEWeapon();

	/**
	 * Sets up mesh and parts from the data, fills the magazine.
	 * First person: drawn in the view model. World: invisible, only casts the shadow.
	 */
	void Initialize(UMEWeaponData* InData, bool bFirstPerson);

	/** Plays a weapon-side animation (bolt, magazine...). */
	void PlayWeaponAnimation(UAnimSequenceBase* Animation);

	UMEWeaponData* GetData() const { return Data; }
	USkeletalMeshComponent* GetMesh() const { return Mesh; }

	int32 GetAmmoInMagazine() const { return AmmoInMagazine; }
	int32 GetReserveAmmo() const { return ReserveAmmo; }

	/** Removes one round from the magazine. */
	void ConsumeRound() { AmmoInMagazine = FMath::Max(0, AmmoInMagazine - 1); }

	/** Magazine not full and rounds left in reserve. */
	bool CanReload() const;

	/** Fills the magazine from the reserve. */
	void RefillMagazine();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> Mesh;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMEWeaponData> Data;

	int32 AmmoInMagazine = 0;
	int32 ReserveAmmo = 0;
};
