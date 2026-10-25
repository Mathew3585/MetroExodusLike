#include "Survival/METoxicityComponent.h"
#include "GameFramework/Pawn.h"
#include "Combat/MEHealthComponent.h"
#include "Combat/MEDamageType_Toxic.h"
#include "Kismet/GameplayStatics.h"

UMEToxicityComponent::UMEToxicityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMEToxicityComponent::BeginPlay()
{
	Super::BeginPlay();

	// Anyone with health can be poisoned: the player, enemies without a mask
	Health = GetOwner()->FindComponentByClass<UMEHealthComponent>();
	if (!Health)
	{
		SetComponentTickEnabled(false);
	}
}

void UMEToxicityComponent::AddToxicZone(const UObject* Zone, float Density)
{
	Zones.Add(Zone, Density);
}

void UMEToxicityComponent::RemoveToxicZone(const UObject* Zone)
{
	Zones.Remove(Zone);
}

float UMEToxicityComponent::GetExposure() const
{
	float Exposure = 0.f;
	for (const TPair<TWeakObjectPtr<const UObject>, float>& Zone : Zones)
	{
		if (Zone.Key.IsValid())
		{
			Exposure = FMath::Max(Exposure, Zone.Value);
		}
	}
	return Exposure;
}

void UMEToxicityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Health->IsDead())
	{
		return;
	}

	const float Exposure = bProtected ? 0.f : GetExposure();
	Toxicity = Exposure > 0.f
		? FMath::Min(1.f, Toxicity + Exposure * DeltaTime / BuildUpTime)
		: FMath::Max(0.f, Toxicity - DeltaTime / RecoveryTime);

	UpdateDamage(DeltaTime);
	UpdateCoughs(DeltaTime);
}

void UMEToxicityComponent::UpdateDamage(float DeltaTime)
{
	if (Toxicity <= DamageThreshold || DamageThreshold >= 1.f)
	{
		PendingDamage = 0.f;
		DamageTimer = 0.f;
		return;
	}

	const float Severity = (Toxicity - DamageThreshold) / (1.f - DamageThreshold);
	PendingDamage += MaxDamagePerSecond * Severity * DeltaTime;
	DamageTimer += DeltaTime;
	if (DamageTimer >= DamageInterval)
	{
		UGameplayStatics::ApplyDamage(GetOwner(), PendingDamage, nullptr, nullptr, UMEDamageType_Toxic::StaticClass());
		PendingDamage = 0.f;
		DamageTimer = 0.f;
	}
}

void UMEToxicityComponent::UpdateCoughs(float DeltaTime)
{
	if (!CoughSound || Toxicity < CoughThreshold)
	{
		// First cough comes quickly once the air turns bad
		CoughTimer = FMath::Min(CoughTimer, 1.f);
		return;
	}

	CoughTimer -= DeltaTime;
	if (CoughTimer <= 0.f)
	{
		// The player hears their own coughs in the head, the others' where they are
		const APawn* Pawn = Cast<APawn>(GetOwner());
		if (Pawn && Pawn->IsLocallyControlled() && Pawn->IsPlayerControlled())
		{
			UGameplayStatics::PlaySound2D(this, CoughSound);
		}
		else
		{
			UGameplayStatics::PlaySoundAtLocation(this, CoughSound, GetOwner()->GetActorLocation() + FVector(0.f, 0.f, 60.f));
		}
		const float Severity = FMath::GetRangePct(CoughThreshold, 1.f, Toxicity);
		CoughTimer = FMath::Lerp(CoughIntervalMild, CoughIntervalSevere, Severity) * FMath::FRandRange(0.8f, 1.2f);
	}
}
