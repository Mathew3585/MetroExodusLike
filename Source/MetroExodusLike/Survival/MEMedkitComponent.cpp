#include "Survival/MEMedkitComponent.h"
#include "MetroExodusLike.h"
#include "Player/MECharacter.h"
#include "Weapons/MEWeaponComponent.h"
#include "Inventory/MEInventoryComponent.h"
#include "Combat/MEHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UMEMedkitComponent::UMEMedkitComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMEMedkitComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AMECharacter>(GetOwner());
}

void UMEMedkitComponent::UseMedkit()
{
	if (!Character || !MedkitItem || IsInUse())
	{
		return;
	}

	const UMEHealthComponent* Health = Character->GetHealth();
	if (Health->IsDead() || Health->GetHealth() >= Health->GetMaxHealth())
	{
		return;
	}
	if (Character->GetInventory()->GetCount(MedkitItem) == 0)
	{
		UE_LOG(LogME, Verbose, TEXT("No medkit left."));
		return;
	}

	const float WeaponAway = Character->GetWeapons()->PutAwayFor(InjectTime);
	if (WeaponAway < 0.f)
	{
		return;
	}

	bInjecting = true;
	if (WeaponAway > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(InjectTimer, this, &UMEMedkitComponent::Inject, WeaponAway);
	}
	else
	{
		Inject();
	}
}

void UMEMedkitComponent::Inject()
{
	bInjecting = false;

	// Dead during the gesture, or the medkit is gone: nothing is used
	if (Character->GetHealth()->IsDead() || Character->GetInventory()->Remove(MedkitItem, 1) == 0)
	{
		return;
	}

	if (InjectSound)
	{
		UGameplayStatics::PlaySound2D(this, InjectSound);
	}

	HealRemaining = HealAmount;
	SetComponentTickEnabled(true);
}

void UMEMedkitComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UMEHealthComponent* Health = Character->GetHealth();
	const float Step = HealTime > 0.f ? FMath::Min(HealRemaining, HealAmount / HealTime * DeltaTime) : HealRemaining;
	HealRemaining -= Step;
	Health->Heal(Step);

	if (HealRemaining <= 0.f || Health->IsDead())
	{
		HealRemaining = 0.f;
		SetComponentTickEnabled(false);
	}
}
