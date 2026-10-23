#include "Weapons/MEWeaponComponent.h"
#include "Weapons/MEWeapon.h"
#include "Weapons/MEWeaponData.h"
#include "Player/MECharacter.h"
#include "Player/MEFirstPersonAnimInstance.h"
#include "Player/MEFirstPersonCameraComponent.h"
#include "Inventory/MEInventoryComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

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
	UpdateAiming(DeltaTime);
	UpdateFiring();
	UpdateRecoil(DeltaTime);
}

void UMEWeaponComponent::UpdateAiming(float DeltaTime)
{
	const UMEWeaponData* Data = GetWeaponData();
	if (!Data)
	{
		return;
	}

	const bool bCanAim = bWantsToAim && !IsBusy() && !Character->IsSprinting() && WallBlockAmount <= MaxWallBlockToFire;

	// Hip and aim actions are authored for their stance: switching stance cuts them, so the weapon moves right away
	if (bCanAim != bAimTarget || Character->IsSprinting())
	{
		InterruptAction();
	}
	bAimTarget = bCanAim;

	AimAlpha = FMath::FInterpTo(AimAlpha, bCanAim ? 1.f : 0.f, DeltaTime, Data->AimSpeed);
}

void UMEWeaponComponent::Inspect()
{
	const UMEWeaponData* Data = GetWeaponData();
	if (!Data || IsBusy() || bWantsToAim || Character->IsSprinting() || WallBlockAmount > MaxWallBlockToFire)
	{
		return;
	}

	const bool bEmpty = Weapon->GetAmmoInMagazine() == 0 && Data->InspectEmpty.Arms;
	PlayAction(bEmpty ? Data->InspectEmpty : Data->Inspect, true);
}

int32 UMEWeaponComponent::GetReserveAmmo() const
{
	const UMEWeaponData* Data = GetWeaponData();
	if (!Data)
	{
		return 0;
	}
	return Data->AmmoType ? Character->GetInventory()->GetCount(Data->AmmoType) : MAX_int32;
}

bool UMEWeaponComponent::IsPlayingInterruptibleAction() const
{
	return bActionInterruptible && GetWorld()->GetTimeSeconds() < ActionEndTime;
}

void UMEWeaponComponent::InterruptAction()
{
	if (!IsPlayingInterruptibleAction())
	{
		return;
	}
	bActionInterruptible = false;

	if (UMEFirstPersonAnimInstance* ArmsAnim = Cast<UMEFirstPersonAnimInstance>(Character->GetFirstPersonArms()->GetAnimInstance()))
	{
		ArmsAnim->StopAction(InterruptBlendOutTime);
	}

	const UMEWeaponData* Data = GetWeaponData();
	Weapon->PlayWeaponAnimation(Weapon->GetAmmoInMagazine() == 0 && Data->WeaponEmptyPose ? Data->WeaponEmptyPose : Data->WeaponIdlePose);

	if (UAudioComponent* Sound = ActionSound.Get())
	{
		Sound->FadeOut(InterruptBlendOutTime, 0.f);
	}
}

float UMEWeaponComponent::GetSpreadAngle() const
{
	const UMEWeaponData* Data = GetWeaponData();
	if (!Data)
	{
		return 0.f;
	}

	const FMESpreadSettings& Spread = Data->Spread;
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const float MoveAlpha = FMath::Clamp(Movement->Velocity.Size2D() / FMath::Max(Movement->MaxWalkSpeed, 1.f), 0.f, 1.f);

	// Aiming steadies the base and the movement penalty, not the bloom (it already gets the recoil aim scale)
	const float Steady = FMath::Lerp(1.f, 0.3f, AimAlpha);
	float Angle = FMath::Lerp(Spread.Hip, Spread.Aim, AimAlpha)
		+ Spread.Moving * MoveAlpha * Steady
		+ (Movement->IsFalling() ? Spread.InAir : 0.f)
		+ SpreadBloom;
	return FMath::Min(Angle, FMath::Lerp(Spread.HipMax, Spread.AimMax, AimAlpha));
}

void UMEWeaponComponent::AddRecoil()
{
	const UMEWeaponData* Data = GetWeaponData();
	const FMERecoilSettings& Recoil = Data->Recoil;
	const float Scale = FMath::Lerp(1.f, Recoil.AimScale, AimAlpha);

	PendingRecoil.Pitch += Recoil.PitchPerShot * Scale * FMath::FRandRange(0.8f, 1.2f);
	PendingRecoil.Yaw += Recoil.YawPerShot * Scale * FMath::FRandRange(-1.f, 1.f);

	// View space: X forward, Z up
	ArmsKickVelocity += FVector(-Recoil.ArmsKickBack, 0.f, Recoil.ArmsKickUp) * Scale * FMath::FRandRange(0.9f, 1.1f);

	SpreadBloom += Data->Spread.PerShot * Scale;
}

