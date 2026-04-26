# TowerDefence — Architecture & Migration Guide

> 팀 공유용 작업 테이블. 새 기능을 짜기 전에 **어느 레이어에 둘지**부터 이 문서로 결정한다.
> 원칙 위반이 보이면 PR에서 이 문서 링크로 지적 가능.

---

## 1. 핵심 원칙

### 1-1. 수직 체인 금지, 병렬 레이어 + GameState 매개

```
❌ GameState → Manager → Subsystem          (수직 체인, 결합 과다)

✅ Manager (서버)  ←→  GameState (복제)  ←→  Subsystem (클라)
                       ↑
              양쪽이 GameState를 매개로 통신
```

- **Manager**(서버)는 GameState에 **쓴다**
- **Subsystem**(클라)은 GameState OnRep을 **읽는다**
- 클라가 상태를 바꾸고 싶으면 **PlayerController의 Server RPC**를 거친다

### 1-2. 책임 × 수명 × 권한으로 배치

| 수명 | 권한 | 컨테이너 | 용도 |
|---|---|---|---|
| 앱 실행 중 | 로컬 | **GameInstance / GameInstanceSubsystem** | 세이브, 세션, 정적 데이터 |
| 레벨 1회 | **서버만** | **GameMode + Component / Actor** | 게임 규칙 실행 (스폰/판정) |
| 레벨 1회 | 모두에게 복제 | **GameState** | 공유 상태 (코인/체력/웨이브) |
| 레벨 1회 | 자가 복제 | **Replicated Actor** | Tower/Enemy/Projectile |
| 플레이어 세션 | 본인 클라만 | **LocalPlayerSubsystem** | HUD, 하이라이트, 프리뷰 |
| 레벨 1회 | 로컬 클라 | **WorldSubsystem (클라 전용 초기화)** | VFX/SFX 풀, 화면 이펙트 |

### 1-3. RPC 경계는 단 하나 — PlayerController

모든 **클라 → 서버** 요청은 `ATDPlayerController`의 Server RPC를 거친다.
검증/치팅 방지 포인트를 한 곳으로 모으는 것이 목적.

---

## 2. 현재 vs 목표 구조

### 2-1. 현재 파일 분포 (문제점 포함)

| 파일 | 역할 | 문제 |
|---|---|---|
| `ATDGameMode` | Pool, WaveManager, EventManager 보유 | OK (서버 전용) |
| `ATDGameState` | SharedCoin, BaseHealth, CurrentWave | **복제 미적용** |
| `UTDWaveManagerComponent` | 웨이브 스폰/이동/조회 | OK (GameMode 소속) |
| `UTDEventManagerComponent` | 동적 멀티캐스트 델리게이트 | **클라에 존재하지 않음 → 클라 바인딩 불가** |
| `ATowerManager` (Actor) | TowerData, SpawnTowers, Highlight | **서버 로직 + 클라 UI 혼재** |
| `ATDPlayerCharacter` | 카메라, 입력, 타워 선택, HUD | **싱글플레이 전제, Server RPC 없음** |
| `UTDGameInstance` | 세이브 슬롯 | OK |
| `UTDFL_Utility::GetWaveManager` | GameMode 경유 | **클라에서 null 반환 예정** |

### 2-2. 목표 구조 (도메인별)

