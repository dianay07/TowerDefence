// Fill out your copyright notice in the Description page of Project Settings.


#include "TDTowerBase.h"
#include "TDGameMode.h"
#include "TDGameState.h"
#include "TowerManager.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectTypes.h"
#include "TDTowerSet.h"
#include "Kismet/GameplayStatics.h"

ATDTowerBase::ATDTowerBase()
{
	bReplicates = true;

	HighlightStaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HighlightStaticMeshComp"));
	HighlightStaticMeshComp->SetupAttachment(RootComponent);

	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	BoxComp->SetupAttachment(RootComponent);
	
    ChildActorWeaponComp = CreateDefaultSubobject<UChildActorComponent>(TEXT("ChildActorWeapon"));
    ChildActorWeaponComp->SetupAttachment(RootComponent);
}

void ATDTowerBase::BeginPlay()
{
    Super::BeginPlay();
    // Note(Jiho): Widget - 쪽에 연결되어 있는 부분이 있어서 기존 BP 와 다르게 처리 해둠.
    // 
    // jiho : BP Class 맴버에 접근하기 위해서 BP-BeginPlay 에서 임시처리
    // C++ 코드로 모두 변경되면 해당 로직은 여기로 가져온다.
    // 
    //Super::BeginPlay();

    // [Cast To BP_Weapon] ChildActorWeaponComp의 Child Actor 가져오기
    //AActor* ChildActor = ChildActorWeaponComp->GetChildActor();

    // SET Weapon
   // Weapon = ChildActor;

    // jiho: Weapon의 Tower = Self, Target = ? 설정은 BP 전용 변수라 인터페이스 필요
    // Weapon이 ITDWeaponInterface를 구현하면 Execute_SetTowerRef(Weapon, this) 형태로 호출

    // Get Tower Data
    //GetTowerData();
}

void ATDTowerBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Try Activate Ability by Class (Allow Remote Activation = true)
    AbilitySystemComponent->TryActivateAbilityByClass(DefaultAbility, true);
}

float ATDTowerBase::GetRange() const
{
    // ATTRIBUTE_ACCESSORS_BASIC Define 에 포함되어 있는 인터페이스로 보임.
    bool bFound = false;
    float Value = AbilitySystemComponent->GetGameplayAttributeValue(UTDTowerSet::GetRangeAttribute(), bFound);
    return Value;
}

float ATDTowerBase::GetDamage() const
{
    // jiho: Tag 는 Range 인지 Damage 인지 별도의 String 으로 입력하는데 아래 부분은 없음 확인 필요
    bool bFound = false;
    return AbilitySystemComponent->GetGameplayAttributeValue(UTDTowerSet::GetDamageAttribute(), bFound);
}

int32 ATDTowerBase::GetBreakdownRefund() const
{
    // Then 0: 기본 건설 비용
    int32 TheRefund = TowerData.BuildCost;

    // Then 1~3: 업그레이드 레벨만큼 지출한 비용 누적
    if (UpgradeLevel >= 1) TheRefund += TowerData.UpgradeCost1;
    if (UpgradeLevel >= 2) TheRefund += TowerData.UpgradeCost2;
    if (UpgradeLevel >= 3) TheRefund += TowerData.UpgradeCost3;

    return TheRefund;
}

int32 ATDTowerBase::GetUpgradeCost() const
{
    // Then 0: Switch on UpgradeLevel
    switch (UpgradeLevel)
    {
    case 0: return TowerData.UpgradeCost1;
    case 1: return TowerData.UpgradeCost2;
    case 2: return TowerData.UpgradeCost3;
    default: return 0;
    }
}

float ATDTowerBase::GetRadius() const
{
    return TowerData.Radius;
}

bool ATDTowerBase::CanUpgrade() const
{
    // TowerData에 UpgradeCost1~3이 있으므로 최대 업그레이드 레벨은 3
    return UpgradeLevel < 3;
}

