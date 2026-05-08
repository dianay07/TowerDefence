# 이력서 기여 요약 — TowerDefence (개인용)

> 본 문서는 본인(`dianay07`) 의 커밋과 코드 기반 기여 내용을 STAR(Situation·Task·Action·Result) 4 줄 단위로 정리한 한국어 이력서 자료이다.
> 각 항목은 한국어 이력서 / 자기소개서에 그대로 발췌 가능한 길이로 다듬었다.
> 공동 작업분(예: Multicast OnEnemyDied / TowerSpawnerComponent 등)은 본인 담당 부분을 분리해 객관적으로 명시한다.

---

## 프로젝트 개요

| 항목 | 내용 |
|---|---|
| 프로젝트 | TowerDefence (Unreal Engine 5, C++ + GAS + UMG, 멀티플레이) |
| 기간 | 2026-01-13 ~ 2026-04-30 (약 3.5 개월) |
| 협업 | 2 인 (본인 `dianay07` 91 커밋, 협업자 `Jiho`/`박지호` ident 분리, 합산 125 커밋) |
| 본인 담당 | C++ 포팅 (BP→C++), Replication 인프라, Multicast 통합, OnlineSubsystem 로비/세션, 메인 메뉴/세이브 |
| 검증 | 2 인 PIE Listen Server Multi Play Test 통과 (2026-04-30) |
| 단일 진실 소스 | `CLAUDE.md` — 5 레이어 아키텍처, 도메인별 배치, 마이그레이션 순서, 데이터 흐름 도표를 PR 단위로 갱신 |

---

## 1. OnlineSubsystem 기반 멀티 세션 시스템 구축

- **상황(S)**: 강의 기반으로 시작한 싱글플레이 타워디펜스에 멀티플레이를 도입해야 했다. 기존 코드는 `ATDPlayerCharacter` 가 모든 입력·UI·상태 변경을 직접 처리하는 싱글 전제 구조였고, 세션 관리/방 목록/참가자 동기화 인프라가 전무했다.
- **과제(T)**: OnlineSubsystem(Null) 위에 세션 생성·검색·참가·종료, 로비 룸(참가자 슬롯 표시), 호스트의 게임 시작(ServerTravel) 까지의 흐름을 구축하고, 코드는 `IOnlineSession` 추상화 의존으로 두어 Steam/EOS 확장 가능하게 만든다.
- **행동(A)**:
    - `UTDLobbySessionSubsystem` (`GameInstanceSubsystem`) 을 단일 허브로 도입해 Create/Find/Join/Destroy + 4 동적 멀티캐스트 델리게이트를 노출. `FOnlineSessionSearchResult` 의 BP 노출 한계는 `FTDSessionInfo` USTRUCT 래핑으로 우회.
    - 호스트가 선택한 스테이지 ID 를 `MultiStageId` 로 보관하고 `Server_RequestStart` 가 `DT_Stages` 에서 맵 경로를 조회하도록 설계 → 로비/게임플레이 GameState 분리 원칙 유지.
    - 5 개 위젯 C++ 베이스 (`UTDPlayModeSelectWidget` / `UTDMultiLobbyWidget` / `UTDWaitingRoomWidget` / `UTDSessionEntryWidget` / `UTDPlayerEntryWidget`) 를 도입해 데이터·로직은 C++, 시각·스타일은 BP 자식으로 분리.
    - 솔로 분기는 `UTDLevelSessionSubsystem` 직접 호출(멀티 세션 미경유), 멀티 분기는 `UTDLobbySessionSubsystem` 으로 라우팅하는 단일 진입점(`UTDPlayModeSelectWidget`) 구현.
- **결과(R)**: 2 인 PIE Listen Server 환경에서 Host → Search → Join → Start → 게임플레이 진입 + `OnRep_SharedCoin` 동기화까지 일관 동작 확인. 이후 Steam 백엔드는 Config 의 `DefaultPlatformService` 만 변경하면 코드 무수정 확장 가능.
- **주요 커밋**: `2762542`, `3090906`, `26b38e0`, `dbceacc`, `aa85f91`, `435dc64`.

