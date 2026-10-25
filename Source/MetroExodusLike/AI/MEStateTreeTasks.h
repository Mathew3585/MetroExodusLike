#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "MEStateTreeTasks.generated.h"

class AAIController;

/**
 * StateTree tasks of the human enemies. Each one only starts / stops an action of AMEAIController:
 * the tree (ST_Enemy_Human) decides when, the controller does it.
 */

USTRUCT()
struct FMEStateTreeTask_PatrolInstanceData
{
	GENERATED_BODY()

	/** Bound automatically to the tree's AI controller. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;
};

/** Calm: walks the patrol path (or stands at its post) until the state is left. */
USTRUCT(meta = (DisplayName = "ME Patrol", Category = "MetroExodusLike"))
struct METROEXODUSLIKE_API FMEStateTreeTask_Patrol : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMEStateTreeTask_PatrolInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
