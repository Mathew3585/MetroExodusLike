#include "AI/MEStateTreeTasks.h"
#include "AI/MEAIController.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMEStateTreeTask_Patrol::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AMEAIController* Controller = Cast<AMEAIController>(Context.GetInstanceData(*this).AIController);
	if (!Controller)
	{
		return EStateTreeRunStatus::Failed;
	}
	Controller->SetAlertState(EMEAlertState::Calm);
	Controller->StartPatrol();
	// Patrolling never ends by itself: the tree leaves the state when something happens
	return EStateTreeRunStatus::Running;
}

void FMEStateTreeTask_Patrol::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (AMEAIController* Controller = Cast<AMEAIController>(Context.GetInstanceData(*this).AIController))
	{
		Controller->StopPatrol();
	}
}
