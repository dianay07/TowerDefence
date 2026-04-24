#include "TDGameMode.h"
#include "TDGameInstance.h"
#include "TDGameState.h"
#include "TowerManager.h"
#include "Kismet/GameplayStatics.h"
#include "TDFL_Utility.h"
#include "GameData/TDEnemyDataTableSubsystem.h"
#include "Server/TDPoolComponent.h"
#include "Server/TDTowerSpawnerComponent.h"
#include "Session/TDLevelSessionSubsystem.h"  // FStageRow

ATDGameMode::ATDGameMode()
{
	// PlayerController 등록
	PlayerControllerClass = ATDPlayerController::StaticClass();

	// 게임 관리 컴포넌트 생성
	WaveManager   = CreateDefaultSubobject<UTDWaveManagerComponent>(TEXT("WaveManager"));
	EnemySpawner  = CreateDefaultSubobject<UTDEnemySpawnerComponent>(TEXT("EnemySpawner"));
	TowerSpawner  = CreateDefaultSubobject<UTDTowerSpawnerComponent>(TEXT("TowerSpawner"));
	Pool          = CreateDefaultSubobject<UTDPoolComponent>(TEXT("Pool"));
}

void ATDGameMode::BeginPlay()
{
	Super::BeginPlay();

	// EnemyClasses TMap을 GameInstanceSubsystem에 주입
	if (UTDEnemyDataTableSubsystem* DT = UTDFL_Utility::GetEnemyDataTable(this))
	{
		DT->RegisterEnemyClasses(EnemyClasses);
	}
}

// ── 게임 상태 ─────────────────────────────────────────────────────────────────

void ATDGameMode::GameEnded(bool bWin)
{
	// 결과 이벤트 브로드캐스트 후 일시정지
	// 0417 : 테스트 위해 게임 종료 로직 잠시 보류
	/*if (ATDGameState* GS = GetGameState<ATDGameState>())
	{
		GS->BroadcastGameEnded(bWin);
	}

	UGameplayStatics::SetGamePaused(this, true);*/

	if (bWin)
	{
		// TODO: UTDGameInstance::OnLevelComplete() 포팅 필요
		UTDGameInstance* GI = Cast<UTDGameInstance>(GetGameInstance());
		// if (GI) GI->OnLevelComplete();
	}
}

void ATDGameMode::CheckIfWin()
{
	// 모든 적이 처리됐으면 승리
	if (WaveManager->DoEnemiesRemain())
		GameEnded(true);
}

void ATDGameMode::CheckIfLoss()
{
	// 기지 체력이 0이면 패배
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

// ── 스테이지 초기화 ────────────────────────────────────────────────────────────

void ATDGameMode::InitializeStage(const FStageRow& Row)
{
	if (!HasAuthority()) return;

	if (TowerSpawner)
	{
		TowerSpawner->SpawnInitialTowerBases(Row);
	}

	// 추후: WaveManager 초기화, 이코노미 초기화 등 여기에 집결
}
