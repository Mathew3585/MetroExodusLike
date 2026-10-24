#include "Player/MEFirstPersonCameraComponent.h"
#include "Player/MECharacter.h"
#include "Weapons/MEWeaponComponent.h"
#include "Weapons/MEWeaponData.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/**
	 * Critically damped spring toward Target: no overshoot, and unlike an exponential interp the speed
	 * ramps up and down, so a target that jumps (animation switching on start / stop) never snaps the view.
	 */
	FVector::FReal SmoothDamp(FVector::FReal Current, FVector::FReal& Velocity, FVector::FReal Target, float SmoothTime, float DeltaTime)
	{
		if (SmoothTime <= 0.f)
		{
			Velocity = 0.;
			return Target;
		}

		const FVector::FReal Omega = 2. / SmoothTime;
		const FVector::FReal X = Omega * DeltaTime;
		const FVector::FReal Decay = 1. / (1. + X + 0.48 * X * X + 0.235 * X * X * X);
		const FVector::FReal Change = Current - Target;
		const FVector::FReal Temp = (Velocity + Omega * Change) * DeltaTime;
		Velocity = (Velocity - Omega * Temp) * Decay;
		return Target + (Change + Temp) * Decay;
	}
}

UMEFirstPersonCameraComponent::UMEFirstPersonCameraComponent()
{
	// After physics so the head bone of the current frame's pose is available
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	// The view follows the head bone of the arms, the pivot carries the control rotation
	bUsePawnControlRotation = false;

	// Same framing as the arm animations were authored with
	FieldOfView = 90.f;

	// First person rendering: arms and weapon are pulled toward the eye so they never clip into walls.
	// Scaling around the eye does not change the framing. A separate first-person FOV stays off for the same reason.
	// Small enough that the muzzle stays closer than the capsule lets the eye get to a wall (near clip is 1 cm for the stock).
	bEnableFirstPersonFieldOfView = false;
	bEnableFirstPersonScale = true;
	FirstPersonScale = 0.25f;

	SprintBob.StepFrequency = 2.6f;
	SprintBob.VerticalAmplitude = 1.2f;
	SprintBob.HorizontalAmplitude = 0.8f;
	SprintBob.RollAmplitude = 0.5f;
}

void UMEFirstPersonCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	HipFieldOfView = FieldOfView;

	Character = Cast<AMECharacter>(GetOwner());
	if (Character)
	{
		// The head bone must be animated before we read it
		AddTickPrerequisiteComponent(Character->GetMesh());
	}
}

void UMEFirstPersonCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Character || !ViewPivot || !Character->GetMesh()->GetSkeletalMeshAsset())
	{
		return;
	}

	FVector PivotLocation = UpdatePlacement(DeltaTime);
	const float Death = UpdateDeath(DeltaTime, PivotLocation);
	UpdateAiming();

	FVector OffsetLocation = FVector::ZeroVector;
	FRotator OffsetRotation = FRotator::ZeroRotator;
	UpdateHeadBob(DeltaTime, OffsetLocation, OffsetRotation);
	UpdateBreathing(DeltaTime, OffsetLocation, OffsetRotation);
	UpdateImpacts(DeltaTime, OffsetLocation, OffsetRotation);
	UpdateStrafeTilt(DeltaTime, OffsetRotation);
	UpdateHeavyLanding(DeltaTime, OffsetRotation);

	// Arms and camera follow: the view turns around the head, the procedural motion is in view space.
	// Dead, the input is off: the view goes from where it looked to lying on the ground
	FRotator ViewRotation = Character->GetControlRotation();
	if (Death > 0.f)
	{
		const FRotator Lying(DeathPitch, DeathStartRotation.Yaw, DeathRoll);
		ViewRotation = FQuat::Slerp(DeathStartRotation.Quaternion(), Lying.Quaternion(), Death).Rotator();
	}
	const FTransform View(ViewRotation, PivotLocation);
	ViewPivot->SetWorldTransform(FTransform(OffsetRotation, OffsetLocation) * View);
}

