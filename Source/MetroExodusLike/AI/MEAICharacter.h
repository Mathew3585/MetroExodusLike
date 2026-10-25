#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MEAICharacter.generated.h"

class UMEHealthComponent;
class AMEWeapon;
class AMEPatrolPath;
class AController;

/**
 * A human enemy: a weapon in hand, health, and two bodies:
 * - GetMesh: the MetaHuman animation body, hidden. It runs the player's AnimBP (same locomotion) and holds the weapon.
 * - Visual: the body seen (and shot), any skeleton, copying that pose through a retargeting AnimBP.
 *   Placeholder: the mannequin, until real enemy models.
 * Its brain is AMEAIController; this class only holds the body and what the level designer sets
 * on each placed enemy (patrol path...).
 */
UCLASS()
class METROEXODUSLIKE_API AMEAICharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMEAICharacter();

	UMEHealthComponent* GetHealth() const { return Health; }
	USkeletalMeshComponent* GetVisual() const { return Visual; }
	AMEWeapon* GetWeapon() const { return Weapon; }
	AMEPatrolPath* GetPatrolPath() const { return PatrolPath; }
	int32 GetPatrolStartIndex() const { return PatrolStartIndex; }

	/** Walking speed while calm. */
	float GetPatrolSpeed() const { return PatrolSpeed; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEHealthComponent> Health;

	/** Body seen in the world: its AnimBP retargets the pose of GetMesh (its parent). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> Visual;

	/** Extra meshes of the visible body (clothing, head...), following its pose. */
	UPROPERTY(EditDefaultsOnly, Category = "Body")
	TArray<TObjectPtr<USkeletalMesh>> BodyParts;

	/** Weapon in hand (BP_Weapon_*). Held with its GripBone / GripOffset, like the player's world weapon. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AMEWeapon> StartingWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0", Units = "cm/s"))
	float PatrolSpeed = 150.f;

	/** Route walked while calm. None: stays where placed. */
	UPROPERTY(EditInstanceOnly, Category = "Patrol")
	TObjectPtr<AMEPatrolPath> PatrolPath;

	/** Point of the path to start from (enemies sharing a path start apart). */
	UPROPERTY(EditInstanceOnly, Category = "Patrol", meta = (ClampMin = "0"))
	int32 PatrolStartIndex = 0;

private:
	void EquipStartingWeapon();
	void HandleDeath(AController* Killer);

	UPROPERTY(Transient)
	TObjectPtr<AMEWeapon> Weapon;
};
