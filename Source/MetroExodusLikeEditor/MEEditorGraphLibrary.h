#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MEEditorGraphLibrary.generated.h"

class UBlueprint;

/**
 * Lets editor scripts (Python) wire Blueprint / AnimBP graphs.
 * Nodes are pasted from text (links to existing nodes are dropped on paste); these functions connect them.
 */
UCLASS()
class UMEEditorGraphLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Connects FromNode.FromPin to ToNode.ToPin in the named graph (replacing the links an input pin
	 * can only have once) and marks the Blueprint modified. Returns false if a node or pin is missing
	 * or the schema refuses the connection.
	 */
	UFUNCTION(BlueprintCallable, Category = "ME Editor|Graphs")
	static bool LinkPins(UBlueprint* Blueprint, FName GraphName, FName FromNode, FName FromPin, FName ToNode, FName ToPin);

	/** Names of the nodes of the named graph. */
	UFUNCTION(BlueprintCallable, Category = "ME Editor|Graphs")
	static TArray<FName> GetNodeNames(UBlueprint* Blueprint, FName GraphName);
};
