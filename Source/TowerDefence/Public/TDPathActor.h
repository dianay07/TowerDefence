#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDPathActor.generated.h"

class USplineComponent;

UCLASS()
class TOWERDEFENCE_API ATDPath : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Path")
	TObjectPtr<USplineComponent> SplineComponent;
	
public:
	ATDPath();

	/** 스플라인을 NumPoints 등분하여 월드 좌표 배열 반환. WaveManager에서 InitializePath() 호출 시 사용 */
	UFUNCTION(BlueprintCallable, Category = "Path")
	TArray<FVector> GetBakedWaypoints(int32 NumPoints = 20) const;

	/** 스플라인 전체 길이 반환 */
	UFUNCTION(BlueprintPure, Category = "Path")
	float GetLength() const;

	/** 스플라인 시작점으로부터 Distance만큼 떨어진 월드 좌표 반환 */
	UFUNCTION(BlueprintPure, Category = "Path")
	FVector GetLocation(float Distance) const;
};
