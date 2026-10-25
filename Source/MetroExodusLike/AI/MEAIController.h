#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MEAIController.generated.h"

class UStateTreeAIComponent;
class UAIPerceptionComponent;
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

	EMEAlertState GetAlertState() const { return AlertState; }
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

	/** Senses (sight, hearing): configured in 5.2. */
	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> Perception;

	/** Stops this close to a patrol point. */
	UPROPERTY(EditDefaultsOnly, Category = "Patrol", meta = (ClampMin = "0", Units = "cm"))
	float PatrolAcceptanceRadius = 40.f;

	/** Time before retrying a point it could not reach. */
	UPROPERTY(EditDefaultsOnly, Category = "Patrol", meta = (ClampMin = "0", Units = "s"))
	float PatrolRetryDelay = 2.f;

private:
	void MoveToPatrolPoint();
	void ArriveAtPatrolPoint();
	void NextPatrolPoint();

	/** Turns the body toward where it walks, or toward a location. */
	void FaceMovement();
	void FaceLocation(const FVector& Location);

	UPROPERTY(Transient)
	TObjectPtr<AMEAICharacter> Enemy;

	EMEAlertState AlertState = EMEAlertState::Calm;

	bool bPatrolling = false;
	bool bWaiting = false;
	bool bPatrolForward = true;
	int32 PatrolIndex = 0;
	FTimerHandle PatrolTimer;
};
