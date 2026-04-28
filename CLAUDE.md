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
│   ├── TD_Weapon.h                        [MODIFY]  Replicated Actor + Multicast 발사
│   ├── TDProjectile.h                     [MODIFY]  비복제 코스메틱 (머신별 로컬 시뮬레이션)
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

### 5-3. 타워 발사 (Multicast 코스메틱 패턴)

```
[Server: Tower Tick → BP Anim/Timer]
ATD_Weapon::FireAtEnemy   (HasAuthority 가드)
  ├ Target/Tower/ProjectileClass 검증
  ├ FirePoint 위치/회전 계산
  └ MulticastFireProjectile(Target, Damage, Radius, Loc, Rot) ─ NetMulticast Reliable
                                                              │
                  ┌───────────────────────────────────────────┴─────────┐
                  ▼                                                     ▼
         [Server _Implementation]                          [Client _Implementation]
         GameMode->Pool->GetActor (재사용)                  SpawnActor<ATDProjectile> (코스메틱)
                  │                                                     │
                  └─────────── 양쪽 다 SetProjectileData ────────────────┘
                                       │
[양쪽 Tick 독립 시뮬레이션 — 복제 없음, bReplicates=false]
ATDProjectile::MoveTowardsTarget   (가드 없음)
  ├ Target 향해 이동
  └ 도착 → OnHitTarget
            ├ [Server] EnemyDamage(GAS GE 적용) → 적 HP 변화 → Replicated
            ├ [Client] 데미지 스킵 (HasAuthority false)
            └ DespawnSelf
                ├ [Server] Pool 반납
                └ [Client] Destroy
                    ↓
[All Clients] 적 HP/사망은 Replicated UPROPERTY 로 자동 동기화
```

**핵심 원칙**:
- Projectile 자체는 비복제 (네트워크 비용 0). 발사 이벤트 1회만 Multicast.
- 클라 Projectile 은 시각 전용 — 데미지/HP 권위는 서버 단일.
- 적이 RPC 도착 직전에 죽으면 클라에서 `IsValid(InTarget)` 실패 → 그 클라만 시각 누락(빈도 0.1% 미만, 게임 상태 영향 없음).

### 5-4. 적 사망 (멀티플레이)

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
| 2026-04-27 | § 11 멀티 세션 섹션 추가 — OnlineSubsystem(Null) 기반 로비/세션 흐름. `UTDLobbySessionSubsystem` + Lobby GM/GS/PC/PS + 메뉴/룸 위젯 도입 계획 | - |

---

## 11. 멀티 세션 (OnlineSubsystem)

### 11-1. 목표

OnlineSubsystem 위에 **세션 생성 / 검색 / 참가 / 퇴장**, **로비 룸(참가자 목록 표시)**, **호스트의 게임 시작(같은 레벨 트래블)** 흐름을 구축한다.

- **백엔드**: `OnlineSubsystemNull` (LAN/개발) 우선. 코드는 `IOnlineSession` 추상화에 의존 → Steam/EOS 확장 가능.
- **호스트 모델**: Listen Server 우선. `HasAuthority()` 분기로 Dedicated 호환.
- **이름 충돌 주의**: 기존 `UTDLevelSessionSubsystem` (레벨 스테이지 전환 담당) 과 구분하기 위해 본 섹션의 신규 클래스는 **`UTDLobbySessionSubsystem`** 으로 명명한다.

### 11-2. 레이어 매핑 (§ 1-2 와 정합)

| 책임 | 컨테이너 | 권한 |
|---|---|---|
| 세션 라이프사이클 (Create / Find / Join / Destroy) + `IOnlineSessionPtr` 보유 | `UTDLobbySessionSubsystem` (GameInstanceSubsystem) | 로컬 |
| 호스트 게임 시작 (`ServerTravel ?listen`) | `UTDLobbySessionSubsystem` (Authority 분기) → GameMode | 서버 |
| 로비 룸 상태 (참가자 목록, Ready 플래그) | `ATDLobbyGameState` (Replicated `PlayerSlots`) | 복제 |
| 로비 PlayerController (Ready 토글, Start 요청, Kick) | `ATDLobbyPlayerController` (Server RPC) | Client→Server RPC |
| 로비 PlayerState (DisplayName, bIsReady) | `ATDLobbyPlayerState` (Replicated) | 복제 |
| 메뉴/룸 UI (방 목록, 참가자 목록) | `UTDLobbyMenuWidget` / `UTDLobbyRoomWidget` (UMG) + BP 자식 | 로컬 |
| 게임플레이 진입 후 공유 상태 | 기존 `ATDGameState` (그대로 사용) | 복제 |

**규칙**:
- 세션 API 호출은 **반드시 `UTDLobbySessionSubsystem` 한 곳에서**. 위젯/PlayerController 가 `IOnlineSession` 직접 호출 금지 (§ 1-3 의 "단일 진입점" 원칙 확장).
- 로비 ↔ 게임플레이 레벨 전환은 `ServerTravel` 로. `UGameplayStatics::OpenLevel` 은 클라 단독 메뉴 이동에만 사용.

