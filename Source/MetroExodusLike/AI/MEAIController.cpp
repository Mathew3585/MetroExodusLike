#include "AI/MEAIController.h"
#include "AI/MEAICharacter.h"
#include "World/MEPatrolPath.h"
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
}

void AMEAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	Enemy = Cast<AMEAICharacter>(InPawn);
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