---

## 2. Replication 인프라 도입 — 권위/관찰자 분리

- **상황(S)**: GAS 어트리뷰트(타워 비용·기지 체력·코인) 와 적/타워 액터가 모두 비복제 상태였다. `ATDPlayerCharacter::OnCoinsChanged` 같은 로컬 델리게이트로만 UI 갱신을 하고, `UTDEventManagerComponent` 가 클라이언트에 존재하지 않아 멀티 환경에서는 동작 불가능한 구조였다.
- **과제(T)**: GameState 를 매개로 한 병렬 레이어 패턴(서버는 GameState 에 쓰고, 클라는 OnRep 으로 읽음)을 도입해 모든 공유 상태를 자동 동기화한다.
- **행동(A)**:
    - Player/Enemy/Tower 액터의 `bReplicates` 활성화 + `GetLifetimeReplicatedProps` 등록 (`b3ebb48`, `7f21e4d`).
    - 적 데이터/이벤트를 `UTDEventManagerComponent` 에서 `ATDGameState` 의 Multicast RPC 로 이전 (`ee725ed`). 협업자 `Jiho` 의 `2a171cf` (Multicast OnEnemyDied/OnEnemyAttacked) 와 짝 작업.
    - `UTDEnemyDataTableSubsystem` 도입 + `UTDWaveManagerComponent` 가 Subsystem 에서 데이터를 조회하도록 변경 (`fbc6fef`).
    - `OnRep_SharedCoin` / `OnRep_BaseHealth` / `OnRep_CurrentWave` 를 통해 기존 델리게이트(`OnCoinsChanged` 등)를 그대로 호출하게 설계 → UI 코드 수정 없이 동기화 획득.
- **결과(R)**: 2 인 PIE 에서 한쪽이 타워 설치/판매 시 양쪽 HUD 의 코인이 자동 갱신. 적 사망 / 공격 / 웨이브 종료 이벤트가 모든 클라이언트에 일관 전파.
- **주요 커밋**: `b3ebb48`, `7f21e4d`, `ee725ed`, `fbc6fef`.

---

## 3. BP→C++ 마이그레이션 — GameMode/GameState/Path/Wave/Event

- **상황(S)**: 게임 규칙이 `ATDGameMode` BP 노드와 `ATDPlayerCharacter` 에 산재해 있고, `ATowerManager` 가 데이터 조회·슬롯 탐색·스폰·하이라이트·UI 까지 모두 보유한 비대 클래스였다. 클라이언트가 `GetWaveManager(this)` 호출 시 null 을 받는 등 멀티 가능성을 막는 결합도가 컸다.
- **과제(T)**: 권위/수명/책임 기준의 컨테이너 매핑(`CLAUDE.md` § 1-2) 에 따라 BP 로직을 C++ 로 옮기되, 도메인을 5 개 레이어(GameInstance / GameMode / GameState / Replicated Actor / LocalPlayer) 로 쪼개 단일 책임 원칙을 만족시킨다.
- **행동(A)**:
    - `UTDFL_Utility.cpp` + `ATDGameState.h/cpp` 분리 (`e5ca96f`).
    - `TDPath.h/cpp` 신설로 BP_Path 를 코드화 (`0760736`).
    - `ATDGameMode` 와 `ATDGameStatus` 분리해 GameMode 는 라이프사이클 조립만, GameStatus(=GameState 계열) 가 공유 상태 보유 (`720b70f`, `9888ca5`).
    - `UTDWaveManagerComponent` + `UTDEventManagerComponent` 도입 (`bb82290`).
    - `EnemyActor` 이벤트 + 스테이지 종료 로직 포팅 (`e804373`).
    - 다수 버그 픽스: `Pendingkill on path end` (`aeff84f`), `Some Manager Nullptr` (`fbce4eb`), `WASD Camera Input` (`217b22b`), `Enemy Died Fatal Error` (`5ddf8e5`).
