#include "AI/MESquadSubsystem.h"
#include "AI/MEAIController.h"
#include "AI/MEAICharacter.h"
#include "World/MEAlarm.h"
#include "Combat/MEHealthComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"

void UMESquadSubsystem::Cleanup()
{
	auto IsGone = [](const TWeakObjectPtr<AMEAIController>& Controller) { return !Controller.IsValid() || !Controller->GetPawn(); };
	Fighters.RemoveAll(IsGone);
	Shooters.RemoveAll(IsGone);
	for (auto It = Covers.CreateIterator(); It; ++It)
	{
		if (IsGone(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
}

// --- Fight ---

void UMESquadSubsystem::JoinFight(AMEAIController* Controller)
{
	Cleanup();
	Fighters.AddUnique(Controller);
}

void UMESquadSubsystem::LeaveFight(AMEAIController* Controller)
{
	Fighters.Remove(Controller);
	Shooters.Remove(Controller);
	Covers.Remove(Controller);
}

bool UMESquadSubsystem::RequestAttackTicket(AMEAIController* Controller)
{
	Cleanup();
	if (Shooters.Contains(Controller))
	{
		return true;
	}
	if (Shooters.Num() >= MaxShooters)
	{
		return false;
	}
	Shooters.Add(Controller);
	return true;
}

void UMESquadSubsystem::ReleaseAttackTicket(AMEAIController* Controller)
{
	Shooters.Remove(Controller);
}

bool UMESquadSubsystem::IsFlanker(const AMEAIController* Controller) const
{
	if (Fighters.Num() < 2)
	{
		return false;
	}
	// The boldest fighter flanks; the others hold
	const AMEAIController* Boldest = nullptr;
	float BestAggression = -1.f;
	for (const TWeakObjectPtr<AMEAIController>& Fighter : Fighters)
	{
		if (Fighter.IsValid() && Fighter->GetAggression() > BestAggression)
		{
			Boldest = Fighter.Get();
			BestAggression = Fighter->GetAggression();
		}
	}
	return Boldest == Controller;
}

FVector UMESquadSubsystem::GetFightCenter() const
{
	FVector Sum = FVector::ZeroVector;
	int32 Count = 0;
	for (const TWeakObjectPtr<AMEAIController>& Fighter : Fighters)
	{
		if (Fighter.IsValid() && Fighter->GetPawn())
		{
			Sum += Fighter->GetPawn()->GetActorLocation();
			++Count;
		}
	}
	return Count > 0 ? Sum / Count : FVector::ZeroVector;
}

bool UMESquadSubsystem::IsCoverTaken(const FVector& Location, const AMEAIController* Asker) const
{
	for (const TPair<TWeakObjectPtr<AMEAIController>, FVector>& Cover : Covers)
	{
		if (Cover.Key.Get() != Asker && FVector::DistSquared2D(Cover.Value, Location) < FMath::Square(150.f))
		{
			return true;
		}
	}
	return false;
}

void UMESquadSubsystem::TakeCover(AMEAIController* Controller, const FVector& Location)
{
	Covers.Add(Controller, Location);
}

// --- Voice ---

void UMESquadSubsystem::Callout(AMEAIController* Speaker, const FVector& Location, bool bFight)
{
	const APawn* SpeakerPawn = Speaker ? Speaker->GetPawn() : nullptr;
	if (!SpeakerPawn)
	{
		return;
	}

	for (TActorIterator<AMEAIController> It(GetWorld()); It; ++It)
	{
		AMEAIController* Listener = *It;
		const APawn* ListenerPawn = Listener->GetPawn();
		if (Listener == Speaker || !ListenerPawn)
		{
			continue;
		}

		// A shout carries less through walls
		const float Distance = FVector::Dist(SpeakerPawn->GetActorLocation(), ListenerPawn->GetActorLocation());
		FCollisionQueryParams Params(SCENE_QUERY_STAT(MECallout), false, SpeakerPawn);
		Params.AddIgnoredActor(ListenerPawn);
		const bool bWall = GetWorld()->LineTraceTestByChannel(SpeakerPawn->GetActorLocation(), ListenerPawn->GetActorLocation(), ECC_Visibility, Params);
		if (Distance > (bWall ? CalloutRange * 0.5f : CalloutRange))
		{
			continue;
		}

		// Time to hear, understand and react
		const float Delay = 0.4f + Distance / CalloutRange * 0.8f;
		TWeakObjectPtr<AMEAIController> WeakListener = Listener;
		FTimerHandle Timer;
		GetWorld()->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [WeakListener, Location, bFight]()
		{
			if (AMEAIController* Heard = WeakListener.Get())
			{
				Heard->HearCallout(Location, bFight);
			}
		}), Delay, false);
	}
}

// --- Alarm ---

void UMESquadSubsystem::RegisterAlarm(AMEAlarm* Alarm)
{
	Alarms.AddUnique(Alarm);
}

AMEAlarm* UMESquadSubsystem::ClaimAlarm(AMEAIController* Runner, float Range)
{
	const APawn* RunnerPawn = Runner ? Runner->GetPawn() : nullptr;
	if (bAlarmRaised || !RunnerPawn || (AlarmRunner.IsValid() && AlarmRunner->GetPawn() && AlarmRunner.Get() != Runner))
	{
		return nullptr;
	}

	AMEAlarm* Nearest = nullptr;
	float NearestDistance = Range;
	for (const TWeakObjectPtr<AMEAlarm>& Alarm : Alarms)
	{
		const float Distance = Alarm.IsValid() ? FVector::Dist(Alarm->GetActorLocation(), RunnerPawn->GetActorLocation()) : TNumericLimits<float>::Max();
		if (Distance < NearestDistance)
		{
			Nearest = Alarm.Get();
			NearestDistance = Distance;
		}
	}
	if (Nearest)
	{
		AlarmRunner = Runner;
	}
	return Nearest;
}

void UMESquadSubsystem::ReleaseAlarm(AMEAIController* Runner)
{
	if (AlarmRunner.Get() == Runner)
	{
		AlarmRunner.Reset();
	}
}

void UMESquadSubsystem::AlarmRaised(const FVector& ThreatLocation)
{
	bAlarmRaised = true;
	bStationOnEdge = true;
	AlarmRunner.Reset();
	for (TActorIterator<AMEAIController> It(GetWorld()); It; ++It)
	{
		It->HearCallout(ThreatLocation, true);
	}
}

// --- Deaths ---

void UMESquadSubsystem::EnemyDied(AMEAICharacter* Dead, bool bSeen)
{
	if (bSeen || !Dead)
	{
		return;
	}

	// Nobody saw it: the colleagues nearby notice the silence later
	const FVector Location = Dead->GetActorLocation();
	for (TActorIterator<AMEAIController> It(GetWorld()); It; ++It)
	{
		const APawn* Pawn = It->GetPawn();
		if (!Pawn || Pawn == Dead || FVector::Dist(Pawn->GetActorLocation(), Location) > CheckInRange)
		{
			continue;
		}
		TWeakObjectPtr<AMEAIController> WeakColleague = *It;
		FTimerHandle Timer;
		GetWorld()->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [WeakColleague, Location]()
		{
			if (AMEAIController* Colleague = WeakColleague.Get())
			{
				Colleague->ColleagueSilent(Location);
			}
		}), CheckInDelay * FMath::FRandRange(0.8f, 1.2f), false);
	}
}

int32 UMESquadSubsystem::CountAlliesNear(const AMEAIController* Asker, float Radius) const
{
	const APawn* AskerPawn = Asker ? Asker->GetPawn() : nullptr;
	if (!AskerPawn)
	{
		return 0;
	}
	int32 Count = 0;
	for (TActorIterator<AMEAICharacter> It(GetWorld()); It; ++It)
	{
		if (*It != AskerPawn && !It->GetHealth()->IsDead() && FVector::DistSquared(It->GetActorLocation(), AskerPawn->GetActorLocation()) < FMath::Square(Radius))
		{
			++Count;
		}
	}
	return Count;
}
