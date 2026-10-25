#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MEAICharacter.generated.h"

class UMEHealthComponent;
class UMEAIWeaponComponent;
class UMEToxicityComponent;
class UMEEnemyProfile;
class USpotLightComponent;
class UMEFootstepComponent;
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

	virtual void Tick(float DeltaSeconds) override;

	/** Bullets hurt by where they hit (head, limbs...). */
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UMEHealthComponent* GetHealth() const { return Health; }
	UMEAIWeaponComponent* GetAIWeapon() const { return AIWeapon; }
	/** Its kind of fighter; the default profile if none is set. */
	const UMEEnemyProfile* GetProfile() const;
	/** Bone hit by the last shot (legs: it limps). */
	FName GetLastHitBone() const { return LastHitBone; }

	/** Gives up: the weapon falls in front of it. */
	void DropWeapon();
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEAIWeaponComponent> AIWeapon;

	/** Toxic air hurts it too, unless its profile has a gas mask. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEToxicityComponent> Toxicity;

	/** Its steps: heard by the player (surface and pace like the player's own). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEFootstepComponent> Footsteps;

	/** Weapon light, along the barrel. Turned on per enemy (bFlashlight). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpotLightComponent> Flashlight;

	/** Recruit, veteran... Can be changed on each placed enemy. */
	UPROPERTY(EditAnywhere, Category = "Enemy")
	TObjectPtr<UMEEnemyProfile> Profile;

	/** Carries a lit flashlight (dark places). */
	UPROPERTY(EditAnywhere, Category = "Enemy")
	bool bFlashlight = false;

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

	// --- Hit zones: damage multiplied by the part hit (the torso takes it as is) ---

	/** Head and neck: one rifle round kills. */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Zones", meta = (ClampMin = "0"))
	float HeadDamageScale = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hit Zones", meta = (ClampMin = "0"))
	float ArmDamageScale = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Hit Zones", meta = (ClampMin = "0"))
	float LegDamageScale = 0.7f;

	// --- Hit reaction: the hit part of the body takes the bullet's push (physics), then the animation takes over again ---

	/** Bodies from this bone down react (the legs keep walking). */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Reaction")
	FName HitReactionBone = TEXT("spine_01");

	/** Physics share at the hit, fading to 0. */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Reaction", meta = (ClampMin = "0", ClampMax = "1"))
	float HitReactionBlend = 0.85f;

	UPROPERTY(EditDefaultsOnly, Category = "Hit Reaction", meta = (ClampMin = "0.05", Units = "s"))
	float HitReactionTime = 0.35f;

	/** Speed given to the hit bone, along the shot. */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Reaction", meta = (ClampMin = "0", Units = "cm/s"))
	float HitImpulse = 800.f;

	/** Same for the killing shot, on the ragdoll. */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Reaction", meta = (ClampMin = "0", Units = "cm/s"))
	float DeathImpulse = 900.f;

private:
	void EquipStartingWeapon();
	void HandleDeath(AController* Killer);

	UFUNCTION()
	void HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation, UPrimitiveComponent* HitComponent,
		FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser);

	float HitReactionRemaining = 0.f;
	FName LastHitBone;

	UPROPERTY(Transient)
	TObjectPtr<AMEWeapon> Weapon;
};
