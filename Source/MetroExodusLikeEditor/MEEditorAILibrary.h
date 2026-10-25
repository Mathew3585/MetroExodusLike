#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MEEditorAILibrary.generated.h"

class UStateTree;
class AAIController;

/**
 * Lets editor scripts (Python) build the enemies' StateTree: the asset factory needs a schema
 * picked in a dialog and the tasks are C++ structs, neither reachable from Python.
 */
UCLASS()
class UMEEditorAILibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Creates (or rebuilds) the human enemy StateTree at PackagePath/AssetName on the AI component schema:
	 * Root > Calm (ME Patrol). Compiles it; returns null if the compile fails (see the log).
	 */
	UFUNCTION(BlueprintCallable, Category = "ME Editor|AI")
	static UStateTree* BuildEnemyStateTree(const FString& PackagePath, const FString& AssetName, TSubclassOf<AAIController> ControllerClass, TSubclassOf<AActor> ActorClass);
};
