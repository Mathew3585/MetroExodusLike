#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MEEditorWidgetLibrary.generated.h"

class UWidget;
class UWidgetBlueprint;
class UWidgetTree;

/**
 * Lets editor scripts (Python) build the layout of a Widget Blueprint.
 * Widgets are created and nested from Python; these functions do what Python cannot reach.
 */
UCLASS()
class UMEEditorWidgetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The designer tree of a Widget Blueprint: create new widgets with it as outer. */
	UFUNCTION(BlueprintCallable, Category = "ME Editor|Widgets")
	static UWidgetTree* GetWidgetTree(UWidgetBlueprint* Blueprint);

	/**
	 * Sets the root of the designer tree, registers every widget in it,
	 * flags the named ones as variables (needed for BindWidget) and marks the Blueprint modified.
	 */
	UFUNCTION(BlueprintCallable, Category = "ME Editor|Widgets")
	static void FinalizeWidgetTree(UWidgetBlueprint* Blueprint, UWidget* Root, const TArray<FName>& VariableNames);
};
