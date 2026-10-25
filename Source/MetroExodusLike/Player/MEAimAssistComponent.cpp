#include "Player/MEAimAssistComponent.h"
#include "Player/MECharacter.h"
#include "Player/MEFirstPersonCameraComponent.h"
#include "AI/MEAICharacter.h"
#include "Combat/MEHealthComponent.h"
#include "Weapons/MEWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"

namespace
{
	/** Wide search: each use checks its own zone after. */
	constexpr float SearchAngle = 30.f;
}

UMEAimAssistComponent::UMEAimAssistComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMEAimAssistComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AMECharacter>(GetOwner());
}

void UMEAimAssistComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateCurrentTarget();
	LookScale = ComputeLookScale();
	UpdateSnap(DeltaTime);
	UpdateTracking();
}

bool UMEAimAssistComponent::CanAssist() const
{
	return bEnabled && Character && Character->IsUsingGamepad() && !Character->GetWeapons()->IsHolstered();
}

float UMEAimAssistComponent::GetZoneAngle(float Radius, float Distance, float MinAngle)
{
	return FMath::Max(FMath::RadiansToDegrees(FMath::Atan2(Radius, FMath::Max(Distance, 1.f))), MinAngle);
}

float UMEAimAssistComponent::GetAngleTo(const FVector& Point) const
{
	const FVector ToPoint = (Point - Character->GetFirstPersonCamera()->GetComponentLocation()).GetSafeNormal();
	const FVector Forward = Character->GetControlRotation().Vector();
	return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward, ToPoint), -1.f, 1.f)));
}

void UMEAimAssistComponent::UpdateCurrentTarget()
{
	CurrentTarget.Reset();
	if (!CanAssist())
	{
		return;
	}

	if (AMEAICharacter* Target = FindTarget(SearchAngle, CurrentAimPoint, CurrentAngle))
	{
		CurrentTarget = Target;
		CurrentDistance = FVector::Dist(Character->GetFirstPersonCamera()->GetComponentLocation(), CurrentAimPoint);
	}
}

float UMEAimAssistComponent::ComputeLookScale() const
{
	if (!CurrentTarget.IsValid())
	{
		return 1.f;
	}

	// The zone keeps the same size at the target: smaller on screen with range, never below the minimum
	const float ZoneAngle = GetZoneAngle(SlowdownRadius, CurrentDistance, MinSlowdownAngle);
	if (CurrentAngle >= ZoneAngle)
	{
		return 1.f;
	}
	return FMath::Lerp(SlowdownScale, 1.f, CurrentAngle / ZoneAngle);
}

void UMEAimAssistComponent::StartAimSnap()
{
	// The target of this frame may be stale by a frame: search again, only within the snap zone
	if (!CanAssist())
	{
		return;
	}

	FVector AimPoint;
	float Angle = 0.f;
	if (!FindTarget(SnapAngle, AimPoint, Angle))
	{
		return;
	}

	// Only an enemy already almost in the sights: far away, the aim must be right on him
	const FVector Eye = Character->GetFirstPersonCamera()->GetComponentLocation();
	if (Angle > GetZoneAngle(SnapRadius, FVector::Dist(Eye, AimPoint), 0.f))
	{
		return;
	}

	const FRotator ToTarget = (AimPoint - Eye).Rotation();
	SnapOffset = (ToTarget - Character->GetControlRotation()).GetNormalized() * SnapStrength;
	SnapOffset.Roll = 0.f;

	// A nudge, never a turn
	const float Size = FMath::Sqrt(FMath::Square(SnapOffset.Yaw) + FMath::Square(SnapOffset.Pitch));
	if (Size > MaxSnapRotation && Size > KINDA_SMALL_NUMBER)
	{
		SnapOffset *= MaxSnapRotation / Size;
	}

	SnapElapsed = 0.f;
	bSnapping = true;
	StopTracking();
}

void UMEAimAssistComponent::UpdateSnap(float DeltaTime)
{
	if (!bSnapping)
	{
		return;
	}

	// Ease out: most of the slide at the start, settles at the end
	const auto EaseOut = [](float T) { return 1.f - FMath::Square(1.f - T); };
	const float Before = EaseOut(FMath::Min(SnapElapsed / SnapTime, 1.f));
	SnapElapsed += DeltaTime;
	const float After = EaseOut(FMath::Min(SnapElapsed / SnapTime, 1.f));

	const FRotator Step = SnapOffset * (After - Before);
	Character->AddViewRotation(Step.Yaw, Step.Pitch);

	if (SnapElapsed >= SnapTime)
	{
		bSnapping = false;
	}
}

