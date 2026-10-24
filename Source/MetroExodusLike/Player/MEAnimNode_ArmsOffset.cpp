#include "Player/MEAnimNode_ArmsOffset.h"

FMEAnimNode_ArmsOffset::FMEAnimNode_ArmsOffset()
{
	Bones = { FBoneReference(TEXT("clavicle_l")), FBoneReference(TEXT("clavicle_r")), FBoneReference(TEXT("ik_hand_gun")) };
	PivotBone = FBoneReference(TEXT("ik_hand_gun"));
}

void FMEAnimNode_ArmsOffset::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	const FVector Pivot = Output.Pose.GetComponentSpaceTransform(PivotBone.GetCompactPoseIndex(BoneContainer)).GetLocation();

	// Around the pivot: x -> R (x - P) + P + T
	const FTransform Move = FTransform(-Pivot) * FTransform(Offset.GetRotation(), Pivot + Offset.GetTranslation());

	for (const FBoneReference& Bone : Bones)
	{
		if (Bone.IsValidToEvaluate(BoneContainer))
		{
			const FCompactPoseBoneIndex Index = Bone.GetCompactPoseIndex(BoneContainer);
			OutBoneTransforms.Add(FBoneTransform(Index, Output.Pose.GetComponentSpaceTransform(Index) * Move));
		}
	}

	// Parents before children
	OutBoneTransforms.Sort(FCompareBoneTransformIndex());
}

bool FMEAnimNode_ArmsOffset::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	if (!PivotBone.IsValidToEvaluate(RequiredBones))
	{
		return false;
	}
	for (const FBoneReference& Bone : Bones)
	{
		if (Bone.IsValidToEvaluate(RequiredBones))
		{
			return true;
		}
	}
	return false;
}

void FMEAnimNode_ArmsOffset::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	PivotBone.Initialize(RequiredBones);
	for (FBoneReference& Bone : Bones)
	{
		Bone.Initialize(RequiredBones);
	}
}