```
┌──────────────────────────────────────────────────────────────┐
│ GameInstance Layer (세션 수명, 로컬)                          │
│  ├ UTDGameInstance              세이브/로드 (유지)            │
│  ├ UTDTowerDataTableSubsystem   [NEW] TowerData 정적 조회     │
│  ├ UTDEnemyDataTableSubsystem   [NEW] EnemyData 정적 조회     │
│  └ UTDLevelSessionSubsystem     [NEW] 스테이지 전환 + DT 주입 │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│ GameMode Layer (레벨 수명, 서버 전용, 권위있음)               │
│  ├ ATDGameMode                  라이프사이클 조립             │
│  ├ UTDTowerSpawnerComponent     [NEW] 슬롯 수집, 타워 스폰   │
│  ├ UTDWaveManagerComponent      (유지) 웨이브 실행           │
│  ├ UTDEnemySpawnerComponent     [NEW/분리] Enemy 전담        │
│  └ UTDPoolComponent             [NEW/분리] ActorPool 전담    │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│ GameState Layer (레벨 수명, 복제)                             │
│  └ ATDGameState                                               │
│     ├ Replicated: SharedCoin, BaseHealth, CurrentWave         │
│     ├ Replicated: PlacedTowers, ActiveEnemies                 │
│     └ Multicast RPC: OnEnemyDied, OnEnemyAttacked             │
│                         (← EventManager가 여기로 이동)         │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│ Replicated Actors (자가 복제)                                 │
│  ├ ATDTowerPawn / ATDTowerBase                                │
│  ├ ATDEnemyActor                                              │
│  └ ATDProjectile                                              │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│ LocalPlayer Layer (본인 클라만, 플레이어 세션 수명)           │
│  ├ ATDPlayerController          [EXPAND] Server RPC 진입점   │
│  ├ ATDPlayerCharacter           (축소) 카메라/입력만         │
│  ├ UTDTowerHighlightSubsystem   [NEW] 마우스 호버            │
│  ├ UTDTowerPlacementSubsystem   [NEW] 유령 메시 프리뷰       │
│  └ UTDHUDSubsystem              [NEW] HUD 위젯 수명 관리     │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│ World Subsystem (로컬 전용 초기화)                            │
│  └ UTDEffectPoolSubsystem       [NEW] 파티클/사운드 풀       │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│ UI Layer (위젯, 본인 클라만)                                  │
│  └ UTDTowerActionWidgetBase   [NEW] BP WBP_TowerActions 베이스 │
│       ├ 데이터/로직: C++ (Tower 참조, 비용 계산, RPC 위임)     │
│       └ 시각/스타일: BP 자식 (UMG 디자이너, 텍스처)            │
└──────────────────────────────────────────────────────────────┘
```

---

## 3. 도메인별 배치표

### 3-1. Tower

| 기능 | 현재 위치 | 목표 위치 | 권한 |
|---|---|---|---|
| TowerData 테이블 로드/조회 | `ATowerManager::ImportData/GetTowerData` | `UTDTowerDataTableSubsystem` (GameInstance) | 로컬 readonly |
| 슬롯 탐색 (`tile-dirt`) | `ATowerManager::SpawnTowers` | `UTDTowerSpawnerComponent::CollectSlots` | 서버 |
| 초기 타워 스폰 | 같음 | `UTDTowerSpawnerComponent::SpawnInitial` | 서버 |
| 타워 설치/판매 요청 | `ATDTowerBase::DoTowerAction` 직접 | `ATDPlayerController::Server_DoTowerAction` ✅ 구현됨 | Client→Server RPC |
| 타워 설치/판매 실행 | `ATDTowerBase` | `UTDTowerSpawnerComponent::TryPlace/TrySell` | 서버 |
| 배치된 타워 목록 | (없음) | `ATDGameState::PlacedTowers` (Replicated) | 복제 |
| 마우스 호버 하이라이트 | `ATowerManager::UpdateHightlight` | `UTDTowerHighlightSubsystem` | 로컬 |
| 배치 프리뷰 유령 메시 | (BP 추정) | `UTDTowerPlacementSubsystem` | 로컬 |
| 타워 선택 UI | `ATDPlayerCharacter::SelectTower` | `UTDTowerHighlightSubsystem` | 로컬 |
| 타워 액션 UI (Build/Upgrade/BreakDown 버튼) | `WBP_TowerActions` (BP 단독, 로직 혼재) | `UTDTowerActionWidgetBase` (C++ 베이스, 단일 로직 허브) + `WBP_TowerActions` (BP 자식, 시각만) ✅ 구현됨 | 로컬 |
| 타워 클릭 → 액션 메뉴 표시/숨김 | (BP 분산) | `ATDPlayerController::ShowTowerActionMenu/HideTowerActionMenu` (PC 가 위젯 소유) ✅ 구현됨 | 로컬 |
| 타워 액션 메뉴 슬롯 클릭 | (BP 직접 호출) | `UTDTowerActionWidgetBase::RequestActionTop/Bottom/Left/Right` → `ATDPlayerController::HandleSlotClicked` → `Server_DoTowerAction` ✅ 구현됨 | Client→Server RPC |
| 업그레이드/철거 비용 계산 | `ATDTowerBase::GetTowerDetails` | 유지 (순수 조회) | 로컬 |
| GAS 어트리뷰트 적용 | `ATDTowerBase::SetTowerAttributes` | 유지 (서버에서만 호출) | 서버 |

