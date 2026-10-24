#include "Weapons/MEWeaponComponent.h"
#include "MetroExodusLike.h"
#include "Weapons/MEWeapon.h"
#include "Weapons/MEWeaponData.h"
#include "Weapons/MEShellCasing.h"
#include "Player/MECharacter.h"
#include "Player/MEFirstPersonAnimInstance.h"
#include "Player/MEFirstPersonCameraComponent.h"
#include "Inventory/MEInventoryComponent.h"
#include "Combat/MEHealthComponent.h"
#include "Combat/MEImpactData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/ArrowComponent.h"
#include "Components/AudioComponent.h"

UMEWeaponComponent::UMEWeaponComponent()
{
	// Ticks for the wall check
	PrimaryComponentTick.bCanEverTick = true;
}

UMEWeaponData* UMEWeaponComponent::GetWeaponData() const
{
	return Weapon ? Weapon->GetData() : nullptr;
}

void UMEWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateHolster();
	UpdateWallBlock(DeltaTime);
	UpdateAiming(DeltaTime);
	UpdateFiring();
	// Before the recoil moves the view: the sway follows the player's turns only
	UpdateSway(DeltaTime);
	UpdateRecoil(DeltaTime);

	if (const AController* Controller = Character ? Character->GetController() : nullptr)
	{
		LastControlRotation = Controller->GetControlRotation();
		bHasLastControlRotation = true;
	}
}

