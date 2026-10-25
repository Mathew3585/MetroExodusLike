#include "AI/MEAIController.h"
#include "AI/MEAICharacter.h"
#include "AI/MEAIWeaponComponent.h"
#include "AI/MENoise.h"
#include "AI/MESquadSubsystem.h"
#include "World/MEAlarm.h"
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
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TimerManager.h"

namespace
{
	/** A moving target is followed only once it has moved this far: no re-path every frame. */
	constexpr float RepathDistance = 150.f;
	/** Looking around: turns this far each side, this often. */
	constexpr float LookAngle = 60.f;
	constexpr float LookTurnInterval = 1.3f;
	constexpr float SurroundingsCheckInterval = 0.5f;
	/** The same voice does not talk over itself. */
	constexpr float BarkCooldown = 1.5f;
	/** A body this close is noticed even out of the vision cone. */
	constexpr float BodyNoticeDistance = 400.f;
	/** Alone (no colleague this close) in a fight, morale drains. */
	constexpr float AloneRadius = 3000.f;
	/** Heights above the floor: a ducked body / a standing one, and the player's eyes above his origin. */
	constexpr float DuckedHeight = 60.f;
	constexpr float StandingHeight = 150.f;
	constexpr float PlayerEyeHeight = 60.f;
	const FName RoutineSlot = TEXT("DefaultSlot");

	bool IsLegBone(const FName Bone)
	{
		const FString Name = Bone.ToString();
		return Name.StartsWith(TEXT("thigh")) || Name.StartsWith(TEXT("calf")) || Name.StartsWith(TEXT("foot")) || Name.StartsWith(TEXT("ball"));
	}

	bool IsDeadPlayer(const APawn* Pawn)
	{
		const AMECharacter* Player = Cast<AMECharacter>(Pawn);
		return Player && Player->GetHealth()->IsDead();
	}
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
	if (UMESquadSubsystem* Squad = GetSquad())
	{
		Squad->LeaveFight(this);
		Squad->ReleaseAlarm(this);
	}
	if (Enemy)
	{
		Enemy->GetHealth()->OnHealthChanged.RemoveAll(this);
	}
	Enemy = nullptr;
	Super::OnUnPossess();
}

const UMEEnemyProfile* AMEAIController::GetProfile() const
{
	return Enemy ? Enemy->GetProfile() : GetDefault<UMEEnemyProfile>();
}

UMESquadSubsystem* AMEAIController::GetSquad() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UMESquadSubsystem>() : nullptr;
}

float AMEAIController::GetAggression() const
{
	return GetProfile()->Aggression;
}

// --- Senses ---

void AMEAIController::HandlePerception(AActor* Actor, FAIStimulus Stimulus)
{
	// Only the player matters: other enemies, their shots and steps are ignored
	APawn* Player = Cast<APawn>(Actor);
	if (!Player || !Player->IsPlayerControlled())
	{
		return;
	}
	KnownPlayer = Player;

	const FAISenseID Sense = Stimulus.Type;
	if (Sense == UAISense::GetSenseID<UAISense_Sight>())
	{
		SeenPlayer = Stimulus.WasSuccessfullySensed() && !IsDeadPlayer(Player) ? Player : nullptr;
	}
	else if (Sense == UAISense::GetSenseID<UAISense_Hearing>() && Stimulus.WasSuccessfullySensed())
	{
		LastNoiseTime = GetWorld()->GetTimeSeconds();
		// Far noises are only roughly placed
		const float Error = Enemy ? NoiseLocationError * FVector::Dist(Stimulus.StimulusLocation, Enemy->GetActorLocation()) : 0.f;
		LastNoiseLocation = Stimulus.StimulusLocation + FMath::VRand().GetSafeNormal2D() * FMath::FRandRange(0.f, Error);
		LastNoiseTag = Stimulus.Tag;
		bNewNoise = true;
	}
}

void AMEAIController::HandleHealthChanged(float Health, float Delta, AController* DamageInstigator)
{
	if (Delta >= 0.f || !Enemy)
	{
		return;
	}
	// Shot: it does not know who, but it knows where from
	const APawn* Shooter = DamageInstigator ? DamageInstigator->GetPawn() : nullptr;
	HurtFrom = Shooter ? Shooter->GetActorLocation() : Enemy->GetActorLocation();
	bHurt = true;

	// Pain shakes it, more if it is not brave
	const UMEHealthComponent* HealthComponent = Enemy->GetHealth();
	Morale -= -Delta / FMath::Max(HealthComponent->GetMaxHealth(), 1.f) * WoundMoraleLoss * (1.2f - GetProfile()->Courage);
}

void AMEAIController::NotifyAllyDown(AMEAICharacter* Ally)
{
	KnownBodies.Add(Ally);
	AllyDownLocation = Ally->GetVisual()->GetBoneLocation(TEXT("pelvis"));
	bAllyDown = true;
}

