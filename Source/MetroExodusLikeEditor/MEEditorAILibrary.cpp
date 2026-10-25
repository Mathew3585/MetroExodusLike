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

	// Rebuilt from scratch: the layout lives here, the tree asset is only its result
	Data->SubTrees.Reset();
	UStateTreeState& Root = Data->AddRootState();
	UStateTreeState& Calm = Root.AddChildState(TEXT("Calm"));
	Calm.AddTask<FMEStateTreeTask_Patrol>();

	FStateTreeCompilerLog Log;
	const bool bCompiled = UStateTreeEditingSubsystem::CompileStateTree(Tree, Log);
	Log.DumpToLog(LogTemp);
	Tree->MarkPackageDirty();
	return bCompiled ? Tree : nullptr;
}
