#include "AI/MEAIController.h"
#include "AI/MEAICharacter.h"
#include "AI/MENoise.h"
#include "World/MEPatrolPath.h"
#include "Interaction/MEDoor.h"
#include "Player/MECharacter.h"
#include "Combat/MEHealthComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"

namespace
{
	/** A moving target is followed only once it has moved this far: no re-path every frame. */
	constexpr float RepathDistance = 150.f;
	/** Looking around: turns this far each side, this often. */
	constexpr float LookAngle = 60.f;
	constexpr float LookTurnInterval = 1.3f;
	constexpr float DoorCheckInterval = 0.5f;
}

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

	if (Enemy)
	{
		Enemy->GetHealth()->OnHealthChanged.AddUObject(this, &AMEAIController::HandleHealthChanged);
	}
}

void AMEAIController::OnUnPossess()
{
	EndActivity();
	if (Enemy)
	{
		Enemy->GetHealth()->OnHealthChanged.RemoveAll(this);
	}
	Enemy = nullptr;
	Super::OnUnPossess();
}

// --- Senses ---

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
		bNewNoise = true;
	}
}

void AMEAIController::HandleHealthChanged(float Health, float Delta, AController* DamageInstigator)
{
	if (Delta >= 0.f)
	{
		return;
	}
	// Shot: it does not know who, but it knows where from
	const APawn* Shooter = DamageInstigator ? DamageInstigator->GetPawn() : nullptr;
	HurtFrom = Shooter ? Shooter->GetActorLocation() : Enemy->GetActorLocation();
	bHurt = true;
}

void AMEAIController::NotifyAllyDown(const FVector& Location)
{
	AllyDownLocation = Location;
	bSawAllyDown = true;
}

bool AMEAIController::CanSeeLocation(const FVector& Location, const AActor* Target) const
{
	if (!Enemy)
	{
		return false;
	}

	FVector Eyes;
	FRotator View;
	Enemy->GetActorEyesViewPoint(Eyes, View);
	const FVector ToLocation = Location - Eyes;
	if (ToLocation.SizeSquared() > FMath::Square(SightRadius)
		|| FVector::DotProduct(View.Vector(), ToLocation.GetSafeNormal()) < FMath::Cos(FMath::DegreesToRadians(SightHalfAngle)))
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEAISight), false, Enemy);
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByChannel(Hit, Eyes, Location, ECC_Visibility, Params) || (Target && Hit.GetActor() == Target);
}

void AMEAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Enemy)
	{
		return;
	}

	UpdateDetection(DeltaSeconds);

	DoorCheckTimer -= DeltaSeconds;
	if (DoorCheckTimer <= 0.f)
	{
		DoorCheckTimer = DoorCheckInterval;
		CheckDoors();
	}

	UpdateAlert();

	if (!bActivityRunning)
	{
		return;
	}
	if (bLookingAround)
	{
		UpdateLookAround();
	}
	else if (bMoving && (Activity == EMEAlertState::Suspicious || Activity == EMEAlertState::Alerted)
		&& FVector::Dist(AlertTarget, MoveGoal) > RepathDistance)
	{
		// What it goes to moved (the player it follows, a new noise): follow
		MoveToTarget(Activity == EMEAlertState::Alerted ? 150.f : 100.f);
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
		const bool bMovingPlayer = PlayerCharacter->GetVelocity().Size2D() > 10.f;
		PostureScale = (PlayerCharacter->bIsCrouched ? CrouchScale : 1.f)
			* (!bMovingPlayer ? StillScale : PlayerCharacter->IsSprinting() ? SprintScale : 1.f);
	}

	// On edge (after a scare, or already looking for someone): quicker to notice
	const float AlertScale = (AlertState != EMEAlertState::Calm || Now < WaryUntil) ? WaryDetectionScale : 1.f;

	Detection = FMath::Min(1.f, Detection + DeltaSeconds / FullDetectionTime * DistanceScale * AngleScale * PostureScale * AlertScale);
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

void AMEAIController::CheckDoors()
{
	for (TActorIterator<AMEDoor> It(GetWorld()); It; ++It)
	{
		AMEDoor* Door = *It;
		if (!CanSeeLocation(Door->GetActorLocation() + FVector(0.f, 0.f, 100.f), Door))
		{
			continue;
		}

		// A door is only known once seen: one opened before that is not a change
		bool* WasOpen = KnownDoors.Find(Door);
		if (!WasOpen)
		{
			KnownDoors.Add(Door, Door->IsOpen());
		}
		else if (*WasOpen != Door->IsOpen())
		{
			*WasOpen = Door->IsOpen();
			if (AlertState == EMEAlertState::Calm)
			{
				BecomeSuspicious(Door->GetActorLocation());
			}
		}
	}
}

// --- Alert ---