void UMEWeaponComponent::UpdateSway(float DeltaTime)
{
	const UMEWeaponData* Data = GetWeaponData();
	const AController* Controller = Character ? Character->GetController() : nullptr;
	if (!Data || !Controller || DeltaTime <= 0.f)
	{
		return;
	}
	const FMESwaySettings& Sway = Data->Sway;
	const float Scale = FMath::Lerp(1.f, Sway.AimScale, AimAlpha);
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();

	// Look: the weapon is left behind the turn, tipping with its weight
	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator Turn = bHasLastControlRotation ? (ControlRotation - LastControlRotation).GetNormalized() : FRotator::ZeroRotator;
	const float LagYaw = FMath::Clamp(-Turn.Yaw / DeltaTime * Sway.LookLag, -Sway.MaxLookAngle, Sway.MaxLookAngle);
	const float LagPitch = FMath::Clamp(-Turn.Pitch / DeltaTime * Sway.LookLag, -Sway.MaxLookAngle, Sway.MaxLookAngle);

	// Move: pushed back and left behind sideways, rolled when strafing; vertical speed lifts or sinks it
	const FVector LocalVelocity = FRotator(0.f, ControlRotation.Yaw, 0.f).UnrotateVector(Movement->Velocity);
	const FVector Move = Movement->IsMovingOnGround()
		? (LocalVelocity / Sway.MoveReferenceSpeed).BoundToCube(1.f)
		: FVector::ZeroVector;
	const float Vertical = FMath::Clamp(-LocalVelocity.Z * Sway.VerticalLag, -Sway.MaxVerticalOffset, Sway.MaxVerticalOffset);

	// View space (X forward, Y right, Z up); rotation as (pitch, yaw, roll)
	const FVector TargetLocation = FVector(-Move.X * Sway.MoveOffset.X, -Move.Y * Sway.MoveOffset.Y, Vertical) * Scale;
	const FVector TargetRotation = FVector(LagPitch, LagYaw, LagYaw * Sway.LookRoll + Move.Y * Sway.MoveRoll) * Scale;

	// Landing: thrown down, then the spring brings it back
	const bool bIsFalling = Movement->IsFalling();
	if (bIsFalling)
	{
		SwayFallSpeed = FMath::Max(0.f, -Movement->Velocity.Z);
	}
	else if (bSwayWasFalling)
	{
		SwayLocationVelocity.Z -= SwayFallSpeed * Sway.LandingKick * Scale;
		SwayRotationVelocity.X -= SwayFallSpeed * Sway.LandingPitchKick * Scale;
	}
	bSwayWasFalling = bIsFalling;

	// Damped springs toward the targets, sub-stepped to stay stable on long frames
	constexpr float MaxStep = 1.f / 120.f;
	float Remaining = DeltaTime;
	while (Remaining > 0.f)
	{
		const float Step = FMath::Min(Remaining, MaxStep);
		SwayLocationVelocity += (Sway.Stiffness * (TargetLocation - SwayLocation) - Sway.Damping * SwayLocationVelocity) * Step;
		SwayLocation += SwayLocationVelocity * Step;
		SwayRotationVelocity += (Sway.Stiffness * (TargetRotation - SwayRotation) - Sway.Damping * SwayRotationVelocity) * Step;
		SwayRotation += SwayRotationVelocity * Step;
		Remaining -= Step;
	}

	// Idle: slow figure eight, wider when out of breath
	const float Exertion = Character->GetFirstPersonCamera() ? Character->GetFirstPersonCamera()->GetExertion() : 0.f;
	const float IdleAngle = FMath::Lerp(Sway.RestedIdleAngle, Sway.ExhaustedIdleAngle, Exertion) * Scale;
	SwayIdlePhase = FMath::Fmod(SwayIdlePhase + DeltaTime * Sway.IdleRate * UE_TWO_PI, UE_TWO_PI);
	const FVector Idle(FMath::Sin(SwayIdlePhase * 2.f) * IdleAngle * 0.5f, FMath::Sin(SwayIdlePhase) * IdleAngle, 0.f);

	const FVector Rotation = SwayRotation + Idle;
	ArmsSway = FTransform(FRotator(Rotation.X, Rotation.Y, Rotation.Z), SwayLocation);
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

void UMEWeaponComponent::SwitchFireMode()
{
	const UMEWeaponData* Data = GetWeaponData();
	if (!Data || IsBusy() || Character->IsSprinting() || !Weapon->CycleFireMode())
	{
		return;
	}

	// A burst in progress stops with the selector
	BurstShotsLeft = 0;

	if (const FMEWeaponAnimation* Switch = Data->FireModeSwitch.Find(Weapon->GetFireMode()))
	{
		// The selector clips are hip poses: aiming, only the weapon and the click play so the sights stay up
		FMEWeaponAnimation Action = *Switch;
		if (AimAlpha > 0.5f)
		{
			Action.Arms = nullptr;
		}
		PlayAction(Action, true);
	}
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
	const UMEWeaponData* Data = GetWeaponData();

	if (UMEFirstPersonAnimInstance* ArmsAnim = Cast<UMEFirstPersonAnimInstance>(Character->GetFirstPersonArms()->GetAnimInstance()))
	{
		ArmsAnim->StopAction(InterruptBlendOutTime);
	}

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
	AController* Controller = Character ? Character->GetController() : nullptr;
	const UMEWeaponData* Data = GetWeaponData();
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
	return bHolstered || GetWorld()->GetTimeSeconds() < BusyUntil;
}

void UMEWeaponComponent::UpdateHolster()
{
	const UMEWeaponData* Data = GetWeaponData();
	const bool bWantsHolstered = HolsterRequests > 0;
	if (!Data || bWantsHolstered == bHolstered || GetWorld()->GetTimeSeconds() < BusyUntil)
	{
		return;
	}

	if (bWantsHolstered)
	{
		BurstShotsLeft = 0;
		InterruptAction();
		BusyUntil = GetWorld()->GetTimeSeconds() + PlayAction(Data->Holster, false, true);
		bHolstered = true;
	}
	else
	{
		bHolstered = false;
		BusyUntil = GetWorld()->GetTimeSeconds() + PlayAction(Data->Draw);
	}
}

void UMEWeaponComponent::StartFire()
{
	bWantsToFire = true;
	bFiredThisPress = false;
	const UMEWeaponData* Data = GetWeaponData();
	if (Data && Weapon->GetFireMode() == EMEFireMode::Burst && BurstShotsLeft == 0)
	{
		BurstShotsLeft = Data->BurstCount;
	}
	UpdateFiring();
}

void UMEWeaponComponent::StopFire()
{
	bWantsToFire = false;
}

void UMEWeaponComponent::UpdateFiring()
{
	const UMEWeaponData* Data = GetWeaponData();
	if (!Data)
	{
		return;
	}

	// Single: one shot per pull. Burst: goes on until done, trigger held or not. Auto: while held.
	const EMEFireMode Mode = Weapon->GetFireMode();
	const bool bTriggerFires = Mode == EMEFireMode::Burst ? BurstShotsLeft > 0
		: bWantsToFire && (Mode == EMEFireMode::Auto || !bFiredThisPress);
	if (!bTriggerFires)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	const double Interval = 60.0 / Data->RoundsPerMinute;
	if (Now - LastShotTime < Interval)
	{
		return;
	}
	if (IsBusy() || Character->IsSprinting() || WallBlockAmount > MaxWallBlockToFire)
	{
		// Something got in the way: the rest of the burst is lost
		BurstShotsLeft = 0;
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
		BurstShotsLeft = 0;
		return;
	}

	// Holding the trigger keeps the exact rate, whatever the frame rate
	LastShotTime = (Now - LastShotTime < Interval * 2.0) ? LastShotTime + Interval : Now;
	bFiredThisPress = true;
	BurstShotsLeft = FMath::Max(0, BurstShotsLeft - 1);
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

	const UMEFirstPersonCameraComponent* Camera = Character->GetFirstPersonCamera();
	EjectShell();
	Weapon->PlayMuzzleFlash(Camera->GetComponentLocation());

	// Hitscan from the eye, inside the spread cone around the view (spread taken before this shot's bloom)
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
		// Only something with health confirms a hit (hit marker): walls and props take the damage silently
		const UMEHealthComponent* TargetHealth = Hit.GetActor() ? Hit.GetActor()->FindComponentByClass<UMEHealthComponent>() : nullptr;
		const float HealthBefore = TargetHealth ? TargetHealth->GetHealth() : 0.f;

		UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Data->Damage, Direction, Hit, Character->GetController(), Character, nullptr);

		if (Data->Impacts)
		{
			Data->Impacts->SpawnImpact(this, Hit);
		}

		if (TargetHealth && TargetHealth->GetHealth() < HealthBefore)
		{
			OnTargetHit.Broadcast(TargetHealth->IsDead());
		}

		if (bDebugShots)
		{
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 3.f, 8, FColor::Red, false, 3.f);
			DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Hit.ImpactNormal * 10.f, FColor::Yellow, false, 3.f);
		}
	}

}