void AMEAIController::HearCallout(const FVector& Location, bool bFight)
{
	CalloutLocation = Location;
	bCalloutFight = bFight;
	bCalledOut = true;
}

void AMEAIController::ColleagueSilent(const FVector& Location)
{
	if (AlertState == EMEAlertState::Calm || AlertState == EMEAlertState::Wary)
	{
		Bark(EMEBark::CallingAlly);
		BecomeSuspicious(Location);
	}
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
	return HasLineOfSight(Location, Target);
}

bool AMEAIController::HasLineOfSight(const FVector& Location, const AActor* Target) const
{
	if (!Enemy)
	{
		return false;
	}
	FVector Eyes;
	FRotator View;
	Enemy->GetActorEyesViewPoint(Eyes, View);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEAISight), false, Enemy);
	if (Target)
	{
		Params.AddIgnoredActor(Target);
	}
	return !GetWorld()->LineTraceTestByChannel(Eyes, Location, ECC_Visibility, Params);
}

void AMEAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Enemy)
	{
		return;
	}

	UpdateDetection(DeltaSeconds);
	UpdateMorale(DeltaSeconds);

	SurroundingsTimer -= DeltaSeconds;
	if (SurroundingsTimer <= 0.f)
	{
		SurroundingsTimer = SurroundingsCheckInterval;
		CheckSurroundings();
	}

	UpdateAlert();

	if (!bActivityRunning || AlarmToRaise.IsValid())
	{
		return;
	}
	if (Activity == EMEAlertState::Combat)
	{
		UpdateCombat();
	}
	else if (bLookingAround)
	{
		UpdateLookAround();
	}
	else if (bMoving && (Activity == EMEAlertState::Suspicious || Activity == EMEAlertState::Alerted)
		&& FVector::Dist(AlertTarget, MoveGoal) > RepathDistance)
	{
		// What it goes to moved (a new noise, a new shout): follow
		MoveToTarget(Activity == EMEAlertState::Alerted ? 150.f : 100.f);
	}
}

void AMEAIController::UpdateDetection(float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const APawn* Player = SeenPlayer.Get();
	if (Player && IsDeadPlayer(Player))
	{
		SeenPlayer = nullptr;
		Player = nullptr;
	}
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
	const float AlertScale = AlertState != EMEAlertState::Calm ? WaryDetectionScale : 1.f;

	Detection = FMath::Min(1.f, Detection + DeltaSeconds / FullDetectionTime * DistanceScale * AngleScale * PostureScale * AlertScale * GetProfile()->DetectionSpeed);
	LastSeenTime = Now;
	LastSeenLocation = Player->GetActorLocation();
}

void AMEAIController::UpdateMorale(float DeltaSeconds)
{
	if (AlertState == EMEAlertState::Surrendered)
	{
		return;
	}
	if (AlertState == EMEAlertState::Combat)
	{
		// Fighting alone is frightening
		if (GetSquad()->CountAlliesNear(this, AloneRadius) == 0)
		{
			Morale -= 0.01f * (1.f - GetProfile()->Courage) * DeltaSeconds;
		}
	}
	else if (AlertState != EMEAlertState::Alerted)
	{
		Morale += MoraleRecovery * DeltaSeconds;
	}
	Morale = FMath::Clamp(Morale, 0.f, 1.f);
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

void AMEAIController::CheckSurroundings()
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
			if (AlertState == EMEAlertState::Calm || AlertState == EMEAlertState::Wary)
			{
				BecomeSuspicious(Door->GetActorLocation());
			}
		}
	}

	// A body it had not seen yet: in its sight, or right next to it whatever it looks at
	for (TActorIterator<AMEAICharacter> It(GetWorld()); It; ++It)
	{
		AMEAICharacter* Body = *It;
		if (Body == Enemy || !Body->GetHealth()->IsDead() || KnownBodies.Contains(Body))
		{
			continue;
		}
		// The ragdoll lies where its pelvis is, not where the actor was
		const FVector BodyLocation = Body->GetVisual()->GetBoneLocation(TEXT("pelvis")) + FVector(0.f, 0.f, 20.f);
		const bool bClose = FVector::DistSquared(BodyLocation, Enemy->GetActorLocation()) < FMath::Square(BodyNoticeDistance);
		if (CanSeeLocation(BodyLocation, Body) || (bClose && HasLineOfSight(BodyLocation, Body)))
		{
			NotifyAllyDown(Body);
		}
	}
}

// --- Alert ---

