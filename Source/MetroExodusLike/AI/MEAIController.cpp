#include "AI/MEAIController.h"
#include "AI/MEAICharacter.h"
#include "World/MEPatrolPath.h"
#include "Player/MECharacter.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

AMEAIController::AMEAIController()
{
	StateTree = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*Perception);

	// Everything is sensed (no teams yet): only the player is kept, in HandlePerception
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->DetectionByAffiliation = SightConfig->DetectionByAffiliation;
	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*HearingConfig);
	Perception->SetDominantSense(UAISense_Sight::StaticClass());
}

void AMEAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	Enemy = Cast<AMEAICharacter>(InPawn);

	// Ranges edited in the Blueprint
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = FMath::Max(LoseSightRadius, SightRadius);
	SightConfig->PeripheralVisionAngleDegrees = SightHalfAngle;
	HearingConfig->HearingRange = HearingRange;
	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*HearingConfig);
	Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AMEAIController::HandlePerception);
}

void AMEAIController::HandlePerception(AActor* Actor, FAIStimulus Stimulus)
{
	// Only the player matters: other enemies, their shots and steps are ignored for now
	APawn* Player = Cast<APawn>(Actor);
	if (!Player || !Player->IsPlayerControlled())
	{
		return;
	}

	const FAISenseID Sense = Stimulus.Type;
	if (Sense == UAISense::GetSenseID<UAISense_Sight>())
	{
		SeenPlayer = Stimulus.WasSuccessfullySensed() ? Player : nullptr;
	}
	else if (Sense == UAISense::GetSenseID<UAISense_Hearing>() && Stimulus.WasSuccessfullySensed())
	{
		LastNoiseTime = GetWorld()->GetTimeSeconds();
		LastNoiseLocation = Stimulus.StimulusLocation;
		LastNoiseTag = Stimulus.Tag;
	}
}

void AMEAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Enemy)
	{
		UpdateDetection(DeltaSeconds);
	}
}

void AMEAIController::UpdateDetection(float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const APawn* Player = SeenPlayer.Get();
	if (!Player)
	{
		// Out of sight: it still remembers for a moment, then forgets slowly
		if (Now - LastSeenTime > DetectionHoldTime)
		{
			Detection = FMath::Max(0.f, Detection - DetectionDecay * DeltaSeconds);
		}
		return;
	}

	FVector Eyes;
	FRotator View;
	Enemy->GetActorEyesViewPoint(Eyes, View);
	const FVector ToPlayer = Player->GetActorLocation() - Eyes;
	const float Distance = ToPlayer.Size();

	// Far away and on the side of the cone: slower
	const float DistanceScale = FMath::GetMappedRangeValueClamped(FVector2f(NearDistance, SightRadius), FVector2f(1.f, FarScale), Distance);
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(View.Vector(), ToPlayer.GetSafeNormal())));
	const float AngleScale = FMath::GetMappedRangeValueClamped(FVector2f(0.f, SightHalfAngle), FVector2f(1.f, PeripheralScale), Angle);

	// What the player does: crouched and still is hard to notice, sprinting is not
	float PostureScale = 1.f;
	if (const AMECharacter* PlayerCharacter = Cast<AMECharacter>(Player))
	{
		const bool bMoving = PlayerCharacter->GetVelocity().Size2D() > 10.f;
		PostureScale = (PlayerCharacter->bIsCrouched ? CrouchScale : 1.f)
			* (!bMoving ? StillScale : PlayerCharacter->IsSprinting() ? SprintScale : 1.f);
	}

	Detection = FMath::Min(1.f, Detection + DeltaSeconds / FullDetectionTime * DistanceScale * AngleScale * PostureScale);
	LastSeenTime = Now;
	LastSeenLocation = Player->GetActorLocation();
}

bool AMEAIController::GetLastNoise(FVector& OutLocation, FName& OutTag, float& OutAge) const
{
	if (LastNoiseTime < 0.0)
	{
		return false;
	}
	OutLocation = LastNoiseLocation;
	OutTag = LastNoiseTag;
	OutAge = GetWorld()->GetTimeSeconds() - LastNoiseTime;
	return true;
}