void UMEWeaponComponent::EjectShell()
{
	const FMEShellEjectSettings& Eject = GetWeaponData()->ShellEject;
	if (!Eject.ShellClass)
	{
		return;
	}

	// Recycle the oldest casing once the limit is reached: no spawn / destroy while firing long bursts
	AMEShellCasing* Casing = nullptr;
	if (Casings.Num() < Eject.MaxCasings)
	{
		FActorSpawnParameters Params;
		Params.Owner = Character;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Casing = GetWorld()->SpawnActor<AMEShellCasing>(Eject.ShellClass, Params);
		if (Casing)
		{
			Casings.Add(Casing);
		}
	}
	else
	{
		NextCasing %= Casings.Num();
		Casing = Casings[NextCasing++];
	}
	if (!Casing)
	{
		return;
	}

	// The first-person weapon is drawn scaled toward the eye: the real port is seen at the same place on screen,
	// so the casing flies from there at its real size and speed. Weapon against a wall: just in front of it.
	const UArrowComponent* Port = Weapon->GetShellEjectPort();
	const FVector Eye = Character->GetFirstPersonCamera()->GetComponentLocation();
	FVector Location = Port->GetComponentLocation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEShellEject), false, Character);
	Params.AddIgnoredActor(Weapon);
	Params.AddIgnoredActor(WorldWeapon);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, Location, ECC_WorldStatic, Params))
	{
		Location = Hit.Location + (Eye - Location).GetSafeNormal() * 2.f;
	}

	const FVector Direction = FMath::VRandCone(Port->GetForwardVector(), FMath::DegreesToRadians(Eject.DirectionVariation));
	const float Speed = Eject.Speed * (1.f + FMath::FRandRange(-Eject.SpeedVariation, Eject.SpeedVariation));
	// Thrown from a moving weapon: keeps the player's velocity, does not land on their feet when walking forward
	const FVector Velocity = Direction * Speed + Character->GetVelocity();
	const FVector AngularVelocity = FMath::VRand() * Eject.Spin;
	// Leaves lying along the barrel
	const FRotator Rotation = FRotationMatrix::MakeFromZ(Weapon->GetBarrelDirection()).Rotator();

	Casing->Launch(Location, Rotation, Velocity, AngularVelocity);
}