void AMEAIController::UpdateAlert()
{
	if (AlertState == EMEAlertState::Surrendered)
	{
		bHurt = bAllyDown = bCalledOut = bNewNoise = false;
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const UMEEnemyProfile* Profile = GetProfile();

	// Strongest events first: being shot, a body, a shout, a noise
	if (bHurt)
	{
		bHurt = false;
		if (!bLimping && IsLegBone(Enemy->GetLastHitBone()))
		{
			bLimping = true;
			Enemy->GetCharacterMovement()->MaxWalkSpeed *= LimpSpeedScale;
		}
		const UMEHealthComponent* Health = Enemy->GetHealth();
		if (!bCalledForHelp && Health->GetHealth() < Health->GetMaxHealth() * CallForHelpHealth)
		{
			bCalledForHelp = true;
			Bark(EMEBark::Hurt);
			Shout(Enemy->GetActorLocation(), true);
		}
		else
		{
			Bark(EMEBark::Pain);
		}
		PutStationOnEdge();
		if (AlertState != EMEAlertState::Fleeing)
		{
			if (SeenPlayer.IsValid())
			{
				// It sees who shoots: fights back
				StartFight();
			}
			else
			{
				// Shot from the dark: it only guesses where from, and goes there
				bCheckingBody = false;
				RaiseAlert(GuessShooterLocation(HurtFrom));
			}
		}
	}
	if (bAllyDown)
	{
		bAllyDown = false;
		Morale -= AllyDownMoraleLoss * (1.2f - Profile->Courage);
		Bark(EMEBark::AllyDown);
		PutStationOnEdge();
		if (AlertState != EMEAlertState::Combat && AlertState != EMEAlertState::Fleeing)
		{
			CheckBody(AllyDownLocation);
		}
	}
	if (bCalledOut)
	{
		bCalledOut = false;
		// A body it is checking comes before a shout
		if (AlertState != EMEAlertState::Combat && AlertState != EMEAlertState::Fleeing && !bCheckingBody)
		{
			RaiseAlert(CalloutLocation);
		}
	}
	if (bNewNoise)
	{
		bNewNoise = false;
		if (AlertState == EMEAlertState::Combat || AlertState == EMEAlertState::Fleeing || bCheckingBody)
		{
			// Busy (fighting, running, checking a body): noises add nothing
		}
		else if (LastNoiseTag == MENoise::Gunshot || AlertState == EMEAlertState::Alerted || AlertState == EMEAlertState::Searching)
		{
			// Already looking for someone, any noise is a lead
			RaiseAlert(LastNoiseLocation);
		}
		else
		{
			BecomeSuspicious(LastNoiseLocation);
		}
	}

	// Morale breaking: runs, or gives up when the player is right there
	if (AlertState == EMEAlertState::Combat || AlertState == EMEAlertState::Alerted || AlertState == EMEAlertState::Fleeing)
	{
		const APawn* Player = SeenPlayer.Get();
		if (Morale < SurrenderMorale && Profile->bCanSurrender && Player
			&& FVector::Dist(Player->GetActorLocation(), Enemy->GetActorLocation()) < SurrenderDistance)
		{
			Bark(EMEBark::Surrender);
			SetAlertState(EMEAlertState::Surrendered);
			return;
		}
		if (Morale < FleeMorale && AlertState != EMEAlertState::Fleeing)
		{
			Bark(EMEBark::Fleeing);
			SetAlertState(EMEAlertState::Fleeing);
			return;
		}
	}
	if (AlertState == EMEAlertState::Fleeing)
	{
		// Pulled itself together: back to it
		if (Morale >= FleeMorale + 0.25f)
		{
			RaiseAlert(LastSeenLocation);
		}
		return;
	}

	if (SeenPlayer.IsValid())
	{
		const bool bLooking = AlertState == EMEAlertState::Alerted || AlertState == EMEAlertState::Searching;
		const float Threshold = AlertState == EMEAlertState::Calm ? SuspicionThreshold : WarySuspicionThreshold;
		if (Detection >= 1.f || (bLooking && Detection >= Threshold))
		{
			StartFight();
		}
		else if (Detection >= Threshold && (AlertState == EMEAlertState::Calm || AlertState == EMEAlertState::Wary || AlertState == EMEAlertState::Suspicious))
		{
			BecomeSuspicious(LastSeenLocation);
		}
	}
	else if (AlertState == EMEAlertState::Wary && !GetSquad()->IsStationOnEdge() && Now >= WaryUntil)
	{
		// A false alarm, long ago: calm again
		SetAlertState(EMEAlertState::Calm);
	}
	else if (AlertState == EMEAlertState::Combat && Now - LastSeenTime > LoseTargetTime)
	{
		// Lost him: go where he was
		Bark(EMEBark::LostTarget);
		RaiseAlert(LastSeenLocation);
	}
}

void AMEAIController::BecomeSuspicious(const FVector& Location)
{
	AlertTarget = Location;
	if (AlertState != EMEAlertState::Suspicious)
	{
		Bark(EMEBark::Suspicious);
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
		// The others hear it (and pass it on)
		Shout(Location, false);
		SetAlertState(EMEAlertState::Alerted);
	}
	else if (bActivityRunning && Activity == EMEAlertState::Alerted && !bMoving && !AlarmToRaise.IsValid())
	{
		// Arrived (or looking around): a new lead, hurry there
		bLookingAround = false;
		StartHurrying();
	}
}

void AMEAIController::StartFight()
{
	bCheckingBody = false;
	AlertTarget = LastSeenLocation;
	if (AlertState != EMEAlertState::Combat)
	{
		Bark(EMEBark::Spotted);
		Shout(LastSeenLocation, true);
		PutStationOnEdge();
		SetAlertState(EMEAlertState::Combat);
	}
}

void AMEAIController::PutStationOnEdge()
{
	GetSquad()->SetStationOnEdge();
}

void AMEAIController::CheckBody(const FVector& Location)
{
	BodyToCheck = Location;
	bCheckingBody = true;
	AlertTarget = Location;
	if (AlertState != EMEAlertState::Alerted)
	{
		SetAlertState(EMEAlertState::Alerted);
	}
	else if (bActivityRunning && Activity == EMEAlertState::Alerted && !bMoving && !bExaminingBody && !AlarmToRaise.IsValid())
	{
		bLookingAround = false;
		StartHurrying();
	}
}

void AMEAIController::FinishExaminingBody()
{
	bExaminingBody = false;
	bCheckingBody = false;
	Duck(false);
	// Tells the others, then searches around the body
	Shout(BodyToCheck, false);
	AlertTarget = BodyToCheck;
	SetAlertState(EMEAlertState::Searching);
}

FVector AMEAIController::GuessShooterLocation(const FVector& ShooterLocation) const
{
	const FVector Me = Enemy->GetActorLocation();
	const FVector ToShooter = ShooterLocation - Me;
	const FVector Direction = ToShooter.GetSafeNormal2D().RotateAngleAxis(FMath::FRandRange(-ShotDirectionError, ShotDirectionError), FVector::UpVector);
	return Me + Direction * ToShooter.Size2D() * FMath::FRandRange(0.5f, 1.1f);
}

void AMEAIController::EndAlert()
{
	if (AlertState == EMEAlertState::Searching)
	{
		Bark(EMEBark::GiveUp);
	}
	WaryUntil = GetWorld()->GetTimeSeconds() + WaryTime;
	SetAlertState(EMEAlertState::Wary);
}

void AMEAIController::SetAlertState(EMEAlertState NewState)
{
	UMESquadSubsystem* Squad = GetSquad();
	if (AlertState == EMEAlertState::Combat && NewState != EMEAlertState::Combat)
	{
		Squad->LeaveFight(this);
	}
	else if (NewState == EMEAlertState::Combat && AlertState != EMEAlertState::Combat)
	{
		Squad->JoinFight(this);
	}
	if (NewState == EMEAlertState::Searching)
	{
		Bark(EMEBark::Searching);
	}

	// On edge, it hears further
	const float NewHearingRange = HearingRange * (NewState == EMEAlertState::Calm ? 1.f : WaryHearingScale);
	if (!FMath::IsNearlyEqual(HearingConfig->HearingRange, NewHearingRange))
	{
		HearingConfig->HearingRange = NewHearingRange;
		Perception->ConfigureSense(*HearingConfig);
	}

	// The StateTree follows on its next tick and switches the activity
	AlertState = NewState;
}

void AMEAIController::Bark(EMEBark Line)
{
	const double Now = GetWorld()->GetTimeSeconds();
	USoundBase* Sound = GetProfile()->GetBark(Line);
	if (!Sound || !Enemy || Now - LastBarkTime < BarkCooldown)
	{
		return;
	}
	LastBarkTime = Now;
	UGameplayStatics::PlaySoundAtLocation(this, Sound, Enemy->GetActorLocation() + FVector(0.f, 0.f, 70.f));
}

void AMEAIController::Shout(const FVector& Location, bool bFight)
{
	if (UMESquadSubsystem* Squad = GetSquad())
	{
		Squad->Callout(this, Location, bFight);
	}
}

// --- Activities ---

void AMEAIController::StopBrain()
{
	StateTree->StopLogic(TEXT("Out of the fight"));
}

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
	case EMEAlertState::Wary:
		StartPatrol();
		break;
	case EMEAlertState::Suspicious:
		StartInvestigating();
		break;
	case EMEAlertState::Alerted:
		if (!TryRunToAlarm())
		{
			StartHurrying();
		}
		break;
	case EMEAlertState::Combat:
		if (!TryRunToAlarm())
		{
			StartCombat();
		}
		break;
	case EMEAlertState::Searching:
		StartSearching();
		break;
	case EMEAlertState::Fleeing:
		StartFleeing();
		break;
	case EMEAlertState::Surrendered:
		StartSurrender();
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
	bExaminingBody = false;
	bBurstStarted = false;
	GetWorldTimerManager().ClearTimer(ActivityTimer);
	StopPatrol();
	StopMovement();
	FaceMovement();
	if (Enemy)
	{
		Enemy->GetAIWeapon()->StopFiring();
		if (Activity != EMEAlertState::Surrendered)
		{
			Duck(false);
		}
	}
	if (UMESquadSubsystem* Squad = GetSquad())
	{
		Squad->ReleaseAttackTicket(this);
		if (AlarmToRaise.IsValid())
		{
			Squad->ReleaseAlarm(this);
		}
	}
	AlarmToRaise.Reset();
}