void AMEAIController::OnUnPossess()
{
	StopPatrol();
	Enemy = nullptr;
	Super::OnUnPossess();
}

void AMEAIController::StartPatrol()
{
	if (!Enemy)
	{
		return;
	}

	bPatrolling = true;
	Enemy->GetCharacterMovement()->MaxWalkSpeed = Enemy->GetPatrolSpeed();

	const AMEPatrolPath* Path = Enemy->GetPatrolPath();
	if (!Path || Path->GetNumPoints() == 0)
	{
		return;
	}
	PatrolIndex = FMath::Clamp(Enemy->GetPatrolStartIndex(), 0, Path->GetNumPoints() - 1);
	bPatrolForward = true;
	MoveToPatrolPoint();
}

void AMEAIController::StopPatrol()
{
	if (!bPatrolling)
	{
		return;
	}
	bPatrolling = false;
	bWaiting = false;
	GetWorldTimerManager().ClearTimer(PatrolTimer);
	FaceMovement();
	StopMovement();
}

void AMEAIController::MoveToPatrolPoint()
{
	bWaiting = false;
	FaceMovement();

	const EPathFollowingRequestResult::Type Request = MoveToLocation(Enemy->GetPatrolPath()->GetPointLocation(PatrolIndex), PatrolAcceptanceRadius);
	if (Request == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		ArriveAtPatrolPoint();
	}
	else if (Request == EPathFollowingRequestResult::Failed)
	{
		GetWorldTimerManager().SetTimer(PatrolTimer, this, &AMEAIController::NextPatrolPoint, PatrolRetryDelay);
	}
}

void AMEAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (!bPatrolling || bWaiting)
	{
		return;
	}
	if (Result.IsSuccess())
	{
		ArriveAtPatrolPoint();
	}
	else if (!Result.IsInterrupted())
	{
		// Blocked or off the navmesh: try the next point a bit later
		GetWorldTimerManager().SetTimer(PatrolTimer, this, &AMEAIController::NextPatrolPoint, PatrolRetryDelay);
	}
}

void AMEAIController::ArriveAtPatrolPoint()
{
	const AMEPatrolPath* Path = Enemy->GetPatrolPath();
	const FMEPatrolPoint Point = Path->GetPoint(PatrolIndex);
	bWaiting = true;

	FRotator Facing;
	if (Point.WaitTime != 0.f && Path->GetPointFacing(PatrolIndex, Facing))
	{
		FaceLocation(Enemy->GetActorLocation() + Facing.Vector() * 1000.f);
	}

	if (Point.WaitTime < 0.f || Path->GetNumPoints() == 1)
	{
		// A post: stays here
		return;
	}
	if (Point.WaitTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(PatrolTimer, this, &AMEAIController::NextPatrolPoint, Point.WaitTime);
	}
	else
	{
		NextPatrolPoint();
	}
}

void AMEAIController::NextPatrolPoint()
{
	if (!bPatrolling || !Enemy || !Enemy->GetPatrolPath())
	{
		return;
	}
	PatrolIndex = Enemy->GetPatrolPath()->GetNextIndex(PatrolIndex, bPatrolForward);
	MoveToPatrolPoint();
}

void AMEAIController::FaceMovement()
{
	ClearFocus(EAIFocusPriority::Gameplay);
	if (Enemy)
	{
		Enemy->GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

void AMEAIController::FaceLocation(const FVector& Location)
{
	// Orienting to movement keeps the last heading when standing: the focus would never turn it
	SetFocalPoint(Location, EAIFocusPriority::Gameplay);
	Enemy->GetCharacterMovement()->bOrientRotationToMovement = false;
}

FString AMEAIController::GetActivityDescription() const
{
	if (!bPatrolling)
	{
		return TEXT("idle");
	}
	if (!Enemy || !Enemy->GetPatrolPath())
	{
		return TEXT("standing (no path)");
	}
	const int32 Num = Enemy->GetPatrolPath()->GetNumPoints();
	return FString::Printf(TEXT("%s point %d / %d"), bWaiting ? TEXT("waiting at") : TEXT("walking to"), PatrolIndex + 1, Num);
}
