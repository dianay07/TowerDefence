#include "TDGameMode.h"
#include "TDGameInstance.h"
#include "TDGameState.h"
#include "Kismet/GameplayStatics.h"
#include "TDFL_Utility.h"
#include "GameData/TDEnemyDataTableSubsystem.h"
#include "Server/TDPoolComponent.h"
#include "Server/TDTowerSpawnerComponent.h"
#include "Session/TDLevelSessionSubsystem.h"  // FStageRow

// ── 생명주기 ──────────────────────────────────────────────────────────────────

ATDGameMode::ATDGameMode()
{
	// 클라 → 서버 RPC 단일 진입점 등록 (CLAUDE.md §1-3)
	PlayerControllerClass = ATDPlayerController::StaticClass();

	// 서버 전용 게임 규칙 컴포넌트 생성
	WaveManager   = CreateDefaultSubobject<UTDWaveManagerComponent>(TEXT("WaveManager"));
	EnemySpawner  = CreateDefaultSubobject<UTDEnemySpawnerComponent>(TEXT("EnemySpawner"));
	TowerSpawner  = CreateDefaultSubobject<UTDTowerSpawnerComponent>(TEXT("TowerSpawner"));
	Pool          = CreateDefaultSubobject<UTDPoolComponent>(TEXT("Pool"));
}

void ATDGameMode::BeginPlay()
{
	Super::BeginPlay();

	// BP_GameMode 에서 설정한 EnemyClasses TMap 을 GameInstanceSubsystem 에 주입
	if (UTDEnemyDataTableSubsystem* DT = UTDFL_Utility::GetEnemyDataTable(this))
	{
		DT->RegisterEnemyClasses(EnemyClasses);
	}
}

// ── 게임 판정 ─────────────────────────────────────────────────────────────────

void ATDGameMode::GameEnded(bool bWin)
{
	if (ATDGameState* GS = UTDFL_Utility::GetTDGameState(this))
	{
		GS->BroadcastGameEnded(bWin);
	}

	UGameplayStatics::SetGamePaused(this, true);
}

void ATDGameMode::CheckIfWin()
{
	if (WaveManager->DoEnemiesRemain())
		GameEnded(true);
}

void ATDGameMode::CheckIfLoss()
{
	if (UTDFL_Utility::GetTDGameState(this)->BaseHealth <= 0)
	{
		GameEnded(false);
	}
}

// ── 액터 풀 (UTDPoolComponent 위임 래퍼 — BP 호환용) ─────────────────────────

AActor* ATDGameMode::GetPoolActorFromClass(TSubclassOf<AActor> ActorClass, FTransform Transform, AActor* NewOwner)
{
	return Pool ? Pool->GetPoolActorFromClass(ActorClass, Transform, NewOwner) : nullptr;
}

void ATDGameMode::PoolActor(AActor* PoolActorArg)
{
	if (Pool) Pool->ReturnToPool(PoolActorArg);
}

// ── 스테이지 초기화 ───────────────────────────────────────────────────────────

void ATDGameMode::InitializeStage(const FStageRow& Row)
{
	if (!HasAuthority()) return;

	// 타워 슬롯(타일 메시) 탐색 후 BaseTowerClass 스폰
	if (TowerSpawner)
	{
		TowerSpawner->SpawnInitialTowerBases(Row);
	}

	// 추후: WaveManager->InitStage(Row), 이코노미 초기화 등 여기에 집결
}
