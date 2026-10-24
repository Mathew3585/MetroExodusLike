#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "Player/MEAnimNode_ArmsOffset.h"
#include "AnimGraphNode_MEArmsOffset.generated.h"

/** AnimGraph node of FMEAnimNode_ArmsOffset (the logic is in the runtime node). */
UCLASS()
class UAnimGraphNode_MEArmsOffset : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Settings")
	FMEAnimNode_ArmsOffset Node;

public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;

protected:
	virtual FText GetControllerDescription() const override;
	virtual const FAnimNode_SkeletalControlBase* GetNode() const override { return &Node; }
};
