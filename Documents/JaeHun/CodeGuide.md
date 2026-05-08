# TowerDefence — 협업자용 코드 가이드

> 본 문서는 `Source/TowerDefence/Public` 아래 모든 헤더를 **기능별 도메인** 으로 묶어 정리한 것이다.
> 각 클래스가 (1) 어떤 기능을 위해, (2) 어떤 변수·함수를 가지고, (3) 어떤 다른 클래스와 연관되는지 한눈에 파악할 수 있게 구성했다.
>
> 단일 진실 소스 (아키텍처/배치 원칙) 는 루트의 [`CLAUDE.md`](../CLAUDE.md). 본 문서는 그 원칙이 **실제 코드에 어떻게 매핑돼 있는지** 의 사전 (dictionary) 역할.

---

## 0. 한눈에 보는 5 레이어

```
┌─────────────────────────────────────────────────────────────────┐
│ A. GameInstance (앱 실행 중, 로컬)                               │
│    UTDGameInstance  / DataTable Subsystem (Tower/Enemy)          │
│    Session Subsystem (Level/Lobby)                               │
├─────────────────────────────────────────────────────────────────┤
│ B. GameMode (레벨 1회, 서버 전용, 권위)                          │
│    ATDGameMode + Spawner/Pool/Wave Component                     │
│    ATDLobbyGameMode (로비 전용)                                  │
├─────────────────────────────────────────────────────────────────┤
│ C. GameState (레벨 1회, 모두 복제)                               │
│    ATDGameState (코인/체력/적/타워 + Multicast 이벤트)           │
│    ATDLobbyGameState (참가자 슬롯)                               │
├─────────────────────────────────────────────────────────────────┤
│ D. Replicated Actor (자가 복제)                                  │
│    Tower (TowerBase/Pawn/Weapon) / Enemy (TDEnemyActor)          │
│    Projectile (비복제 코스메틱) / Path                           │
├─────────────────────────────────────────────────────────────────┤
│ E. LocalPlayer (본인 클라만, 플레이어 세션)                      │
│    ATDPlayerController (Server RPC 단일 진입점)                  │
│    ATDPlayerCharacter (카메라/입력)                              │
│    Highlight / Placement Subsystem (호버/프리뷰)                 │
└─────────────────────────────────────────────────────────────────┘
```

원칙 (CLAUDE.md § 1):
- **수직 체인 금지**. Manager → Subsystem 직참조 X. **GameState 매개**.
- **클라→서버 요청은 단 하나의 진입점** = `ATDPlayerController::Server_*`.
- **권한/수명/책임** 으로 컨테이너 결정.

---

## 1. 도메인 ① Core — 게임 진입점 / 공유 상태 / 유틸

### 1-1. 도메인 클래스 표

| 클래스 | 상위 | 수명 | 권한 | 역할 한 줄 | 위치 |
|---|---|---|---|---|---|
| `UTDGameInstance` | `UGameInstance` | 앱 실행 중 | 로컬 | 세이브 슬롯 관리 (Save/Load) | `TDGameInstance.h` |
| `ATDGameMode` | `AGameModeBase` | 레벨 1회 | 서버만 | Spawner/Pool/Wave 컴포넌트 조립 + 게임 판정 | `TDGameMode.h` |
| `ATDGameState` | `AGameStateBase` | 레벨 1회 | 복제 | **공유 상태의 진실 소스** (코인/체력/타워/적) + Multicast 이벤트 허브 | `TDGameState.h` |
| `UTDFL_Utility` | `UBlueprintFunctionLibrary` | 정적 | - | Subsystem/Manager 접근자 일원화 (BP/C++ 양쪽에서) | `TDFL_Utility.h` |
| `ATDPlayerCharacter` | `APawn` | 플레이어 세션 | 로컬 | 카메라 + 엣지 스크롤 + HUD 위젯 보유 | `TDPlayerCharacter.h` |
| `ATDPlayerPawn` | `APawn` | 레벨 1회 | 복제 | 클릭 이동 폰 (Server RPC + OnRep_MoveTarget) | `TDPlayerPawn.h` |
| `UTDMainSaveGame` | `USaveGame` | 영구 | 로컬 | 세이브 슬롯 인덱스 보관 | `TDMainSaveGame.h` |
| `UTDGameUserSettings` | `UGameUserSettings` | 영구 (config) | 로컬 | 사운드 볼륨 (Master/Effects/Music) | `TDGameUserSettings.h` |

### 1-2. `ATDGameState` — 가장 중요한 클래스 (공유 상태 + 이벤트 허브)

| 변수 | 타입 | 복제 | 용도 |
|---|---|---|---|
| `SharedCoin` | `int32` | `OnRep_SharedCoin` | 공유 코인 (시작 2000) |
| `BaseHealth` | `int32` | `OnRep_BaseHealth` | 기지 체력 (시작 20) |
| `MaxBaseHealth` | `int32` | `Replicated` | 최대 기지 체력 |
| `CurrentWave` | `int32` | `OnRep_CurrentWave` | 현재 웨이브 번호 |
| `PlacedTowers` | `TArray<ATDTowerBase*>` | `OnRep_PlacedTowers` | 배치된 타워 목록 |
| `ActiveEnemies` | `TArray<ATDEnemyActor*>` | `OnRep_ActiveEnemies` | 살아있는 적 목록 |

| 함수 | 시그니처 | 호출자 | 비고 |
|---|---|---|---|
| `HasCoins / GetCoins` | `(int32) → bool / int32` | Tower, UI | 순수 조회 |
| `CoinChange` | `(int32 change)` | TowerSpawner, EnemyActor | 서버 호출 → OnRep 전파 |
| `DecreaseBaseHealth` | `()` | Enemy 도달 시 | 서버 호출 → OnRep 전파 |
| `RegisterTower / UnregisterTower` | `(ATDTowerBase*)` | TowerSpawner | 배열 갱신 |
| `RegisterEnemy / UnregisterEnemy` | `(ATDEnemyActor*)` | EnemySpawner / WaveManager | 배열 갱신 |
| `GetFurthestEnemy` | `(FVector, float Radius) → ATDEnemyActor*` | Weapon::FindEnemy | 양쪽 호출 가능 |
| `GetEnemiesInRange` | `(FVector, float) → TArray` | Projectile 광역 데미지 | 양쪽 호출 가능 |
| `NotifyEnemyDied / NotifyEnemyAttacked` | | EnemyActor (서버) | → `Multicast_OnEnemyDied/Attacked` 발화 |