bool AMEAIController::MoveToPoint(const FVector& Point, float AcceptanceRadius)
{
	MoveGoal = Point;
	bMoving = true;
	if (MoveToLocation(Point, AcceptanceRadius, true, true, true) != EPathFollowingRequestResult::RequestSuccessful)
	{
		bMoving = false;
		return false;
	}
	return true;
}

void AMEAIController::MoveToTarget(float AcceptanceRadius)
{
	if (!MoveToPoint(AlertTarget, AcceptanceRadius))
	{
		// Already there, or no way to get there: act as if arrived
		ArriveAtTarget();
	}
}

void AMEAIController::ArriveAtTarget()
{
	// Ran to the alarm: ring it, then back to what it was doing
	if (AMEAlarm* Alarm = AlarmToRaise.Get())
	{
		Alarm->Raise(LastSeenTime >= 0.0 ? LastSeenLocation : AlertTarget);
		AlarmToRaise.Reset();
		if (Activity == EMEAlertState::Combat)
		{
			StartCombat();
		}
		else
		{
			StartHurrying();
		}
		return;
	}

	switch (Activity)
	{
	case EMEAlertState::Suspicious:
		LookAround(LookAroundTime);
		break;
	case EMEAlertState::Searching:
		LookAround(SearchLookTime);
		break;
	case EMEAlertState::Alerted:
		if (bCheckingBody && !bExaminingBody)
		{
			// Crouches over the body for a moment
			bExaminingBody = true;
			Duck(true);
			FaceLocation(BodyToCheck);
			GetWorldTimerManager().SetTimer(ActivityTimer, this, &AMEAIController::FinishExaminingBody, FMath::Max(ExamineBodyTime, 0.01f));
		}
		else if (SeenPlayer.IsValid())
		{
			FaceLocation(LastSeenLocation);
		}
		else
		{
			SetAlertState(EMEAlertState::Searching);
		}
		break;
	case EMEAlertState::Combat:
		// In cover: duck and wait for a chance to shoot
		CombatStep = ECombatStep::Ducked;
		Duck(bHasCover);
		NextCombatAction = GetWorld()->GetTimeSeconds() + FMath::FRandRange(DuckTimeMin, DuckTimeMax) * 0.5f;
		break;
	case EMEAlertState::Fleeing:
		// Hides where it ran to
		Duck(true);
		FaceLocation(LastSeenLocation);
		break;
	default:
		break;
	}
}