void AMEAIController::UpdateAlert()
{
	// Strongest events first: being shot, a body, a gunshot
	if (bHurt)
	{
		bHurt = false;
		RaiseAlert(HurtFrom);
	}
	if (bSawAllyDown)
	{
		bSawAllyDown = false;
		RaiseAlert(AllyDownLocation);
	}
	if (bNewNoise)
	{
		bNewNoise = false;
		if (LastNoiseTag == MENoise::Gunshot || AlertState == EMEAlertState::Alerted || AlertState == EMEAlertState::Searching)
		{
			// Already looking for someone, any noise is a lead
			RaiseAlert(LastNoiseLocation);
		}
		else
		{
			BecomeSuspicious(LastNoiseLocation);
		}
	}

	if (SeenPlayer.IsValid())
	{
		if (Detection >= 1.f || (AlertState == EMEAlertState::Searching && Detection >= SuspicionThreshold))
		{
			RaiseAlert(LastSeenLocation);
		}
		else if (Detection >= SuspicionThreshold && (AlertState == EMEAlertState::Calm || AlertState == EMEAlertState::Suspicious))
		{
			BecomeSuspicious(LastSeenLocation);
		}
	}
}

void AMEAIController::BecomeSuspicious(const FVector& Location)
{
	AlertTarget = Location;
	if (AlertState != EMEAlertState::Suspicious)
	{
		SetAlertState(EMEAlertState::Suspicious);
	}
	else if (bActivityRunning && Activity == EMEAlertState::Suspicious && bLookingAround)
	{
		// Something new while looking around: go check that too
		bLookingAround = false;
		GoCheck();
	}
}

void AMEAIController::RaiseAlert(const FVector& Location)
{
	AlertTarget = Location;
	if (AlertState != EMEAlertState::Alerted)
	{
		SetAlertState(EMEAlertState::Alerted);
	}
	else if (bActivityRunning && Activity == EMEAlertState::Alerted && !bMoving)
	{
		// Arrived (or looking around): a new lead, hurry there
		bLookingAround = false;
		StartHurrying();
	}
}

void AMEAIController::CalmDown()
{
	WaryUntil = GetWorld()->GetTimeSeconds() + WaryTime;
	SetAlertState(EMEAlertState::Calm);
}

void AMEAIController::SetAlertState(EMEAlertState NewState)
{
	// The StateTree follows on its next tick and switches the activity
	AlertState = NewState;
}

// --- Activities ---

void AMEAIController::BeginActivity(EMEAlertState NewActivity)
{
	EndActivity();
	if (!Enemy)
	{
		return;
	}

	Activity = NewActivity;
	bActivityRunning = true;
	switch (Activity)
	{
	case EMEAlertState::Calm:
		StartPatrol();
		break;
	case EMEAlertState::Suspicious:
		StartInvestigating();
		break;
	case EMEAlertState::Searching:
		StartSearching();
		break;
	default:
		// Combat comes in 5.4: until then it keeps hurrying toward the player
		StartHurrying();
		break;
	}
}

void AMEAIController::EndActivity()
{
	if (!bActivityRunning)
	{
		return;
	}
	// Flags first: stopping the move reports a completed move
	bActivityRunning = false;
	bMoving = false;
	bLookingAround = false;
	GetWorldTimerManager().ClearTimer(ActivityTimer);
	StopPatrol();
	StopMovement();
	FaceMovement();
}

void AMEAIController::MoveToTarget(float AcceptanceRadius)
{
	MoveGoal = AlertTarget;
	bMoving = true;
	const EPathFollowingRequestResult::Type Request = MoveToLocation(AlertTarget, AcceptanceRadius);
	if (Request != EPathFollowingRequestResult::RequestSuccessful)
	{
		// Already there, or no way to get there: act as if arrived
		bMoving = false;
		ArriveAtTarget();
	}
}

void AMEAIController::ArriveAtTarget()
{
	switch (Activity)
	{
	case EMEAlertState::Suspicious:
		LookAround(LookAroundTime);
		break;
	case EMEAlertState::Searching:
		LookAround(SearchLookTime);
		break;
	default:
		if (SeenPlayer.IsValid())
		{
			// Still in sight: stays and watches (the fight comes in 5.4)
			FaceLocation(LastSeenLocation);
		}
		else
		{
			SetAlertState(EMEAlertState::Searching);
		}
		break;
	}
}

void AMEAIController::StartInvestigating()
{
	// Stops, stares a moment, then goes to check
	Enemy->GetCharacterMovement()->MaxWalkSpeed = InvestigateSpeed;
	StopMovement();
	FaceLocation(AlertTarget);
	GetWorldTimerManager().SetTimer(ActivityTimer, this, &AMEAIController::GoCheck, FMath::Max(ReactionTime, 0.01f));
}

void AMEAIController::GoCheck()
{
	FaceMovement();
	MoveToTarget(100.f);
}

void AMEAIController::StartHurrying()
{
	Enemy->GetCharacterMovement()->MaxWalkSpeed = AlertSpeed;
	FaceMovement();
	MoveToTarget(150.f);
}

