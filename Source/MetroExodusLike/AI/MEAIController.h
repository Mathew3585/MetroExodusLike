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

/** How aware of the player an enemy is. The StateTree states set it; perception and barks read it. */
UENUM()
enum class EMEAlertState : uint8
{
	Calm,
	Suspicious,
	Alerted,
	Combat,
	Searching
};

/**
 * Brain of a human enemy. The StateTree (ST_Enemy_Human) picks what to do; the actions themselves
 * (patrolling, later investigating, taking cover...) are written here and called by its C++ tasks.
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
	void SetAlertState(EMEAlertState NewState) { AlertState = NewState; }

	/** Walks the pawn's patrol path, waiting at its points. Without a path, stands still. */
	void StartPatrol();
	void StopPatrol();

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

	/** Stops this close to a patrol point. */
	UPROPERTY(EditDefaultsOnly, Category = "Patrol", meta = (ClampMin = "0", Units = "cm"))
	float PatrolAcceptanceRadius = 40.f;

	/** Time before retrying a point it could not reach. */
	UPROPERTY(EditDefaultsOnly, Category = "Patrol", meta = (ClampMin = "0", Units = "s"))
	float PatrolRetryDelay = 2.f;

private:
	UFUNCTION()
	void HandlePerception(AActor* Actor, FAIStimulus Stimulus);

	void UpdateDetection(float DeltaSeconds);

	void MoveToPatrolPoint();
	void ArriveAtPatrolPoint();
	void NextPatrolPoint();

	/** Turns the body toward where it walks, or toward a location. */
	void FaceMovement();
	void FaceLocation(const FVector& Location);

	UPROPERTY(Transient)
	TObjectPtr<AMEAICharacter> Enemy;

	EMEAlertState AlertState = EMEAlertState::Calm;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	TWeakObjectPtr<APawn> SeenPlayer;
	float Detection = 0.f;
	double LastSeenTime = -1.0;
	FVector LastSeenLocation = FVector::ZeroVector;

	double LastNoiseTime = -1.0;
	FVector LastNoiseLocation = FVector::ZeroVector;
	FName LastNoiseTag;

	bool bPatrolling = false;
	bool bWaiting = false;
	bool bPatrolForward = true;
	int32 PatrolIndex = 0;
	FTimerHandle PatrolTimer;
};