void ATDTowerBase::Select()
{
    // SET This Tower Action Top = TowerActionTop (현재 값 읽기)
    ETowerActions ThisTowerActionTop = TowerActionTop;

    // [Sequence Then 0] Upgrade 타입이면 CanUpgrade 검증
    if (ThisTowerActionTop == ETowerActions::Upgrade)
    {
        if (!CanUpgrade())
        {
            // 업그레이드 불가 → 액션 초기화 후 리턴
            TowerActionTop = ETowerActions::None;
            return;
        }
    }

    // [Sequence Then 1] TowerActionsClass 스폰 (Upgrade 가능 or 그 외 모든 타입)
    if (!BP_TowerActionsClass)
    {
        return;
    }

    FVector SpawnLocation = GetActorLocation();
    SpawnLocation.Z += 100.f;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(
        BP_TowerActionsClass,
        SpawnLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );

    // SET BP_TowerActions = Return Value
    // (Tower Action Top/Left/Right/Bottom 값은 Owner(self)를 통해 BP 측에서 읽도록 처리)
    BP_TowerActions = SpawnedActor;
}

void ATDTowerBase::UnSelect()
{
    // GET BP_TowerActions → Is Valid → Destroy Actor → SET BP_TowerActions = null
    if (IsValid(BP_TowerActions))
    {
        BP_TowerActions->Destroy();
    }
    BP_TowerActions = nullptr;
}

void ATDTowerBase::GetTowerDetails(ETowerActions TowerAction, int32& OutCostOrRefund, FString& OutDescription)
{
    OutCostOrRefund = 0;
    OutDescription  = TEXT("");

    // [Sequence Then 0] Get Tower Manager → Is Valid Branch
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATowerManager::StaticClass(), FoundActors);
    if (FoundActors.IsEmpty())
    {
        return;
    }

    ATowerManager* TowerManager = Cast<ATowerManager>(FoundActors[0]);
    if (!IsValid(TowerManager))
    {
        return;
    }

    // [Sequence Then 1] Switch on ETowerActions
    switch (TowerAction)
    {
    case ETowerActions::BuildTurret:
    {
        TowerManager->GetTowerData(ETowerType::Turret, TowerData);
        break;
    }
    case ETowerActions::BuildBallista:
    {
        TowerManager->GetTowerData(ETowerType::Ballista, TowerData);
        break;
    }
    case ETowerActions::BuildCatapult:
    {
        TowerManager->GetTowerData(ETowerType::Catapult, TowerData);
        break;
    }
    case ETowerActions::BuildCannon:
    {
        TowerManager->GetTowerData(ETowerType::Cannon, TowerData);
        break;
    }

    case ETowerActions::Upgrade:
        OutCostOrRefund = GetUpgradeCost();
        OutDescription  = FString::Printf(TEXT("Upgrade the %s for mo Power"), *TowerData.TowerName);
        break;

    case ETowerActions::BreakDown:
        OutCostOrRefund = GetBreakdownRefund();
        OutDescription  = TEXT("Break Down");
        break;

    default:
        break;
    }

    OutCostOrRefund = TowerData.BuildCost;
}

void ATDTowerBase::UpgradeTower()
{
    if (!CanUpgrade())
    {
        return;
    }

    UpgradeLevel++;

    // 각 스탯 = BaseValue + (BaseValue × UpgradeLevel × 0.5)
    TowerData.Range    = TowerData.Range    + (TowerData.Range    * UpgradeLevel * 0.5f);
    TowerData.FireRate = TowerData.FireRate + (TowerData.FireRate * UpgradeLevel * 0.5f);
    TowerData.Damage   = TowerData.Damage   + (TowerData.Damage   * UpgradeLevel * 0.5f);

    SetTowerAttributes();
}