void UMEAimAssistComponent::UpdateTracking()
{
	// Aiming, moving (never aims alone with the stick released), no snap going on
	if (!bTracking || bSnapping || !CanAssist() || Character->GetWeapons()->GetAimAlpha() <= 0.5f || !Character->HasAimInput())
	{
		StopTracking();
		return;
	}

	const FVector Eye = Character->GetFirstPersonCamera()->GetComponentLocation();

	// Locked on one enemy: no jumping to another one passing closer
	AMEAICharacter* Target = TrackedTarget.Get();
	if (!Target)
	{
		Target = CurrentTarget.Get();
		if (!Target || CurrentAngle >= GetZoneAngle(TrackingRadius, CurrentDistance, MinTrackingAngle))
		{
			return;
		}
		TrackedTarget = Target;
		LastTrackDirection = (CurrentAimPoint - Eye).GetSafeNormal();
		return;
	}

	const FVector Point = GetAimPoint(*Target);
	const float Distance = FVector::Dist(Eye, Point);
	const float ZoneAngle = GetZoneAngle(TrackingRadius, Distance, MinTrackingAngle);
	const float Angle = GetAngleTo(Point);
	const bool bTargetIsCurrent = CurrentTarget.Get() == Target;
	if (Target->GetHealth()->IsDead() || Angle >= ZoneAngle || Distance > MaxDistance
		|| (!bTargetIsCurrent && !IsInClearSight(*Target, Point)))
	{
		StopTracking();
		return;
	}

	// Follow part of the change of direction to him (his motion and our strafe), stronger at the center
	const FVector NewDirection = (Point - Eye).GetSafeNormal();
	const FRotator Delta = (NewDirection.Rotation() - LastTrackDirection.Rotation()).GetNormalized();
	LastTrackDirection = NewDirection;

	const float Strength = TrackingStrength * (1.f - Angle / ZoneAngle);
	Character->AddViewRotation(Delta.Yaw * Strength, Delta.Pitch * Strength);
}

void UMEAimAssistComponent::StopTracking()
{
	TrackedTarget.Reset();
}

AMEAICharacter* UMEAimAssistComponent::FindTarget(float MaxAngle, FVector& OutAimPoint, float& OutAngle) const
{
	const FVector Eye = Character->GetFirstPersonCamera()->GetComponentLocation();
	const FVector Forward = Character->GetControlRotation().Vector();

	AMEAICharacter* Best = nullptr;
	float BestAngle = MaxAngle;
	for (TActorIterator<AMEAICharacter> It(GetWorld()); It; ++It)
	{
		AMEAICharacter* Enemy = *It;
		if (Enemy->GetHealth()->IsDead())
		{
			continue;
		}

		const FVector Point = GetAimPoint(*Enemy);
		const FVector ToPoint = Point - Eye;
		const float Distance = ToPoint.Size();
		if (Distance > MaxDistance || Distance < KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward, ToPoint / Distance), -1.f, 1.f)));
		if (Angle < BestAngle && IsInClearSight(*Enemy, Point))
		{
			Best = Enemy;
			BestAngle = Angle;
			OutAimPoint = Point;
		}
	}

	OutAngle = BestAngle;
	return Best;
}

FVector UMEAimAssistComponent::GetAimPoint(const AMEAICharacter& Target) const
{
	// Follows crouching: the capsule shrinks
	const UCapsuleComponent* Capsule = Target.GetCapsuleComponent();
	return Capsule->GetComponentLocation() + FVector(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight() * ChestHeight);
}

bool UMEAimAssistComponent::IsInClearSight(const AMEAICharacter& Target, const FVector& Point) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEAimAssist), false, Character);
	TArray<AActor*> Attached;
	Character->GetAttachedActors(Attached, true, true);
	Params.AddIgnoredActors(Attached);

	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Character->GetFirstPersonCamera()->GetComponentLocation(), Point, ECC_Visibility, Params))
	{
		return true;
	}

	// The target itself, or what it carries (its weapon), does not hide it
	const AActor* HitActor = Hit.GetActor();
	return HitActor == &Target || (HitActor && HitActor->GetAttachParentActor() == &Target);
}
