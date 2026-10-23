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