### 11-3. 흐름

```
[메인 메뉴]
HostGame()  → LobbySubsystem::CreateSession(Settings { NumPublicConnections=4, bIsLAN=true })
                 ├ OnCreateSessionComplete
                 └ Authority: ServerTravel("/Game/Levels/Lobby?listen", true)

[메인 메뉴]
SearchGames() → LobbySubsystem::FindSessions(MaxResults=20, IsLAN=true)
                 └ OnFindSessionsComplete → 위젯에 결과 브로드캐스트

[메인 메뉴]
JoinSelected(SearchResult)
              → LobbySubsystem::JoinSession(SearchResult)
                 └ OnJoinSessionComplete → ClientTravel(ConnectInfo, ETravelType::TRAVEL_Absolute)

[Lobby Level]
ATDLobbyGameMode::PostLogin (서버) → LobbyGameState->PlayerSlots.Add(NewPS)  [Replicated]
                                                ↓
[All Clients] OnRep_PlayerSlots → 로비 룸 위젯 갱신 (이름/Ready 표시)

[Host: Start 클릭]
ATDLobbyPlayerController::Server_RequestStart  (호스트 PC 만 허용)
   └ Authority: ServerTravel("/Game/Levels/Levels-01", true)
         ↓
[All Clients] 자동 클라이언트 트래블 → 게임플레이 시작
              (기존 UTDLevelSessionSubsystem PostLoadMapWithWorld 훅 그대로 동작)

[Player Quit / Disconnect]
- 클라이언트 종료: LobbySubsystem::DestroySession 후 메인 메뉴 복귀.
                     서버는 PostLogout → PlayerSlots 에서 제거.
- 호스트 종료: HostMigration 미지원. 모든 클라에게 ClientReturnToMainMenuWithTextReason 송출 후 세션 종료.
```

### 11-4. 신규 파일 구조

```
Source/TowerDefence/
├── Public/
│   ├── Session/
│   │   ├── TDLobbySessionSubsystem.h       [NEW] OnlineSession 단일 허브
│   │   └── TDLevelSessionSubsystem.h       (기존 — 스테이지 전환 담당, 별개 책임)
│   ├── Lobby/
│   │   ├── TDLobbyGameMode.h               [NEW] PostLogin/Logout → GameState 슬롯 갱신
│   │   ├── TDLobbyGameState.h              [NEW] PlayerSlots (Replicated)
│   │   ├── TDLobbyPlayerController.h       [NEW] Server_RequestStart, Server_SetReady
│   │   └── TDLobbyPlayerState.h            [NEW] DisplayName, bIsReady (Replicated)
│   └── UI/
│       ├── TDLobbyMenuWidget.h             [NEW] 메인 메뉴 (Host/Search/Quit)
│       ├── TDLobbyRoomWidget.h             [NEW] 룸 (참가자 리스트 + Ready/Start)
│       └── TDLobbyEntryWidget.h            [NEW] 검색 결과 / 참가자 한 줄
└── Private/ (대응)

Config/DefaultEngine.ini
  [OnlineSubsystem]
  DefaultPlatformService=Null
  [/Script/Engine.GameEngine]
  +NetDriverDefinitions=...

TowerDefence.Build.cs
  PrivateDependencyModuleNames += { "OnlineSubsystem", "OnlineSubsystemUtils" }

TowerDefence.uproject
  Plugins += { "Name": "OnlineSubsystemNull", "Enabled": true }

Content/Levels/Lobby.umap                   [NEW]   BP_TDLobbyGameMode 사용
Content/UI/Lobby/WBP_LobbyMenu.uasset       [NEW]   UTDLobbyMenuWidget 자식
Content/UI/Lobby/WBP_LobbyRoom.uasset       [NEW]   UTDLobbyRoomWidget 자식
Content/UI/Lobby/WBP_LobbyEntry.uasset      [NEW]   UTDLobbyEntryWidget 자식
```

### 11-5. 실행 단계 (작은 PR 단위)

각 단계는 독립 빌드 가능.

1. **모듈 / 플러그인 / Config 활성화**
   - `TowerDefence.Build.cs`: `PrivateDependencyModuleNames` 에 `"OnlineSubsystem"`, `"OnlineSubsystemUtils"` 추가.
   - `TowerDefence.uproject` Plugins 에 `OnlineSubsystemNull` 활성화.
   - `DefaultEngine.ini` 에 `[OnlineSubsystem] DefaultPlatformService=Null` + `NetDriverDefinitions` 등록.
2. **`UTDLobbySessionSubsystem` 도입**
   - Create / Find / Join / Destroy + 4 개 OnComplete 핸들러.
   - 위젯이 구독할 동적 멀티캐스트: `OnSessionCreated`, `OnSessionsFound`, `OnSessionJoined`, `OnSessionDestroyed`.
   - `UTDFL_Utility::GetLobbySession(WCO)` 접근자 추가 (§ 6-3 패턴).
