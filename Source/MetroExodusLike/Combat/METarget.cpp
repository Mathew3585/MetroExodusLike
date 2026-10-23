#include "Combat/METarget.h"
#include "Combat/MEHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AMETarget::AMETarget()
{
	// Only ticks while moving (hit kick, falling, standing up)
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(GetRootComponent());

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Pivot);

	Health = CreateDefaultSubobject<UMEHealthComponent>(TEXT("Health"));
}

void AMETarget::BeginPlay()
{
	Super::BeginPlay();

	Health->OnHealthChanged.AddUObject(this, &AMETarget::HandleHealthChanged);
	Health->OnDeath.AddUObject(this, &AMETarget::HandleDeath);
}

void AMETarget::HandleHealthChanged(float NewHealth, float Delta, AController* DamageInstigator)
{
	if (Delta >= 0.f)
	{
		return;
	}

	KickAngle = FMath::Min(KickAngle + HitKickAngle, HitKickAngle * 3.f);
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, Mesh->GetComponentLocation());
	}
	SetActorTickEnabled(true);
}

void AMETarget::HandleDeath(AController* Killer)
{
	TargetFallAngle = DownAngle;
	if (DownSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DownSound, Mesh->GetComponentLocation());
	}
	SetActorTickEnabled(true);

	if (ResetDelay > 0.f)
	{
		GetWorldTimerManager().SetTimer(ResetTimer, this, &AMETarget::StandUp, ResetDelay);
	}
}

void AMETarget::StandUp()
{
	TargetFallAngle = 0.f;
	Health->Revive();
	SetActorTickEnabled(true);
}

void AMETarget::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FallAngle = FMath::FInterpTo(FallAngle, TargetFallAngle, DeltaSeconds, FallSpeed);
	KickAngle = FMath::FInterpTo(KickAngle, 0.f, DeltaSeconds, HitRecoverySpeed);

	const bool bSettled = FMath::IsNearlyEqual(FallAngle, TargetFallAngle, 0.1f) && KickAngle < 0.05f;
	if (bSettled)
	{
		FallAngle = TargetFallAngle;
		KickAngle = 0.f;
		SetActorTickEnabled(false);
	}

	// Positive pitch tips the top toward -X: away from a shooter in front
	Pivot->SetRelativeRotation(FRotator(FMath::Min(FallAngle + KickAngle, 180.f), 0.f, 0.f));
}
