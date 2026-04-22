#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "TDLevelSessionSubsystem.generated.h"

class UWorld;
class UDataTable;

/**
 * 스테이지 레지스트리(DT_Stages) 한 행.
 * - StageId: 스테이지 식별자 (Row 이름과 별개로 명시, 메뉴/세이브에서 참조)
 * - Map: 로드할 월드 에셋
 * - TowerDT / EnemyDT / WaveDT: 월드 로드 후 각 Subsystem 에 주입할 DT
 */
USTRUCT(BlueprintType)
struct FStageRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	FName StageId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> Map;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Data")
	TSoftObjectPtr<UDataTable> TowerDT;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Data")
	TSoftObjectPtr<UDataTable> EnemyDT;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage|Data")
	TSoftObjectPtr<UDataTable> WaveDT;
};

/**
 * 스테이지 전환과 레벨별 DataTable 주입을 담당.
 * - 수명: GameInstance (앱 실행 중 유지)
 * - 권한: 서버/클라 각자의 머신에서 독립 실행 (로컬 캐시 갱신, 복제 아님)
 * - 동작:
 *     1) RequestLoadStage(Id) → DT_Stages 에서 Row 찾고 OpenLevel
 *     2) PostLoadMapWithWorld 훅에서 Row 를 역조회해 ApplyStageData()
 *     3) Tower/Enemy/Wave DT 를 각 Subsystem 에 주입
 * - 메뉴 경유 로드와 PIE 직접 열기 모두 동일하게 초기화됨.
 */
UCLASS()
class TOWERDEFENCE_API UTDLevelSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** StageId 로 해당 맵을 OpenLevel. 레지스트리에 없으면 false. */
	UFUNCTION(BlueprintCallable, Category = "TD|LevelSession")
	bool RequestLoadStage(FName StageId);

	UFUNCTION(BlueprintPure, Category = "TD|LevelSession")
	FName GetCurrentStageId() const { return CurrentStageId; }

	/** 디버그/에디터에서 레지스트리를 교체할 때 사용. 보통은 Initialize 에서 고정 경로 로드. */
	UFUNCTION(BlueprintCallable, Category = "TD|LevelSession")
	void SetStageRegistry(UDataTable* InRegistry) { StageRegistry = InRegistry; }

private:
	/** 월드 로드 완료 콜백. 새 월드의 경로로 Row 를 역조회하여 DT 주입. */
	void OnPostLoadMap(UWorld* LoadedWorld);

	/** 해당 Row 의 DT 들을 각 Subsystem 에 주입. */
	void ApplyStageData(const FStageRow& Row);

	/** StageId 로 Row 검색. 없으면 nullptr. */
	const FStageRow* FindRowByStageId(FName StageId) const;

	/** LoadedWorld 의 패키지 경로로 Row 역조회. 없으면 nullptr. */
	const FStageRow* FindRowByWorld(const UWorld* LoadedWorld) const;

private:
	/** 스테이지 레지스트리 DT. Initialize() 에서 고정 경로로 로드하거나 BP 에서 주입. */
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> StageRegistry;

	/** 가장 최근에 RequestLoadStage 로 요청한 스테이지 ID (PostLoadMap 단계에서 보조용). */
	FName PendingStageId;

	/** 현재 로드가 완료된 스테이지 ID. */
	FName CurrentStageId;

	FDelegateHandle PostLoadMapHandle;
};
