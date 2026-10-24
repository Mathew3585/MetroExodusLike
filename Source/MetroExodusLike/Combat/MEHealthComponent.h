#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MEHealthComponent.generated.h"

class AController;
class UDamageType;

/** Health went down (damage) or up (heal). Delta is negative for damage. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FMEHealthChangedSignature, float /*Health*/, float /*Delta*/, AController* /*Instigator*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FMEDeathSignature, AController* /*Killer*/);

/**
 * Health of anything that can be hurt: targets, enemies, the player.
 * Listens to the owner's damage (ApplyDamage / ApplyPointDamage), so damage dealers need no special code.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMEHealthComponent();

	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }
	bool IsDead() const { return Health <= 0.f; }

	/** Gives health back. Does nothing once dead. */
	void Heal(float Amount);

	/** Back to full health, alive again (respawn, target reset...). */
	void Revive();

	/** Kind of the damage being dealt, valid inside OnHealthChanged / OnDeath only (null for heals). */
	const UDamageType* GetCurrentDamageType() const { return CurrentDamageType; }

	FMEHealthChangedSignature OnHealthChanged;
	FMEDeathSignature OnDeath;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	/** Takes no damage (scripted moments, tests). */
	UPROPERTY(EditAnywhere, Category = "Health")
	bool bInvulnerable = false;

private:
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	float Health = 0.f;

	/** Class default object of the damage type: never collected, no UPROPERTY needed. */
	const UDamageType* CurrentDamageType = nullptr;
};