void AMEAIController::SetMoveSpeed(float Speed)
{
	Enemy->GetCharacterMovement()->MaxWalkSpeed = Speed * (bLimping ? LimpSpeedScale : 1.f);
}

void AMEAIController::StartInvestigating()
{
	// Stops, stares a moment, then goes to check
	SetMoveSpeed(InvestigateSpeed);
	StopMovement();
	FaceLocation(AlertTarget);
	GetWorldTimerManager().SetTimer(ActivityTimer, this, &AMEAIController::GoCheck, FMath::Max(GetProfile()->ReactionTime, 0.01f));
}

void AMEAIController::GoCheck()
{
	FaceMovement();
	MoveToTarget(100.f);
}

void AMEAIController::StartHurrying()
{
	SetMoveSpeed(AlertSpeed);
	FaceMovement();
	MoveToTarget(150.f);
}

bool AMEAIController::TryRunToAlarm()
{
	AMEAlarm* Alarm = GetSquad()->ClaimAlarm(this, AlarmRange);
	if (!Alarm)
	{
		return false;
	}
	AlarmToRaise = Alarm;
	Bark(EMEBark::Alarm);
	SetMoveSpeed(AlertSpeed);
	FaceMovement();
	if (!MoveToPoint(Alarm->GetActorLocation(), 120.f))
	{
		ArriveAtTarget();
	}
	return true;
}

void AMEAIController::StartSearching()
{
	SetMoveSpeed(SearchSpeed);
	SearchCenter = AlertTarget;
	SearchPointsLeft = SearchPoints;
	NextSearchPoint();
}

void AMEAIController::NextSearchPoint()
{
	if (SearchPointsLeft <= 0)
	{
		// Nothing found: the alert is over, it stays wary
		EndAlert();
		return;
	}
	--SearchPointsLeft;

	const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation Point;
	if (!Navigation || !Navigation->GetRandomReachablePointInRadius(SearchCenter, SearchRadius, Point))
	{
		EndAlert();
		return;
	}

	FaceMovement();
	if (!MoveToPoint(Point.Location, 80.f))
	{
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
		EndAlert();
		break;
	case EMEAlertState::Searching:
		NextSearchPoint();
		break;
	case EMEAlertState::Wary:
	{
		const AMEPatrolPath* Path = Enemy->GetPatrolPath();
		if (bPatrolling && Path && Path->GetNumPoints() > 1)
		{
			NextPatrolPoint();
		}
		else
		{
			// A post: keeps scanning
			LookAround(WaryLookTime);
		}
		break;
	}
	default:
		SetAlertState(EMEAlertState::Searching);
		break;
	}
}

