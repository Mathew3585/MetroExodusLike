#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "AI/MEEnemyProfile.h"
#include "MEAIController.generated.h"

class UStateTreeAIComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UMESquadSubsystem;
class UAnimSequenceBase;
class AMEAICharacter;
class AMEDoor;
class AMELamp;
class AMEAlarm;

/** How aware of the player an enemy is, and what it makes of it. The controller decides; the StateTree follows. */
UENUM()
enum class EMEAlertState : uint8
{
	/** Patrolling, or at its post. */
	Calm,
	/** Something happened here: patrols slowly, stops to look around at every point, quicker to notice.
	 *  For good once the station is on edge (a body, a fight, the alarm), else for a while. */
	Wary,
	/** Saw or heard something: stops, looks, goes to check, slowly. */
	Suspicious,
	/** Knows someone is there (shot at, a gunshot, a body, a shout): hurries to where it happened. */
	Alerted,
	/** Sees the player: cover, bursts, flanking. */
	Combat,
	/** Lost the trail: searches around the last known place, then calms down, wary for a while. */
	Searching,
	/** Morale broken: runs away from the player and hides. */
	Fleeing,
	/** Gave up: weapon dropped, does nothing more. The player decides. */
	Surrendered
};

/**
 * Brain of a human enemy.
 * - Its senses fill a detection gauge and note noises; with what it is told (shouts, the alarm,
 *   a silent colleague) and its morale, it decides the alert state.
 * - ST_Enemy_Human has one state per alert state; each runs the matching activity written here
 *   (patrol, investigate, hurry, fight, search, flee, surrender) through its C++ task.
 */
