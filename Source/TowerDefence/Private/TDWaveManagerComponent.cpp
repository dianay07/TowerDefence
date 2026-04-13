#include "TDWaveManagerComponent.h"
#include "TDEnemyActor.h"
#include "TDFL_Utility.h"
#include "TDGameMode.h"
#include "TDPathActor.h"
#include "Kismet/DataTableFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

UTDWaveManagerComponent::UTDWaveManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTDWaveManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// 씬에 배치된 모든 경로 액터 수집
	TArray<AActor*> FoundPaths;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATDPath::StaticClass(), FoundPaths);
	for (AActor* Actor : FoundPaths)
	{
		if (ATDPath* Path = Cast<ATDPath>(Actor))
		{
			Paths.Add(Path);
		}
	}

	ImportData();
}

void UTDWaveManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateWave(DeltaTime);
}

// ── 웨이브 진행 ───────────────────────────────────────────────────────────────

void UTDWaveManagerComponent::UpdateWave(float Delta)
{
	AdvanceEnemies(Delta);

	// 각 웨이브 데이터를 순회하며 스폰 타이밍 제어
	for(int i = 0; i < WaveData.Num(); i++)
	{
		if(GetWorld()->GetTimeSeconds() > WaveData[i].StartTime)
		{
			if (WaveData[i].SpawnCount <= 0) continue;

			float LocalDelayRemining = WaveData[i].DelayRemaining - Delta;

			if(LocalDelayRemining <= 0.f)
			{
				// 적 스폰 후 딜레이 리셋 및 카운트 감소
				SpawnEnemy(WaveData[i].EnemyType);
				LocalDelayRemining = WaveData[i].SpawnDeley;
				WaveData[i].SpawnCount--;
			}

			WaveData[i].DelayRemaining = LocalDelayRemining;
		}
	}
}

void UTDWaveManagerComponent::AdvanceEnemies(float Delta)
{
	// 경로 끝에 도달한 적 처리
	for (int i = Enemies.Num() - 1; i >= 0; i--)
	{
		if (!IsValid(Enemies[i]))
		{
			Enemies.RemoveAt(i);
			continue;
		}

		if(!(Enemies[i]->Advance(Delta) < PathLengths[0]))
		{
			ExpiredEnemies.Add(Enemies[i]);
		}
	}

	// 만료 적 제거
	for(int i = 0; i < ExpiredEnemies.Num(); i++)
	{
		ATDEnemyActor* Enemy = ExpiredEnemies[i];
		if (!IsValid(Enemy))
		{
			Enemies.Remove(Enemy);
			continue;
		}

		Enemies.Remove(Enemy);
		Enemy->Destroy();

		if (DoEnemiesRemain())
		{
			UTDFL_Utility::GetTDGameMode(this)->CheckIfWin();
		}
	}

	ExpiredEnemies.Empty();
}

// ── 데이터 로드 ───────────────────────────────────────────────────────────────
void UTDWaveManagerComponent::ImportData()
{
	if (!IsValid(DataTable)) return;

	// 데이터 테이블에서 웨이브 정보 로드
	TArray<FName> OutRowNames;
	UDataTableFunctionLibrary::GetDataTableRowNames(DataTable, OutRowNames);

	for(int i = 0; i < OutRowNames.Num(); i++)
	{
		FWaveData* Row = DataTable->FindRow<FWaveData>(OutRowNames[i], TEXT("ImportData"));
		if (!Row) continue;
		WaveData.Add(*Row);

		TotalEnemyCount += Row->SpawnCount;
	}

	// 경로 길이 캐싱
	for(int i = 0; i < Paths.Num(); i++)
	{
		PathLengths.Add(Paths[i]->GetLength());
	}
}

// ── 적 스폰 ───────────────────────────────────────────────────────────────────
void UTDWaveManagerComponent::SpawnEnemy(EEnemyType EnemyType)
{
	TSubclassOf<ATDEnemyActor>* EnemyActor = EnemyTypeClass.Find(EnemyType);

	if (!EnemyActor || !*EnemyActor) return;
	if (Paths.IsEmpty() || !IsValid(Paths[0])) return;

	// 경로 시작 위치에 적 스폰
	FTransform SpawnTransform(Paths[0]->GetLocation(0.f));
	ATDEnemyActor* Enemy = GetWorld()->SpawnActor<ATDEnemyActor>(*EnemyActor, SpawnTransform);

	if (!IsValid(Enemy)) return;

	Enemies.Add(Enemy);
	Enemy->InitializePath(Paths[0]);
	// 사망 이벤트 바인딩
	Enemy->OnDied.AddDynamic(this, &UTDWaveManagerComponent::OnEnemyDied);
}

// ── 적 조회 ───────────────────────────────────────────────────────────────────
ATDEnemyActor* UTDWaveManagerComponent::GetFurthestEnemy(FVector Location, float Radius)
{
	// 범위 내에서 가장 멀리 진행한 적 반환
	float HighDist = 0.f;
	ATDEnemyActor* HighEnemy = nullptr;

	for(int i = 0; i < Enemies.Num(); i++)
	{
		if (FVector::Dist(Enemies[i]->GetActorLocation(), Location) <= Radius)
		{
			if (Enemies[i]->GetDistance() > HighDist)
			{
				HighDist = Enemies[i]->GetDistance();
				HighEnemy = Enemies[i];
			}
		}
	}

	return HighEnemy;
}

TArray<ATDEnemyActor*> UTDWaveManagerComponent::GetEnemiesInRange(FVector Location, float Radius)
{
	// 범위 내 모든 적 반환
	TArray<ATDEnemyActor*> TheEnemies;

	for (int i = 0; i < Enemies.Num(); i++)
	{
		if (FVector::Dist(Enemies[i]->GetActorLocation(), Location) <= Radius)
		{
			TheEnemies.Add(Enemies[i]);
		}
	}

	return TheEnemies;
}

// ── 사망 이벤트 ───────────────────────────────────────────────────────────────
void UTDWaveManagerComponent::OnEnemyDied(ATDEnemyActor* Enemy)
{
	if (!IsValid(Enemy)) return;

	Enemies.Remove(Enemy);
	KillCount++;

	UE_LOG(LogTemp, Warning, TEXT("[WaveManager] KillCount: %.0f | Enemies Remaining: %d"), KillCount, Enemies.Num());

	ATDGameMode* GM = Cast<ATDGameMode>(GetOwner());
	if (IsValid(GM))
	{
		// EventManager에 사망 이벤트 전파
		if (IsValid(GM->EventManager))
			GM->EventManager->BroadcastEnemyDied(Enemy);

		GM->CheckIfWin();
	}
}

// ── 웨이브 상태 확인 ──────────────────────────────────────────────────────────

bool UTDWaveManagerComponent::DoEnemiesRemain() const
{
	// 살아있는 적이 없고 스폰할 적도 없으면 true
	if (Enemies.IsEmpty())
	{
		for(int i = 0; i < WaveData.Num(); i++)
		{
			if (WaveData[i].SpawnCount > 0)
			{
				return false;
			}
		}

		return true;
	}

	return false;
}
