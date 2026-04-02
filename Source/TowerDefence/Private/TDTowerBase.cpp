// Fill out your copyright notice in the Description page of Project Settings.


#include "TDTowerBase.h"
#include "TDPooledGameMode.h"
#include "TowerManager.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectTypes.h"
#include "TDTowerSet.h"
#include "Kismet/GameplayStatics.h"

ATDTowerBase::ATDTowerBase()
{
	HighlightStaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HighlightStaticMeshComp"));
	HighlightStaticMeshComp->SetupAttachment(RootComponent);

	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	BoxComp->SetupAttachment(RootComponent);
	
	ChildActorCompWeapon = CreateDefaultSubobject<UChildActorComponent>(TEXT("ChildActorWeapon"));
	ChildActorCompWeapon->SetupAttachment(RootComponent);
}

float ATDTowerBase::GetRange() const
{
    // jiho: Tag 는 Range 인지 Damage 인지 별도의 String 으로 입력하는데 아래 부분은 없음 확인 필요
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
    // [Sequence Then 0] Switch on TowerActionTop
    // Upgrade 액션일 때만 처리
    ETowerActions ThisTowerActionTop;

    ThisTowerActionTop = TowerActionTop;

    if (ThisTowerActionTop != ETowerActions::Upgrade)
    {
        return;
    }


    // [Sequence Then 1] Branch: CanUpgrade?
    if (CanUpgrade())
    {
        // SpawnActor BP Tower Actions
        // 위치: 현재 액터 위치 + Z 100
        FVector SpawnLocation = GetActorLocation();
        SpawnLocation.Z += 100.f;

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(
            TowerActionsClass,
            SpawnLocation,
            FRotator::ZeroRotator,
            SpawnParams
        );

        // SET BP_TowerActions = Return Value
        BP_TowerActions = SpawnedActor;

        // 스폰된 액터에 현재 타워의 Action 슬롯 값 전달
        // (BP Tower Actions 측에서 Owner를 통해 읽거나, 별도 인터페이스로 전달 필요)

        // Jiho:BP_TowerActions -> 에 Enum 값을 설정 하는 부분이 있는데 별도 처리 해야함.
    }
    else
    {
        // 업그레이드 불가 시 TowerActionTop 초기화
        TowerActionTop = ETowerActions::None;
    }
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
    // UpgradeLevel에 따라 다음 업그레이드 비용 반환
    // BreakDown은 건설 비용의 절반 환급
    switch (TowerAction)
    {
    case ETowerActions::BuildTurret:
    case ETowerActions::BuildBallista:
    case ETowerActions::BuildCatapult:
    case ETowerActions::BuildCannon:
        OutCostOrRefund = TowerData.BuildCost;
        OutDescription  = TowerData.TowerName;
        break;

    case ETowerActions::Upgrade:
        if      (UpgradeLevel == 0) OutCostOrRefund = TowerData.UpgradeCost1;
        else if (UpgradeLevel == 1) OutCostOrRefund = TowerData.UpgradeCost2;
        else                        OutCostOrRefund = TowerData.UpgradeCost3;
        OutDescription = FString::Printf(TEXT("%s Lv%d"), *TowerData.TowerName, UpgradeLevel + 1);
        break;

    case ETowerActions::BreakDown:
        OutCostOrRefund = TowerData.BuildCost / 2;
        OutDescription  = TEXT("Refund");
        break;

    default:
        OutCostOrRefund = 0;
        OutDescription  = TEXT("");
        break;
    }
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
    ATDPooledGameMode* GM = Cast<ATDPooledGameMode>(UGameplayStatics::GetGameMode(this));

    // [Sequence Then 0] 플레이어의 현재 선택 해제
    // (BP Player에 UnSelectTower 함수가 존재한다고 가정 — BP 측에서 연결 필요)
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (IsValid(Player))
    {
        // jiho - 로직이 비었음 BP 함수를 호출해야 되서. 부가 사항은 아래 더 있음.
        // BP_Player의 UnSelectTower 호출은 BP 이벤트로 처리
        // C++ 베이스에 추가 시 여기서 직접 호출 가능
    }

    // [Sequence Then 1] 비용/환급 계산 및 코인 처리
    int32 CostOrRefund = 0;
    FString Description;
    // jiho - 코인관련된 기능을 뺏음
    //GetTowerDetails(TowerAction, CostOrRefund, Description);

    // 스폰할 타워 클래스 (None이면 Then 2에서 스폰 스킵)
    TSubclassOf<AActor> NewTowerClass = nullptr;

    if (TowerAction == ETowerActions::BreakDown)
    {
        // 철거: 코인 환급 후 빈 타워로 교체
        if (IsValid(GM))
        {
            //GM->RefundCoins(CostOrRefund);
        }
        NewTowerClass = BaseTowerClass;
    }
    else
    {
        // 건설/업그레이드: 코인 차감 시도
        //if (!IsValid(GM) || !GM->SpendCoins(CostOrRefund))
        //{
        //    return;  // 잔액 부족 → Return Node
        //}

        // Switch on ETowerActions
     
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
	HighlightStaticMeshComp->SetVisibility(IsHightlighted);
}
