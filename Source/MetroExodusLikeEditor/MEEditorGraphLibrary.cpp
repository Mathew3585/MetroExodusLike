#include "MEEditorGraphLibrary.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "Kismet2/BlueprintEditorUtils.h"

namespace
{
	UEdGraph* FindGraph(UBlueprint* Blueprint, FName GraphName)
	{
		if (!Blueprint)
		{
			return nullptr;
		}

		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		UEdGraph* const* Found = Graphs.FindByPredicate([GraphName](const UEdGraph* Graph) { return Graph->GetFName() == GraphName; });
		return Found ? *Found : nullptr;
	}

	UEdGraphPin* FindPin(UEdGraph* Graph, FName NodeName, FName PinName)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node && Node->GetFName() == NodeName)
			{
				return Node->FindPin(PinName);
			}
		}
		return nullptr;
	}
}

bool UMEEditorGraphLibrary::LinkPins(UBlueprint* Blueprint, FName GraphName, FName FromNode, FName FromPin, FName ToNode, FName ToPin)
{
	UEdGraph* Graph = FindGraph(Blueprint, GraphName);
	if (!Graph)
	{
		return false;
	}

	UEdGraphPin* From = FindPin(Graph, FromNode, FromPin);
	UEdGraphPin* To = FindPin(Graph, ToNode, ToPin);
	if (!From || !To)
	{
		return false;
	}

	Graph->Modify();
	const bool bLinked = Graph->GetSchema()->TryCreateConnection(From, To);
	if (bLinked)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}
	return bLinked;
}

bool UMEEditorGraphLibrary::RemoveNode(UBlueprint* Blueprint, FName GraphName, FName NodeName)
{
	UEdGraph* Graph = FindGraph(Blueprint, GraphName);
	const TObjectPtr<UEdGraphNode>* Found = Graph ? Graph->Nodes.FindByPredicate([NodeName](const UEdGraphNode* Node) { return Node && Node->GetFName() == NodeName; }) : nullptr;
	if (!Found)
	{
		return false;
	}

	// Same as deleting it in the graph editor: links broken, node destroyed
	Graph->Modify();
	FBlueprintEditorUtils::RemoveNode(Blueprint, *Found, true);
	return true;
}

TArray<FString> UMEEditorGraphLibrary::GetPinNames(UBlueprint* Blueprint, FName GraphName, FName NodeName)
{
	TArray<FString> Names;
	if (UEdGraph* Graph = FindGraph(Blueprint, GraphName))
	{
		for (const UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node && Node->GetFName() == NodeName)
			{
				for (const UEdGraphPin* Pin : Node->Pins)
				{
					Names.Add(FString::Printf(TEXT("%s (%s)"), *Pin->PinName.ToString(), Pin->Direction == EGPD_Input ? TEXT("in") : TEXT("out")));
				}
			}
		}
	}
	return Names;
}

TArray<FName> UMEEditorGraphLibrary::GetNodeNames(UBlueprint* Blueprint, FName GraphName)
{
	TArray<FName> Names;
	if (UEdGraph* Graph = FindGraph(Blueprint, GraphName))
	{
		for (const UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node)
			{
				Names.Add(Node->GetFName());
			}
		}
	}
	return Names;
}
