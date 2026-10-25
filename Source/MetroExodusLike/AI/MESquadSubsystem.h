#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MESquadSubsystem.generated.h"

class AMEAIController;
class AMEAICharacter;
class AMEAlarm;

/**
 * What the enemies of a level share: who may shoot right now (attack tickets), who flanks,
 * shouted warnings, the station alarm, and colleagues who stopped answering.
 */
UCLASS()
class METROEXODUSLIKE_API UMESquadSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- Fight ---

	void JoinFight(AMEAIController* Controller);
	void LeaveFight(AMEAIController* Controller);

	/** At most MaxShooters fire at once: the others duck, move, wait. True if this one may shoot now. */
	bool RequestAttackTicket(AMEAIController* Controller);
	void ReleaseAttackTicket(AMEAIController* Controller);

	/** With two fighters or more, one of them (the boldest) works around the player's side. */
	bool IsFlanker(const AMEAIController* Controller) const;

	/** Middle of the fighters, to judge what "the side" is. */
	FVector GetFightCenter() const;

	/** Cover spots taken by fighters, so two do not pick the same. */
	bool IsCoverTaken(const FVector& Location, const AMEAIController* Asker) const;
	void TakeCover(AMEAIController* Controller, const FVector& Location);

	// --- Voice ---

	/**
	 * Speaker shouts about Location: the others within range hear it after a short delay
	 * (shorter range through walls). bFight: "he is here", not just "something is wrong".
	 */
	void Callout(AMEAIController* Speaker, const FVector& Location, bool bFight);

	// --- Alarm ---

	void RegisterAlarm(AMEAlarm* Alarm);

	/** The nearest alarm in range for this enemy to run to, if nobody else is already going. */
	AMEAlarm* ClaimAlarm(AMEAIController* Runner, float Range);
	void ReleaseAlarm(AMEAIController* Runner);

	/** Rung: every enemy of the level comes, toward where the threat was. */
	void AlarmRaised(const FVector& ThreatLocation);
	bool IsAlarmRaised() const { return bAlarmRaised; }

	// --- Station mood ---

	/** After a real incident (fight, body, wound, alarm) nobody here goes back to calm: they stay wary. */
	void SetStationOnEdge() { bStationOnEdge = true; }
	bool IsStationOnEdge() const { return bStationOnEdge; }

	// --- Deaths ---

	/** An enemy died. Unseen: its colleagues nearby call it a while later and come check when it does not answer. */
	void EnemyDied(AMEAICharacter* Dead, bool bSeen);

	/** How many enemies still alive near a place (morale: alone is scary). */
	int32 CountAlliesNear(const AMEAIController* Asker, float Radius) const;

private:
	static constexpr int32 MaxShooters = 2;
	static constexpr float CalloutRange = 2500.f;
	static constexpr float CheckInRange = 3000.f;
	static constexpr float CheckInDelay = 20.f;

	TArray<TWeakObjectPtr<AMEAIController>> Fighters;
	TArray<TWeakObjectPtr<AMEAIController>> Shooters;
	TMap<TWeakObjectPtr<AMEAIController>, FVector> Covers;

	TArray<TWeakObjectPtr<AMEAlarm>> Alarms;
	TWeakObjectPtr<AMEAIController> AlarmRunner;
	bool bAlarmRaised = false;
	bool bStationOnEdge = false;

	void Cleanup();
};