| 델리게이트 (Multicast) | 구독자 | 트리거 |
|---|---|---|
| `OnBaseHealthChanged` | UI HUD | OnRep_BaseHealth |
| `OnCoinsChanged` | UI (TowerActionWidget 등) | OnRep_SharedCoin |
| `OnEnemyDied` | UI / 사운드 / VFX | Multicast_OnEnemyDied |
| `OnEnemyAttacked` | UI / 사운드 | Multicast_OnEnemyAttacked |
| `OnGameEnded` | UI 결과 화면 | `BroadcastGameEnded(bWin)` |

**연관 클래스**: 거의 모든 도메인이 이 클래스로 모임. 특히 — Tower, Enemy, Weapon, UI 위젯, PlayerController.

### 1-3. `ATDGameMode` (서버 전용)

| 변수 | 타입 | 비고 |
|---|---|---|
| `WaveManager` | `UTDWaveManagerComponent*` | 웨이브 진행 |
| `EnemySpawner` | `UTDEnemySpawnerComponent*` | 적 SpawnActor |
| `TowerSpawner` | `UTDTowerSpawnerComponent*` | 타워 SpawnActor |
| `Pool` | `UTDPoolComponent*` | ActorPool |
| `EnemyClasses` | `TMap<EEnemyType, TSubclassOf<ATDEnemyActor>>` | BeginPlay 에서 EnemyDataTableSubsystem 에 주입 |

| 함수 | 호출자 | 비고 |
|---|---|---|
| `BeginPlay` | 엔진 | 컴포넌트 조립 + Enemy 클래스 매핑 주입 |
| `GameEnded(bool bWin)` | `CheckIfWin/Loss` | `GameState->BroadcastGameEnded` |
| `CheckIfWin / CheckIfLoss` | WaveManager / Enemy | 종료 조건 검사 |
| `InitializeStage(FStageRow)` | `UTDLevelSessionSubsystem::ApplyStageData` | TowerSpawner::SpawnInitialTowerBases 호출 |
| `GetPoolActorFromClass / PoolActor` | (BP 호환) | `Pool` 컴포넌트로 위임 |

**연관**: `UTDLevelSessionSubsystem` (스테이지 주입 트리거), `UTDPlayerController` (`GameMode->Pool` 접근).

### 1-4. `ATDPlayerCharacter` vs `ATDPlayerPawn`

- **PlayerCharacter**: 클라 본인의 카메라/입력. 비복제 파트는 카메라, 복제 파트는 `PlayerPawn` 참조.
- **PlayerPawn**: 클릭 이동을 위한 별도 폰. `Server_SetMoveTarget` Server RPC + `OnRep_MoveTarget` 으로 모든 클라 동기화.

### 1-5. `UTDFL_Utility` — Static 접근자

서버/클라/BP/C++ 어디서든 동일한 인터페이스로 핵심 객체 가져오는 Function Library.
주요 함수: `GetTDGameMode`, `GetTDGameState`, `GetLobbySession`, `GetWaveManager`, `GetEnemyDataTable`, `GetPool`, `GetPlayer`, `GetTDPlayerController`, `GetTowerUnderMouse`, `EnemyDamage(Enemy, Damage, GE_Class)`, `GetVolume / SetVolume / GetVideoQuality / SetVideoQuality`.

---

## 2. 도메인 ② GameData — 정적 데이터 조회

`GameInstanceSubsystem` 패턴. 앱 실행 중 한 번 캐싱 → 어디서든 readonly 조회.

| 클래스 | 캐시 데이터 | 주입 시점 | 조회 함수 |
|---|---|---|---|
| `UTDTowerDataTableSubsystem` | `TMap<ETowerType, FTowerData>` + `BaseTowerClass` | `UTDLevelSessionSubsystem::ApplyStageData` 가 `LoadFromDataTable(TowerDT)` 호출 | `GetTowerData(Type, OutTowerData)` |
| `UTDEnemyDataTableSubsystem` | `TMap<EEnemyType, TSubclassOf<ATDEnemyActor>>` | `ATDGameMode::BeginPlay` 가 `RegisterEnemyClasses(EnemyClasses)` 호출 | `GetEnemyClass(Type) → TSubclassOf` |

**연관**:
- Tower → `UTDTowerDataTableSubsystem::GetTowerData` (어트리뷰트 적용용)
- WaveManager / EnemySpawner → `UTDEnemyDataTableSubsystem::GetEnemyClass` (스폰 클래스 결정)

---

## 3. 도메인 ③ Session — 레벨 / 멀티 세션 라이프사이클

> **이름 구분 주의**: `LevelSession` ≠ `LobbySession`. 책임 분리됨.

### 3-1. `UTDLevelSessionSubsystem` — 스테이지 전환 + DT 주입

`FStageRow` 구조체 (DT_Stages 한 행):
- `StageId` (FName), `Map` (TSoftObjectPtr<UWorld>)
- `TowerDT / EnemyDT / WaveDT` (각 도메인 DT)
- `BaseTowerClass / TileMeshName / TileSpawnZOffset` (초기 타워 스폰 설정)

| 함수 | 시그니처 | 동작 |
|---|---|---|
| `RequestLoadStage` | `(FName) → bool` | DT_Stages → Row 검색 → `OpenLevel(Map)` |
| `GetMapPathByStageId` | `(FName) → FString` | ServerTravel URL 조립용 |
| `GetAllStages` | `() → TArray<FStageRow>` | 레벨 선택 위젯 초기화 |
| `K2_ReloadCurrentStage` | (BP 진입점) | 현재 스테이지 재초기화 (테스트용) |
| `DebugApplyStage` | `(StageId, TowerDT, ...)` | DT_Stages 우회 — 테스트 레벨용 |
| `OnPostLoadMap` (private) | 엔진 콜백 | `PostLoadMapWithWorld` 훅에서 자동 호출 → DT 주입 |

