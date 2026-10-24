#include "AnimGraphNode_MEArmsOffset.h"

#define LOCTEXT_NAMESPACE "AnimGraphNode_MEArmsOffset"

FText UAnimGraphNode_MEArmsOffset::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FText UAnimGraphNode_MEArmsOffset::GetTooltipText() const
{
	return LOCTEXT("Tooltip", "Moves the arms and the weapon as one block around the grip (recoil, sway). The head stays.");
}

FText UAnimGraphNode_MEArmsOffset::GetControllerDescription() const
{
	return LOCTEXT("Title", "ME Arms Offset");
}

#undef LOCTEXT_NAMESPACE
