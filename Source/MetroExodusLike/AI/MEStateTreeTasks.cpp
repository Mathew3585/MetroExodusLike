#include "AI/MEStateTreeTasks.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMEStateTreeTask_AlertActivity::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	AMEAIController* Controller = Cast<AMEAIController>(Data.AIController);
	if (!Controller)
	{
		return EStateTreeRunStatus::Failed;
	}
	Controller->BeginActivity(Data.Activity);
	// Never ends by itself: the tree leaves the state when the alert state changes
	return EStateTreeRunStatus::Running;
}

void FMEStateTreeTask_AlertActivity::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (AMEAIController* Controller = Cast<AMEAIController>(Context.GetInstanceData(*this).AIController))
	{
		Controller->EndActivity();
	}
}

bool FMEStateTreeCondition_AlertState::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	const AMEAIController* Controller = Cast<AMEAIController>(Data.AIController);
	return Controller && Controller->GetAlertState() == Data.State;
}