**연관**: `UTDTowerDataTableSubsystem`, `UTDEnemyDataTableSubsystem`, `ATDGameMode::InitializeStage`.

### 3-2. `UTDLobbySessionSubsystem` — OnlineSubsystem 단일 허브

`FTDSessionInfo` 구조체 (BP 노출용 경량 래퍼):
- `OwnerName`, `NumOpenPublicConnections / MaxPublicConnections`, `PingInMs`, `SearchResultIndex`

| 함수 | 시그니처 | 완료 시 발화 |
|---|---|---|
| `CreateSession` | `(NumPublicConnections, bIsLAN)` | `OnSessionCreated(bSuccess)` |
| `FindSessions` | `(MaxResults, bIsLAN)` | `OnSessionsFound(bSuccess)` |
| `JoinSession` | `(SearchResultIndex)` | `OnSessionJoined(bSuccess)` |
| `DestroySession` | `()` | `OnSessionDestroyed(bSuccess)` |
| `GetLastFoundSessions` | `() → TArray<FTDSessionInfo>` | 위젯이 결과 조회 |
| `SetMultiStageId / GetMultiStageId` | `(FName) / () → FName` | 호스트가 선택한 스테이지 ID 보관 — `Server_RequestStart` 가 사용 |

**규칙**: 위젯/PC 가 `IOnlineSession` 직접 호출 금지. 반드시 이 클래스 경유.

**연관**: `UTDPlayModeSelectWidget` (멀티 분기 시 SetMultiStageId), `UTDMultiLobbyWidget` (Create/Find), `UTDSessionEntryWidget` (Join), `ATDLobbyPlayerController::Server_RequestStart` (GetMultiStageId).

---

## 4. 도메인 ④ Lobby — 멀티 로비 (Levels-Lobby.umap)

> 로비는 **본 게임플레이와 별개의 GameMode/GameState/PC/PS** 사용.

| 클래스 | 상위 | 책임 |
|---|---|---|
| `ATDLobbyGameMode` | `AGameModeBase` | PostLogin/Logout → GameState 슬롯 동기화. 호스트 Logout 시 모든 클라 복귀 |
| `ATDLobbyGameState` | `AGameStateBase` | `PlayerSlots: TArray<ATDLobbyPlayerState*>` (Replicated) + `OnRep_PlayerSlots` |
| `ATDLobbyPlayerController` | `APlayerController` | `Server_SetReady`, `Server_RequestStart` (Authority 게이트), `Client_ReturnToMainMenu` |
| `ATDLobbyPlayerState` | `APlayerState` | `bIsReady` (Replicated) + `OnRep_bIsReady` |

### 4-1. `ATDLobbyPlayerController` 핵심 함수

| 함수 | 종류 | 동작 |
|---|---|---|
| `Server_SetReady(bReady)` | Server RPC | PlayerState 의 SetReady 호출 → 자동 복제 |
| `Server_RequestStart()` | Server RPC | Authority + AreAllPlayersReady 검증 → `ServerTravel(GameLevelPath?listen)` |
| `Client_ReturnToMainMenu(Reason)` | Client RPC | 호스트 종료 / 강제 퇴장 시 메뉴 복귀 |

| 변수 | 비고 |
|---|---|
| `GameLevelPath` | 기본값 `/Game/Levels/Levels-01`. `LobbySubsystem::GetMultiStageId` 가 우선. |
| `WaitingRoomWidgetClass` | BP_LobbyPlayerController Details 에서 `WBP_WaitingRoom` 지정 |

**연관**: `ATDLobbyGameMode` (GameStateClass / PlayerControllerClass / PlayerStateClass 지정), `UTDLobbySessionSubsystem` (GetMultiStageId), `UTDWaitingRoomWidget` (위젯 인스턴스).

---

## 5. 도메인 ⑤ Player (LocalPlayer 레이어)

> 본인 클라만 가지는 객체들. 입력 / 호버 / 프리뷰 / Server RPC 진입점.

### 5-1. `ATDPlayerController` — Server RPC 단일 진입점

> **CLAUDE.md § 1-3 의 핵심 원칙 구현체.** 모든 클라→서버 요청이 여기를 거침.

| 변수 | 비고 |
|---|---|
| `ClickAction` | `UInputAction*` (IA_Click 에셋) |
| `ActionWidgetClass` | `TSoftClassPtr<UTDTowerActionWidgetBase>` — 첫 표시 시 CreateWidget |
| `ActiveActionWidget` | 캐시된 위젯 인스턴스 (재사용) |
| `SelectedTower` | 메뉴 표시 중인 타워 |

| 함수 | 종류 | 동작 |
|---|---|---|
| `HandleClick()` | 로컬 | Tower 히트 감지 → `ShowTowerActionMenu(Tower)` 또는 `HideTowerActionMenu()` |
| `ShowTowerActionMenu(Tower)` | 로컬 | 위젯 첫 표시 시 CreateWidget + AddToPlayerScreen, 같은 Tower 재클릭은 토글 |
| `HideTowerActionMenu()` | 로컬 | Visibility = Collapsed |
| `HandleSlotClicked(Action)` | 로컬 | 슬롯 위젯 → `Server_DoTowerAction` 위임 |
| **`Server_DoTowerAction(Tower, Action)`** | **Server RPC** | 타워 액션 (Build/Upgrade/BreakDown) 의 유일한 진입점 |
| `HandleSelectedTowerDestroyed` | OnDestroyed 콜백 | 타워 파괴 시 메뉴 자동 닫힘 |

**연관**: `ATDTowerBase`, `UTDTowerActionWidgetBase`, `UTDTowerSpawnerComponent::DoTowerAction`.

### 5-2. `UTDTowerHighlightSubsystem` (LocalPlayerSubsystem)

마우스 호버 하이라이트 + 타워 선택 상태. **본인 클라만**.

| 변수 (private) | 타입 |
|---|---|
| `HoveredTower` / `SelectedTower` | `TWeakObjectPtr<ATDTowerBase>` (GC 비방해) |

