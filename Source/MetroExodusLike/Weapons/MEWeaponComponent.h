#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MEWeaponComponent.generated.h"

class AMECharacter;
class AMEWeapon;
class UMEWeaponData;
struct FMEWeaponAnimation;

/**
 * The player's weapon handling: spawns the weapon in the first-person arms,
 * and a copy in the world body's hands (shadow, AI).
 * Firing, reloading and aiming come in the next steps (3.3, 3.4).
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

	/** How fast the weapon goes aside / comes back. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapons|Wall", meta = (ClampMin = "0"))
	float WallBlockBlendSpeed = 8.f;

private:
	void Equip(UMEWeaponData* WeaponData);
	void PlayAction(const FMEWeaponAnimation& Action);
	void UpdateWallBlock(float DeltaTime);

	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;

	/** Weapon drawn in first person, in the arms. */
	UPROPERTY(Transient)
	TObjectPtr<AMEWeapon> Weapon;

	/** Copy held by the world body: invisible, casts the shadow. */
	UPROPERTY(Transient)
	TObjectPtr<AMEWeapon> WorldWeapon;

	float WallBlockAmount = 0.f;
};
