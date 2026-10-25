#include "MEEditorAILibrary.h"
#include "AI/MEStateTreeTasks.h"
#include "AIController.h"
#include "AssetToolsModule.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeFactory.h"
#include "StateTreeState.h"
#include "UObject/UnrealType.h"

namespace
{
	/** The schema's classes are protected properties, edited in the details panel: set them the same way. */
	void SetSchemaClass(UObject* Schema, const FName PropertyName, UClass* Value)
	{
		if (FClassProperty* Property = FindFProperty<FClassProperty>(Schema->GetClass(), PropertyName))
		{
			Property->SetObjectPropertyValue_InContainer(Schema, Value);
		}
	}
}

UStateTree* UMEEditorAILibrary::BuildEnemyStateTree(const FString& PackagePath, const FString& AssetName, TSubclassOf<AAIController> ControllerClass, TSubclassOf<AActor> ActorClass)
{
	UStateTree* Tree = LoadObject<UStateTree>(nullptr, *(PackagePath / AssetName + TEXT(".") + AssetName));
	if (!Tree)
	{
		UStateTreeFactory* Factory = NewObject<UStateTreeFactory>();
		Factory->SetSchemaClass(UStateTreeAIComponentSchema::StaticClass());
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		Tree = Cast<UStateTree>(AssetTools.CreateAsset(AssetName, PackagePath, UStateTree::StaticClass(), Factory));
	}
	UStateTreeEditorData* Data = Tree ? Cast<UStateTreeEditorData>(Tree->EditorData) : nullptr;
	if (!Data || !Data->Schema)
	{
		return nullptr;
	}

	Data->Modify();
	SetSchemaClass(Data->Schema, TEXT("AIControllerClass"), ControllerClass);
	SetSchemaClass(Data->Schema, TEXT("ContextActorClass"), ActorClass);

	// Rebuilt from scratch: the layout lives here, the tree asset is only its result.
	// One state per alert state; each one switches to another as soon as the controller's alert state says so.
	Data->SubTrees.Reset();
	UStateTreeState& Root = Data->AddRootState();
	const UEnum* AlertEnum = StaticEnum<EMEAlertState>();
	TArray<TPair<EMEAlertState, UStateTreeState*>> States;
	for (int32 Index = 0; Index < AlertEnum->NumEnums() - 1; ++Index)
	{
		const EMEAlertState Alert = static_cast<EMEAlertState>(AlertEnum->GetValueByIndex(Index));
		UStateTreeState& State = Root.AddChildState(FName(AlertEnum->GetNameStringByIndex(Index)));
		State.AddTask<FMEStateTreeTask_AlertActivity>().GetInstanceData().Activity = Alert;
		States.Emplace(Alert, &State);
	}
	for (const TPair<EMEAlertState, UStateTreeState*>& From : States)
	{
		for (const TPair<EMEAlertState, UStateTreeState*>& To : States)
		{
			if (From.Key != To.Key)
			{
				FStateTreeTransition& Transition = From.Value->AddTransition(EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, To.Value);
				Transition.AddConditionWithOuter<FMEStateTreeCondition_AlertState>(From.Value).GetInstanceData().State = To.Key;
			}
		}
	}

	FStateTreeCompilerLog Log;
	const bool bCompiled = UStateTreeEditingSubsystem::CompileStateTree(Tree, Log);
	Log.DumpToLog(LogTemp);
	Tree->MarkPackageDirty();
	return bCompiled ? Tree : nullptr;
}
