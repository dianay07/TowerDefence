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

	/**
	 * BP 용 진입점 — 지정한 월드(또는 기본값으로 현재 월드)에 대해 스테이지 로드 로직을 수동 실행.
	 * 엔진이 자동으로 발화하는 PostLoadMapWithWorld 훅과 동일한 경로를 타므로,
	 * DT_Stages 조회 → DT 주입 → OnStageReady 브로드캐스트 까지 전체가 다시 돈다.
	 *
	 * 사용 예:
	 *  - 테스트/디버그용 강제 재초기화
	 *  - 레벨 스트리밍 완료 후 수동 트리거
	 *  - SetStageRegistry 로 DT 를 바꾼 뒤 다시 적용하고 싶을 때
	 *
	 * @param WorldContextObject  월드 컨텍스트. 생략 시 GameInstance 의 기본 월드 사용.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|LevelSession",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Reload Current Stage"))
	void K2_ReloadCurrentStage(UObject* WorldContextObject);

	/**
	 * 디버그/테스트 전용 — DT_Stages 를 우회하고 지정한 DT 를 즉시 주입.
	 * 프로덕션 경로는 RequestLoadStage 를 사용할 것.
	 *
	 * 사용 예: 테스트 레벨의 Level Blueprint BeginPlay 에서 호출하여
	 * 레벨을 바로 PIE 로 실행해도 데이터가 준비되게 함.
	 *
	 * @param StageId  로그/식별용. NAME_None 이면 "Debug" 로 기록.
	 * @param TowerDT  즉시 주입할 타워 DT. null 이면 스킵.
	 * @param EnemyDT  (향후 추가될 Enemy Subsystem 용 자리. 현재는 로그만)
	 * @param WaveDT   (향후 추가될 Wave 주입 자리. 현재는 로그만)
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|LevelSession|Debug",
		meta = (DisplayName = "Debug Apply Stage"))
	void DebugApplyStage(FName StageId,
	                     UDataTable* TowerDT,
	                     UDataTable* EnemyDT = nullptr,
	                     UDataTable* WaveDT  = nullptr);

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
