# CLAUDE.md

이 파일은 이 저장소에서 작업할 때 Claude Code(claude.ai/code)에게 지침을 제공합니다.

## 프로젝트 개요

**Unreal Engine 5.7** 기반의 타워 디펜스 게임입니다. C++로 작성되었으며, 적과 타워의 속성 관리에 **GAS(Gameplay Ability System)** 를 사용합니다.

## 빌드 및 개발 환경

**요구사항:**
- Visual Studio 2022 (C++ 및 Unreal Engine 워크로드 포함, `.vsconfig` 참고)
- Unreal Engine 5.7

**빌드 방법:** `TowerDefence.sln`을 Visual Studio에서 열고 솔루션을 빌드합니다. 내부적으로 Unreal Build Tool(UBT)이 실행됩니다.

**실행 방법:** Epic Games Launcher에서 프로젝트를 Unreal Editor로 열고 Play 버튼을 누릅니다. 게임은 `MainMenu/SplashLevel`에서 시작합니다.

**패키징:** 에디터의 "Package Project" 메뉴를 사용합니다. 쿠킹 대상 맵은 `Config/DefaultGame.ini`에 정의되어 있습니다 (`MainMenuLevel`, `Levels-01`).

별도의 테스트 프레임워크는 없으며, 게임플레이 테스트는 에디터 내에서 진행합니다.

## 아키텍처

### 모듈 및 빌드
- `Source/TowerDefence/TowerDefence.Build.cs` — 모듈 의존성: 기본 UE 모듈 + `GameplayAbilities`, `GameplayTags`, `GameplayTasks`, `EnhancedInput`
- `Source/TowerDefence.Target.cs` / `TowerDefenceEditor.Target.cs` — 빌드 타겟 설정

### 핵심 시스템

**GAS (Gameplay Ability System)**
적과 타워 모두 `UAbilitySystemComponent`와 커스텀 어트리뷰트 셋을 사용합니다:
- `TDEnemySet` — Health, MaxHealth, MoveSpeed, Damage (`TDBaseSet` 상속)
- `TDTowerSet` — Range, FireRate, Damage (`TDBaseSet` 상속)
- `TDBaseSet` — 모든 어트리뷰트 셋이 공유하는 클램핑 로직

**오브젝트 풀링**
`ATDPooledGameMode`가 액터 풀을 관리하여 반복적인 스폰/소멸 비용을 줄입니다:
- `GetPoolActorFromClass()` — 풀에서 액터를 꺼내거나 새로 생성
- `PoolActor()` — 액터를 풀에 반환
- 풀링에 참여하는 액터는 `ITDPoolActorInterface`를 구현하고 `ATDPoolActor`를 상속합니다

**세이브 시스템**
`UTDGameInstance`가 3개의 세이브 슬롯(`SaveSlotOne/Two/Three`)을 `UTDMainSaveGame`으로 관리합니다. 마지막으로 선택한 슬롯 인덱스가 저장되어 다음 실행 시 해당 세이브를 불러옵니다.

**설정**
`UTDGameUserSettings`가 `UGameUserSettings`를 확장하여 오디오 볼륨(Master, SoundEffects, Music)을 추가합니다.

### 데이터

- `TD.h` — 핵심 열거형 및 구조체: `EEnemyType`, `ETowerType`, `FWaveData`, `FTowerData`
- `Content/Levels/WaveData-01.json` — 웨이브 스폰 데이터 (적 타입, 수량, 스폰 시간)

### 블루프린트 연동
- `Content/Blueprints/`의 `BP_GameMode`(`ATDPooledGameMode` 상속)와 `BP_GameInstance`(`UTDGameInstance` 상속)가 실제 사용되는 게임 모드 및 게임 인스턴스입니다
- 모든 C++ 클래스는 블루프린트에서 확장 가능하도록 설계되어 있습니다

## 코드 규칙

- 헤더 파일: `Source/TowerDefence/Public/`, 구현 파일: `Source/TowerDefence/Private/`
- 모든 커스텀 클래스는 `TD` 접두사를 사용합니다 (예: `TDEnemyActor`, `TDTowerPawn`)
- `EnemyActor.h/.cpp`는 레거시 파일이므로 신규 작업에는 `TDEnemyActor`를 사용합니다
- GAS 어트리뷰트 셋은 `ATTRIBUTE_ACCESSORS` 매크로를 사용하며, `PostAttributeChange`에서 값을 클램핑합니다