| 함수 | 동작 |
|---|---|
| `Tick(DeltaTime)` (FTickableGameObject) | 매 프레임 마우스 아래 타워 검사 → `SetHoveredTower` |
| `SelectTower / UnSelectTower` | 로컬 UI 상태만 변경 |
| `GetHoveredTower / GetSelectedTower` | (BP) 조회 |

| 델리게이트 | 트리거 |
|---|---|
| `OnHoveredTowerChanged` | 호버 변경 시 |
| `OnSelectedTowerChanged` | Select/UnSelect 호출 시 |

### 5-3. `UTDTowerPlacementSubsystem` (LocalPlayerSubsystem)

타워 설치 프리뷰(유령 메시) 관리. 실제 설치는 PC Server RPC 위임.

| 함수 | 동작 |
|---|---|
| `BeginPlacement(TowerClass)` | 유령 메시 표시 시작 |
| `ConfirmPlacement()` | Server RPC 전송 후 프리뷰 종료 |
| `CancelPlacement()` | 유령 메시 제거 |
| `IsPlacing / IsValidPlacement / GetPlacementLocation` | 조회 |

| 델리게이트 | 트리거 |
|---|---|
| `OnPlacementStateChanged(bIsPlacing)` | Begin/Cancel/Confirm 시 |

---

## 6. 도메인 ⑥ Server — 서버 권위 컴포넌트

> `ATDGameMode` 가 소유하는 컴포넌트들 + 풀링 인프라.

### 6-1. 컴포넌트 표

| 컴포넌트 | 책임 | 핵심 함수 |
|---|---|---|
| `UTDTowerSpawnerComponent` | 타일 스캔 + 타워 스폰/교체/업그레이드 | `SpawnInitialTowerBases(Row)`, `SpawnTower(BaseTower, NewClass)`, `DoTowerAction(Tower, Action)` |
| `UTDEnemySpawnerComponent` | EnemyType + Path → SpawnActor | `SpawnEnemy(Type, Path) → ATDEnemyActor*` |
| `UTDPoolComponent` | ActorPool 관리 | `GetPoolActorFromClass(Class, Transform, Owner)`, `ReturnToPool(Actor)` |

### 6-2. Pool 인프라

```
ITDPoolActorInterface  (인터페이스)
    │
    ├── OnAddedToPool()      ← Hidden + Tick 비활성
    └── OnRemovedFromPool()  ← Visible + Tick 활성

ATDPoolActor : public AActor, ITDPoolActorInterface  (베이스 액터)
    │
    └── 자식: ATDProjectile (재사용 대상)
```

`UTDPoolComponent::ActorPool` = `TMap<TSubclassOf<AActor>, TArray<AActor*>>` — 클래스별 풀.

**연관**: `ATDProjectile` (서버에서만 풀 사용), `ATDGameMode::Pool`.

---

## 7. 도메인 ⑦ Tower

### 7-1. 클래스 계층

```
APawn ──── ATDTowerPawn ──── ATDTowerBase
            │ (GAS 베이스)    │ (BP_Tower 의 부모)
            │ - ASC           │ - 4 슬롯 ETowerActions
            │ - TowerSet      │ - UpgradeLevel (Replicated)
            │ - DefaultEffect │ - GetTowerDetails / UpgradeTower

AActor ──── ATD_Weapon  (TowerBase 의 자식 ChildActor)
            │
            └── MulticastFireProjectile (모든 머신 동시 발사)

AActor ──── ATDPoolActor ──── ATDProjectile  (비복제 코스메틱)
```

### 7-2. `ATDTowerBase`

| 변수 | 타입 | 비고 |
|---|---|---|
| `Type` | `ETowerType` | 어떤 타워인지 |
| `TowerData` | `FTowerData` | DataTable 에서 조회한 스탯 |
| `UpgradeLevel` | `int32` | `OnRep_UpgradeLevel` (Replicated) |
| `ChildActorWeaponComp` | `UChildActorComponent*` | 무기 자식 액터 |
| `HighlightStaticMeshComp` | 하이라이트 메시 | 호버 시 켜짐 |
| `BoxComp` | `UBoxComponent*` | 클릭 충돌 |
| `BP_TowerActionsClass / BP_TowerActions` | 위젯 액터 | (레거시 BP 호환) |
| `TowerActionTop/Bottom/Left/Right` | `ETowerActions` | 4 방향 슬롯 매핑 |
| `TurretClass / BallistaClass / CatapultClass / CannonClass / BaseTowerClass` | `TSubclassOf<AActor>` | DoTowerAction 시 교체 대상 |

| 함수 | 비고 |
|---|---|
| `SetHighlight(bool)` | HighlightStaticMeshComp 가시성 제어 |
| `CanUpgrade() → bool` | UpgradeLevel < 3 |
| `GetRange / GetDamage / GetRadius` | TowerSet (GAS) 어트리뷰트 조회 |
| `GetUpgradeCost / GetBreakdownRefund` | TowerData 기반 |
| `Select / UnSelect` | (선택 상태 표시) |
| `GetTowerDetails(Action, OutCost, OutDesc)` | UI 가 표시할 비용/설명 반환 |
| `UpgradeTower()` | UpgradeLevel++ — `TowerSpawnerComponent::DoTowerAction` 에서 호출 |
| `GetTowerData()` | `UTDTowerDataTableSubsystem` 직접 조회 후 GAS 적용 |
| `SetTowerAttributes()` (private) | TowerData → ASC InitialEffect |

### 7-3. `ATDTowerPawn` (GAS 베이스)

`IAbilitySystemInterface` 구현. 자식 클래스가 어트리뷰트셋 (TowerSet) 보유.

| 변수 | 타입 |
|---|---|
| `AbilitySystemComponent` | `UAbilitySystemComponent*` |
| `TowerSet` | `UTDTowerSet*` |
| `DefaultAbility` | `TSubclassOf<UGameplayAbility>` |
| `DefaultEffect` | `TSubclassOf<UGameplayEffect>` |

### 7-4. `UTDTowerSet` (GAS AttributeSet)

상속: `UTDBaseSet` ← `UAttributeSet`

| 어트리뷰트 | 용도 |
|---|---|
| `Range` | 발사 사거리 |
| `FireRate` | 발사 속도 |
| `Damage` | 발사 데미지 |

