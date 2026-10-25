#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "MEAIController.generated.h"

class UStateTreeAIComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class AMEAICharacter;
class AMEDoor;

/** How aware of the player an enemy is. The controller decides it from its senses; the StateTree follows it. */
UENUM()
enum class EMEAlertState : uint8
{
	/** Patrolling, or at its post. */
	Calm,
	/** Saw or heard something: stops, looks, goes to check, slowly. */
	Suspicious,
	/** Knows someone is there (spotted, shot at, a gunshot, a body): hurries to where it happened. */
	Alerted,
	/** Fighting the player (5.4). */
	Combat,
	/** Lost the trail: searches around the last known place, then calms down, wary for a while. */
	Searching
};

/**
 * Brain of a human enemy.
 * - Its senses fill a detection gauge and note noises (5.2); from them it decides the alert state.
 * - ST_Enemy_Human has one state per alert state and switches to the matching one; each runs an
 *   activity written here (patrol, investigate, hurry, search) through its C++ task.
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
	/** Where it last saw the player (valid once Detection has started filling). */
	FVector GetLastSeenLocation() const { return LastSeenLocation; }

	/** Last noise heard: where, what (MENoise tags) and how long ago. False if none yet. */
	bool GetLastNoise(FVector& OutLocation, FName& OutTag, float& OutAge) const;

	/** True if a point is in its vision cone, in range and not behind a wall. */
	bool CanSeeLocation(const FVector& Location, const AActor* Target = nullptr) const;

	/** Another enemy died there, in its sight: it knows something is wrong. */
	void NotifyAllyDown(const FVector& Location);

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

	// --- Hearing: noises (MENoise) carry their own loudness, scaling this range ---

	UPROPERTY(EditDefaultsOnly, Category = "Perception|Hearing", meta = (ClampMin = "0", Units = "cm"))
	float HearingRange = 3000.f;

	// --- Alert ---

	/** Seeing the player fills the gauge past this: suspicious. Full: alerted. */
	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", ClampMax = "1"))
	float SuspicionThreshold = 0.35f;

	/** Suspicious: stands and stares this long before going to check. */
	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", Units = "s"))
	float ReactionTime = 1.f;

	/** Looks around this long at the place it checks. */
	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", Units = "s"))
	float LookAroundTime = 4.f;

	/** Back to calm after a scare: detection faster for this long. */
	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "0", Units = "s"))
	float WaryTime = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Alert", meta = (ClampMin = "1"))
	float WaryDetectionScale = 1.5f;

	/** Moving speeds: checking something, hurrying when alerted, searching. */
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

	void UpdateDetection(float DeltaSeconds);

	/** Decides the alert state from what it sensed since last frame. */
	void UpdateAlert();
	void BecomeSuspicious(const FVector& Location);
	void RaiseAlert(const FVector& Location);
	void CalmDown();
	void SetAlertState(EMEAlertState NewState);

	/** Doors seen open that were closed (or the opposite) make it suspicious. */
	void CheckDoors();

	// Activities
	void StartPatrol();
	void StopPatrol();
	void MoveToPatrolPoint();
	void ArriveAtPatrolPoint();
	void NextPatrolPoint();

	void StartInvestigating();
	void GoCheck();
	void StartHurrying();
	void StartSearching();
	void NextSearchPoint();
	void LookAround(float Duration);
	void UpdateLookAround();
	void FinishLookAround();

	/** Moves to the alert target; reaching it (or failing to) calls ArriveAtTarget. */
	void MoveToTarget(float AcceptanceRadius);
	void ArriveAtTarget();

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
	TWeakObjectPtr<APawn> SeenPlayer;
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
	bool bSawAllyDown = false;
	FVector AllyDownLocation = FVector::ZeroVector;

	TMap<TWeakObjectPtr<AMEDoor>, bool> KnownDoors;
	float DoorCheckTimer = 0.f;

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

	bool bPatrolling = false;
	/** The first patrol starts at the point set in the level; later ones resume at the nearest. */
	bool bHasPatrolled = false;
	bool bWaiting = false;
	bool bPatrolForward = true;
	int32 PatrolIndex = 0;
	FTimerHandle PatrolTimer;
};