// --- Combat ---

void AMEAIController::StartCombat()
{
	SetMoveSpeed(AlertSpeed);
	bFirstShot = true;
	BurstsFromCover = 0;
	MoveToCover();
}

void AMEAIController::Duck(bool bDuck)
{
	if (bDuck)
	{
		Enemy->Crouch();
	}
	else
	{
		Enemy->UnCrouch();
	}
}

bool AMEAIController::FindCover(FVector& OutCover) const
{
	const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const UMESquadSubsystem* Squad = GetSquad();
	if (!Navigation || !Squad)
	{
		return false;
	}

	const FVector Me = Enemy->GetActorLocation();
	const FVector Threat = LastSeenLocation;
	const FVector ThreatEyes = Threat + FVector(0.f, 0.f, PlayerEyeHeight);
	const float Preferred = FMath::Lerp(FarRange, CloseRange, GetAggression());

	// The flanker wants spots off the line between the player and the others
	const bool bFlanker = Squad->IsFlanker(this);
	const FVector SquadSide = (Squad->GetFightCenter() - Threat).GetSafeNormal2D();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MECover), false, Enemy);
	if (const APawn* Player = KnownPlayer.Get())
	{
		Params.AddIgnoredActor(Player);
	}

	float BestScore = -TNumericLimits<float>::Max();
	bool bFound = false;
	const float Radii[] = { 300.f, 700.f, 1100.f, CoverSearchRadius };
	for (int32 Ring = 0; Ring < UE_ARRAY_COUNT(Radii); ++Ring)
	{
		for (int32 Step = 0; Step < 16; ++Step)
		{
			const float Yaw = Step * 22.5f + Ring * 11.25f;
			FNavLocation Candidate;
			if (!Navigation->ProjectPointToNavigation(Me + FRotator(0.f, Yaw, 0.f).Vector() * Radii[Ring], Candidate, FVector(200.f, 200.f, 300.f)))
			{
				continue;
			}
			const FVector Point = Candidate.Location;
			const float ThreatDistance = FVector::Dist2D(Point, Threat);
			if (ThreatDistance < CloseRange * 0.7f || ThreatDistance > FarRange * 1.3f || Squad->IsCoverTaken(Point, this))
			{
				continue;
			}

			// Hidden when ducked; ideally able to shoot over it when standing (half cover)
			if (!GetWorld()->LineTraceTestByChannel(ThreatEyes, Point + FVector(0.f, 0.f, DuckedHeight), ECC_Visibility, Params))
			{
				continue;
			}
			const bool bFullCover = GetWorld()->LineTraceTestByChannel(ThreatEyes, Point + FVector(0.f, 0.f, StandingHeight), ECC_Visibility, Params);

			float Score = bFullCover ? 1.f : 2.f;
			Score -= FVector::Dist(Me, Point) / 1000.f * 0.8f;
			Score -= FMath::Abs(ThreatDistance - Preferred) / 1000.f;
			if (bFlanker)
			{
				const float Side = 1.f - FMath::Abs(FVector::DotProduct((Point - Threat).GetSafeNormal2D(), SquadSide));
				Score += Side * 1.5f;
			}
			if (Score > BestScore)
			{
				BestScore = Score;
				OutCover = Point;
				bFound = true;
			}
		}
	}
	return bFound;
}

void AMEAIController::MoveToCover()
{
	FVector Cover;
	bHasCover = FindCover(Cover);
	bBurstStarted = false;
	Duck(false);
	if (bHasCover)
	{
		GetSquad()->TakeCover(this, Cover);
		FaceMovement();
		CombatStep = ECombatStep::ToCover;
		if (!MoveToPoint(Cover, 50.f))
		{
			ArriveAtTarget();
		}
		return;
	}
	// Nowhere to hide: fights in the open
	CombatStep = ECombatStep::Ducked;
	NextCombatAction = GetWorld()->GetTimeSeconds() + 0.3f;
}