`ATTRIBUTE_ACCESSORS_BASIC` 매크로로 Get/Set/Init 자동 생성.

### 7-5. `ATD_Weapon` — Multicast 코스메틱 패턴

| 변수 | 타입 | 비고 |
|---|---|---|
| `Bottom / Top` | `UStaticMeshComponent*` | 회전축 분리 |
| `Target` | `ATDEnemyActor*` | 현재 타겟 |
| `Tower` | `ATDTowerBase*` | 부모 타워 |
| `ProjectileClass` | `TSubclassOf<ATDProjectile>` | 어떤 투사체 |
| `FirePoints` | `TArray<USceneComponent*>` | 발사 위치 (다중 포인트) |
| `FirePointIndex` | 다음 발사 인덱스 |

| 함수 | 비고 |
|---|---|
| `FindEnemy()` | `GameState->GetFurthestEnemy` 로 타겟 갱신 |
| `FaceEnemy()` | 회전 (Top/Bottom 분리) |
| `FireAtEnemy()` (Server) | HasAuthority 가드 → MulticastFireProjectile |
| `MulticastFireProjectile(Target, Damage, Radius, Loc, Rot)` | **NetMulticast Reliable** — 모든 머신 동시 발사 |
| `GetFirePoint(OutLoc, OutRot)` | FirePoints 순환 |

### 7-6. `ATDProjectile` — 비복제 코스메틱

`ATDPoolActor` 상속. **bReplicates = false** (각 머신 로컬 시뮬레이션). 데미지 권위는 서버 단일.

| 변수 | 타입 | 비고 |
|---|---|---|
| `Target` | `ATDEnemyActor*` | 추적 대상 |
| `Damage / Speed / Radius` | float | |
| `BP_GE_DamageClass` | `TSubclassOf<UGameplayEffect>` | 적용할 GE |
| `LastDistance` | float | 도착 판정용 |

| 함수 | 비고 |
|---|---|
| `MoveTowardsTarget(Delta)` | Tick |
| `OnHitTarget()` | 도달 시 → 서버 = `EnemyDamage`, 클라 = 스킵 → `DespawnSelf` |
| `SetProjectileData(Target, Damage, Radius)` | 발사 시 초기화 |
| `OnAddedToPool / OnRemovedFromPool` | (서버 측만 의미) |
| `DespawnSelf()` (private) | 서버: 풀 반납 / 클라: Destroy |

**연관**: `ATD_Weapon` (스폰 트리거), `UTDFL_Utility::EnemyDamage` (서버 데미지 적용).

---

## 8. 도메인 ⑧ Enemy

### 8-1. 클래스 계층

```
AActor ──── AEnemyActor       (레거시 — 사용 X, 폐기 예정)
       └── ATDEnemyActor       (현행)
            │ - ASC + EnemySet (GAS)
            │ - CurrentPath (Replicated)
            │ - Distance (Replicated)
            │ - IsDead (Replicated)
            │ - OnDied (델리게이트)
```

### 8-2. `ATDEnemyActor`

| 변수 | 타입 | 비고 |
|---|---|---|
| `AbilitySystemComponent` / `EnemySet` | GAS | Health/MaxHealth/MoveSpeed/Damage |
| `DefaultEffect` | `TSubclassOf<UGameplayEffect>` | InitialAttribute 적용 |
| `InitialHealth / InitialMoveSpeed / InitialDamage / RewardCoin` | float / int32 | BP 디폴트 |
| `CurrentPath` | `ATDPath*` | Replicated, OnRep_CurrentPath |
| `Distance` | float | Replicated, OnRep_Distance — 경로 진행도 |
| `IsDead` | bool | Replicated |
| `DeathDelay` | float | 사망 후 Destroy 까지 시간 |

| 함수 | 비고 |
|---|---|
| `InitializePath(Path)` | 스폰 후 호출 |
| `Advance(DeltaTime) → float` | 경로 진행 (서버) |
| `GetDistance() → float` | 조회 |
| `OnHealthAttributeChanged(Data)` | GAS 콜백 → HP<=0 → `OnEnemyDied` |
| `OnEnemyDied()` (protected) | `GameState->CoinChange(+reward)` + `NotifyEnemyDied` + Timer→Destroy |
| `PlayDeathAnimation()` | BlueprintImplementableEvent (BP 측 구현) |

| 델리게이트 | 비고 |
|---|---|
| `OnDied(ATDEnemyActor*)` | WaveManager 가 구독 (스폰 후 바인딩) |

**연관**: `ATDPath` (경로), `ATDGameState` (등록/해제, 사망 통보), `UTDEnemySet` (GAS), `UTDWaveManagerComponent` (스폰자).

### 8-3. `UTDEnemySet` (GAS)

| 어트리뷰트 | 용도 |
|---|---|
| `Health / MaxHealth` | 체력 |
| `MoveSpeed` | 이동속도 |
| `Damage` | 기지에 가하는 데미지 |

`ClampAttributeOnChange` 오버라이드 — Health 0~MaxHealth 클램프.

### 8-4. `UTDBaseSet` (공통 GAS 베이스)

`UAttributeSet` 상속. `PreAttributeBaseChange / PreAttributeChange` 에서 자식의 `ClampAttributeOnChange` 호출.

`ATTRIBUTE_ACCESSORS_BASIC(Class, Property)` 매크로 정의:
- `GAMEPLAYATTRIBUTE_PROPERTY_GETTER` + `VALUE_GETTER` + `VALUE_SETTER` + `VALUE_INITTER` 자동 생성.

---

## 9. 도메인 ⑨ Wave — 웨이브 / 경로

### 9-1. `UTDWaveManagerComponent` (GameMode 소속, 서버 전용)

| 변수 | 타입 | 비고 |
|---|---|---|
| `Paths` | `TArray<ATDPath*>` | BeginPlay 에서 월드 수집 |
| `PathLengths` | `TArray<float>` | 캐싱 |
| `Enemies` | `TArray<ATDEnemyActor*>` | (GameState 로 이전 예정) |
| `ExpiredEnemies` | `TArray<ATDEnemyActor*>` | 정리 대기 |
| `DataTable` / `WaveData` | DT / `TArray<FWaveData>` | 웨이브 시간/유형 정의 |
| `TotalEnemyCount / KillCount / TimeUntilEnemy` | float | 카운트 |

