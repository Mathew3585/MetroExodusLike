#include "Player/MEFirstPersonCameraComponent.h"
#include "Player/MECharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UMEFirstPersonCameraComponent::UMEFirstPersonCameraComponent()
{
	// After physics so the head bone of the current frame's pose is available
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	bUsePawnControlRotation = true;

	// First person rendering: arms and weapon get their own FOV and are pulled toward the eye, so they never clip into walls
	bEnableFirstPersonFieldOfView = true;
	FirstPersonFieldOfView = 70.f;
	bEnableFirstPersonScale = true;
	FirstPersonScale = 0.6f;

	SprintBob.StepFrequency = 2.6f;
	SprintBob.VerticalAmplitude = 1.2f;
	SprintBob.HorizontalAmplitude = 0.8f;
	SprintBob.RollAmplitude = 0.5f;
}

void UMEFirstPersonCameraComponent::BeginPlay()
{
	Super::BeginPlay();

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

	if (!Character || !Character->GetMesh()->GetSkeletalMeshAsset())
	{
		return;
	}

	UpdatePlacement(DeltaTime);

	FVector OffsetLocation = FVector::ZeroVector;
	FRotator OffsetRotation = FRotator::ZeroRotator;
	UpdateHeadBob(DeltaTime, OffsetLocation, OffsetRotation);
	UpdateBreathing(DeltaTime, OffsetLocation, OffsetRotation);
	UpdateImpacts(DeltaTime, OffsetLocation, OffsetRotation);
	UpdateStrafeTilt(DeltaTime, OffsetRotation);
	UpdateHeavyLanding(DeltaTime, OffsetRotation);

	ProceduralOffset = FTransform(OffsetRotation, OffsetLocation);
	ClearAdditiveOffset();
	AddAdditiveOffset(ProceduralOffset, 0.f);
}

void UMEFirstPersonCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	// Applies the pawn control rotation to this component, then the additive offset to the view
	Super::GetCameraView(DeltaTime, DesiredView);

	if (ViewModelPivot)
	{
		ViewModelPivot->SetRelativeTransform(ProceduralOffset);
	}
}

void UMEFirstPersonCameraComponent::UpdatePlacement(float DeltaTime)
{
	const FVector ActorLocation = Character->GetActorLocation();
	const FRotator ViewYaw(0.f, Character->GetControlRotation().Yaw, 0.f);

	// Everything below is in view-yaw space, relative to the actor, so smoothing never lags behind movement
	const FVector HeadLocal = ViewYaw.UnrotateVector(Character->GetMesh()->GetSocketLocation(HeadBoneName) - ActorLocation);
	const FVector StableLocal(0.f, 0.f, Character->BaseEyeHeight);

	// In the air the camera mostly ignores the head, so the fall animation does not shake the view
	const float TargetAirScale = Character->GetCharacterMovement()->IsFalling() ? InAirHeadFollow : 1.f;
	AirFollowScale = FMath::FInterpTo(AirFollowScale, TargetAirScale, DeltaTime, InAirFollowBlendSpeed);

	FVector TargetLocal;
	TargetLocal.X = FMath::Lerp(StableLocal.X, HeadLocal.X, HeadFollowHorizontal * AirFollowScale);
	TargetLocal.Y = FMath::Lerp(StableLocal.Y, HeadLocal.Y, HeadFollowHorizontal * AirFollowScale);
	TargetLocal.Z = FMath::Lerp(StableLocal.Z, HeadLocal.Z, HeadFollowVertical * AirFollowScale);
	TargetLocal += EyeOffset;

	if (PlacementSmoothingSpeed > 0.f && bHasPlacement)
	{
		SmoothedLocalOffset = FMath::VInterpTo(SmoothedLocalOffset, TargetLocal, DeltaTime, PlacementSmoothingSpeed);
	}
	else
	{
		SmoothedLocalOffset = TargetLocal;
	}
	bHasPlacement = true;

	SetWorldLocation(ActorLocation + ViewYaw.RotateVector(SmoothedLocalOffset));
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

	OutLocation.Y += Sway * Horizontal * MoveWeight;
	OutLocation.Z += Bob * Vertical * MoveWeight;
	OutRotation.Roll += Sway * Roll * MoveWeight;
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

void UMEFirstPersonCameraComponent::PlayHeavyLanding(float Duration)
{
	HeavyLandingElapsed = 0.f;
	HeavyLandingDuration = FMath::Max(Duration, HeavyLandingDropTime + 0.1f);
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