void AMEAIController::UpdateCombat()
{
	if (CombatStep == ECombatStep::ToCover)
	{
		// Arriving is handled by OnMoveCompleted
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	const APawn* Player = SeenPlayer.Get();
	UMEAIWeaponComponent* Gun = Enemy->GetAIWeapon();
	UMESquadSubsystem* Squad = GetSquad();
	FaceLocation(LastSeenLocation);

	if (CombatStep == ECombatStep::Shooting)
	{
		if (!bBurstStarted)
		{
			// Standing up (or reacting, the first time) before the burst
			if (Now >= NextCombatAction)
			{
				const UMEEnemyProfile* Profile = GetProfile();
				const bool bSuppressing = Player == nullptr;
				const float Spread = Profile->Spread * (bSuppressing ? 2.f : 1.f) * (bLimping ? 1.3f : 1.f);
				Gun->StartBurst(const_cast<APawn*>(Player), LastSeenLocation, FMath::RandRange(Profile->BurstMin, FMath::Max(Profile->BurstMin, Profile->BurstMax)), Spread, Profile->DamageScale);
				bBurstStarted = true;
			}
			return;
		}
		if (Gun->IsFiring())
		{
			return;
		}

		// Burst over: back down, let another one shoot
		bBurstStarted = false;
		Squad->ReleaseAttackTicket(this);
		++BurstsFromCover;
		CombatStep = ECombatStep::Ducked;
		Duck(bHasCover);
		NextCombatAction = Now + FMath::FRandRange(DuckTimeMin, DuckTimeMax) * FMath::Lerp(1.3f, 0.5f, GetAggression());
		if (Gun->IsEmpty())
		{
			Bark(EMEBark::Reloading);
			Gun->Reload();
		}
		return;
	}

	// Waiting between bursts
	if (Gun->IsReloading())
	{
		return;
	}
	if (Gun->IsEmpty())
	{
		Bark(EMEBark::Reloading);
		Gun->Reload();
		return;
	}

	// Cover no longer covers (flanked) or the player is on top of it: move
	bool bCoverLost = false;
	const APawn* Known = KnownPlayer.Get();
	if (bHasCover && Known)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(MECoverCheck), false, Enemy);
		Params.AddIgnoredActor(Known);
		const FVector Floor = Enemy->GetActorLocation() - FVector(0.f, 0.f, Enemy->GetSimpleCollisionHalfHeight());
		bCoverLost = !GetWorld()->LineTraceTestByChannel(Known->GetActorLocation() + FVector(0.f, 0.f, PlayerEyeHeight), Floor + FVector(0.f, 0.f, DuckedHeight), ECC_Visibility, Params)
			|| FVector::Dist2D(Known->GetActorLocation(), Enemy->GetActorLocation()) < CloseRange * 0.6f;
	}
	if (bCoverLost || BurstsFromCover >= BurstsPerCover || (!bHasCover && Now >= NextCombatAction + 3.0))
	{
		BurstsFromCover = 0;
		MoveToCover();
		return;
	}
	if (Now < NextCombatAction)
	{
		return;
	}

	// Sees him, or saw him a moment ago (fires where he was to pin him down); otherwise peeks
	const bool bCanSuppress = !Player && Now - LastSeenTime < SuppressTime;
	if (!Player && !bCanSuppress)
	{
		Duck(false);
		return;
	}
	if (!Squad->RequestAttackTicket(this))
	{
		NextCombatAction = Now + 0.5;
		return;
	}
	Duck(false);
	CombatStep = ECombatStep::Shooting;
	bBurstStarted = false;
	NextCombatAction = Now + (bFirstShot ? GetProfile()->ReactionTime : 0.3f);
	bFirstShot = false;
}

// --- Flee / surrender ---

void AMEAIController::StartFleeing()
{
	SetMoveSpeed(AlertSpeed);
	Duck(false);

	const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const FVector Me = Enemy->GetActorLocation();
	const FVector Threat = LastSeenLocation;
	const FVector Away = (Me - Threat).GetSafeNormal2D();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEFlee), false, Enemy);

	// Far from the player, out of his sight if possible, never toward him
	float BestScore = -TNumericLimits<float>::Max();
	FVector Best = Me;
	bool bFound = false;
	for (const float Radius : { 1500.f, 2500.f, 3500.f })
	{
		for (int32 Step = 0; Step < 16; ++Step)
		{
			const FVector Direction = FRotator(0.f, Step * 22.5f, 0.f).Vector();
			FNavLocation Candidate;
			if (!Navigation || FVector::DotProduct(Direction, Away) < 0.2f
				|| !Navigation->ProjectPointToNavigation(Me + Direction * Radius, Candidate, FVector(300.f, 300.f, 300.f)))
			{
				continue;
			}
			const bool bOutOfSight = GetWorld()->LineTraceTestByChannel(Threat + FVector(0.f, 0.f, PlayerEyeHeight), Candidate.Location + FVector(0.f, 0.f, StandingHeight), ECC_Visibility, Params);
			const float Score = FVector::Dist(Candidate.Location, Threat) + (bOutOfSight ? 1500.f : 0.f);
			if (Score > BestScore)
			{
				BestScore = Score;
				Best = Candidate.Location;
				bFound = true;
			}
		}
	}

	FaceMovement();
	if (!bFound || !MoveToPoint(Best, 100.f))
	{
		ArriveAtTarget();
	}
}

void AMEAIController::StartSurrender()
{
	StopMovement();
	Enemy->DropWeapon();
	// Kneels (until there is a hands-up animation), facing the player
	Duck(true);
	FaceLocation(KnownPlayer.IsValid() ? KnownPlayer->GetActorLocation() : LastSeenLocation);
	GetSquad()->LeaveFight(this);
}

// --- Patrol ---