| 함수 | 비고 |
|---|---|
| `BeginPlay` | Path 수집 + 길이 캐싱 |
| `TickComponent` | `UpdateWave` |
| `UpdateWave(Delta)` | 스폰 타이밍 계산 |
| `AdvanceEnemies(Delta)` | 모든 적 `Advance` 호출 (서버) |
| `ImportData()` | DT → `WaveData` |
| `SpawnEnemy(EEnemyType)` | EnemySpawner 위임 + GameState 등록 |
| `OnEnemyDied(Enemy)` | KillCount++ + 종료 검사 |
| `DoEnemiesRemain() → bool` | 종료 판정 |
| `GetFurthestEnemy / GetEnemiesInRange` | (GameState 와 동일 — 호환용) |

### 9-2. `ATDPath` — Spline 기반 경로

| 변수 | 타입 |
|---|---|
| `SplineComponent` | `USplineComponent*` |

| 함수 | 비고 |
|---|---|
| `GetBakedWaypoints(int32 NumPoints) → TArray<FVector>` | 등분된 월드 좌표 |
| `GetLength() → float` | 스플라인 전체 길이 |
| `GetLocation(float Distance) → FVector` | 진행도별 위치 — `ATDEnemyActor::Advance` 가 호출 |

---

## 10. 도메인 ⑩ GAS — 베이스만

| 클래스 | 위치 | 비고 |
|---|---|---|
| `UTDBaseSet` | `GAS/TDBaseSet.h` | 모든 AttributeSet 의 부모. 클램프 훅 제공 |

자식: `UTDTowerSet` (Range/FireRate/Damage), `UTDEnemySet` (Health/MaxHealth/MoveSpeed/Damage)

매크로: `ATTRIBUTE_ACCESSORS_BASIC` — UE 표준 4 개 (`PROPERTY_GETTER / VALUE_GETTER / VALUE_SETTER / VALUE_INITTER`) 한꺼번에.

---

## 11. 도메인 ⑪ UI — 위젯 (모두 C++ 베이스 + BP 자식)

> 패턴: **C++ 베이스 = 데이터/로직/RPC 위임**, **BP 자식 = 시각/스타일/UMG 레이아웃**.

### 11-1. 위젯 표

| 위젯 | BP 자식 | 어디서 사용 | 책임 |
|---|---|---|---|
| `UTDTowerActionWidgetBase` | `WBP_TowerActions` | `ATDPlayerController::ShowTowerActionMenu` | 4 슬롯 (Build/Upgrade/BreakDown) — 비용 계산 + Server RPC 위임 |
| `UTDPlayModeSelectWidget` | `WBP_PlayModeSelect` | 메인 메뉴 → 스테이지 선택 후 표시 | Solo / Multi 분기 진입점 |
| `UTDMultiLobbyWidget` | `WBP_MultiLobby` | Multi 분기 후 | 방 목록 (Create/Refresh/Back) |
| `UTDSessionEntryWidget` | `WBP_SessionEntry` | MultiLobby 의 ScrollBox 한 줄 | 호스트명/인원/Join 버튼 |
| `UTDWaitingRoomWidget` | `WBP_WaitingRoom` | Levels-Lobby 진입 시 | 참가자 슬롯 + Ready/Start/Leave |
| `UTDPlayerEntryWidget` | `WBP_PlayerEntry` | WaitingRoom 의 ScrollBox 한 줄 | 참가자 이름 표시 |

### 11-2. `UTDTowerActionWidgetBase` (가장 복잡)

| 변수 | 비고 |
|---|---|
| `ActionTop/Bottom/Left/Right` | `ETowerActions` (BP CDO 에서 EditDefaults) |
| `ActionSlotTop/Bottom/Left/Right` | `UUserWidget*` (BindWidgetOptional) |
| `TargetTower` | `ATDTowerBase*` |
| `BoundGameState` | `TWeakObjectPtr<ATDGameState>` (NativeDestruct 시 RemoveDynamic 용) |

| 공개 API | 동작 |
|---|---|
| `InitForTower(Tower)` | TargetTower 설정 + GameState OnCoinsChanged 바인딩 + InitAllSlots + RefreshAllSlots |
| `Show / Hide` | Visibility 토글 |
| `RefreshSlots()` | 4 슬롯 재계산 (외부 트리거용) |
| `RequestTowerAction(Action)` | PlayerController Server RPC 위임 |
| `RequestActionTop/Bottom/Left/Right` | 편의 wrapper (BP OnSlotClicked 디스패처용) |
| `GetActionForSlot(Widget) → ETowerActions` | BP 가 슬롯 식별용 |

| BP 인터페이스 (ImplementableEvent) | 호출 시점 |
|---|---|
| `OnSlotInitialized(SlotWidget, Action)` | InitForTower 시 슬롯마다 1 회 (정적 데이터) |
| `OnSlotRefreshed(Slot, Action, Cost, Description, bVisible)` | 코인 변경 / 업그레이드 시마다 |

**연관**: `ATDPlayerController` (위젯 라이프사이클 소유자), `ATDTowerBase::GetTowerDetails`, `ATDGameState::OnCoinsChanged`.

### 11-3. `UTDPlayModeSelectWidget`

| 함수 | 동작 |
|---|---|
| `InitWithStage(StageId)` | 스테이지 ID 보관 + `OnStageSet(StageId)` 발화 |
| `RequestSoloMode()` | `UTDLevelSessionSubsystem::RequestLoadStage` |
| `RequestMultiMode()` | `UTDLobbySessionSubsystem::SetMultiStageId` + `OnMultiModeRequested` 발화 |
| BP: `OnStageSet / OnMultiModeRequested / OnBackRequested` | 시각/네비게이션 |

### 11-4. `UTDMultiLobbyWidget`

`SessionsScrollBox` 에 `SessionEntryWidgetClass` (= `WBP_SessionEntry`) 인스턴스 채움.

