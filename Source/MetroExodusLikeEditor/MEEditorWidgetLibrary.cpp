#include "MEEditorWidgetLibrary.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Kismet2/BlueprintEditorUtils.h"

UWidgetTree* UMEEditorWidgetLibrary::GetWidgetTree(UWidgetBlueprint* Blueprint)
{
	return Blueprint ? Blueprint->WidgetTree.Get() : nullptr;
}

void UMEEditorWidgetLibrary::FinalizeWidgetTree(UWidgetBlueprint* Blueprint, UWidget* Root, const TArray<FName>& VariableNames)
{
	if (!Blueprint || !Blueprint->WidgetTree || !Root)
	{
		return;
	}

	Blueprint->Modify();
	Blueprint->WidgetTree->Modify();
	Blueprint->WidgetTree->RootWidget = Root;

	// Every widget needs a GUID in the Blueprint; BindWidget targets must be variables
	Blueprint->WidgetTree->ForEachWidget([Blueprint, &VariableNames](UWidget* Widget)
	{
		Widget->bIsVariable = VariableNames.Contains(Widget->GetFName());
		Blueprint->OnVariableAdded(Widget->GetFName());
	});

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}