- **결과(R)**: BP 의존도 대폭 축소. 이후 Replication 도입(2 항)·Online 세션 도입(1 항) 의 토대가 됨. CLAUDE.md § 7 마이그레이션 1~7 단계의 백본 완료.
- **주요 커밋**: `e5ca96f`, `0760736`, `5c5e49c`, `283a2e3`, `9888ca5`, `bb82290`, `e804373`, `5ec2f5f`.

---

## 4. PlayerController 단일 RPC 진입점 + Object Pool 분리

- **상황(S)**: 타워 설치/판매 같은 클라→서버 요청이 `ATDTowerBase::DoTowerAction` 에서 직접 발생하고, `ATDGameMode` 가 ActorPool 을 직접 보유해 책임이 뒤섞여 있었다. 검증/치팅 방지 포인트가 분산돼 멀티 환경에서 위험.
- **과제(T)**: 모든 클라→서버 요청을 `ATDPlayerController` 의 Server RPC 한 곳으로 모으고, ActorPool 을 별도 컴포넌트로 분리해 GameMode 를 라이프사이클 조립만 담당하게 한다.
- **행동(A)**:
    - `Player RPC` (`2163333`) 도입 — Server RPC 진입점 통합.
    - `IA_Click` 입력을 `ATDPlayerCharacter` → `ATDPlayerController` 로 이관 (`6824769`).
    - `ATowerManager` 삭제 + PlayerController 가 타워 액션 메뉴 라이프사이클(`ShowTowerActionMenu`/`HideTowerActionMenu`/`HandleSlotClicked`) 소유 (`bb4ce6b`).
    - `UTDPoolComponent` 분리 + 데드 코드 제거 (`4faabad`) — GameMode 에서 풀 책임 분리.
- **결과(R)**: CLAUDE.md § 1-3 의 "RPC 경계는 단 하나 — PlayerController" 원칙 준수. 협업자 `Jiho` 의 `b83488a` (TowerActionMenu PC 이관) 와 함께 § 7-8 단계 완료.
- **주요 커밋**: `2163333`, `6824769`, `bb4ce6b`, `4faabad`.

---

## 5. 메인 메뉴 / 세이브 시스템 / 옵션

- **상황(S)**: 프로젝트 초기엔 PIE 직접 시작만 가능했고 메인 메뉴·세이브 슬롯·해상도/사운드 옵션이 없었다.
- **과제(T)**: 멀티 세션 도입 전 단계로, 메인 메뉴에서 스테이지 선택 → 세이브 슬롯 관리 → 옵션 변경까지 갖추고, 이후 멀티 분기 진입점을 끼워 넣을 수 있게 만든다.
- **행동(A)**:
    - `UTDGameInstance` + `UTDMainSaveGame` 도입 (`0dd7897`, `ba83ac7`, `45cf10a`).
    - 메인 메뉴 → 레벨 선택 → 세이브 슬롯 흐름 구현 (`6a9dd9d`).
    - 사운드/해상도/품질/스플래시 옵션 (`a94f1c9`, `9fd1144`, `14d2629`, `5e45c4c`) — `UTDGameUserSettings` 활용.
- **결과(R)**: 이후 1 항(멀티 세션) 의 진입점 `UTDPlayModeSelectWidget` 이 이 메인 메뉴 위에 자연스럽게 결합됨. 본인 PR 단위로 7 개 챕터(41~44, 48~51) 완결.
- **주요 커밋**: `0dd7897`, `ba83ac7`, `6a9dd9d`, `45cf10a`, `a94f1c9`, `9fd1144`, `14d2629`, `5e45c4c`.

---

## 6. 게임플레이 코어 — Tower / Enemy / Wave / Pool

