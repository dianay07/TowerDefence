# TowerDefence

![Engine](https://img.shields.io/badge/Unreal%20Engine-5-black?logo=unrealengine)
![Language](https://img.shields.io/badge/Language-C%2B%2B%20%2F%20Blueprint-blue)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)
![Multiplayer](https://img.shields.io/badge/Multiplayer-Listen%20Server%20(LAN)-green)

**다운로드 링크 : [Windows 빌드 (Google Drive)](https://drive.google.com/file/d/1POVyOAO6e0ciDEdxMnEzNwb74lKHMDrQ/view?usp=drive_link)**

> UE5 C++로 구현한 멀티플레이 타워 디펜스 게임.  
> GAS 어트리뷰트 시스템 · OnlineSubsystem 로비 · 서버 권위 Replication 구조를 5-Layer 아키텍처로 설계.

---

## 시연 영상 (Demo)

> ▶ **[플레이 시연 영상 — YouTube](https://youtu.be/0GwIIA0CC5E)**

<!-- 스크린샷 자리: 로비 화면 + 인게임 화면 각 1장
![로비 화면](추후_스크린샷_경로)
![인게임 화면](추후_스크린샷_경로)
-->

---

## 프로젝트 개요 (Overview)

| 항목 | 내용 |
|------|------|
| 장르 | 타워 디펜스 (PC, 멀티플레이) |
| 개발 인원 | 2인 (프로그래머 2명) |
| 개발 기간 | 약 3.5개월 (2026-01-13 ~ 2026-04-30) |
| 플랫폼 | PC (Windows) |
| 네트워크 | Listen Server 기반 LAN 멀티플레이 (OnlineSubsystemNull) |
| 주요 달성 | 2인 PIE 멀티플레이 테스트 통과, BP → C++ 전면 마이그레이션 완료 |

---

## 기술 스택 (Tech Stack)

### 엔진 & 언어
- **Unreal Engine 5**, C++, Blueprint

### 핵심 시스템

| 시스템 | 구현 내용 |
|--------|----------|
| **GAS** (Gameplay Ability System) | 타워·적 어트리뷰트 (공격력·체력·이동속도), GameplayEffect 데미지 파이프라인, GameplayTag 기반 상태 관리 |
| **OnlineSubsystem** (Null) | LAN 세션 Create / Find / Join / Destroy 단일 허브 (`UTDLobbySessionSubsystem`), 5종 로비 UI |
| **Replication** | GameState Replicated 프로퍼티 (코인·체력·웨이브), Multicast RPC 코스메틱 패턴, OnRep HUD 자동 갱신 |
| **UMG / Slate** | 로비 UI 5종 C++ 베이스 클래스 + BP 자식 (데이터/로직 C++, 시각 BP 분리) |
| **Enhanced Input** | 입력 레이어 분리, PlayerController 단일 처리 |

---

## 아키텍처 (Architecture)

5-Layer 구조로 **수명 × 권한 × 책임**을 분리해 멀티플레이 결합도를 최소화했습니다.

```
┌─────────────────────────────────────────────────────────┐
│ GameInstance Layer  세션·정적 데이터  (수명: 앱 전체)    │
│   UTDLobbySessionSubsystem  UTDTowerDataTableSubsystem  │
│   UTDLevelSessionSubsystem                              │
└─────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────┐
│ GameMode Layer  게임 규칙 실행  (서버 전용)              │
│   UTDTowerSpawnerComponent  UTDWaveManagerComponent      │
│   UTDPoolComponent                                      │
└─────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────┐
│ GameState Layer  공유 상태 복제  (모든 클라이언트)        │
│   SharedCoin · BaseHealth · CurrentWave  (Replicated)   │
│   Multicast RPC: OnEnemyDied · OnGameEnded              │
└─────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────┐
│ Replicated Actors  자가 복제                             │
│   ATDTowerBase · ATDEnemyActor · ATDProjectile          │
└─────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────┐
│ LocalPlayer Layer  UI·HUD·입력  (본인 클라이언트만)      │
│   ATDPlayerController  UTDTowerActionWidgetBase          │
│   UTDTowerHighlightSubsystem  UTDHUDSubsystem           │
└─────────────────────────────────────────────────────────┘
```

> 📊 클래스별 담당자·역할 전체 다이어그램 → [C_Poring_Diagram.drawio](./Documents/JaeHun/C_Poring_Diagram.drawio)

**핵심 원칙 3가지**
1. **수직 체인 금지** — 서버(GameMode)와 클라(Subsystem)는 GameState를 매개로 통신
2. **단일 RPC 진입점** — 클라 → 서버 모든 요청은 `ATDPlayerController` Server RPC 경유
3. **코스메틱 분리** — 데미지/HP 권위는 서버 단일, 발사 이펙트는 Multicast 비복제 패턴

자세한 설계 근거 → [`CLAUDE.md`](./CLAUDE.md)

---

## 주요 기능 (Key Features)

### 1. 멀티플레이 세션 로비
- 방 생성 / 검색 / 참가 / 퇴장 전 과정을 `UTDLobbySessionSubsystem` 단일 허브로 관리
- 로비 대기실에서 참가자 목록 실시간 동기화 (`ATDLobbyGameState::PlayerSlots` Replicated)
- 호스트 퇴장 시 모든 클라이언트에 복귀 명령 자동 전파

### 2. 서버 권위 Replication
- 코인·기지 체력·웨이브 번호를 GameState에서 Replicated 관리 → OnRep으로 모든 클라 HUD 자동 갱신
- 타워 발사: 서버에서 1회 Multicast RPC → 각 클라에서 로컬 투사체 시뮬레이션 (네트워크 비용 최소화)

### 3. GAS 기반 타워 · 적 시스템
- 타워·적 어트리뷰트를 `UAttributeSet` 상속 구조로 정의
- `GameplayEffect`로 데미지/힐 적용, `GameplayTag`로 디버프/상태 관리

### 4. 타워 액션 시스템
- 타워 클릭 → `ATDPlayerController`가 4슬롯 액션 메뉴 표시
- 슬롯 클릭 → `Server_DoTowerAction` RPC → 서버에서 설치/업그레이드/판매 처리
- `UTDTowerActionWidgetBase` C++ 클래스에 로직 집중, BP 자식은 시각 전담

### 5. 웨이브 시스템
- DataTable(`DT_WaveData`) 기반 웨이브 스폰 타이밍 제어
- 적 경로(`ATDPath`) 추적 및 기지 피해 처리, 승리/패배 판정 서버 단일 권위

---

## 참여자 및 역할 (Contributors)

### 이재훈 [@dianay07](https://github.com/dianay07)

| 영역 | 담당 클래스 / 시스템 |
|------|---------------------|
| **게임 Framework** | `ATDGameMode` (서버 라이프사이클 조립), `ATDGameState` (Replicated 공유 상태 + Multicast RPC) |
| **멀티 세션 로비** | `UTDLobbySessionSubsystem` (OnlineSubsystem 단일 허브), `ATDLobbyGameMode/GameState/PlayerController/PlayerState` (로비 레벨 전체 구조) |
| **적 시스템** | `ATDEnemyActor` (GAS ASC · 경로 이동), `UTDEnemySpawnerComponent` (Enemy 스폰 전담), `UTDWaveManagerComponent` (웨이브 스폰 · DataTable 로드), `UTDEnemyDataTableSubsystem` (EnemyData 캐시) |
| **플레이어 시스템** | `ATDPlayerController` (Server_DoTowerAction RPC 진입점 · 액션 메뉴 라이프사이클), `ATDPlayerCharacter` (카메라 · EnhancedInput · EdgeScroll), `ATDPlayerPawn` (Click-to-Move · SnapToGround) |
| **GAS 어트리뷰트** | `UTDEnemySet` (체력·이동속도·데미지) |
| **로비 UI (5종)** | `UTDPlayModeSelectWidget` · `UTDMultiLobbyWidget` · `UTDWaitingRoomWidget` · `UTDSessionEntryWidget` · `UTDPlayerEntryWidget` |
| **오브젝트 풀 기반** | `ATDPoolActor`, `UTDPoolComponent` |
| **경로 · 공용 유틸** | `ATDPathActor` (Spline · GetBakedWaypoints),`UTDFL_Utility` (전역 조회점, 부분) |

### 박지호

| 영역 | 담당 클래스 / 시스템 |
|------|---------------------|
| **게임 Framework** | `ATDGameMode` (서버 라이프사이클 조립) |
| **타워 시스템** | `ATDTowerBase` (배치·업그레이드·판매), `ATDTowerPawn` (GAS ASC · TowerSet) |
| **타워 발사 · 투사체** | `ATD_Weapon` (FindEnemy · Multicast 발사), `ATDProjectile` (비복제 코스메틱 발사체) |
| **타워 스폰 · 데이터** | `UTDTowerSpawnerComponent` (초기 타워 일괄 스폰), `UTDTowerDataTableSubsystem` (TowerData 캐시), `UTDTowerActionWidgetBase` (액션 메뉴 C++ 베이스) |
| **스테이지 전환** | `UTDLevelSessionSubsystem` (맵 전환 + DT 주입, PostLoadMapWithWorld 훅) |
| **Tower UI** | `TD_WBP_TowerActions` · `WBP_TowerActions` · `WBP_TowerActionSlot` (C++ 과 BP 연동 ) |
| **GAS 어트리뷰트** | `UTDTowerSet` (사거리·발사속도·데미지) |

### 공통 영역

| 영역 | 담당 클래스 / 시스템 |
|------|---------------------|
| **GAS 공용 어트리뷰트** | `UTDBaseSet` (Clamp · PreAttributeChange), `UTDGameUserSettings` (오디오·비디오 설정), `UTDGameInstance` (세이브 슬롯) |
| **경로 · 공용 유틸** | `ATDPathActor` (Spline · GetBakedWaypoints), `TowerDefence.h` (공용 열거형·구조체), `UTDFL_Utility` (전역 조회점, 부분) |

---

## 빌드 & 실행 (Getting Started)

### 요구사항
- Unreal Engine 5 (5.5 이상 권장)
- Visual Studio 2022 (C++ 게임 개발 워크로드 포함)

### 실행 방법
```
1. TowerDefence.uproject 우클릭 → "Generate Visual Studio project files"
2. TowerDefence.sln 열기 → Development Editor / Win64 빌드
3. 에디터에서 Content/Levels/MainMenuLevel 열기
```

### 멀티플레이 테스트 (PIE)
```
Play 버튼 옆 설정:
  Number of Players     : 2
  Net Mode              : Play As Listen Server
  Run Under One Process : OFF

Window 1 (호스트)      → Host Game 클릭
Window 2 (클라이언트)  → Search Games → Join 클릭
```
