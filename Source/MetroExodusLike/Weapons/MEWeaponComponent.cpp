#include "Weapons/MEWeaponComponent.h"
#include "Weapons/MEWeapon.h"
#include "Weapons/MEWeaponData.h"
#include "Player/MECharacter.h"
#include "Player/MEFirstPersonAnimInstance.h"
#include "Player/MEFirstPersonCameraComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

UMEWeaponComponent::UMEWeaponComponent()
{
	// Ticks for the wall check
	PrimaryComponentTick.bCanEverTick = true;
	WeaponClass = AMEWeapon::StaticClass();
}

UMEWeaponData* UMEWeaponComponent::GetWeaponData() const
{
	return Weapon ? Weapon->GetData() : nullptr;
}

void UMEWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateWallBlock(DeltaTime);
	UpdateFiring();
}

bool UMEWeaponComponent::IsBusy() const
{
	return GetWorld()->GetTimeSeconds() < BusyUntil;
}

void UMEWeaponComponent::StartFire()
{
	bWantsToFire = true;
	bFiredThisPress = false;
	UpdateFiring();
}

void UMEWeaponComponent::StopFire()
{
	bWantsToFire = false;
}

void UMEWeaponComponent::UpdateFiring()
{
	const UMEWeaponData* Data = GetWeaponData();
	if (!bWantsToFire || !Data || (!Data->bFullAuto && bFiredThisPress))
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	const double Interval = 60.0 / Data->RoundsPerMinute;
	if (Now - LastShotTime < Interval || IsBusy() || Character->IsSprinting() || WallBlockAmount > MaxWallBlockToFire)
	{
		return;
	}

	// Empty: one click per trigger pull
	if (Weapon->GetAmmoInMagazine() == 0)
	{
		if (!bFiredThisPress)
		{
			PlayAction(Data->DryFire);
		}
		bFiredThisPress = true;
		return;
	}

	// Holding the trigger keeps the exact rate, whatever the frame rate
	LastShotTime = (Now - LastShotTime < Interval * 2.0) ? LastShotTime + Interval : Now;
	bFiredThisPress = true;
	FireShot();
}

void UMEWeaponComponent::FireShot()
{
	const UMEWeaponData* Data = GetWeaponData();

	Weapon->ConsumeRound();
	const bool bLastRound = Weapon->GetAmmoInMagazine() == 0 && Data->FireLast.Arms;
	PlayAction(bLastRound ? Data->FireLast : Data->Fire);
	if (Data->FireSound)
	{
		UGameplayStatics::SpawnSoundAttached(Data->FireSound, Weapon->GetMesh());
	}

	// Hitscan from the eye, along the view
	const UMEFirstPersonCameraComponent* Camera = Character->GetFirstPersonCamera();
	const FVector Start = Camera->GetComponentLocation();
	const FVector Direction = Camera->GetForwardVector();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEWeaponShot), true, Character);
	Params.AddIgnoredActor(Weapon);
	Params.AddIgnoredActor(WorldWeapon);
	Params.bReturnPhysicalMaterial = true;

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Direction * Data->Range, ECC_Visibility, Params))
	{
		UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Data->Damage, Direction, Hit, Character->GetController(), Character, nullptr);

		if (bDebugShots)
		{
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 3.f, 8, FColor::Red, false, 3.f);
			DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Hit.ImpactNormal * 10.f, FColor::Yellow, false, 3.f);
		}
	}

	PrintAmmo();
}

void UMEWeaponComponent::Reload()
{
	const UMEWeaponData* Data = GetWeaponData();
	if (!Data || IsBusy() || !Weapon->CanReload())
	{
		return;
	}

	// A round left in the chamber: faster tactical reload, no bolt release
	const FMEWeaponAnimation& Action = Weapon->GetAmmoInMagazine() > 0 ? Data->Reload : Data->ReloadEmpty;
	const float Duration = FMath::Max(PlayAction(Action), 0.1f);
	BusyUntil = GetWorld()->GetTimeSeconds() + Duration;

	// Rounds go in when the animation is done
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &UMEWeaponComponent::FinishReload, Duration);
}

