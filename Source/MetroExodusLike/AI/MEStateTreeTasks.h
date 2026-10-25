#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeConditionBase.h"
#include "AI/MEAIController.h"
#include "MEStateTreeTasks.generated.h"

class AAIController;

/**
 * StateTree nodes of the human enemies. ST_Enemy_Human has one state per alert state:
 * a condition switches to the state matching the controller's alert state, and the state's task
 * runs the matching activity of AMEAIController. The tree decides which, the controller does it.
 */

USTRUCT()
struct FMEStateTreeTask_AlertActivityInstanceData
{
	GENERATED_BODY()

	/** Bound automatically to the tree's AI controller. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EMEAlertState Activity = EMEAlertState::Calm;
};

/** Runs what the enemy does in an alert state (patrol, investigate, hurry, search) until the state is left. */
USTRUCT(meta = (DisplayName = "ME Alert Activity", Category = "MetroExodusLike"))
struct METROEXODUSLIKE_API FMEStateTreeTask_AlertActivity : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMEStateTreeTask_AlertActivityInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FMEStateTreeCondition_AlertStateInstanceData
{
	GENERATED_BODY()

	/** Bound automatically to the tree's AI controller. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EMEAlertState State = EMEAlertState::Calm;
};

/** True when the controller's alert state is State. */
USTRUCT(meta = (DisplayName = "ME Alert State Is", Category = "MetroExodusLike"))
struct METROEXODUSLIKE_API FMEStateTreeCondition_AlertState : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMEStateTreeCondition_AlertStateInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
