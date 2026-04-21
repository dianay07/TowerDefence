#include "TDGameMode.h"
#include "TDGameInstance.h"
#include "TDGameState.h"
#include "TowerManager.h"
#include "Kismet/GameplayStatics.h"
#include "TDFL_Utility.h"

ATDGameMode::ATDGameMode()
{
	// 게임 관리 컴포넌트 생성
	EventManager = CreateDefaultSubobject<UTDEventManagerComponent>(TEXT("EventManager"));
	WaveManager  = CreateDefaultSubobject<UTDWaveManagerComponent>(TEXT("WaveManager"));
}

void ATDGameMode::BeginPlay()
{
	Super::BeginPlay();
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

// ── 액터 풀 ───────────────────────────────────────────────────────────────────

AActor* ATDGameMode::GetPoolActorFromClass(TSubclassOf<AActor> ActorClass, FTransform Transform, AActor* NewOwner)
{
	/*ITDPoolActorInterface* CastCheck = Cast<ITDPoolActorInterface>(ActorClass);
	if (!ActorClass || !CastCheck)
	{
		return nullptr;
	}*/

	AActor* PoolActor = nullptr;

	// 풀에 재사용 가능한 액터가 있으면 꺼내 사용
	if (ActorPool.Contains(ActorClass))
	{
		TArray<AActor*>* PoolActorArray = ActorPool.Find(ActorClass);
		if (PoolActorArray && !PoolActorArray->IsEmpty())
		{
			PoolActor = PoolActorArray->Pop();
		}
	}

	if (!PoolActor)
		PoolActor = Cast<AActor>(GetWorld()->SpawnActor<AActor>(ActorClass));

	if (PoolActor)
	{
		PoolActor->SetActorTransform(Transform);
		PoolActor->SetOwner(NewOwner);
	/*	ITDPoolActorInterface* PoolActorInter = Cast<ITDPoolActorInterface>(PoolActor);
		if (PoolActorInter)
		{
			PoolActorInter->OnRemovedFromPool();
		}*/
	}

	return PoolActor;
}

void ATDGameMode::PoolActor(AActor* PoolActor)
{
	if (!PoolActor || !Cast<ITDPoolActorInterface>(PoolActor))
	{
		return;
	}

	// 풀에 액터 반환
	TSubclassOf<AActor> ActorClass = PoolActor->GetClass();
	if (ActorPool.Contains(ActorClass))
	{
		TArray<AActor*>* PoolActorArray = ActorPool.Find(ActorClass);
		if (PoolActorArray)
		{
			PoolActorArray->Add(PoolActor);
		}
	}
	else
	{
		TArray<AActor*> PoolActorArray;
		PoolActorArray.Add(PoolActor);
		ActorPool.Add(ActorClass, PoolActorArray);
	}

	Cast<ITDPoolActorInterface>(PoolActor)->OnAddedToPool();
}
