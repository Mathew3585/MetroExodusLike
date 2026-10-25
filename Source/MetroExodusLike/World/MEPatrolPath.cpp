#include "World/MEPatrolPath.h"
#include "Components/SplineComponent.h"

AMEPatrolPath::AMEPatrolPath()
{
	PrimaryActorTick.bCanEverTick = false;

	Path = CreateDefaultSubobject<USplineComponent>(TEXT("Path"));
	SetRootComponent(Path);
}

void AMEPatrolPath::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// The drawn curve shows the route: closed for a loop
	Path->SetClosedLoop(Mode == EMEPatrolMode::Loop && Path->GetNumberOfSplinePoints() > 2);

	// One settings entry per point, so every point shows up in the details panel
	if (Points.Num() < Path->GetNumberOfSplinePoints())
	{
		Points.SetNum(Path->GetNumberOfSplinePoints());
	}
}

int32 AMEPatrolPath::GetNumPoints() const
{
	return Path->GetNumberOfSplinePoints();
}

FVector AMEPatrolPath::GetPointLocation(int32 Index) const
{
	return Path->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
}

FMEPatrolPoint AMEPatrolPath::GetPoint(int32 Index) const
{
	return Points.IsValidIndex(Index) ? Points[Index] : FMEPatrolPoint();
}

bool AMEPatrolPath::GetPointFacing(int32 Index, FRotator& OutFacing) const
{
	if (!GetPoint(Index).bFaceTangent)
	{
		return false;
	}
	const FVector Tangent = Path->GetTangentAtSplinePoint(Index, ESplineCoordinateSpace::World).GetSafeNormal2D();
	if (Tangent.IsNearlyZero())
	{
		return false;
	}
	OutFacing = Tangent.Rotation();
	return true;
}

int32 AMEPatrolPath::GetNextIndex(int32 Index, bool& bForward) const
{
	const int32 Num = GetNumPoints();
	if (Num <= 1)
	{
		return 0;
	}
	if (Mode == EMEPatrolMode::Loop)
	{
		return (Index + 1) % Num;
	}

	if ((bForward && Index >= Num - 1) || (!bForward && Index <= 0))
	{
		bForward = !bForward;
	}
	return Index + (bForward ? 1 : -1);
}
