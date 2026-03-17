# [Session Network] Create Session Test — 진행 상황 & TODO

## 현재 검증 체크리스트

- [x] Solo 경로: 레벨선택 → PlayModeSelect → Solo → 게임 레벨 로드
- [x] Multi 방 만들기: Button_CreateRoom → OnCreateComplete(true) → (WaitingRoom 미구현)
- [ ] **[BUG]** Multi 방 찾기: Refresh → "검색 중..." 고착 (아래 해결 방법 참조)
- [ ] Multi 방 참가: SessionEntry Join → WaitingRoom 진입
- [ ] 게임 시작: Button_Start → ServerTravel
- [ ] 나가기: Button_Leave → MultiLobby 복귀

---

## 버그 분석: FindSessions 응답 없음

### 증상
`WBP_MultiLobby`의 Refresh 버튼을 누르면 `Text_Status`가 "검색 중..."으로 설정된 뒤
`OnFindComplete` 이벤트가 끝내 호출되지 않아 영구 고착.

### 원인

`Config/DefaultEngine.ini`에 `[OnlineSubsystem]` 섹션이 없음.
→ DefaultPlatformService가 명시되지 않아 Null 서브시스템이 암묵적으로 사용되지만,
LAN 비콘(UDP 브로드캐스트)이 동일 머신의 두 에디터 인스턴스 간에 루프백되지 않음.

- `CreateSession`: 내부 메모리에 세션 저장 → **성공처럼 보임**
- `FindSessions`: UDP 멀티캐스트 브로드캐스트 → 같은 머신에서는 **응답 없음**
  - Windows에서 멀티캐스트 루프백이 기본 비활성
  - Windows Defender Firewall이 에디터 간 UDP 패킷 차단 가능

---

## 해결 방법 (우선순위 순)

### 방법 1 — DefaultEngine.ini에 Null 서브시스템 명시 (권장) 🔴

`Config/DefaultEngine.ini`에 추가:
```ini
[OnlineSubsystem]
DefaultPlatformService=Null

[OnlineSubsystemNull]
bEnabled=true
```
이후 에디터 재시작 → PIE 두 인스턴스로 재테스트.

### 방법 2 — 단일 PIE 세션에서 멀티 플레이어 테스트

Editor 상단 Play 옆 화살표 → **Advanced Settings**
- **Number of Players**: 2
- **Net Mode**: Play as Listen Server

→ 같은 프로세스 내에서 Host/Client 분리 테스트 가능 (FindSessions 불필요)

### 방법 3 — WBP_MultiLobby에 타임아웃 처리 추가 (UI 개선)

`Button_Refresh OnClicked`에서 타이머 설정:
```
Set Timer by Event (5.0초, 반복 없음)
  → OnTimeout: Text_Status → Set Text "검색 시간 초과 — 다시 시도해주세요"
```
`OnFindComplete`가 호출되면 타이머 해제.

---

## 남은 작업

| # | 작업 | 파일 | 우선순위 |
|---|------|------|---------|
| A | FindSessions 버그 픽스 (방법 1 적용 후 재테스트) | Config/DefaultEngine.ini | 🔴 최우선 |
| B | WBP_WaitingRoom 신규 생성 및 이벤트 연결 | Content/MainMenu/ | 🟠 |
| C | WBP_PlayerEntry 신규 생성 | Content/MainMenu/ | 🟠 |
| D | Multi 방 참가(JoinSession) 검증 | WBP_MultiLobby / WBP_SessionEntry | 🟡 |
| E | 게임 시작(ServerTravel) 검증 | WBP_WaitingRoom | 🟡 |
| F | 나가기(DestroySession) → MultiLobby 복귀 검증 | WBP_WaitingRoom | 🟡 |
