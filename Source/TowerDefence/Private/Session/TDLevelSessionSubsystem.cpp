#include "Session/TDLevelSessionSubsystem.h"
#include "GameData/TDTowerDataTableSubsystem.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/SoftObjectPath.h"

namespace
{
	// 스테이지 레지스트리의 기본 경로. BP 에서 SetStageRegistry() 로 교체 가능.
	static const TCHAR* GDefaultStageRegistryPath = TEXT("/Game/Data/DT_Stages.DT_Stages");
}

void UTDLevelSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 기본 레지스트리 로드. 프로젝트 설정/BP 에서 교체하고 싶으면 SetStageRegistry() 사용.
	if (!StageRegistry)
	{
		if (UDataTable* Loaded = LoadObject<UDataTable>(nullptr, GDefaultStageRegistryPath))
		{
			StageRegistry = Loaded;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[LevelSession] Default stage registry not found at %s."),
				GDefaultStageRegistryPath);
		}
	}

	// 월드 로드 완료 훅: 메뉴 경유든 PIE 직접 열기든 둘 다 잡힘.
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UTDLevelSessionSubsystem::OnPostLoadMap);
}

void UTDLevelSessionSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	StageRegistry = nullptr;
	PendingStageId = NAME_None;
	CurrentStageId = NAME_None;

	Super::Deinitialize();
}

bool UTDLevelSessionSubsystem::RequestLoadStage(FName StageId)
{
	const FStageRow* Row = FindRowByStageId(StageId);
	if (!Row)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LevelSession] RequestLoadStage: StageId '%s' not found in registry."),
			*StageId.ToString());
		return false;
	}

	if (Row->Map.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LevelSession] RequestLoadStage: StageId '%s' has null Map."),
			*StageId.ToString());
		return false;
	}

	PendingStageId = StageId;

	const FString MapPath = Row->Map.ToSoftObjectPath().GetLongPackageName();
	UGameplayStatics::OpenLevel(this, FName(*MapPath));

	UE_LOG(LogTemp, Log, TEXT("[LevelSession] OpenLevel for stage '%s' → %s"),
		*StageId.ToString(), *MapPath);
	return true;
}

void UTDLevelSessionSubsystem::OnPostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld) return;

	// 1) 우선 PendingStageId 로 조회 (메뉴 경유 로드 케이스)
	const FStageRow* Row = nullptr;
	if (!PendingStageId.IsNone())
	{
		Row = FindRowByStageId(PendingStageId);
	}

	// 2) 없거나 일치하지 않으면 월드 경로로 역조회 (PIE 직접 열기 케이스)
	if (!Row)
	{
		Row = FindRowByWorld(LoadedWorld);
	}

	if (!Row)
	{
		// 레지스트리에 없는 월드 (메인 메뉴 등). 조용히 스킵.
		PendingStageId = NAME_None;
		return;
	}

	ApplyStageData(*Row);

	CurrentStageId = Row->StageId;
	PendingStageId = NAME_None;

	UE_LOG(LogTemp, Log, TEXT("[LevelSession] Stage '%s' ready."), *CurrentStageId.ToString());
}

void UTDLevelSessionSubsystem::ApplyStageData(const FStageRow& Row)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	// Tower DT 주입
	if (UTDTowerDataTableSubsystem* TowerDT = GI->GetSubsystem<UTDTowerDataTableSubsystem>())
	{
		if (UDataTable* DT = Row.TowerDT.LoadSynchronous())
		{
			TowerDT->LoadFromDataTable(DT);
		}
		else if (!Row.TowerDT.IsNull())
		{
			UE_LOG(LogTemp, Warning, TEXT("[LevelSession] TowerDT failed to load for stage '%s'."),
				*Row.StageId.ToString());
		}
	}

	// Enemy DT / Wave DT: 대응 Subsystem 추가 시 여기에 동일한 패턴으로 주입.
	// if (UTDEnemyDataTableSubsystem* EnemyDT = GI->GetSubsystem<UTDEnemyDataTableSubsystem>())
	// {
	//     if (UDataTable* DT = Row.EnemyDT.LoadSynchronous()) EnemyDT->LoadFromDataTable(DT);
	// }
}

const FStageRow* UTDLevelSessionSubsystem::FindRowByStageId(FName StageId) const
{
	if (!StageRegistry || StageId.IsNone()) return nullptr;

	static const FString Context(TEXT("TDLevelSessionSubsystem::FindRowByStageId"));
	for (const FName& RowName : StageRegistry->GetRowNames())
	{
		const FStageRow* Row = StageRegistry->FindRow<FStageRow>(RowName, Context);
		if (Row && Row->StageId == StageId)
		{
			return Row;
		}
	}
	return nullptr;
}

const FStageRow* UTDLevelSessionSubsystem::FindRowByWorld(const UWorld* LoadedWorld) const
{
	if (!StageRegistry || !LoadedWorld) return nullptr;

	const FString LoadedPath = FSoftObjectPath(LoadedWorld).GetLongPackageName();
	if (LoadedPath.IsEmpty()) return nullptr;

	static const FString Context(TEXT("TDLevelSessionSubsystem::FindRowByWorld"));
	for (const FName& RowName : StageRegistry->GetRowNames())
	{
		const FStageRow* Row = StageRegistry->FindRow<FStageRow>(RowName, Context);
		if (!Row || Row->Map.IsNull()) continue;

		if (Row->Map.ToSoftObjectPath().GetLongPackageName() == LoadedPath)
		{
			return Row;
		}
	}
	return nullptr;
}