**결론**: `ATowerManager`는 **삭제**. 기능을 4곳으로 쪼갬.

### 3-2. Enemy

| 기능 | 현재 위치 | 목표 위치 | 권한 |
|---|---|---|---|
| EnemyData / `EnemyTypeClass` TMap | `UTDWaveManagerComponent::EnemyTypeClass` | `UTDEnemyDataTableSubsystem` (GameInstance) | 로컬 readonly |
| 적 스폰 (`SpawnActor`) | `UTDWaveManagerComponent::SpawnEnemy` | `UTDEnemySpawnerComponent` 또는 Wave 유지 | 서버 |
| 경로 진행 (`Advance`) | `ATDEnemyActor::Advance` | 유지 | 서버 |
| 체력/이동속도/데미지 (GAS) | `ATDEnemyActor` + `UTDEnemySet` | 유지 | 서버 |
| 적 목록 (탐색용) | `UTDWaveManagerComponent::Enemies` | `ATDGameState::ActiveEnemies` (Replicated) | 복제 |
| 사망 이벤트 브로드캐스트 | `ATDEnemyActor::OnDied` + `EventManager` | `ATDGameState::MulticastOnEnemyDied` | 서버→All RPC |
| 보상 코인 지급 | `ATDEnemyActor::OnEnemyDied` | 유지 (`GameState->CoinChange`) | 서버 |
| 사망 애니메이션 | `PlayDeathAnimation` (BP) | 유지 (Multicast RPC로 트리거) | 모든 클라 |