void UMEWeaponComponent::FinishReload()
{
	if (Weapon)
	{
		Weapon->RefillMagazine();
		PrintAmmo();
	}
}

void UMEWeaponComponent::PrintAmmo() const
{
	if (bDebugShots && GEngine && Weapon)
	{
		// Same key every time: the line is replaced instead of stacking
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 5.f, FColor::White,
			FString::Printf(TEXT("%s  %d / %d"), *Weapon->GetData()->DisplayName.ToString(), Weapon->GetAmmoInMagazine(), Weapon->GetReserveAmmo()));
	}
}

void UMEWeaponComponent::UpdateWallBlock(float DeltaTime)
{
	const UMEWeaponData* Data = GetWeaponData();
	if (!Data || !Character)
	{
		return;
	}

	// Free space in front of the eye, along the view
	const UMEFirstPersonCameraComponent* Camera = Character->GetFirstPersonCamera();
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * Data->WallCheckLength;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEWeaponWallCheck), false, Character);
	Params.AddIgnoredActor(Weapon);
	Params.AddIgnoredActor(WorldWeapon);

	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(WallCheckRadius), Params);

	// 0 at the check length, 1 at the against-wall distance
	const float Distance = bHit ? Hit.Distance : Data->WallCheckLength;
	const float Target = 1.f - FMath::GetRangePct(Data->AgainstWallDistance, Data->WallCheckLength, FMath::Clamp(Distance, Data->AgainstWallDistance, Data->WallCheckLength));
	WallBlockAmount = FMath::FInterpTo(WallBlockAmount, Target, DeltaTime, WallBlockBlendSpeed);
}

void UMEWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AMECharacter>(GetOwner());
	if (Character && StartingWeapon)
	{
		Equip(StartingWeapon);
	}
}

void UMEWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);

	for (AMEWeapon* Spawned : { Weapon.Get(), WorldWeapon.Get() })
	{
		if (Spawned)
		{
			Spawned->Destroy();
		}
	}
	Weapon = nullptr;
	WorldWeapon = nullptr;

	Super::EndPlay(EndPlayReason);
}

void UMEWeaponComponent::Equip(UMEWeaponData* WeaponData)
{
	FActorSpawnParameters Params;
	Params.Owner = Character;
	Params.Instigator = Character;

	// First-person weapon, in the arms
	Weapon = GetWorld()->SpawnActor<AMEWeapon>(WeaponClass, Params);
	Weapon->Initialize(WeaponData, true);

	USkeletalMeshComponent* Arms = Character->GetFirstPersonArms();
	Weapon->AttachToComponent(Arms, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponData->ArmsGripBone);
	Weapon->SetActorRelativeTransform(WeaponData->ArmsGripOffset);

	if (UMEFirstPersonAnimInstance* ArmsAnim = Cast<UMEFirstPersonAnimInstance>(Arms->GetAnimInstance()))
	{
		ArmsAnim->SetWeaponData(WeaponData);
	}

	// World weapon, in the world body's hands: casts the shadow, seen by the AI
	WorldWeapon = GetWorld()->SpawnActor<AMEWeapon>(WeaponClass, Params);
	WorldWeapon->Initialize(WeaponData, false);
	WorldWeapon->AttachToComponent(Character->GetWorldBody(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponData->GripBone);
	WorldWeapon->SetActorRelativeTransform(WeaponData->GripOffset);

	Character->SetBodyWeaponPose(WeaponData->BodyHoldPose);

	BusyUntil = GetWorld()->GetTimeSeconds() + PlayAction(WeaponData->Draw);
	PrintAmmo();
}

float UMEWeaponComponent::PlayAction(const FMEWeaponAnimation& Action)
{
	if (UMEFirstPersonAnimInstance* ArmsAnim = Cast<UMEFirstPersonAnimInstance>(Character->GetFirstPersonArms()->GetAnimInstance()))
	{
		ArmsAnim->PlayAction(Action.Arms);
	}
	Weapon->PlayWeaponAnimation(Action.Weapon);

	if (Action.Sound)
	{
		UGameplayStatics::SpawnSoundAttached(Action.Sound, Weapon->GetMesh());
	}

	return Action.Arms ? Action.Arms->GetPlayLength() : 0.f;
}
