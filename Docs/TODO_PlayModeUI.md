# TODO: 플레이 모드 선택 UI 구현

**브랜치:** NetworkTest_HostServer
**작업 기준 플로우:**
```
레벨 선택(WBP_LevelSelect) → WBP_PlayModeSelect → [Solo] 또는 [Multi]
                                                        └─ WBP_MultiLobby → WBP_WaitingRoom
```

---

## 완료된 작업 ✅

- [x] `TDGameInstance.h/cpp` — 세션 API 전체 구현 (CreateSession, FindSessions, JoinFoundSession, DestroySession, ServerTravelToGameMap)
- [x] `TDGameInstance` — `SetGameMapPath(FString)` 함수 추가
- [x] `WBP_PlayModeSelect` — 위젯 생성 (Solo / Multi 버튼)
- [x] `WBP_MultiLobby` — 위젯 생성 (방 만들기 / 세션 목록)
- [x] `WBP_SessionEntry` — 세션 목록 아이템 위젯 생성

---

## 남은 작업

### [ ] 1. WBP_LevelSelect 수정
레벨 버튼 클릭 시 동작 변경:
```
기존: OpenLevel("Levels-01") 직접 호출
변경:
  1. GetGameInstance → Cast to BP_GameInstance
  2. SetGameMapPath("/Game/Levels/01/Levels-01")
  3. Create Widget → WBP_PlayModeSelect → Add to Viewport
  4. Self → Remove from Parent
```

### [ ] 2. WBP_PlayModeSelect 이벤트 연결
| 버튼 | 동작 |
|------|------|
| Button_Solo | GetGameInstance→Cast→Get GameMapPath → OpenLevel(GameMapPath) |
| Button_Multi | Self Remove from Parent → Create WBP_MultiLobby → Add to Viewport |
| Button_Back | Self Remove from Parent → Create WBP_LevelSelect → Add to Viewport |

### [ ] 3. WBP_MultiLobby 이벤트 연결 ← **지금 여기서부터 시작**

#### 3-1. Event Construct (위젯이 화면에 처음 나타날 때 자동 호출)
```
[Event Construct]
  │
  ├─ GetGameInstance → Cast to BP_GameInstance → 변수 저장 (GI)
  │
  ├─ GI.OnCreateSessionCompleteDelegate에 이벤트 바인딩
  │     → 바인딩할 함수: OnCreateComplete (아래 3-3 참고)
  │
  ├─ GI.OnFindSessionsCompleteDelegate에 이벤트 바인딩
  │     → 바인딩할 함수: OnFindComplete (아래 3-4 참고)
  │
  ├─ GI.OnJoinSessionCompleteDelegate에 이벤트 바인딩
  │     → 바인딩할 함수: OnJoinComplete (아래 3-5 참고)
  │
  └─ GI.FindSessions(bIsLAN=true) 호출  ← 진입 즉시 방 목록 새로고침
```

#### 3-2. 버튼 클릭 이벤트
```
[Button_CreateRoom OnClicked]
  └─ GI.CreateSession(NumPublicConnections=2, bIsLAN=true)

[Button_Refresh OnClicked]
  └─ GI.FindSessions(bIsLAN=true)

[Button_Back OnClicked]
  └─ Self Remove from Parent
  └─ Create WBP_PlayModeSelect → Add to Viewport
```

#### 3-3. OnCreateComplete(bWasSuccessful: bool)
방 만들기 결과를 받는 커스텀 이벤트:
```
[OnCreateComplete]
  ├─ Branch (bWasSuccessful == true?)
  │     YES:
  │       └─ Create Widget WBP_WaitingRoom → Add to Viewport
  │       └─ Self Remove from Parent
  │     NO:
  │       └─ Text_Status.SetText("방 만들기 실패. 다시 시도해주세요.")
```

#### 3-4. OnFindComplete(bWasSuccessful: bool) ← **핵심, 자세한 설명**
세션 검색이 끝났을 때 호출되는 커스텀 이벤트. `FoundSessions` 배열을 읽어 목록을 그립니다.

```
[OnFindComplete]
  ├─ Branch (bWasSuccessful == true?)
  │
  │   YES (검색 성공):
  │     ├─ ScrollBox_Sessions.ClearChildren()
  │     │     ↳ 이전에 표시된 세션 목록을 모두 지웁니다.
  │     │
  │     ├─ GI.FoundSessions (TArray<FTDSessionInfo>) 가져오기
  │     │
  │     └─ ForEachLoop (FoundSessions 배열을 Index 0부터 끝까지 순회)
  │           │
  │           │   [루프 바디 - 세션 1개마다 실행]
  │           │
  │           ├─ Create Widget → WBP_SessionEntry
  │           │     (아직 뷰포트에 추가하지 않음, 변수에만 저장)
  │           │
  │           ├─ WBP_SessionEntry에 데이터 전달:
  │           │     ├─ SetHostName(Array Element.HostName)
  │           │     │     ↳ "TowerDefence_Host" 같은 호스트 이름 표시
  │           │     ├─ SetPlayers(Array Element.CurrentPlayers, Array Element.MaxPlayers)
  │           │     │     ↳ "1/2" 형태로 표시
  │           │     └─ SetSessionIndex(Array Index)
  │           │           ↳ 나중에 "참가" 버튼 클릭 시 JoinFoundSession(Index)에 넘길 번호
  │           │
  │           └─ ScrollBox_Sessions.AddChild(WBP_SessionEntry)
  │                 ↳ 스크롤박스 안에 추가하여 화면에 표시
  │
  │   NO (검색 실패):
  │     └─ Text_Status.SetText("세션 검색 실패.")
  │
  └─ (완료)
```