| 버튼 | 동작 |
|---|---|
| Create Room | `LobbySubsystem::CreateSession` → `OnCreateRoomSuccess` BP (호스트는 OpenLevel ?listen) |
| Refresh | `LobbySubsystem::FindSessions` → 결과로 ScrollBox 채움 |
| Back | `OnBackRequested` BP |

### 11-5. `UTDWaitingRoomWidget`

| 위젯 변수 | 용도 |
|---|---|
| `Text_MapName` | LobbySubsystem MultiStageId → 자동 표시 |
| `ScrollBox_players` | PlayerSlots 기반으로 PlayerEntry 채움 |
| `StartButton` | 호스트만 — `PC->Server_RequestStart` |
| `LeaveButton` | DestroySession + ClientTravel 메인 메뉴 |

`ATDLobbyGameState::OnPlayerSlotsChanged` 구독 → `RefreshPlayerList`.

### 11-6. 한 줄 위젯 2 종

- `UTDSessionEntryWidget` — `InitWithSessionInfo(FTDSessionInfo)` 한 번에 호스트명/인원/인덱스 설정. Join 클릭 시 `LobbySubsystem::JoinSession(CachedSessionIndex)`.
- `UTDPlayerEntryWidget` — `SetPlayerName(Name)` 한 함수만.

---

## 12. 레이어 간 의존 관계도 (요약)

```
   ┌──────────────────────┐
   │  UTDFL_Utility       │  ◄── BP/C++ 어디서든 정적 접근
   └─┬────────────────────┘
     ▼
┌────────────────────────────────────────────────────────────────┐
│ A. GameInstance (앱 수명)                                       │
│  UTDGameInstance ──► UTDMainSaveGame                            │
│  UTDLevelSessionSubsystem ──► UTDTowerDataTableSubsystem        │
│                          └──► UTDEnemyDataTableSubsystem        │
│  UTDLobbySessionSubsystem (OnlineSubsystem 단일 허브)            │
└─────────────┬───────────────────────────────────────────────────┘
              │ 스테이지 진입 (PostLoadMapWithWorld 훅)
              ▼
┌────────────────────────────────────────────────────────────────┐
│ B. GameMode (서버 전용, 권위)                                    │
│  ATDGameMode                                                    │
│   ├─ UTDWaveManagerComponent  (Tick → SpawnEnemy)               │
│   ├─ UTDEnemySpawnerComponent (Type+Path → SpawnActor)          │
│   ├─ UTDTowerSpawnerComponent (Tile 스캔 + DoTowerAction)        │
│   └─ UTDPoolComponent         (TDPoolActor 풀링)                │
└─────────────┬───────────────────────────────────────────────────┘
              │ 쓰기                  ▲ 읽기 (OnRep)
              ▼                       │
┌────────────────────────────────────────────────────────────────┐
│ C. GameState (모두에 복제)                                       │
│  ATDGameState                                                   │
│   ├─ SharedCoin / BaseHealth / CurrentWave (Replicated)         │
│   ├─ PlacedTowers / ActiveEnemies          (Replicated)         │
│   ├─ Multicast: OnEnemyDied / OnEnemyAttacked                   │
│   └─ Delegate (BP/C++): OnCoinsChanged / OnBaseHealthChanged …  │
└────────────────────────────────────────────────────────────────┘
              │
              ▼ (참조)
┌────────────────────────────────────────────────────────────────┐
│ D. Replicated Actor                                             │
│  ATDTowerBase ─ ATD_Weapon ─ Multicast → ATDProjectile (비복제) │
│  ATDEnemyActor (GAS) ─ ATDPath (Spline)                         │
└────────────────────────────────────────────────────────────────┘
              ▲
              │ Server RPC
┌────────────────────────────────────────────────────────────────┐
│ E. LocalPlayer (본인 클라)                                      │
│  ATDPlayerController                                            │
│   ├─ Server_DoTowerAction  (단일 RPC 진입점)                    │
│   ├─ ShowTowerActionMenu / HideTowerActionMenu                  │
│   └─ ActiveActionWidget (UTDTowerActionWidgetBase)              │
│  ATDPlayerCharacter (카메라/입력) ─ ATDPlayerPawn (클릭 이동)    │
│  UTDTowerHighlightSubsystem (호버)                              │
│  UTDTowerPlacementSubsystem (프리뷰)                            │
└────────────────────────────────────────────────────────────────┘

   ┌──────────────────────────┐
   │ F. Lobby (별도 GameMode) │
   │  ATDLobbyGameMode        │  PostLogin/Logout
   │  ATDLobbyGameState       │  PlayerSlots (Replicated)
   │  ATDLobbyPlayerController│  Server_RequestStart → ServerTravel
   │  ATDLobbyPlayerState     │  bIsReady (Replicated)
   └──────────────────────────┘
```

---

## 13. 자주 쓰는 데이터 흐름 (한 줄 요약)

| 흐름 | 한 줄 |
|---|---|
| **타워 설치** | 클릭 → `PC::HandleClick` → `ShowTowerActionMenu` → 슬롯 클릭 → `Server_DoTowerAction` → `TowerSpawner::DoTowerAction` → `GameState::CoinChange` + `SpawnTower` → `OnRep_PlacedTowers` |
| **타워 발사** | (서버) `Weapon::FindEnemy` → `FaceEnemy` → `FireAtEnemy` (Authority) → `MulticastFireProjectile` → 양 머신에서 `Projectile` 스폰/이동/Hit → 서버만 `EnemyDamage` (GAS GE) |
| **적 사망** | (서버) GAS HP<=0 → `EnemyActor::OnEnemyDied` → `GameState::CoinChange(+reward)` + `NotifyEnemyDied` → `Multicast_OnEnemyDied` → 모든 클라 BP `PlayDeathAnimation` |
| **적 경로 도달** | (서버) `Advance` 끝 도달 → `GameState::DecreaseBaseHealth` → `OnRep_BaseHealth` → HUD 갱신 |
| **스테이지 진입** | `LevelSession::RequestLoadStage(Id)` → `OpenLevel(Map)` → 엔진 `PostLoadMapWithWorld` → `OnPostLoadMap` → `ApplyStageData(Row)` → DataTable Subsystem 들에 주입 + `GameMode::InitializeStage` → `TowerSpawner::SpawnInitialTowerBases` |
| **멀티 세션 시작** | `MultiLobby` Create → `LobbySession::CreateSession` → `OnSessionCreated` → BP OpenLevel `Levels-Lobby?listen` → `LobbyGameMode::PostLogin` → `LobbyGameState::PlayerSlots` 갱신 → `OnRep_PlayerSlots` → WaitingRoom UI |
| **멀티 게임 시작** | 호스트 Start → `LobbyPC::Server_RequestStart` (Authority + AllReady 검증) → `LobbySession::GetMultiStageId` → `LevelSession::GetMapPathByStageId` → `ServerTravel(Map?listen)` → 모든 클라 자동 ClientTravel |