3. **로비 레벨 + GM/GS/PC/PS**
   - `Levels-Lobby.umap` 신규.
   - `ATDLobbyGameMode::PostLogin/Logout` → `ATDLobbyGameState::PlayerSlots` (`TArray<TObjectPtr<ATDLobbyPlayerState>>` Replicated) 동기화.
   - `ATDLobbyPlayerState::DisplayName / bIsReady` 복제.
   - `ATDLobbyPlayerController::Server_SetReady`, `Server_RequestStart` (Authority 게이트).
4. **메뉴 / 룸 위젯**
   - `UTDLobbyMenuWidget` (C++) + `WBP_LobbyMenu` (BP, 시각만).
   - `UTDLobbyRoomWidget` (C++) — `OnRep_PlayerSlots` 구독, 슬롯마다 `WBP_LobbyEntry` 인스턴스화.
   - 패턴은 § 3-1 의 `UTDTowerActionWidgetBase` 와 동일: 데이터/로직 C++, 시각 BP.
5. **게임 시작 트래블**
   - `Server_RequestStart` 내부에서 `GetWorld()->ServerTravel("/Game/Levels/Levels-01?listen", true)`.
   - 기존 `UTDLevelSessionSubsystem::OnPostLoadMap` 훅이 그대로 동작 → 스테이지 DT 주입 흐름 보존.
6. **연결 종료/이탈 처리**
   - 호스트 종료: `LobbySubsystem::DestroySession` → 모든 클라 `ClientReturnToMainMenuWithTextReason("Host left")`.
   - 클라이언트 종료: 자체 세션만 정리. 호스트는 `PostLogout` 으로 슬롯 제거 + Replicated 자동 갱신.
7. **(선택) Steam 확장**
   - `OnlineSubsystemSteam` 플러그인 활성화 + `SteamAppId.txt` (480 = Spacewar) 배치.
   - Subsystem 코드 무변경 (`IOnlineSession` 추상화 덕분). Config 의 `DefaultPlatformService` 만 `Steam` 으로 전환.

### 11-6. 자주 틀리는 체크 포인트

- **Null 서브시스템에서 LAN 검색은 `bIsLanQuery=true` 필수** — 안 그러면 EmptyResult. `FOnlineSessionSearch::QuerySettings.Set(SEARCH_PRESENCE, true)` 도 같이 설정.
- **Listen Server 는 호스트 본인 PC + 자기 자신을 위한 Server PC 양쪽 PostLogin 트리거** — 슬롯 중복 추가 주의. `NewPlayer->PlayerState` 의 `bIsABot` / `bOnlySpectator` 필터링 필요.
- **`ServerTravel` 직후 `PlayerController*` 객체가 새로 생성됨** — 트래블 전 캐시한 PC 포인터 무효. 트래블 후 위젯/델리게이트 재바인딩 필요.
- **`DestroySession` 호출 안 하고 새 `CreateSession`** — 잔존 세션과 충돌해 실패. 항상 `IsValid` → `Destroy` 후 Create.
- **로비 위젯을 Multicast/Replication 만으로 채우려 하지 말 것** — 본인 위젯은 `OnRep_PlayerSlots` + `LobbySubsystem` 델리게이트 둘 다 구독해야 호스트/클라 모두 표시.
- **로비 GameState 와 게임플레이 GameState 분리** — `ATDLobbyGameState` ≠ `ATDGameState`. 각 GameModeBase 의 `GameStateClass` 지정으로 레벨별로 다른 GameState 사용.
- **`OnlineSubsystemNull` 은 동일 머신/LAN 만** — 외부 IP 로 접속 시 실패. WAN 테스트는 Steam 으로 갈 것.

### 11-7. 검증

- **PIE 멀티 (Listen Server)**:
  1. PIE 설정: `Number of Players` = 2, `Net Mode` = Play As Listen Server, `Run Under One Process` On.
  2. 호스트가 `Host Game` → `Levels-Lobby` 진입 + `PlayerSlots[0]` 본인 표시.
  3. 클라이언트가 `Search Games` → 1건 결과 → `Join` → 양쪽에서 `PlayerSlots[1]` 표시.
  4. 호스트 `Start` → 두 인스턴스 모두 `Levels-01` 진입.
  5. 코인 변동(타워 설치 등) → `OnRep_SharedCoin` 으로 양쪽 HUD 동기화.
- **클라이언트 이탈**: 클라 종료 → 호스트 슬롯 자동 제거 (`OnRep_PlayerSlots` 발동).
- **호스트 이탈**: 호스트 종료 → 클라 메뉴로 복귀 + 에러 메시지.
- **Stand-alone 회귀**: 기존 싱글플레이 PIE (Play Standalone) 가 깨지지 않는지 확인 (메뉴에서 "Single Play" 분기 유지).