**주의**: 현재 [TDWaveManagerComponent.cpp:95](Source/TowerDefence/Private/TDWaveManagerComponent.cpp#L95) 에서 `Enemy->Destroy()` 직접 호출 중. 서버에서만 실행되므로 OK지만, `Destroy` 전에 반드시 `OnRep_ActiveEnemies`가 먼저 퍼지도록 순서 주의.

### 3-3. Wave

| 기능 | 현재 위치 | 목표 위치 | 권한 |
|---|---|---|---|
| WaveData 테이블 로드 | `UTDWaveManagerComponent::ImportData` | 유지 | 서버 |
| 스폰 타이밍 Tick | `UTDWaveManagerComponent::UpdateWave` | 유지 | 서버 |
| 경로(`ATDPath`) 수집 | `UTDWaveManagerComponent::BeginPlay` | 유지 | 서버 (필요시 양쪽) |
| 경로 길이 캐싱 | `PathLengths` | 유지 | 서버 |
| KillCount / TotalEnemyCount | `UTDWaveManagerComponent` | `ATDGameState` (Replicated) | 복제 |
| CurrentWave | `ATDGameState::CurrentWave` | 유지 + Replicated 지정 | 복제 |
| 웨이브 시작/종료 이벤트 | (없음) | `ATDGameState::MulticastOnWaveStarted/Ended` | 서버→All RPC |
| 승리 판정 | `ATDGameMode::CheckIfWin` | 유지 | 서버 |

### 3-4. Player

| 기능 | 현재 위치 | 목표 위치 | 권한 |
|---|---|---|---|
| 카메라 조작 (WASD, 엣지 스크롤) | `ATDPlayerCharacter` | 유지 | 로컬 |
| 마우스 입력 | `ATDPlayerCharacter::HandleClick` | `ATDPlayerController::HandleClick` ✅ 구현됨 | 로컬 |
| Tower 클릭 감지 → 메뉴 표시 | `BP_Player::SelectTower` (Tower → 자기 위젯 spawn) | `ATDPlayerController::HandleClick` (raycast) → `ShowTowerActionMenu(Tower)` ✅ 구현됨 | 로컬 |
| 타워 액션 위젯 인스턴스 소유 | (BP 측, Tower/Pawn 분산) | `ATDPlayerController::ActiveActionWidget` (생성/캐시/Show/Hide 전담) ✅ 구현됨 | 로컬 |
| 타워 선택 상태 관리 | `ATDPlayerCharacter::SelectTower` | `UTDTowerHighlightSubsystem` (호버) + `ATDPlayerController::SelectedTower` (액션 메뉴 대상) | 로컬 |
| HUD 위젯 생성/보유 | `ATDPlayerCharacter::HUDWidget` | `UTDHUDSubsystem` (LocalPlayer) | 로컬 |
| 코인/체력 UI 갱신 바인딩 | `ATDPlayerCharacter::OnCoinsChanged` | `ATDGameState` OnRep 구독 | 로컬 |
| 타워 설치/판매 요청 | (BP 추정) | `ATDPlayerController::Server_DoTowerAction` ✅ 구현됨 | Client→Server RPC |
| 기지 체력 감소 알림 | `ATDPlayerCharacter::NotifyBaseHealthDecreased` | GameState OnRep_BaseHealth | 복제 |

**결론**: `ATDPlayerCharacter`는 **카메라/입력만** 남기고 UI/상태 로직은 Subsystem으로.

### 3-5. Pool (보조)

| 기능 | 현재 위치 | 목표 위치 | 권한 |
|---|---|---|---|
| `GetPoolActorFromClass` / `PoolActor` | `ATDGameMode` | `UTDPoolComponent` (GameMode 소속) | 서버 |
| `ITDPoolActorInterface` | `TDPoolActorInterface.h` | 유지 | - |
| 투사체 풀링 | `ATDProjectile` | 유지 (서버 스폰 → 자동 복제) | 서버 |

---

## 4. 파일 맵 (목표)

```
Source/TowerDefence/
├── Public/
│   ├── GameData/
│   │   ├── TDTowerDataTableSubsystem.h     [NEW] GameInstanceSubsystem
│   │   └── TDEnemyDataTableSubsystem.h     [NEW] GameInstanceSubsystem
│   ├── Session/
│   │   └── TDLevelSessionSubsystem.h       [NEW] GameInstanceSubsystem, 스테이지 전환+DT 주입
│   ├── Server/
│   │   ├── TDTowerSpawnerComponent.h      [NEW]
│   │   ├── TDEnemySpawnerComponent.h      [NEW/선택]
│   │   └── TDPoolComponent.h              [NEW/선택]
│   ├── Player/
│   │   ├── TDPlayerController.h           [EXPAND]  Server RPC 집약
│   │   ├── TDTowerHighlightSubsystem.h    [NEW]     LocalPlayerSubsystem
│   │   ├── TDTowerPlacementSubsystem.h    [NEW]     LocalPlayerSubsystem
│   │   └── TDHUDSubsystem.h               [NEW]     LocalPlayerSubsystem
│   ├── UI/
│   │   └── TDTowerActionWidgetBase.h      [NEW]     UUserWidget 베이스 (BP WBP_TowerActions 부모)
│   ├── TDGameMode.h                       [MODIFY]  Spawner 조립
│   ├── TDGameState.h                      [MODIFY]  Replicated + Multicast
│   ├── TDWaveManagerComponent.h           (유지)
│   ├── TDEventManagerComponent.h          [DEPRECATE → GameState Multicast]
│   ├── TowerManager.h                     [DELETE]
│   ├── TDPlayerCharacter.h                [SHRINK]  카메라/입력만
│   ├── TDTowerBase.h / TDTowerPawn.h      (유지)    Replicated Actor
│   ├── TDEnemyActor.h                     (유지)    Replicated Actor
│   ├── TDProjectile.h                     (유지)    Replicated Actor
│   └── TDFL_Utility.h                     [MODIFY]  Subsystem 접근자 추가
└── Private/
    └── (대응)
```

---

## 5. 데이터 흐름 샘플

### 5-1. 타워 설치 (멀티플레이)

```
[Client A]                                  [Server]
마우스 호버
  ↓
UTDTowerPlacementSubsystem::BeginPlacement
  ↓ (유령 메시 표시, 본인만)
ConfirmPlacement (클릭)
  ↓
ATDPlayerController::ServerPlaceTower ─RPC─▶ GameMode->TowerSpawner->TryPlace
                                              ├ 슬롯 점유 체크
                                              ├ GameState->HasCoins 확인
                                              ├ GameState->CoinChange(-cost)   [Replicated]
                                              ├ SpawnActor<ATDTowerBase>       [자동 복제]
                                              └ GameState->PlacedTowers.Add    [Replicated]
                                                             ↓
[All Clients] ◀─── OnRep_PlacedTowers / OnRep_SharedCoin 수신 → HUD 갱신
```

### 5-2. 스테이지 전환 (레벨별 DT 주입)

```
[메뉴]
UTDLevelSessionSubsystem::RequestLoadStage(StageId)
  ├ DT_Stages 에서 Row 검색 → Map 경로 획득
  └ UGameplayStatics::OpenLevel(MapPath)
         ↓
[월드 로드 완료]  (메뉴 경유 / PIE 직접 열기 둘 다 동일)
FCoreUObjectDelegates::PostLoadMapWithWorld  ─► OnPostLoadMap(NewWorld)
         ↓
  DT_Stages 에서 NewWorld 경로로 Row 역조회
         ↓
  ApplyStageData(Row)
    ├ TowerDT → UTDTowerDataTableSubsystem::LoadFromDataTable
    ├ EnemyDT → UTDEnemyDataTableSubsystem::LoadFromDataTable
    └ WaveDT  → (WaveManager 에 주입)

※ 서버/클라 각자의 머신에서 독립적으로 실행 → 각자 로컬 Subsystem 캐시만 갱신 (복제 아님)
```

### 5-3. 적 사망 (멀티플레이)

```
[Server]
ATDEnemyActor::OnHealthAttributeChanged (HP<=0)
  ↓
OnEnemyDied
  ├ GameState->CoinChange(+reward)                           [Replicated]
  ├ GameState->ActiveEnemies.Remove(Enemy)                   [Replicated]
  ├ GameState->MulticastOnEnemyDied(Enemy) ─ NetMulticast ─┐
  ├ SetActorEnableCollision(false)                          │
  └ Timer(DeathDelay) → Destroy()                           │
                                                             ▼
[All Clients] PlayDeathAnimation, UI 갱신, 사운드 재생
```

---

## 6. 컨벤션

### 6-1. 네이밍

- **Subsystem**: `U<Domain><Role>Subsystem` (예: `UTDTowerHighlightSubsystem`)
- **Component (서버 로직)**: `U<Domain><Role>Component` (예: `UTDTowerSpawnerComponent`)
- **Replicated Actor**: 기존대로 `A<Domain><Type>` (예: `ATDEnemyActor`)
- **Server RPC**: `ServerXxx`, **Multicast RPC**: `MulticastXxx`, **Client RPC**: `ClientXxx`
- **OnRep 함수**: `OnRep_<PropertyName>`

### 6-2. 참조 관리

| 패턴 | 용도 |
|---|---|
| `UPROPERTY() TObjectPtr<T>` | 소유/강참조, GC 추적 |
| `TWeakObjectPtr<T>` | GC를 방해하면 안 되는 참조 (예: `HighlightedTower`) |
| `TSoftObjectPtr<T>` | 에셋 지연 로드 (예: `BP_DT_TowerData`) |

### 6-3. Utility 접근자

신규 Subsystem은 `UTDFL_Utility`에 접근자를 추가해 호출 지점을 단일화한다.

```cpp
static UTDTowerDataTableSubsystem* GetTowerDataTable(const UObject* WorldContextObject);
static UTDTowerHighlightSubsystem* GetTowerHighlight(const UObject* WorldContextObject);
```

### 6-4. 복제 체크리스트

새 상태를 복제로 공개할 때:

- [ ] `UPROPERTY(ReplicatedUsing=OnRep_Xxx)` 지정
- [ ] `GetLifetimeReplicatedProps` 오버라이드에 `DOREPLIFETIME` 등록
- [ ] `OnRep_Xxx` 함수 구현 (UI 갱신 델리게이트 브로드캐스트)
- [ ] 기존 델리게이트(`OnCoinsChanged` 등)를 OnRep에서 호출하도록 전환
- [ ] 클라에서 **직접 쓰기 금지** — Server RPC 경유 확인

### 6-5. Subsystem에 금지 사항

- `SpawnActor` (권위 로직) → Component로 이동
- `HasAuthority` 체크 → Subsystem은 로컬이므로 분기 자체가 신호 오류
- 다른 클라에 영향을 미치는 상태 변경 → GameState/RPC 경유

---

## 7. 마이그레이션 순서 (작은 PR 단위)

진행 중인 개발을 끊지 않도록 아래 순서대로. 각 단계는 독립 빌드 가능.

1. **GameState 복제화**
   - `SharedCoin`, `BaseHealth`, `CurrentWave`에 `Replicated` 지정
   - `OnRep_*`에서 기존 델리게이트 브로드캐스트
2. **TowerDataTableSubsystem 도입**
   - `ATowerManager::GetTowerData` 사용처를 전부 Subsystem으로 교체
   - `ATowerManager` 아직 유지 (호환용)
3. **TowerSpawnerComponent 도입**
   - `SpawnTowers` 이식, GameMode에서 컴포넌트 생성
   - `ATowerManager` 폐기
4. **TowerHighlightSubsystem 도입**
   - `UpdateHightlight` 이식, `ATDPlayerCharacter::SelectTower` 경로 이관
5. **EventManager → GameState Multicast 이전**
   - `OnEnemyDied`를 `ATDGameState::MulticastOnEnemyDied`로 변환
   - `UTDEventManagerComponent` 삭제
6. **PlayerController Server RPC 도입**
   - 기존 BP의 설치/판매 호출부를 RPC로 교체
7. **ActiveEnemies 복제**
   - WaveManager의 Enemies를 GameState로 이동
   - `GetFurthestEnemy`는 GameState 조회로 전환
8. **TowerActionWidget BP→C++ 분리 + PC 가 위젯 라이프사이클 소유** ✅ C++ 완료
   - `UTDTowerActionWidgetBase` = 단일 로직 허브 (데이터/비용/이벤트/RPC 위임 모두 집결)
       - `InitForTower`, `Show/Hide`, `RefreshSlots`, `RequestActionTop/Bottom/Left/Right`, `GetActionForSlot`
       - 4 슬롯 BindWidget (`ActionSlotTop/Bottom/Left/Right`)
       - OnCoinsChanged 자동 바인딩/해제, OnSlotRefreshed BP 이벤트로 시각 위임
   - `ATDPlayerController` 가 메뉴 라이프사이클 소유 (CLAUDE.md §1-2 의 "UI=LocalPlayer 권한" 부합)
       - `HandleClick` 에서 Tower 히트 감지 → `ShowTowerActionMenu(Tower)`
       - 첫 표시 시 `CreateWidget(this, ActionWidgetClass)` + `AddToPlayerScreen` (캐시 후 재사용)
       - Tower `OnDestroyed` 구독 → 자동 메뉴 닫힘
       - 슬롯 클릭 → `HandleSlotClicked(Action)` → `Server_DoTowerAction` (단일 RPC 진입점)
   - 남은 작업: `WBP_TowerActions` Reparent → `UTDTowerActionWidgetBase` + `WBP_TowerActionSlot` 변수 정리 + 슬롯 OnSlotClicked 디스패처 + `BP_TDPlayerController` 의 `ActionWidgetClass` 지정
9. **EnemyDataTableSubsystem / EnemySpawnerComponent 분리 (선택)**
10. **PoolComponent 분리 (선택)**

---

## 8. 자주 틀리는 체크 포인트

- **`UTDFL_Utility::GetWaveManager(this)` 가 클라에서 null** — GameMode는 서버에만 존재. 클라 코드는 GameState 경유로 바꿀 것.
- **Subsystem에 `EditAnywhere` 기대 금지** — 에디터에서 값 못 고침. `UDeveloperSettings` 또는 DataAsset 사용.
- **Subsystem `Initialize()` 타이밍** — 레벨 액터 아직 없음. 월드 액터 탐색은 `OnWorldBeginPlay`에서.
- **`UGameplayStatics::GetPlayerController(this, 0)` 로컬 플레이어 의도일 때** — 리슨 서버에서 의도와 다르게 서버 플레이어를 집을 수 있음. `GetWorld()->GetFirstLocalPlayerController()` 고려.
- **`TObjectPtr` 으로 잡힌 Actor가 `Destroy()` 되어도 소멸 안 됨** — 강참조 해제 누락. 불필요한 참조는 `TWeakObjectPtr`로.
- **GameMode의 Owner로 스폰해도 자동 소멸 안 됨** — Owner는 논리적 표시일 뿐. `UChildActorComponent` 또는 `Destroyed()` 오버라이드 필요.
- **`ProcessEvent`로 BP 함수 호출 시 파라미터 오정렬** — 직접 C++ 함수로 노출하거나 `UFUNCTION(BlueprintCallable)`로 바인딩 권장.
- **GameMode에 붙인 컴포넌트는 클라에서 실행되지 않는다** — 레벨 관련 초기화(DT 주입 등)가 양쪽에 필요하면 `GameInstanceSubsystem` + `PostLoadMapWithWorld` 훅에 둘 것.
- **`FCoreUObjectDelegates::PostLoadMapWithWorld`는 PIE 에서도 호출** — 직접 열기(Play-in-Editor)와 메뉴 경유 OpenLevel 둘 다 잡아준다. 메뉴 전용으로 가정하지 말 것.

---

## 9. 용어

- **권위(Authoritative)**: 해당 상태의 진실 소유자. 서버만이 가짐.
- **관찰자(Observer)**: 복제된 상태를 읽기만. Subsystem, HUD, 클라 측 로직.
- **리슨 서버(Listen Server)**: 호스트가 게임도 플레이하는 구조. `NM_ListenServer`.
- **데디케이티드 서버(Dedicated Server)**: 호스트는 플레이하지 않음. `NM_DedicatedServer`.

---

## 10. 변경 로그

| 날짜 | 내용 | 담당 |
|---|---|---|
| 2026-04-21 | 초안 작성: 5레이어 아키텍처, 도메인별 배치, 마이그레이션 순서 정의 | - |
| 2026-04-21 | `Database` → `DataTable` 네이밍 통일, 폴더 `Data/` → `GameData/` | - |
| 2026-04-22 | `UTDLevelSessionSubsystem` 추가 — 스테이지 전환 및 레벨별 DT 주입 (`DT_Stages` + `PostLoadMapWithWorld` 훅) | - |
| 2026-04-26 | `ATDTowerBase` 의 `ATowerManager` 직접 참조 제거 → `UTDTowerDataTableSubsystem` 직접 조회로 전환. `GetTowerDetails` 의 멤버 `TowerData` 오염 버그 + `DoTowerAction` 의 BreakDown 케이스 누락 버그 수정 | - |
| 2026-04-26 | `UTDLevelSessionSubsystem::ShowStageDataError` 추가 — 스테이지 데이터 어셋 누락/로드 실패 시 에디터 모달 + 에러 로그 | - |
| 2026-04-26 | `UTDTowerActionWidgetBase` 추가 — `WBP_TowerActions` 의 C++ 베이스. 데이터/로직(비용 계산, OnCoinsChanged 바인딩, Server RPC 위임) C++, 시각/스타일 BP 자식. UI Layer 정의 추가 | - |
| 2026-04-27 | `ATDPlayerController` 메뉴 라이프사이클 소유 — `ShowTowerActionMenu/HideTowerActionMenu/HandleSlotClicked` + `ActionWidgetClass`/`ActiveActionWidget`/`SelectedTower` 멤버. `HandleClick` 에서 Tower 히트 감지 → 메뉴 표시. Tower `OnDestroyed` 구독으로 자동 닫힘 | - |
| 2026-04-27 | `UTDTowerActionWidgetBase` 단일 허브 강화 — `Show/Hide/RefreshSlots`/`RequestActionTop/Bottom/Left/Right`/`GetActionForSlot` 추가. WBP 측 BP 함수/변수 모두 C++ 로 흡수 (BP 는 시각 + 4 슬롯 클릭 wiring 만 남김) | - |
