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

	AMEWeapon* GetWeapon() const { return Weapon; }
	UMEWeaponData* GetWeaponData() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Weapon the player starts with. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	TObjectPtr<UMEWeaponData> StartingWeapon;

	/** Actor class spawned for the weapon (a Blueprint child can add effects). */
	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	TSubclassOf<AMEWeapon> WeaponClass;

private:
	void Equip(UMEWeaponData* WeaponData);
	void PlayAction(const FMEWeaponAnimation& Action);

	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;

	/** Weapon drawn in first person, in the arms. */
	UPROPERTY(Transient)
	TObjectPtr<AMEWeapon> Weapon;

	/** Copy held by the world body: invisible, casts the shadow. */
	UPROPERTY(Transient)
	TObjectPtr<AMEWeapon> WorldWeapon;
};