void AMEAIController::StartSearching()
{
	Enemy->GetCharacterMovement()->MaxWalkSpeed = SearchSpeed;
	SearchCenter = AlertTarget;
	SearchPointsLeft = SearchPoints;
	NextSearchPoint();
}

void AMEAIController::NextSearchPoint()
{
	if (SearchPointsLeft <= 0)
	{
		// Nothing found: back to calm, but wary
		CalmDown();
		return;
	}
	--SearchPointsLeft;

	const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation Point;
	if (!Navigation || !Navigation->GetRandomReachablePointInRadius(SearchCenter, SearchRadius, Point))
	{
		CalmDown();
		return;
	}

	FaceMovement();
	MoveGoal = Point.Location;
	bMoving = true;
	if (MoveToLocation(Point.Location, 80.f) != EPathFollowingRequestResult::RequestSuccessful)
	{
		bMoving = false;
		ArriveAtTarget();
	}
}

void AMEAIController::LookAround(float Duration)
{
	StopMovement();
	bMoving = false;
	bLookingAround = true;
	const double Now = GetWorld()->GetTimeSeconds();
	LookAroundEnd = Now + Duration;
	NextLookTurn = Now;
	LookTurns = 0;
	LookBase = FRotator(0.f, Enemy->GetActorRotation().Yaw, 0.f);
}

void AMEAIController::UpdateLookAround()
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now >= LookAroundEnd)
	{
		FinishLookAround();
		return;
	}
	if (Now >= NextLookTurn)
	{
		// Left, right, left...
		const float Yaw = LookBase.Yaw + (LookTurns % 2 == 0 ? LookAngle : -LookAngle);
		FaceLocation(Enemy->GetActorLocation() + FRotator(0.f, Yaw, 0.f).Vector() * 1000.f);
		++LookTurns;
		NextLookTurn = Now + LookTurnInterval;
	}
}

void AMEAIController::FinishLookAround()
{
	bLookingAround = false;
	FaceMovement();
	switch (Activity)
	{
	case EMEAlertState::Suspicious:
		CalmDown();
		break;
	case EMEAlertState::Searching:
		NextSearchPoint();
		break;
	default:
		SetAlertState(EMEAlertState::Searching);
		break;
	}
}

// --- Patrol ---

void AMEAIController::StartPatrol()
{
	bPatrolling = true;
	Enemy->GetCharacterMovement()->MaxWalkSpeed = Enemy->GetPatrolSpeed();

	const AMEPatrolPath* Path = Enemy->GetPatrolPath();
	if (!Path || Path->GetNumPoints() == 0)
	{
		return;
	}
	// Back from an alert: resumes at the nearest point, not at the start
	int32 Nearest = 0;
	float NearestDistance = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Path->GetNumPoints(); ++Index)
	{
		const float Distance = FVector::DistSquared(Path->GetPointLocation(Index), Enemy->GetActorLocation());
		if (Distance < NearestDistance)
		{
			Nearest = Index;
			NearestDistance = Distance;
		}
	}
	PatrolIndex = bHasPatrolled ? Nearest : FMath::Clamp(Enemy->GetPatrolStartIndex(), 0, Path->GetNumPoints() - 1);
	bHasPatrolled = true;
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

	// A move replaced by a newer one: the newer one reports later
	if (Result.IsInterrupted())
	{
		return;
	}

	if (bPatrolling)
	{
		if (bWaiting)
		{
			return;
		}
		if (Result.IsSuccess())
		{
			ArriveAtPatrolPoint();
		}
		else
		{
			// Blocked or off the navmesh: try the next point a bit later
			GetWorldTimerManager().SetTimer(PatrolTimer, this, &AMEAIController::NextPatrolPoint, PatrolRetryDelay);
		}
		return;
	}

	if (bActivityRunning && bMoving)
	{
		// Reached, or could not go further: looks from where it is
		bMoving = false;
		ArriveAtTarget();
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
	if (!bActivityRunning || !Enemy)
	{
		return TEXT("idle");
	}
	const float TargetDistance = FVector::Dist(Enemy->GetActorLocation(), AlertTarget) / 100.f;
	switch (Activity)
	{
	case EMEAlertState::Calm:
		if (!Enemy->GetPatrolPath())
		{
			return TEXT("standing (no path)");
		}
		return FString::Printf(TEXT("%s point %d / %d"), bWaiting ? TEXT("waiting at") : TEXT("walking to"), PatrolIndex + 1, Enemy->GetPatrolPath()->GetNumPoints());
	case EMEAlertState::Suspicious:
		return bLookingAround ? TEXT("looking around")
			: bMoving ? FString::Printf(TEXT("checking %.0f m away"), TargetDistance)
			: TEXT("staring");
	case EMEAlertState::Searching:
		return FString::Printf(TEXT("%s (%d places left)"), bLookingAround ? TEXT("looking around") : TEXT("searching"), SearchPointsLeft);
	default:
		return bMoving ? FString::Printf(TEXT("hurrying %.0f m away"), TargetDistance) : TEXT("watching");
	}
}