- **상황(S)**: 강의 챕터를 따라가며 GAS 어트리뷰트 · 타워/적 베이스 · 웨이브 시스템을 처음부터 구축해야 했다.
- **과제(T)**: 강의 흐름을 따라가되, 이후 C++ 포팅·멀티화에 적합한 형태로 구조를 잡는다.
- **행동(A)**:
    - `[Chapter 2~3]` Tower/Enemy AttributeSet + GameplayEffect (`5e9c68c`).
    - `[Chapter 5]` Spline 기반 경로 (`e9304a4`).
    - `[Chapter 8-9]` WaveData + `UTDWaveManagerComponent` SpawnEnemy/ImportData (`5e13409`).
    - `[Chapter 17~19]` Weapon Setup / Fire GA + Cooldown / Damage (`a06de81`, `7e07aa3`, `30e4ca9`).
    - `[Chapter 24~28]` Ballista / Catapult / Sticky Mud / Cannon Tower + Effect (`c3723b9`, `8673a07`, `e9032ee`, `7bad474`).
    - `[Chapter 33~36]` Action Tooltips / Event Manager / Resource Management / Tower Upgrade-Breakdown (`d29eebf`, `53b6f5e`, `830eea5`, `2214830`).
    - `[Chapter 56~57]` Object Pooling + Interface (`4c5ba59`, `abe193f`).
- **결과(R)**: 강의 챕터 기반 코어가 완성된 시점에서 협업자와 함께 BP→C++ 포팅 → Replication → 멀티 세션 순으로 자체 확장. 강의 따라가기에서 끝나지 않고 마이그레이션 가능한 구조로 전환.

---

## 7. 협업 워크플로 — CLAUDE.md 단일 진실 소스

- **상황(S)**: 2 인 협업이고 도메인이 겹치는 구간이 많아(Tower/Enemy/Replication/UI), PR 마다 "어디에 둘지" 의사결정이 갈라지면 결합이 다시 비대해질 위험이 있었다.
- **과제(T)**: 새 기능을 짜기 전에 어느 레이어에 둘지부터 합의할 수 있는 문서 한 개를 두고, PR 에서 그 문서를 근거로 리뷰한다.
- **행동(A)**:
    - 협업자(`박지호` ident, `86ceb00`) 가 신설한 `CLAUDE.md` 위에서, 본인은 마이그레이션 진행 단위로 § 10 변경 로그 / § 11 멀티 세션 섹션 / § 11-4 위젯 표 / § 11-5 단계 마킹을 직접 갱신.
    - 마이그레이션 순서(§ 7) 를 작은 PR 단위로 쪼개 Replicated 1 단계 → Multicast 2 단계 → Subsystem 3 단계 식으로 점진 적용 — 빌드 끊김 없음.
    - 충돌 위험이 있는 도메인(타워 데이터, PlacedTowers, Multicast OnEnemyDied) 은 협업자와 짝 작업으로 정렬.
- **결과(R)**: 91 + 125 = 216 커밋이 단일 master 브랜치에서 큰 충돌 없이 진행. CLAUDE.md 가 PR 리뷰의 단일 진실 소스로 안착해, 같은 결정을 두 번 하지 않는 협업 패턴을 만듦.

---

## 부록 — 사용 기술 / 패턴

- **엔진/언어**: Unreal Engine 5, C++ (UFUNCTION/UPROPERTY 매크로 시스템, GAS), UMG, Enhanced Input.
- **네트워킹**: Replication (`Replicated`, `OnRep_*`, `DOREPLIFETIME`), Server/Client/Multicast RPC, `ServerTravel`/`ClientTravel`, OnlineSubsystem(Null) — `IOnlineSessionPtr`, `FOnlineSessionSearch`, `FOnlineSessionSettings`.
- **GAS**: AttributeSet, GameplayEffect (SetByCaller), GameplayAbility (Fire Weapon + Cooldown), AbilitySystemComponent.
- **아키텍처**: 5 레이어 컨테이너 매핑 (GameInstance / GameMode / GameState / Replicated Actor / LocalPlayer), Subsystem 패턴(GameInstanceSubsystem / LocalPlayerSubsystem), Component 분리(`UTDPoolComponent`, `UTDTowerSpawnerComponent`), DataTable Subsystem(per-level injection via `PostLoadMapWithWorld`).
- **데이터/에셋**: DataTable (Tower/Enemy/Wave/Stage), DataAsset, Soft/Weak Object Pointer.
- **협업**: Git (master 단일 브랜치 + PR 단위 작은 단계), Markdown 문서 (CLAUDE.md), drawio 다이어그램 (협업자 주도).
