#pragma once

#include "CoreMinimal.h"
#include "BoneContainer.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "MEAnimNode_ArmsOffset.generated.h"

/**
 * Moves the arms and the weapon as one rigid block: rotation around a pivot bone, then translation,
 * applied to the roots of the arms and the gun bone (component space). The head (camera) stays.
 * Used for procedural weapon motion: recoil kick, sway.
 */
USTRUCT(BlueprintInternalUseOnly)
struct METROEXODUSLIKE_API FMEAnimNode_ArmsOffset : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

	/** Offset in component space: rotation around the pivot, then translation. */
	UPROPERTY(EditAnywhere, Category = "Offset", meta = (PinShownByDefault))
	FTransform Offset;

	/** Bones moved: the roots of everything that must stay together (clavicles, gun bone). */
	UPROPERTY(EditAnywhere, Category = "Offset")
	TArray<FBoneReference> Bones;

	/** The rotation turns around this bone (the grip): the weapon pivots in the hands. */
	UPROPERTY(EditAnywhere, Category = "Offset")
	FBoneReference PivotBone;

	FMEAnimNode_ArmsOffset();

protected:
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
};