FVector UMEFirstPersonCameraComponent::UpdatePlacement(float DeltaTime)
{
	const FRotator ViewYaw(0.f, Character->GetControlRotation().Yaw, 0.f);

	// Everything below is in view-yaw space, relative to the feet of the body, so smoothing never lags behind movement.
	// Not relative to the actor: crouching moves it by the capsule height change in one frame (the mesh is moved back).
	const FVector Feet = Character->GetMesh()->GetComponentLocation();
	const FVector HeadLocal = ViewYaw.UnrotateVector(Character->GetMesh()->GetSocketLocation(HeadBoneName) - Feet);
	const FVector StableLocal(0.f, 0.f, Character->GetActorLocation().Z + Character->BaseEyeHeight - Feet.Z);

	// In the air the camera mostly ignores the head, so the fall animation does not shake the view
	const float TargetAirScale = Character->GetCharacterMovement()->IsFalling() ? InAirHeadFollow : 1.f;
	AirFollowScale = FMath::FInterpTo(AirFollowScale, TargetAirScale, DeltaTime, InAirFollowBlendSpeed);

	FVector TargetLocal;
	TargetLocal.X = FMath::Lerp(StableLocal.X, HeadLocal.X, HeadFollowHorizontal * AirFollowScale);
	TargetLocal.Y = FMath::Lerp(StableLocal.Y, HeadLocal.Y, HeadFollowHorizontal * AirFollowScale);
	TargetLocal.Z = FMath::Lerp(StableLocal.Z, HeadLocal.Z, HeadFollowVertical * AirFollowScale);
	TargetLocal += EyeOffset;

	if (bHasPlacement)
	{
		SmoothedLocalOffset.X = SmoothDamp(SmoothedLocalOffset.X, SmoothedLocalVelocity.X, TargetLocal.X, HorizontalSmoothTime, DeltaTime);
		SmoothedLocalOffset.Y = SmoothDamp(SmoothedLocalOffset.Y, SmoothedLocalVelocity.Y, TargetLocal.Y, HorizontalSmoothTime, DeltaTime);
		SmoothedLocalOffset.Z = SmoothDamp(SmoothedLocalOffset.Z, SmoothedLocalVelocity.Z, TargetLocal.Z, VerticalSmoothTime, DeltaTime);
	}
	else
	{
		SmoothedLocalOffset = TargetLocal;
		SmoothedLocalVelocity = FVector::ZeroVector;
	}
	bHasPlacement = true;

	return Feet + ViewYaw.RotateVector(SmoothedLocalOffset);
}

