#include "TDPathActor.h"
#include "Components/SplineComponent.h"

ATDPath::ATDPath()
{
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	SplineComponent->SetupAttachment(RootComponent);
}

TArray<FVector> ATDPath::GetBakedWaypoints(int32 NumPoints) const
{
	TArray<FVector> Waypoints;
	if (!SplineComponent || NumPoints <= 0) return Waypoints;

	const float SplineLength = SplineComponent->GetSplineLength();
	const float Step = SplineLength / NumPoints;

	for (int32 i = 0; i <= NumPoints; ++i)
	{
		const float Distance = Step * i;
		Waypoints.Add(SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World));
	}

	return Waypoints;
}

float ATDPath::GetLength() const
{
	return SplineComponent ? SplineComponent->GetSplineLength() : 0.f;
}

FVector ATDPath::GetLocation(float Distance) const
{
	if (!SplineComponent) return FVector::ZeroVector;
	return SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}