void UMEWeaponComponent::Reload()
{
	const UMEWeaponData* Data = GetWeaponData();
	if (!Data || IsBusy() || Weapon->GetRoomInMagazine() == 0 || GetReserveAmmo() == 0)
	{
		return;
	}

	BurstShotsLeft = 0;

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

	for (AMEShellCasing* Casing : Casings)
	{
		if (Casing)
		{
			Casing->Destroy();
		}
	}
	Casings.Reset();

	Super::EndPlay(EndPlayReason);
}

void UMEWeaponComponent::Equip(TSubclassOf<AMEWeapon> WeaponClass)
{
	FActorSpawnParameters Params;
	Params.Owner = Character;
	Params.Instigator = Character;

	// First-person weapon, in the arms
	Weapon = GetWorld()->SpawnActor<AMEWeapon>(WeaponClass, Params);
	const UMEWeaponData* Data = Weapon->GetData();
	if (!Data)
	{
		UE_LOG(LogME, Error, TEXT("%s has no weapon Data, cannot be equipped."), *GetNameSafe(WeaponClass));
		Weapon->Destroy();
		Weapon = nullptr;
		return;
	}
	Weapon->Initialize(true);

	USkeletalMeshComponent* Arms = Character->GetFirstPersonArms();
	Weapon->AttachToComponent(Arms, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Data->ArmsGripBone);
	Weapon->SetActorRelativeTransform(Data->ArmsGripOffset);

	if (UMEFirstPersonAnimInstance* ArmsAnim = Cast<UMEFirstPersonAnimInstance>(Arms->GetAnimInstance()))
	{
		ArmsAnim->SetWeaponData(Data);
	}

	// World weapon, in the world body's hands: casts the shadow, seen by the AI
	WorldWeapon = GetWorld()->SpawnActor<AMEWeapon>(WeaponClass, Params);
	WorldWeapon->Initialize(false);
	WorldWeapon->AttachToComponent(Character->GetWorldBody(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Data->GripBone);
	WorldWeapon->SetActorRelativeTransform(Data->GripOffset);

	Character->SetBodyWeaponPose(Data->BodyHoldPose);

	BusyUntil = GetWorld()->GetTimeSeconds() + PlayAction(Data->FirstDraw.Arms ? Data->FirstDraw : Data->Draw);
}

float UMEWeaponComponent::PutAwayFor(float HiddenTime)
{
	const UMEWeaponData* Data = GetWeaponData();
	if (!Data || (bHolstered && GetWorld()->GetTimeSeconds() >= BusyUntil))
	{
		// Unarmed or already put away: the hands are free
		return 0.f;
	}
	if (IsBusy())
	{
		return -1.f;
	}

	BurstShotsLeft = 0;

	const float HolsterLength = PlayAction(Data->Holster, false, true);
	const float DrawLength = Data->Draw.Arms ? Data->Draw.Arms->GetPlayLength() : 0.f;
	BusyUntil = GetWorld()->GetTimeSeconds() + HolsterLength + HiddenTime + DrawLength;
	GetWorld()->GetTimerManager().SetTimer(DrawBackTimer, this, &UMEWeaponComponent::DrawBack, FMath::Max(HolsterLength + HiddenTime, 0.01f));
	return HolsterLength;
}

void UMEWeaponComponent::DrawBack()
{
	const UMEWeaponData* Data = GetWeaponData();
	if (!Data)
	{
		return;
	}

	// Walked into a safe zone during the gesture: the weapon simply stays away
	if (HolsterRequests > 0)
	{
		bHolstered = true;
		BusyUntil = GetWorld()->GetTimeSeconds();
		return;
	}
	PlayAction(Data->Draw);
}

float UMEWeaponComponent::PlayAction(const FMEWeaponAnimation& Action, bool bInterruptible, bool bHoldLastFrame)
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
		ArmsAnim->PlayAction(Action.Arms, bHoldLastFrame);
	}
	Weapon->PlayWeaponAnimation(Action.Weapon);

	if (Action.Sound)
	{
		ActionSound = UGameplayStatics::SpawnSoundAttached(Action.Sound, Weapon->GetMesh());
	}

	// No arms clip (aiming): the weapon clip sets the length
	const UAnimSequenceBase* Timing = Action.Arms ? Action.Arms.Get() : Action.Weapon.Get();
	const float Length = Timing ? Timing->GetPlayLength() : 0.f;
	bActionInterruptible = bInterruptible;
	ActionEndTime = GetWorld()->GetTimeSeconds() + Length;
	return Length;
}