---

## 14. 신규 기능 추가 시 — 어느 클래스에 손을 댈까

새 기능을 어디에 두느냐는 **권한 + 수명 + 책임** 으로 결정. CLAUDE.md § 1-2 표 참고.

| 추가하려는 것 | 어디에 |
|---|---|
| 새 타워 타입 (Mortar 등) | DT_Towers 행 추가 + `ETowerType` enum + (필요 시) BP_NewTower 자식 |
| 새 적 타입 | DT_Enemies 행 + `EEnemyType` enum + BP_NewEnemy + GameMode `EnemyClasses` 매핑 |
| 새 GAS 어트리뷰트 | `UTDTowerSet` 또는 `UTDEnemySet` 에 `FGameplayAttributeData` 추가 + `ATTRIBUTE_ACCESSORS_BASIC` |
| 새 공유 상태 (예: 점수) | `ATDGameState` 에 `UPROPERTY(ReplicatedUsing=OnRep_X)` + `DOREPLIFETIME` + 델리게이트 |
| 새 클라→서버 요청 | `ATDPlayerController::Server_*` Server RPC 추가 (CLAUDE.md § 1-3) |
| 새 클라 전용 UI 상태 | `ULocalPlayerSubsystem` 신규 또는 기존 (`Highlight/Placement`) 에 추가 |
| 새 위젯 | `UUserWidget` C++ 베이스 + BP 자식 (`UTDTowerActionWidgetBase` 패턴) |
| 새 멀티 세션 기능 | `UTDLobbySessionSubsystem` 에만 추가. 위젯/PC 가 직접 `IOnlineSession` 호출 금지 |
| 새 스테이지 | DT_Stages 행 추가 (StageId / Map / TowerDT / EnemyDT / WaveDT / BaseTowerClass / TileMeshName) |

---

## 15. 빠른 색인 — "이 기능 어디 있나?"

| 찾고 싶은 것 | 위치 |
|---|---|
| 코인 증감 | `ATDGameState::CoinChange` / `HasCoins` |
| 기지 체력 감소 | `ATDGameState::DecreaseBaseHealth` |
| 타워 스폰/교체 | `UTDTowerSpawnerComponent::DoTowerAction / SpawnTower` |
| 타워 업그레이드 비용 | `ATDTowerBase::GetTowerDetails` |
| 적 스폰 | `UTDEnemySpawnerComponent::SpawnEnemy` (위임: `UTDWaveManagerComponent::SpawnEnemy`) |
| 적 경로 진행 | `ATDEnemyActor::Advance` + `ATDPath::GetLocation` |
| 타워 발사 (Multicast) | `ATD_Weapon::MulticastFireProjectile` |
| 풀에서 액터 꺼내기 | `UTDPoolComponent::GetPoolActorFromClass` |
| Server RPC 진입점 | `ATDPlayerController::Server_DoTowerAction` |
| 마우스 호버 타워 | `UTDTowerHighlightSubsystem::GetHoveredTower` |
| 타워 액션 메뉴 표시 | `ATDPlayerController::ShowTowerActionMenu` |
| 멀티 세션 생성 | `UTDLobbySessionSubsystem::CreateSession` |
| 호스트 게임 시작 | `ATDLobbyPlayerController::Server_RequestStart` |
| 스테이지 전환 | `UTDLevelSessionSubsystem::RequestLoadStage` |
| GAS 데미지 적용 | `UTDFL_Utility::EnemyDamage(Enemy, Damage, GE_Class)` |
| 사운드 볼륨 | `UTDGameUserSettings::GetMasterVolume / SetMasterVolume` |

---

## 16. 함께 보면 좋은 자료

| 문서 | 내용 |
|---|---|
| [`CLAUDE.md`](../CLAUDE.md) | 아키텍처 원칙 / 도메인 배치 표 / 마이그레이션 순서 / 멀티 세션 § 11 |
| [`Documents/JaeHun/C_Poring_Diagram.drawio`](JaeHun/C_Poring_Diagram.drawio) | 클래스 도해 v2 (9 레이어 38 박스) |
| [`Documents/JaeHun/C_FlowDiagrams.drawio`](JaeHun/C_FlowDiagrams.drawio) | 함수 단위 흐름도 8 페이지 (메인메뉴/멀티세션/로비/적생성/타워배치/타워발사/적사망/타워액션메뉴) |
| [`Documents/JaeHun/GAS Efffect Apply Pipeline.drawio.png`](JaeHun/GAS%20Efffect%20Apply%20Pipeline.drawio.png) | GAS 적용 파이프라인 |
| [`Documents/Resume/Resume_TowerDefence.md`](Resume/Resume_TowerDefence.md) | 본인 (`dianay07`) 기여 STAR 7 항목 |

---

## 17. 변경 / 폐기 예정

- `AEnemyActor` (`Enemy/EnemyActor.h`): 레거시. 모든 신규 코드는 `ATDEnemyActor` 사용.
- `ATowerManager`: 이미 삭제됨 — 4 개 컨테이너로 분할 (`TowerDataTableSubsystem` / `TowerSpawnerComponent` / `TowerHighlightSubsystem` / `TowerPlacementSubsystem`).
- `UTDEventManagerComponent`: 이미 삭제됨 — `ATDGameState` 의 Multicast RPC 로 통합.
- `UTDWaveManagerComponent::Enemies` 배열: `ATDGameState::ActiveEnemies` 로 이전 진행 중 (호환용 잔존).