UCLASS()
class METROEXODUSLIKE_API AMEAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMEAIController();

	virtual void Tick(float DeltaSeconds) override;

	EMEAlertState GetAlertState() const { return AlertState; }

	/** 0..1: how sure it is the player is there. Fills while it sees the player, 1 = spotted. */
	float GetDetection() const { return Detection; }
	bool CanSeePlayer() const { return SeenPlayer.IsValid(); }

	/** The player is in its view cone, unhidden, but maybe too far in the dark to be seen. */
	bool HasPlayerInView() const { return InViewPlayer.IsValid(); }

	/** Range at which it can see the player right now (shrinks in the dark). */
	float GetPlayerSightDistance() const { return PlayerSightDistance; }

	/** Time the player's flashlight beam has caught its eye (it gets suspicious at BeamNoticeTime). */
	float GetBeamExposure() const { return BeamExposure; }
	/** Where it last saw the player (valid once Detection has started filling). */
	FVector GetLastSeenLocation() const { return LastSeenLocation; }

	/** Last noise heard: where, what (MENoise tags) and how long ago. False if none yet. */
	bool GetLastNoise(FVector& OutLocation, FName& OutTag, float& OutAge) const;

	/** 0..1: 1 = steady, low = about to run or give up. */
	float GetMorale() const { return Morale; }
	float GetAggression() const;

	/** True if a point is in its vision cone, in range and not behind a wall (Target itself does not block). */
	bool CanSeeLocation(const FVector& Location, const AActor* Target = nullptr) const;

	/** Nothing between its eyes and the point, whatever the direction it looks (Target itself does not block). */
	bool HasLineOfSight(const FVector& Location, const AActor* Target = nullptr) const;

	/** Another enemy died in its sight. */
	void NotifyAllyDown(AMEAICharacter* Ally);

	/** A colleague shouted about Location (bFight: "he is here"), or the alarm rang. */
	void HearCallout(const FVector& Location, bool bFight);

	/** A colleague nearby stopped answering: it goes to see where he was. */
	void ColleagueSilent(const FVector& Location);

	/** The fight is over for it (dead or knocked out): the StateTree stops. */
	void StopBrain();

	/** Starts / stops what it does in an alert state (called by the StateTree task of that state). */
	void BeginActivity(EMEAlertState Activity);
	void EndActivity();

	/** What it is doing right now, for the debug panel. */
	FString GetActivityDescription() const;

	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTree;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> Perception;

	// --- Sight: a cone, then a detection gauge so the player has time to hide ---

	UPROPERTY(EditDefaultsOnly, Category = "Perception|Sight", meta = (ClampMin = "0", Units = "cm"))
	float SightRadius = 2500.f;

	/** Once seen, the player stays in sight up to this distance. */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Sight", meta = (ClampMin = "0", Units = "cm"))
	float LoseSightRadius = 3000.f;

	/** Half the vision cone. */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Sight", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float SightHalfAngle = 50.f;

	/** Time to spot a player standing close, in the open, in front of it. Everything else is slower. */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Detection", meta = (ClampMin = "0.05", Units = "s"))
	float FullDetectionTime = 1.2f;

	/** Closer than this, distance does not slow the detection. */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Detection", meta = (ClampMin = "0", Units = "cm"))
	float NearDistance = 500.f;

	/** Detection speed at the edge of sight (far away) and at the edge of the cone (side vision). */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Detection", meta = (ClampMin = "0", ClampMax = "1"))
	float FarScale = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Perception|Detection", meta = (ClampMin = "0", ClampMax = "1"))
	float PeripheralScale = 0.4f;

	/** Detection speed by what the player does. */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Detection", meta = (ClampMin = "0"))
	float CrouchScale = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Perception|Detection", meta = (ClampMin = "0"))
	float StillScale = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Perception|Detection", meta = (ClampMin = "0"))
	float SprintScale = 1.5f;

	/** Out of sight, the gauge holds this long, then empties at this speed (per second). */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Detection", meta = (ClampMin = "0", Units = "s"))
	float DetectionHoldTime = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Perception|Detection", meta = (ClampMin = "0"))
	float DetectionDecay = 0.15f;

	// --- Light: the player's visibility (UMEVisibilityComponent) ---

	/** Range at which it sees a player in pitch dark; fully lit, it is SightRadius (LoseSightRadius once seen). */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Light", meta = (ClampMin = "0", Units = "cm"))
	float DarkSightDistance = 400.f;

	/** Detection speed for a player in pitch dark (fully lit: 1). */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Light", meta = (ClampMin = "0", ClampMax = "1"))
	float DarkDetectionScale = 0.25f;

	/** Closer than this it sees the player whatever the light (bumping into him). */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Light", meta = (ClampMin = "0", Units = "cm"))
	float TouchDistance = 150.f;

	/** The player's flashlight beam (pointed at it, or its spot in its view) caught this long: suspicious. */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Light", meta = (ClampMin = "0", Units = "s"))
	float BeamNoticeTime = 0.5f;

	/** The beam counts this far from the flashlight (pointed at it) or from it (a lit spot it looks at). */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Light", meta = (ClampMin = "0", Units = "cm"))
	float BeamNoticeDistance = 2500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Perception|Light", meta = (ClampMin = "0", Units = "cm"))
	float BeamSpotNoticeDistance = 1500.f;

	/** After noticing a beam, it does not react to another for this long. */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Light", meta = (ClampMin = "0", Units = "s"))
	float BeamNoticeCooldown = 3.f;

	// --- Hearing: noises (MENoise) carry their own loudness, scaling this range ---

	UPROPERTY(EditDefaultsOnly, Category = "Perception|Hearing", meta = (ClampMin = "0", Units = "cm"))
	float HearingRange = 3000.f;

	// --- Alert ---

	/** Seeing the player fills the gauge past this: suspicious. Full: fight. */
	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", ClampMax = "1"))
	float SuspicionThreshold = 0.35f;

	/** Looks around this long at the place it checks. */
	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", Units = "s"))
	float LookAroundTime = 4.f;

	/** After a false alarm it stays wary this long (after a real incident, for good). */
	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", Units = "s"))
	float WaryTime = 60.f;

	/** On edge (wary or worse): detection faster, suspicious sooner, hears further. */
	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "1"))
	float WaryDetectionScale = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", ClampMax = "1"))
	float WarySuspicionThreshold = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "1"))
	float WaryHearingScale = 1.3f;

	/** Wary: looks around this long at each patrol point. */
	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", Units = "s"))
	float WaryLookTime = 2.5f;

	/** Moving speeds: checking something, hurrying, searching. */
	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", Units = "cm/s"))
	float InvestigateSpeed = 130.f;

	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", Units = "cm/s"))
	float AlertSpeed = 380.f;

	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", Units = "cm/s"))
	float SearchSpeed = 220.f;

	/** Searching: places checked around the last known one, within this radius. */
	UPROPERTY(EditDefaultsOnly, Category = "Alert|Search", meta = (ClampMin = "1"))
	int32 SearchPoints = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Alert|Search", meta = (ClampMin = "0", Units = "cm"))
	float SearchRadius = 900.f;

	/** Looks around this long at each searched place. */
	UPROPERTY(EditDefaultsOnly, Category = "Alert|Search", meta = (ClampMin = "0", Units = "s"))
	float SearchLookTime = 2.f;

	/** Time spent crouched over a body it found before searching around it. */
	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", Units = "s"))
	float ExamineBodyTime = 3.f;

	/** Shot without seeing the shooter: it guesses the direction within this angle (and the distance badly). */
	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", ClampMax = "90", Units = "Degrees"))
	float ShotDirectionError = 25.f;

	/** Noises are placed with an error growing with distance (share of the distance). */
	UPROPERTY(EditDefaultsOnly, Category = "Perception|Hearing", meta = (ClampMin = "0", ClampMax = "1"))
	float NoiseLocationError = 0.15f;

	/** Runs to an alarm closer than this when it is alerted. */
	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", Units = "cm"))
	float AlarmRange = 3000.f;

	// --- Combat ---

	/** Player out of sight this long: the fight is over, it goes to look where he was. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0", Units = "s"))
	float LoseTargetTime = 4.f;

	/** Keeps between these distances to the player: a cautious enemy at the far end, an aggressive one close. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0", Units = "cm"))
	float CloseRange = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0", Units = "cm"))
	float FarRange = 2000.f;

	/** Looks for cover this far around it. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0", Units = "cm"))
	float CoverSearchRadius = 1500.f;

	/** Ducked in cover between bursts, between these (shorter when aggressive). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0", Units = "s"))
	float DuckTimeMin = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0", Units = "s"))
	float DuckTimeMax = 2.5f;

	/** Changes cover after this many bursts from the same one. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "1"))
	int32 BurstsPerCover = 3;

	/** Fires at where the player was for this long after losing sight of him (pins him down). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0", Units = "s"))
	float SuppressTime = 2.5f;

	// --- Morale ---

	/** Below this it runs; below that (and close to the player) it gives up, if its profile can. */
	UPROPERTY(EditDefaultsOnly, Category = "Morale", meta = (ClampMin = "0", ClampMax = "1"))
	float FleeMorale = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Morale", meta = (ClampMin = "0", ClampMax = "1"))
	float SurrenderMorale = 0.12f;

	UPROPERTY(EditDefaultsOnly, Category = "Morale", meta = (ClampMin = "0", Units = "cm"))
	float SurrenderDistance = 1200.f;

	/** Morale lost when a colleague falls (a brave one loses less). */
	UPROPERTY(EditDefaultsOnly, Category = "Morale", meta = (ClampMin = "0", ClampMax = "1"))
	float AllyDownMoraleLoss = 0.35f;

	/** Morale lost per share of its health taken. */
	UPROPERTY(EditDefaultsOnly, Category = "Morale", meta = (ClampMin = "0"))
	float WoundMoraleLoss = 0.8f;

	/** Morale regained per second away from the fight. */
	UPROPERTY(EditDefaultsOnly, Category = "Morale", meta = (ClampMin = "0"))
	float MoraleRecovery = 0.02f;

	// --- Wounds ---

	/** Hit in the legs: moves this much slower for good. */
	UPROPERTY(EditDefaultsOnly, Category = "Wounds", meta = (ClampMin = "0.1", ClampMax = "1"))
	float LimpSpeedScale = 0.6f;

	/** Below this share of health it calls for help (once). */
	UPROPERTY(EditDefaultsOnly, Category = "Wounds", meta = (ClampMin = "0", ClampMax = "1"))
	float CallForHelpHealth = 0.4f;

	// --- Patrol ---

	/** Stops this close to a patrol point. */
	UPROPERTY(EditDefaultsOnly, Category = "Patrol", meta = (ClampMin = "0", Units = "cm"))
	float PatrolAcceptanceRadius = 40.f;

	/** Time before retrying a point it could not reach. */
	UPROPERTY(EditDefaultsOnly, Category = "Patrol", meta = (ClampMin = "0", Units = "s"))
	float PatrolRetryDelay = 2.f;

