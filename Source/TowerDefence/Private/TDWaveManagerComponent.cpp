#include "TDWaveManagerComponent.h"
#include "TDEnemyActor.h"
#include "TDFL_Utility.h"
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

void UTDWaveManagerComponent::UpdateWave(float Delta)
{
	AdvanceEnemies(Delta);

	for(int i = 0; i < WaveData.Num(); i++)
	{
		int32 LocalSpawnCount = 0;
		float LocalDelayRemining = 0.f;

		// GetWorld()->GetTimeSeconds() == GetGameTimeInSeconds(this)
		if(GetWorld()->GetTimeSeconds() > WaveData[i].StartTime)
		{
			LocalSpawnCount = WaveData[i].SpawnCount;

			if (LocalSpawnCount > 0)
			{
				LocalDelayRemining = WaveData[i].DelayRemaining - Delta;

				if(LocalDelayRemining <= 0.f)
				{
					SpawnEnemy(WaveData[i].EnemyType);
					LocalDelayRemining = WaveData[i].SpawnDeley;
					LocalSpawnCount--;
				}

				FWaveData NewData;
				NewData.StartTime = WaveData[i].StartTime;
				NewData.EnemyType = WaveData[i].EnemyType;
				NewData.SpawnCount = LocalSpawnCount;
				NewData.SpawnDeley = WaveData[i].SpawnDeley;
				NewData.DelayRemaining = LocalDelayRemining;

				// BP -> SetArrayElem
				WaveData[i] = NewData;
			}
		}
	}
}

void UTDWaveManagerComponent::AdvanceEnemies(float Delta) 
{
	for (int i = 0; i < Enemies.Num(); i++)
	{
		// TODO : ���� Path�� 1�� ���̴� 0���� ���� ����, ��ȹ�� �Ѱ��� �ʿ� ���� path ���� �� ���� ����
		if(!(Enemies[i]->Advance(Delta) < PathLengths[0]))
		{
			ExpiredEnemies.Add(Enemies[i]);
		}	
	}

	for(int i = 0; i < ExpiredEnemies.Num(); i++)
	{
		ATDEnemyActor* Enemy = ExpiredEnemies[i];
		Enemies.Remove(ExpiredEnemies[i]);
		Enemy->Destroy();

		if (!DoEnemiesRemain())
		{
			// ���� �ȿű�
			//GetTDGameMode(this)->CheckIfWin();
		}
	}

	ExpiredEnemies.Empty();
}

void UTDWaveManagerComponent::ImportData() 
{
	TArray<FName> OutRowNames;
	
	if (!IsValid(DataTable)) return;
	UDataTableFunctionLibrary::GetDataTableRowNames(DataTable, OutRowNames);

	for(int i = 0; i < OutRowNames.Num(); i++)
	{
		FWaveData* Row = DataTable->FindRow<FWaveData>(OutRowNames[i], TEXT("ImportData"));
		
		if (!Row) continue;
		WaveData.Add(*Row);
	}

	for(int i = 0; i < Paths.Num(); i++)
	{
		PathLengths.Add(Paths[i]->GetLength());
	}
}

void UTDWaveManagerComponent::SpawnEnemy(EEnemyType EnemyType)
{
	TSubclassOf<ATDEnemyActor>* EnemyActor = EnemyTypeClass.Find(EnemyType);                          
	
	if (!EnemyActor || !*EnemyActor) return;
	if (Paths.IsEmpty() || !IsValid(Paths[0])) return;
	
	FTransform SpawnTransform(Paths[0]->GetLocation(0.f));
	ATDEnemyActor * Enemy = GetWorld()->SpawnActor<ATDEnemyActor>(*EnemyActor, SpawnTransform);
	
	if (!IsValid(Enemy)) return;

	Enemies.Add(Enemy);
	Enemy->InitializePath(Paths[0]);
}

ATDEnemyActor* UTDWaveManagerComponent::GetFurthestEnemy(FVector Location, float Radius)
{
	float HighDist = 0.f;
	ATDEnemyActor* HighEnemy = nullptr;

	for(int i = 0; i< Enemies.Num(); i++)
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

void UTDWaveManagerComponent::OnEnemyDied(ATDEnemyActor* Enemy)
{
	// TOOD : Check ���
	Enemies.Remove(Enemy);

	KillCount++;
}

TArray<ATDEnemyActor*> UTDWaveManagerComponent::GetEnemiesInRange(FVector Location, float Radius)
{
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

bool UTDWaveManagerComponent::DoEnemiesRemain() const
{
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