void ATDTowerBase::DoTowerAction(ETowerActions TowerAction)
{
    // JaeHoon : coin 관련 기능은 GameMode에서 GameState로 이양
    ATDGameState* GameState = Cast<ATDGameState>(UGameplayStatics::GetGameState(this));

    // [Sequence Then 0] 플레이어의 현재 선택 해제
    // (BP Player에 UnSelectTower 함수가 존재한다고 가정 — BP 측에서 연결 필요)
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (IsValid(Player))
    {
        // jiho - 로직이 비었음 BP 함수를 호출해야 되서. 부가 사항은 아래 더 있음.
        // BP_Player의 UnSelectTower 호출은 BP 이벤트로 처리
        // C++ 베이스에 추가 시 여기서 직접 호출 가능
        //BP_Player->UnSelecTower();
    }

    // [Sequence Then 1] 비용/환급 계산 및 코인 처리
    int32 CostOrRefund = 0;
    FString Description;
    // jiho - 코인관련된 기능을 뺏음
    GetTowerDetails(TowerAction, CostOrRefund, Description);

    // 스폰할 타워 클래스 (None이면 Then 2에서 스폰 스킵)
    TSubclassOf<AActor> NewTowerClass = nullptr;

    if (TowerAction == ETowerActions::BreakDown)
    {
        // 철거: 코인 환급 후 빈 타워로 교체
        if (IsValid(GameState))
        {
            GameState->CoinChange(CostOrRefund);
        }
        NewTowerClass = BaseTowerClass;
    }
    else
    {
        // 건설/업그레이드: 코인 차감 시도
		// Turret만 있으니까 일단 1번거 강제로 가져와서 사용
		if (!IsValid(GameState) || !GameState->HasCoins(CostOrRefund))
            return;  // 잔액 부족 → Return Node

        // Switch on ETowerActions
        // JaeHoon
		GameState->CoinChange(-CostOrRefund);
    }

    switch (TowerAction)
    {
    case ETowerActions::BuildTurret:    NewTowerClass = TurretClass;   break;
    case ETowerActions::BuildBallista:  NewTowerClass = BallistaClass; break;
    case ETowerActions::BuildCatapult:  NewTowerClass = CatapultClass; break;
    case ETowerActions::BuildCannon:    NewTowerClass = CannonClass;   break;
    case ETowerActions::Upgrade:
        UpgradeTower();
        return;  // 업그레이드는 자기 자신을 교체하지 않음
    default:
        return;
    }

    // [Sequence Then 2] NewTowerClass가 유효하면 스폰 후 자신 Destroy
    if (!NewTowerClass)
    {
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* NewTower = GetWorld()->SpawnActor<AActor>(NewTowerClass, GetActorTransform(), SpawnParams);
    if (IsValid(NewTower))
    {
       UnSelect();
       Destroy();
    }
}

void ATDTowerBase::GetTowerData()
{
    // [Then 0] 초기화
    IsDataValid = false;

    // Type이 None이면 조기 리턴
    if (Type == ETowerType::None)
    {
        return;
    }

    // Get Tower Manager
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATowerManager::StaticClass(), FoundActors);
    if (FoundActors.IsEmpty())
    {
        return;
    }

    ATowerManager* TowerManager = Cast<ATowerManager>(FoundActors[0]);
    if (!IsValid(TowerManager))
    {
        return;
    }

    // Get Tower Data
    FTowerData OutTowerData;
    if (TowerManager->GetTowerData(Type, OutTowerData))
    {
        TowerData    = OutTowerData;
        IsDataValid  = true;
    }

    // [Then 1] IsDataValid 이면 GAS 어트리뷰트 적용
    if (IsDataValid)
    {
        SetTowerAttributes();
    }
}

void ATDTowerBase::SetTowerAttributes()
{
    if (!AbilitySystemComponent || !DefaultEffect)
    {
        return;
    }

    // Make Outgoing Spec (GE_Tower_Attrib, Level 0)
    FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
        DefaultEffect, 0.f, AbilitySystemComponent->MakeEffectContext());

    if (!SpecHandle.IsValid())
    {
        return;
    }

    // Assign Tag Set by Caller Magnitude × 3
    UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
        SpecHandle, FGameplayTag::RequestGameplayTag(FName("Tower.Range.SetByCaller")),    TowerData.Range);

    UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
        SpecHandle, FGameplayTag::RequestGameplayTag(FName("Tower.FireRate.SetByCaller")), TowerData.FireRate);

    UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
        SpecHandle, FGameplayTag::RequestGameplayTag(FName("Tower.Damage.SetByCaller")),   TowerData.Damage);

    // Apply to Self
    AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void ATDTowerBase::SetHighlight(bool IsHightlighted)
{
	HighlightStaticMeshComp->SetVisibility(IsHightlighted, true);
}
