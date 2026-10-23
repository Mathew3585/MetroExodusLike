#include "Combat/MEHealthComponent.h"
#include "GameFramework/Actor.h"

UMEHealthComponent::UMEHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMEHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	GetOwner()->OnTakeAnyDamage.AddDynamic(this, &UMEHealthComponent::HandleTakeAnyDamage);
}

void UMEHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f || bInvulnerable || IsDead())
	{
		return;
	}

	const float OldHealth = Health;
	Health = FMath::Max(0.f, Health - Damage);
	OnHealthChanged.Broadcast(Health, Health - OldHealth, InstigatedBy);

	if (IsDead())
	{
		OnDeath.Broadcast(InstigatedBy);
	}
}

void UMEHealthComponent::Heal(float Amount)
{
	if (Amount <= 0.f || IsDead())
	{
		return;
	}

	const float OldHealth = Health;
	Health = FMath::Min(MaxHealth, Health + Amount);
	if (Health != OldHealth)
	{
		OnHealthChanged.Broadcast(Health, Health - OldHealth, nullptr);
	}
}

void UMEHealthComponent::Revive()
{
	const float OldHealth = Health;
	Health = MaxHealth;
	OnHealthChanged.Broadcast(Health, Health - OldHealth, nullptr);
}