> **왜 Index를 저장하는가?**
> `FoundSessions` 배열과 내부 `SessionSearch->SearchResults` 배열이 같은 순서로 채워집니다.
> `JoinFoundSession(0)`을 호출하면 0번째 검색 결과에 참가하는 방식이므로,
> 각 WBP_SessionEntry가 자신의 Index를 알고 있어야 "참가" 버튼이 올바르게 동작합니다.

#### 3-5. OnJoinComplete(bWasSuccessful: bool)
```
[OnJoinComplete]
  ├─ Branch (bWasSuccessful == true?)
  │     YES:
  │       └─ Create Widget WBP_WaitingRoom → Add to Viewport
  │       └─ Self Remove from Parent
  │     NO:
  │       └─ Text_Status.SetText("참가 실패. 방이 가득 찼거나 사라졌습니다.")
```

---

### [ ] 4. WBP_SessionEntry 이벤트 연결
위젯에 변수 3개 추가: `HostName(FString)`, `CurrentPlayers(int32)`, `MaxPlayers(int32)`, `SessionIndex(int32)`

| 함수/이벤트 | 동작 |
|------------|------|
| SetHostName(Name) | TextBlock_HostName.SetText(Name) |
| SetPlayers(Current, Max) | TextBlock_Players.SetText(Current + "/" + Max) |
| SetSessionIndex(Index) | SessionIndex 변수에 저장 |
| Button_Join OnClicked | GetGameInstance→Cast→JoinFoundSession(SessionIndex) |

---

### [ ] 5. WBP_WaitingRoom 생성 및 이벤트 연결
**레이아웃:**
```
CanvasPanel
└── VerticalBox
    ├── TextBlock_MapName   ← GameInstance.GameMapPath 표시
    ├── ScrollBox_Players   ← 접속 중인 플레이어 목록
    ├── Button_Start        ← "게임 시작" (호스트만 Visible)
    └── Button_Leave        ← "나가기"
```

**이벤트 그래프:**

```
[Event Construct]
  ├─ GetGameInstance→Cast → GI 변수 저장
  ├─ TextBlock_MapName.SetText(GI.GameMapPath)
  ├─ GI.OnDestroySessionCompleteDelegate 바인딩 → OnDestroyComplete
  ├─ 호스트 판별:
  │     GetNetMode == NM_ListenServer?
  │       YES: Button_Start.SetVisibility(Visible)
  │       NO:  Button_Start.SetVisibility(Collapsed)
  └─ Set Timer by Function Name
        FunctionName = "RefreshPlayerList"
        Time = 1.0
        Looping = true

[RefreshPlayerList] (1초마다 호출)
  ├─ ScrollBox_Players.ClearChildren()
  └─ GetGameState → PlayerArray ForEach:
        Create WBP_PlayerEntry → SetPlayerName(PlayerState.GetPlayerName())
        ScrollBox_Players.AddChild(WBP_PlayerEntry)

[Button_Start OnClicked]
  └─ GI.ServerTravelToGameMap()

[Button_Leave OnClicked]
  └─ GI.DestroySession()

[OnDestroyComplete(bWasSuccessful)]
  ├─ Timer 정지: Clear and Invalidate Timer by Handle
  ├─ Self Remove from Parent
  └─ Create WBP_MultiLobby → Add to Viewport
```

---

### [ ] 6. WBP_PlayerEntry 생성
단순한 위젯:
```
HorizontalBox
└── TextBlock_PlayerName

함수: SetPlayerName(Name: FString) → TextBlock_PlayerName.SetText(Name)
```

---

## 검증 체크리스트

- [ ] Solo 경로: 레벨 선택 → PlayModeSelect → Solo → 게임 레벨 로드
- [ ] Multi 방 만들기: 에디터 A → Multi → 방 만들기 → WaitingRoom 진입
- [ ] Multi 방 찾기: 에디터 B → Multi → 새로고침 → A 세션 목록 표시
- [ ] Multi 방 참가: B가 A 세션 참가 → WaitingRoom에 두 플레이어 표시
- [ ] 게임 시작: A(호스트) 시작 버튼 → 두 인스턴스 게임 레벨로 이동
- [ ] 나가기: WaitingRoom → MultiLobby 복귀