void AMEAIController::StartPatrol()
{
	bPatrolling = true;
	// Wary: slower, careful
	SetMoveSpeed(Activity == EMEAlertState::Wary ? InvestigateSpeed : Enemy->GetPatrolSpeed());

	const AMEPatrolPath* Path = Enemy->GetPatrolPath();
	if (!Path || Path->GetNumPoints() == 0)
	{
		if (Activity == EMEAlertState::Wary)
		{
			// No route: keeps scanning around where it stands
			bWaiting = true;
			LookAround(WaryLookTime);
		}
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
	StopRoutine();
}

void AMEAIController::MoveToPatrolPoint()
{
	bWaiting = false;
	StopRoutine();
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
		// Reached, or could not go further: carries on from where it is
		bMoving = false;
		ArriveAtTarget();
	}
}

void AMEAIController::ArriveAtPatrolPoint()
{
	const AMEPatrolPath* Path = Enemy->GetPatrolPath();
	const FMEPatrolPoint Point = Path->GetPoint(PatrolIndex);
	bWaiting = true;

	// Wary: no routine, it stops at every point to look around, then goes on
	if (Activity == EMEAlertState::Wary)
	{
		LookAround(WaryLookTime);
		return;
	}

	FRotator Facing;
	if (Point.WaitTime != 0.f && Path->GetPointFacing(PatrolIndex, Facing))
	{
		FaceLocation(Enemy->GetActorLocation() + Facing.Vector() * 1000.f);
	}
	if (Point.WaitTime != 0.f && Point.Routine)
	{
		PlayRoutine(Point.Routine);
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

void AMEAIController::PlayRoutine(UAnimSequenceBase* Routine)
{
	// Looped until it leaves the point (smoking, warming up...)
	if (UAnimInstance* AnimInstance = Enemy->GetMesh()->GetAnimInstance())
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(Routine, RoutineSlot, 0.4f, 0.4f, 1.f, 1000);
		bRoutinePlaying = true;
	}
}

void AMEAIController::StopRoutine()
{
	if (!bRoutinePlaying || !Enemy)
	{
		return;
	}
	bRoutinePlaying = false;
	if (UAnimInstance* AnimInstance = Enemy->GetMesh()->GetAnimInstance())
	{
		AnimInstance->StopSlotAnimation(0.3f, RoutineSlot);
	}
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
	if (AlarmToRaise.IsValid())
	{
		return TEXT("running to the alarm");
	}
	const float TargetDistance = FVector::Dist(Enemy->GetActorLocation(), AlertTarget) / 100.f;
	FString Description;
	switch (Activity)
	{
	case EMEAlertState::Wary:
		Description = bLookingAround ? TEXT("looking around") : TEXT("patrolling carefully");
		if (GetSquad()->IsStationOnEdge())
		{
			Description += TEXT(" (on edge)");
		}
		break;
	case EMEAlertState::Calm:
		Description = !Enemy->GetPatrolPath() ? TEXT("standing (no path)")
			: FString::Printf(TEXT("%s point %d / %d"), bWaiting ? TEXT("waiting at") : TEXT("walking to"), PatrolIndex + 1, Enemy->GetPatrolPath()->GetNumPoints());
		break;
	case EMEAlertState::Suspicious:
		Description = bLookingAround ? TEXT("looking around")
			: bMoving ? FString::Printf(TEXT("checking %.0f m away"), TargetDistance)
			: TEXT("staring");
		break;
	case EMEAlertState::Searching:
		Description = FString::Printf(TEXT("%s (%d places left)"), bLookingAround ? TEXT("looking around") : TEXT("searching"), SearchPointsLeft);
		break;
	case EMEAlertState::Combat:
	{
		const UMEAIWeaponComponent* Gun = Enemy->GetAIWeapon();
		Description = CombatStep == ECombatStep::ToCover ? TEXT("to cover")
			: CombatStep == ECombatStep::Shooting ? (Gun->IsFiring() ? TEXT("SHOOTING") : TEXT("aiming"))
			: Gun->IsReloading() ? TEXT("reloading")
			: bHasCover ? TEXT("in cover") : TEXT("in the open");
		if (GetSquad()->IsFlanker(this))
		{
			Description += TEXT(" (flanker)");
		}
		break;
	}
	case EMEAlertState::Fleeing:
		Description = bMoving ? TEXT("running away") : TEXT("hiding");
		break;
	case EMEAlertState::Surrendered:
		Description = TEXT("hands up");
		break;
	default:
		Description = bExaminingBody ? TEXT("examining a body")
			: bCheckingBody ? FString::Printf(TEXT("going to a body %.0f m away"), TargetDistance)
			: bMoving ? FString::Printf(TEXT("hurrying %.0f m away"), TargetDistance) : TEXT("watching");
		break;
	}
	return FString::Printf(TEXT("%s, morale %.0f %%%s"), *Description, Morale * 100.f, bLimping ? TEXT(", limping") : TEXT(""));
}