void UMEWeaponComponent::UpdateRecoil(float DeltaTime)
{
	const UMEWeaponData* Data = GetWeaponData();
	AController* Controller = Character ? Character->GetController() : nullptr;
	if (!Data || !Controller)
	{
		return;
	}
	const FMERecoilSettings& Recoil = Data->Recoil;

	// View kick: reaches the view over a few frames, part of it is given back after the burst
	FRotator ViewDelta = FRotator::ZeroRotator;
	if (!PendingRecoil.IsNearlyZero())
	{
		const FRotator Step = PendingRecoil * FMath::Min(1.f, DeltaTime * Recoil.KickSpeed);
		PendingRecoil -= Step;
		RecoveryBudget += Step * Recoil.Recovery;
		ViewDelta += Step;
	}

	const bool bFiringBurst = GetWorld()->GetTimeSeconds() - LastShotTime < 60.0 / Data->RoundsPerMinute * 1.5;
	if (!bFiringBurst && !RecoveryBudget.IsNearlyZero())
	{
		const FRotator Step = RecoveryBudget * FMath::Min(1.f, DeltaTime * Recoil.RecoverySpeed);
		RecoveryBudget -= Step;
		ViewDelta -= Step;
	}

	if (!ViewDelta.IsNearlyZero())
	{
		Controller->SetControlRotation(Controller->GetControlRotation() + ViewDelta);
	}

	// Arms kick: damped spring, sub-stepped to stay stable on long frames
	constexpr float MaxStep = 1.f / 120.f;
	float Remaining = DeltaTime;
	while (Remaining > 0.f)
	{
		const float Step = FMath::Min(Remaining, MaxStep);
		const FVector Acceleration = -Recoil.ArmsKickStiffness * ArmsKick - Recoil.ArmsKickDamping * ArmsKickVelocity;
		ArmsKickVelocity += Acceleration * Step;
		ArmsKick += ArmsKickVelocity * Step;
		Remaining -= Step;
	}

	SpreadBloom = FMath::Max(0.f, SpreadBloom - Data->Spread.RecoverySpeed * DeltaTime);
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
			// The dry fire clip is a hip pose: aiming, only the trigger click plays so the sights stay up
			FMEWeaponAnimation DryFire = Data->DryFire;
			if (AimAlpha > 0.5f)
			{
				DryFire.Arms = nullptr;
			}
			PlayAction(DryFire, true);
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
	const bool bAiming = AimAlpha > 0.5f && Data->AimFire.Arms;
	PlayAction(bAiming ? Data->AimFire : bLastRound ? Data->FireLast : Data->Fire, true);
	if (Data->FireSound)
	{
		UGameplayStatics::SpawnSoundAttached(Data->FireSound, Weapon->GetMesh());
	}

	// Hitscan from the eye, inside the spread cone around the view (spread taken before this shot's bloom)
	const UMEFirstPersonCameraComponent* Camera = Character->GetFirstPersonCamera();
	const FVector Start = Camera->GetComponentLocation();
	const FVector Direction = FMath::VRandCone(Camera->GetForwardVector(), FMath::DegreesToRadians(GetSpreadAngle()));
	AddRecoil();

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
	if (!Data || IsBusy() || Weapon->GetRoomInMagazine() == 0 || GetReserveAmmo() == 0)
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
	const UMEWeaponData* Data = GetWeaponData();
	if (!Data)
	{
		return;
	}

	// The rounds come out of the inventory now (some may have been used or picked up during the reload)
	const int32 Rounds = Data->AmmoType
		? Character->GetInventory()->Remove(Data->AmmoType, Weapon->GetRoomInMagazine())
		: Weapon->GetRoomInMagazine();
	Weapon->LoadRounds(Rounds);
	PrintAmmo();
}

void UMEWeaponComponent::PrintAmmo() const
{
	if (bDebugShots && GEngine && Weapon)
	{
		// Same key every time: the line is replaced instead of stacking
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 5.f, FColor::White,
			FString::Printf(TEXT("%s  %d / %d"), *Weapon->GetData()->DisplayName.ToString(), Weapon->GetAmmoInMagazine(), GetReserveAmmo()));
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
	if (!Character)
	{
		return;
	}

	// Picked-up ammo changes the reserve
	Character->GetInventory()->OnChanged.AddWeakLambda(this, [this](const UMEItemData*, int32) { PrintAmmo(); });

	if (StartingWeapon)
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

float UMEWeaponComponent::PlayAction(const FMEWeaponAnimation& Action, bool bInterruptible)
{
	// A new action replaces the previous one: an interrupted inspect must not keep talking
	if (IsPlayingInterruptibleAction())
	{
		if (UAudioComponent* Sound = ActionSound.Get())
		{
			Sound->FadeOut(InterruptBlendOutTime, 0.f);
		}
	}
	ActionSound.Reset();

	if (UMEFirstPersonAnimInstance* ArmsAnim = Cast<UMEFirstPersonAnimInstance>(Character->GetFirstPersonArms()->GetAnimInstance()))
	{
		ArmsAnim->PlayAction(Action.Arms);
	}
	Weapon->PlayWeaponAnimation(Action.Weapon);

	if (Action.Sound)
	{
		ActionSound = UGameplayStatics::SpawnSoundAttached(Action.Sound, Weapon->GetMesh());
	}

	const float Length = Action.Arms ? Action.Arms->GetPlayLength() : 0.f;
	bActionInterruptible = bInterruptible;
	ActionEndTime = GetWorld()->GetTimeSeconds() + Length;
	return Length;
}