void UMEFirstPersonCameraComponent::UpdateHeadBob(float DeltaTime, FVector& OutLocation, FRotator& OutRotation)
{
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();

	const float TargetMoveWeight = Movement->IsMovingOnGround()
		? FMath::Clamp(Movement->Velocity.Size2D() / FullBobSpeed, 0.f, 1.f)
		: 0.f;
	MoveWeight = FMath::FInterpTo(MoveWeight, TargetMoveWeight, DeltaTime, BobBlendSpeed);
	SprintWeight = FMath::FInterpTo(SprintWeight, Character->IsSprinting() ? 1.f : 0.f, DeltaTime, BobBlendSpeed);

	if (MoveWeight < KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float Frequency = FMath::Lerp(WalkBob.StepFrequency, SprintBob.StepFrequency, SprintWeight);
	const float Vertical = FMath::Lerp(WalkBob.VerticalAmplitude, SprintBob.VerticalAmplitude, SprintWeight);
	const float Horizontal = FMath::Lerp(WalkBob.HorizontalAmplitude, SprintBob.HorizontalAmplitude, SprintWeight);
	const float Roll = FMath::Lerp(WalkBob.RollAmplitude, SprintBob.RollAmplitude, SprintWeight);

	// One step = PI, so the sway (sin) does one cycle every two steps and the bob (sin x2) one per step
	StepPhase = FMath::Fmod(StepPhase + DeltaTime * Frequency * UE_PI, UE_TWO_PI);
	const float Sway = FMath::Sin(StepPhase);
	const float Bob = FMath::Sin(StepPhase * 2.f);

	const float AimScale = FMath::Lerp(1.f, AimBobScale, Character->GetWeapons()->GetAimAlpha());
	OutLocation.Y += Sway * Horizontal * MoveWeight * AimScale;
	OutLocation.Z += Bob * Vertical * MoveWeight * AimScale;
	OutRotation.Roll += Sway * Roll * MoveWeight * AimScale;
}

void UMEFirstPersonCameraComponent::UpdateBreathing(float DeltaTime, FVector& OutLocation, FRotator& OutRotation)
{
	const float ExertionDelta = Character->IsSprinting()
		? Breathing.ExertionGainPerSecond
		: -Breathing.ExertionRecoveryPerSecond;
	Exertion = FMath::Clamp(Exertion + ExertionDelta * DeltaTime, 0.f, 1.f);

	BreathWeight = FMath::FInterpTo(BreathWeight, bHoldingBreath ? 0.f : 1.f, DeltaTime, Breathing.HoldBreathBlendSpeed);

	const float Rate = FMath::Lerp(Breathing.RestedRate, Breathing.ExhaustedRate, Exertion);
	BreathPhase = FMath::Fmod(BreathPhase + DeltaTime * Rate * UE_TWO_PI, UE_TWO_PI);
	const float Breath = FMath::Sin(BreathPhase) * BreathWeight;

	OutLocation.Z += Breath * FMath::Lerp(Breathing.RestedAmplitude, Breathing.ExhaustedAmplitude, Exertion);
	OutRotation.Pitch += Breath * FMath::Lerp(Breathing.RestedPitch, Breathing.ExhaustedPitch, Exertion);
}

void UMEFirstPersonCameraComponent::UpdateImpacts(float DeltaTime, FVector& OutLocation, FRotator& OutRotation)
{
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const bool bIsFalling = Movement->IsFalling();

	if (bIsFalling)
	{
		if (!bWasFalling && Movement->Velocity.Z > 0.f)
		{
			SpringVelocity += Impacts.JumpKick;
		}
		LastFallSpeed = FMath::Max(0.f, -Movement->Velocity.Z);
	}
	else if (bWasFalling && LastFallSpeed > Impacts.MinLandingSpeed)
	{
		const float LandingSpeed = FMath::Min(LastFallSpeed, Impacts.MaxLandingSpeed);
		SpringVelocity -= LandingSpeed * Impacts.LandingKickScale;
	}
	bWasFalling = bIsFalling;

	// Damped spring, sub-stepped to stay stable on long frames
	constexpr float MaxStep = 1.f / 120.f;
	float Remaining = DeltaTime;
	while (Remaining > 0.f)
	{
		const float Step = FMath::Min(Remaining, MaxStep);
		const float Acceleration = -Impacts.Stiffness * SpringOffset - Impacts.Damping * SpringVelocity;
		SpringVelocity += Acceleration * Step;
		SpringOffset += SpringVelocity * Step;
		Remaining -= Step;
	}

	OutLocation.Z += SpringOffset;
	OutRotation.Pitch += SpringOffset * Impacts.PitchPerCm;
}

void UMEFirstPersonCameraComponent::UpdateStrafeTilt(float DeltaTime, FRotator& OutRotation)
{
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const FRotator ViewYaw(0.f, Character->GetControlRotation().Yaw, 0.f);

	// Sideways speed relative to the view, -1 (left) .. 1 (right) at full bob speed
	const float Sideways = Movement->IsMovingOnGround()
		? FMath::Clamp(ViewYaw.UnrotateVector(Movement->Velocity).Y / FullBobSpeed, -1.f, 1.f)
		: 0.f;

	StrafeRoll = FMath::FInterpTo(StrafeRoll, Sideways * StrafeRollAngle, DeltaTime, StrafeRollSpeed);
	OutRotation.Roll += StrafeRoll;
}

void UMEFirstPersonCameraComponent::UpdateAiming()
{
	const UMEWeaponComponent* Weapons = Character->GetWeapons();
	const UMEWeaponData* WeaponData = Weapons->GetWeaponData();
	const float AimAlpha = Weapons->GetAimAlpha();

	SetFieldOfView(FMath::Lerp(HipFieldOfView, WeaponData ? WeaponData->AimFieldOfView : HipFieldOfView, AimAlpha));

	// Aiming: the breath is held so the sights stay still
	SetHoldingBreath(AimAlpha > 0.5f);
}

void UMEFirstPersonCameraComponent::PlayHeavyLanding(float Duration)
{
	HeavyLandingElapsed = 0.f;
	HeavyLandingDuration = FMath::Max(Duration, HeavyLandingDropTime + 0.1f);
}

FVector UMEFirstPersonCameraComponent::GetFirstPersonVisualLocation(const FVector& Location) const
{
	// The first-person scale shrinks primitives toward the eye
	const FVector Eye = GetComponentLocation();
	return Eye + (Location - Eye) * GetFirstPersonVisualScale();
}

void UMEFirstPersonCameraComponent::PlayDeath()
{
	DeathElapsed = 0.f;
	DeathStartRotation = Character ? Character->GetControlRotation() : GetComponentRotation();
	HeavyLandingDuration = 0.f;
}

float UMEFirstPersonCameraComponent::UpdateDeath(float DeltaTime, FVector& InOutPivotLocation)
{
	if (DeathElapsed < 0.f)
	{
		return 0.f;
	}
	DeathElapsed += DeltaTime;

	// Knees give way first, then the body drops: accelerating fall
	const float Fall = FMath::Square(FMath::Clamp(DeathElapsed / DeathFallTime, 0.f, 1.f));
	const float Ground = Character->GetMesh()->GetComponentLocation().Z + DeathEyeHeight;
	InOutPivotLocation.Z = FMath::Lerp(InOutPivotLocation.Z, FMath::Min(Ground, InOutPivotLocation.Z), Fall);
	return Fall;
}

void UMEFirstPersonCameraComponent::UpdateHeavyLanding(float DeltaTime, FRotator& OutRotation)
{
	if (HeavyLandingElapsed >= HeavyLandingDuration)
	{
		return;
	}
	HeavyLandingElapsed += DeltaTime;

	// Fast drop on impact, hold while absorbing, then ease back up
	float Weight;
	if (HeavyLandingElapsed < HeavyLandingDropTime)
	{
		const float Drop = HeavyLandingElapsed / HeavyLandingDropTime;
		Weight = 1.f - FMath::Square(1.f - Drop);
	}
	else
	{
		const float Release = (HeavyLandingElapsed - HeavyLandingDropTime) / (HeavyLandingDuration - HeavyLandingDropTime);
		Weight = 1.f - FMath::SmoothStep(0.35f, 1.f, Release);
	}

	OutRotation.Pitch -= HeavyLandingLookDown * Weight;
}