private:
	UFUNCTION()
	void HandlePerception(AActor* Actor, FAIStimulus Stimulus);

	void HandleHealthChanged(float Health, float Delta, AController* DamageInstigator);

	const UMEEnemyProfile* GetProfile() const;
	UMESquadSubsystem* GetSquad() const;

	void UpdateDetection(float DeltaSeconds);
	/** Filters the player in view by the light on him: SeenPlayer is who it really sees. */
	void UpdatePlayerVisible();
	void UpdateBeamNotice(float DeltaSeconds);
	bool IsCaughtByBeam(const class AMECharacter& Player) const;
	void UpdateMorale(float DeltaSeconds);

	/** Decides the alert state from what it sensed and was told since last frame. */
	void UpdateAlert();
	void BecomeSuspicious(const FVector& Location);
	void RaiseAlert(const FVector& Location);
	void StartFight();
	/** The alert is over: wary (for good if the station is on edge), never straight back to calm. */
	void EndAlert();
	/** A real incident (fight, body, wound): the whole station stays on edge. */
	void PutStationOnEdge();

	/** Found a body: goes to it, examines it, warns the others, then searches around it. */
	void CheckBody(const FVector& Location);
	void FinishExaminingBody();

	/** Where it thinks an unseen shooter is: the right direction, roughly, at a guessed distance. */
	FVector GuessShooterLocation(const FVector& ShooterLocation) const;
	void SetAlertState(EMEAlertState NewState);

	/** Doors that changed, bodies lying around: things it notices while looking. */
	void CheckSurroundings();

	void Bark(EMEBark Line);
	void Shout(const FVector& Location, bool bFight);

	// Activities
	void StartPatrol();
	void StopPatrol();
	void MoveToPatrolPoint();
	void ArriveAtPatrolPoint();
	void NextPatrolPoint();
	void PlayRoutine(UAnimSequenceBase* Routine);
	void StopRoutine();

	void StartInvestigating();
	void GoCheck();
	void StartHurrying();
	void StartSearching();
	void NextSearchPoint();
	void LookAround(float Duration);
	void UpdateLookAround();
	void FinishLookAround();

	void StartCombat();
	void UpdateCombat();
	/** Best spot around: hidden from the player when ducked, able to shoot when standing, at its range. */
	bool FindCover(FVector& OutCover) const;
	void MoveToCover();
	void Duck(bool bDuck);

	void StartFleeing();
	void StartSurrender();

	bool TryRunToAlarm();

	/** Moves to the alert target; reaching it (or failing to) calls ArriveAtTarget. */
	void MoveToTarget(float AcceptanceRadius);
	bool MoveToPoint(const FVector& Point, float AcceptanceRadius);
	void ArriveAtTarget();

	/** Speed for what it does, slower if it limps. */
	void SetMoveSpeed(float Speed);

	/** Turns the body toward where it walks, or toward a location. */
	void FaceMovement();
	void FaceLocation(const FVector& Location);

	UPROPERTY(Transient)
	TObjectPtr<AMEAICharacter> Enemy;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	EMEAlertState AlertState = EMEAlertState::Calm;
	/** Where the alert is about: what it saw, heard, or where the shot came from. */
	FVector AlertTarget = FVector::ZeroVector;
	double WaryUntil = -1.0;

	// Perception
	/** The player as the sight sense gives him: in the cone, not hidden. */
	TWeakObjectPtr<APawn> InViewPlayer;
	/** The player it really sees: in view, and lit enough for the distance. */
	TWeakObjectPtr<APawn> SeenPlayer;
	float PlayerSightDistance = 0.f;
	float BeamExposure = 0.f;
	double BeamNoticedTime = -100.0;
	/** The player, once sensed at least once (cover is judged from his eyes). */
	TWeakObjectPtr<APawn> KnownPlayer;
	float Detection = 0.f;
	double LastSeenTime = -1.0;
	FVector LastSeenLocation = FVector::ZeroVector;

	double LastNoiseTime = -1.0;
	FVector LastNoiseLocation = FVector::ZeroVector;
	FName LastNoiseTag;

	// Events since last frame, handled by UpdateAlert
	bool bNewNoise = false;
	bool bHurt = false;
	FVector HurtFrom = FVector::ZeroVector;
	bool bAllyDown = false;
	FVector AllyDownLocation = FVector::ZeroVector;
	bool bCalledOut = false;
	bool bCalloutFight = false;
	FVector CalloutLocation = FVector::ZeroVector;

	TMap<TWeakObjectPtr<AMEDoor>, bool> KnownDoors;
	/** Lamps seen, and whether they were lit then. */
	TMap<TWeakObjectPtr<AMELamp>, bool> KnownLamps;
	TSet<TWeakObjectPtr<AActor>> KnownBodies;
	float SurroundingsTimer = 0.f;

	// Morale and wounds
	float Morale = 1.f;
	bool bLimping = false;
	bool bCalledForHelp = false;
	double LastBarkTime = -10.0;

	// Activity
	EMEAlertState Activity = EMEAlertState::Calm;
	bool bActivityRunning = false;
	bool bMoving = false;
	FVector MoveGoal = FVector::ZeroVector;
	FTimerHandle ActivityTimer;

	bool bLookingAround = false;
	double LookAroundEnd = 0.0;
	double NextLookTurn = 0.0;
	int32 LookTurns = 0;
	FRotator LookBase = FRotator::ZeroRotator;

	int32 SearchPointsLeft = 0;
	FVector SearchCenter = FVector::ZeroVector;

	// Combat
	enum class ECombatStep : uint8 { ToCover, Ducked, Shooting };
	ECombatStep CombatStep = ECombatStep::Ducked;
	bool bHasCover = false;
	double NextCombatAction = 0.0;
	int32 BurstsFromCover = 0;
	bool bFirstShot = true;
	/** Shooting step: stood up, the burst starts at NextCombatAction. */
	bool bBurstStarted = false;

	TWeakObjectPtr<AMEAlarm> AlarmToRaise;

	/** Going to a body it found / crouched over it. Only a direct threat interrupts it. */
	bool bCheckingBody = false;
	bool bExaminingBody = false;
	FVector BodyToCheck = FVector::ZeroVector;

	bool bRoutinePlaying = false;

	bool bPatrolling = false;
	/** The first patrol starts at the point set in the level; later ones resume at the nearest. */
	bool bHasPatrolled = false;
	bool bWaiting = false;
	bool bPatrolForward = true;
	int32 PatrolIndex = 0;
	FTimerHandle PatrolTimer;
};
