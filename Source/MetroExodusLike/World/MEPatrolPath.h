#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MEPatrolPath.generated.h"

class USplineComponent;

UENUM()
enum class EMEPatrolMode : uint8
{
	/** Last point back to the first. */
	Loop,
	/** There and back along the same points. */
	PingPong
};

/** What a patroller does at one point of the path. */
USTRUCT(BlueprintType)
struct FMEPatrolPoint
{
	GENERATED_BODY()

	/** Stop here this long. 0 = walk past, negative = stay for good (a guard post). */
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (Units = "s"))
	float WaitTime = 2.f;

	/** While waiting, face along the spline at this point (drag its tangent to aim). */
	UPROPERTY(EditAnywhere, Category = "Patrol")
	bool bFaceTangent = true;
};

/**
 * A patrol route placed in the level: the spline points are the stops, the patroller walks
 * from one to the next on the navmesh (it does not follow the curve exactly).
 * Several enemies can share one path; a single point is a fixed post.
 */
UCLASS()
class METROEXODUSLIKE_API AMEPatrolPath : public AActor
{
	GENERATED_BODY()

public:
	AMEPatrolPath();

	virtual void OnConstruction(const FTransform& Transform) override;

	int32 GetNumPoints() const;
	FVector GetPointLocation(int32 Index) const;
	/** Facing while waiting at the point, if it has one. */
	bool GetPointFacing(int32 Index, FRotator& OutFacing) const;
	FMEPatrolPoint GetPoint(int32 Index) const;

	/** Next stop after Index. bForward flips at the ends of a ping-pong path. */
	int32 GetNextIndex(int32 Index, bool& bForward) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Patrol")
	TObjectPtr<USplineComponent> Path;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	EMEPatrolMode Mode = EMEPatrolMode::Loop;

	/** One entry per spline point, in order. Missing entries use the defaults. */
	UPROPERTY(EditAnywhere, Category = "Patrol")
	TArray<FMEPatrolPoint> Points;
};
