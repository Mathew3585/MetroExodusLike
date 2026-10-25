#include "AI/MEAIWeaponComponent.h"
#include "AI/MENoise.h"
#include "Weapons/MEWeapon.h"
#include "Weapons/MEWeaponData.h"
#include "Combat/MEImpactData.h"
#include "GameFramework/Pawn.h"
#include "AI/MEAICharacter.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UMEAIWeaponComponent::UMEAIWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool UMEAIWeaponComponent::IsEmpty() const
{
	return !Weapon || Weapon->GetAmmoInMagazine() == 0;
}

void UMEAIWeaponComponent::StartBurst(AActor* InTarget, const FVector& Location, int32 Rounds, float Spread, float DamageScale)
{
	if (!Weapon || !Weapon->GetData() || bReloading || IsEmpty())
	{
		return;
	}
	Target = InTarget;
	TargetLocation = Location;
	RoundsLeft = Rounds;
	BurstSpread = Spread;
	BurstDamageScale = DamageScale;
	ShotCooldown = 0.f;
	SetComponentTickEnabled(true);
}

void UMEAIWeaponComponent::StopFiring()
{
	RoundsLeft = 0;
	SetComponentTickEnabled(false);
}

void UMEAIWeaponComponent::Reload()
{
	if (bReloading || !Weapon)
	{
		return;
	}
	StopFiring();
	bReloading = true;
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &UMEAIWeaponComponent::FinishReload, FMath::Max(ReloadTime, 0.01f));
}

void UMEAIWeaponComponent::FinishReload()
{
	bReloading = false;
	if (Weapon)
	{
		Weapon->LoadRounds(Weapon->GetRoomInMagazine());
	}
}

void UMEAIWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ShotCooldown -= DeltaTime;
	while (RoundsLeft > 0 && ShotCooldown <= 0.f)
	{
		if (IsEmpty())
		{
			StopFiring();
			return;
		}
		FireShot();
		--RoundsLeft;
		ShotCooldown += 60.f / FMath::Max(Weapon->GetData()->RoundsPerMinute, 1.f);
	}
	if (RoundsLeft <= 0)
	{
		StopFiring();
	}
}

void UMEAIWeaponComponent::FireShot()
{
	const UMEWeaponData* Data = Weapon->GetData();
	AActor* Shooter = GetOwner();
	Weapon->ConsumeRound();

	const FVector Muzzle = Weapon->GetMuzzle()->GetComponentLocation();
	float Spread = BurstSpread;
	if (const AActor* Followed = Target.Get())
	{
		TargetLocation = Followed->GetActorLocation() + FVector(0.f, 0.f, AimHeight);
		Spread += MovingTargetSpread * Followed->GetVelocity().Size() / 100.f;
	}
	const FVector Direction = FMath::VRandCone((TargetLocation - Muzzle).GetSafeNormal(), FMath::DegreesToRadians(Spread));

	if (Data->FireSound)
	{
		UGameplayStatics::SpawnSoundAttached(Data->FireSound, Weapon->GetMesh());
	}
	FVector Eyes;
	FRotator View;
	Shooter->GetActorEyesViewPoint(Eyes, View);
	Weapon->PlayMuzzleFlash(Eyes);
	MENoise::Report(Shooter, Muzzle, Data->ShotNoise, MENoise::Gunshot);

	// The Pawn channel: the player's capsule stops it (it ignores Visibility), walls and cover too.
	// Other enemies are not in the way: no friendly fire turning them on each other.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEAIShot), true, Shooter);
	Params.AddIgnoredActor(Weapon);
	for (TActorIterator<AMEAICharacter> It(GetWorld()); It; ++It)
	{
		Params.AddIgnoredActor(*It);
	}
	Params.bReturnPhysicalMaterial = true;
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Muzzle, Muzzle + Direction * Data->Range, ECC_Pawn, Params))
	{
		const APawn* Pawn = Cast<APawn>(Shooter);
		UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Data->Damage * BurstDamageScale, Direction, Hit, Pawn ? Pawn->GetController() : nullptr, Shooter, nullptr);
		if (Data->Impacts)
		{
			Data->Impacts->SpawnImpact(this, Hit);
		}
	}
}
