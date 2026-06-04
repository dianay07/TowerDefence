# Packet / Bunch / Channel / RPC 디버깅 정리

## 1. 코드 보는 흐름 + 현재 프로젝트에서 확인하기 좋은 곳

### 추천 시작점

현재 프로젝트에서는 `ATDPlayerPawn::Server_SetMoveTarget` RPC를 기준으로 보는 것이 가장 단순하다.

이유:

- Client에서 땅 클릭 한 번으로 Server RPC가 호출된다.
- 파라미터가 `FVector` 하나라서 흐름이 단순하다.
- `PlayerPawn`은 클라이언트 소유 Actor 흐름을 보기 좋다.
- Packet -> Bunch -> Channel -> RPC 실행 흐름을 확인하기 적합하다.

### 프로젝트 코드 흐름

```text
Client 전용 창에서 땅 클릭
↓
ATDPlayerController::HandleClick()
↓
ATDPlayerPawn::SetMoveTarget()
↓
Server_SetMoveTarget(WorldLocation)
↓
서버에서 ATDPlayerPawn::Server_SetMoveTarget_Implementation()
```

위 흐름에서 `Server_SetMoveTarget(WorldLocation)` 한 줄이 실제 네트워크 RPC의 시작점이다.
클라이언트에서 이 함수를 호출하면 바로 `_Implementation`으로 들어가는 것이 아니라, Unreal의 RPC 처리 코드를 거쳐 서버로 전송된다.

조금 더 자세히 보면 다음과 같다.

```text
ATDPlayerPawn::SetMoveTarget()
↓
Server_SetMoveTarget(WorldLocation) 호출
↓
UHT가 생성한 RPC 래퍼 / ProcessEvent 경유
↓
AActor::CallRemoteFunction
↓
UNetDriver::ProcessRemoteFunction
↓
ActorChannel을 통해 RPC 정보를 Bunch에 기록
↓
UChannel::SendBunch
↓
NetConnection을 통해 Packet으로 전송
↓
서버에서 Bunch를 해석
↓
ATDPlayerPawn::Server_SetMoveTarget_Implementation()
```

즉 프로젝트 코드에서 보이는 RPC 함수는 다음 두 부분으로 나뉜다.

```text
Server_SetMoveTarget(...)
→ 클라이언트에서 호출하는 RPC 요청 함수

Server_SetMoveTarget_Implementation(...)
→ 서버에서 실제로 실행되는 함수 본문
```

`UFUNCTION(Server, Reliable)` 선언 때문에 UnrealHeaderTool이 중간 RPC 처리 코드를 생성한다.
그래서 디버깅할 때 `Server_SetMoveTarget(WorldLocation)`에서 Step Into를 하면 생성 코드, `ProcessEvent`, `CallRemoteFunction` 계열 코드로 들어갈 수 있다.

확인하기 좋은 프로젝트 위치:

```text
Source/TowerDefence/Private/Player/TDPlayerController.cpp
- ATDPlayerController::HandleClick()

Source/TowerDefence/Private/TDPlayerPawn.cpp
- ATDPlayerPawn::SetMoveTarget()
- ATDPlayerPawn::Server_SetMoveTarget_Implementation()

Source/TowerDefence/Public/TDPlayerPawn.h
- UFUNCTION(Server, Reliable)
- ATDPlayerPawn::Server_SetMoveTarget(...)
```

### 엔진 코드 추적 흐름

Client 송신 쪽:

```text
ATDPlayerPawn::Server_SetMoveTarget 호출
↓
UHT 생성 RPC 래퍼 / ProcessEvent
↓
AActor::CallRemoteFunction
↓
UNetDriver::ProcessRemoteFunction
↓
ActorChannel에 RPC Bunch 작성
↓
UChannel::SendBunch
↓
UChannel::SendRawBunch
↓
UNetConnection::SendRawBunch
↓
UNetConnection::FlushNet
↓
UIpConnection::LowLevelSend / SendToRemote
```

Server 수신 쪽:

```text
UIpNetDriver::TickDispatch
↓
UIpConnection::ReceivedRawPacket
↓
UNetConnection::ReceivedRawPacket
↓
UNetConnection::ReceivedPacket
↓
UChannel::ReceivedRawBunch / ReceivedNextBunch
↓
UActorChannel::ProcessBunch
↓
FObjectReplicator::ReceivedBunch
↓
ATDPlayerPawn::Server_SetMoveTarget_Implementation
```


### 서버 수신 RPC 디버깅 포인트

`TickDispatch`는 모든 수신 패킷에서 걸리기 때문에, 특정 RPC만 보고 싶을 때는 아래 단계 중 목적에 맞는 지점에 브레이크포인트를 잡는 것이 좋다.

```text
[참고] UIpNetDriver::TickDispatch
    → 소켓에서 패킷을 받는 입구. 모든 패킷에서 자주 걸리므로 특정 RPC 디버깅용으로는 비추천
↓
[참고] UIpConnection::ReceivedRawPacket
    → IP 연결 수신 성공 처리 후 UNetConnection으로 넘김
↓
[참고] UNetConnection::ReceivedRawPacket
    → PacketHandler 처리, LastByte 기반 BitSize 계산, FBitReader 생성
↓
[참고] UNetConnection::ReceivedPacket
    → PacketId / ACK / NACK / 순서 처리
↓
[중요] UNetConnection::DispatchPacket
    → Packet 안의 Bunch를 분리하고 Bunch.ChIndex로 Channel을 찾거나 생성
↓
[참고] UChannel::ReceivedRawBunch
    → Reliable Bunch 순서 검사, 순서가 빠진 Bunch는 InRec에 대기 저장
↓
[참고] UChannel::ReceivedNextBunch
    → Reliable 처리 번호 갱신, Partial Bunch 조립, Channel open 상태 확인
↓
[참고] UChannel::ReceivedSequencedBunch
    → 완성된 Bunch를 Channel별 ReceivedBunch로 넘김
↓
[중요] UActorChannel::ReceivedBunch
    → Actor Channel이 받은 Bunch 처리 시작
↓
[중요] UActorChannel::ProcessBunch
    → ActorChannel에서 Replicator로 넘어가는 구조를 보고 싶을 때
↓
[중요] UActorChannel::ReadContentBlockPayload
    → Actor/SubObject 대상과 payload를 읽는 부분을 보고 싶을 때
↓
[중요] FObjectReplicator::ReceivedBunch
    → Replicator가 Bunch 내용을 해석하기 시작
↓
[핵심] FObjectReplicator::ReceivedRPC
    → 이동 RPC만 필터링해서 보고 싶을 때
    → FunctionName == Server_SetMoveTarget 조건부 브레이크
↓
[핵심] FRepLayout::ReceivePropertiesForRPC
    → WorldLocation 값이 어떻게 복원되는지 보고 싶을 때
↓
[핵심] FObjectReplicator::CallProcessEventForReceivedRPC
    → 복원된 RPC를 UObject::ProcessEvent로 넘기는 지점
↓
[중요] UObject::ProcessEvent
↓
[중요] UHT 생성 execServer_SetMoveTarget
↓
[핵심] ATDPlayerPawn::Server_SetMoveTarget_Implementation
    → 최종적으로 RPC가 서버에 도착했는지만 확인할 때
```

표시 기준:

```text
[핵심]
→ Server_SetMoveTarget RPC를 직접 확인할 때 우선 볼 지점

[중요]
→ RPC가 ActorChannel / Replicator / ProcessEvent로 넘어가는 구조를 이해할 때 볼 지점

[참고]
→ Packet, ACK/NACK, Reliable 순서, Partial Bunch까지 공부할 때 볼 지점
→ 특정 RPC만 볼 때는 자주 걸리므로 후순위
```

목적별로 정리하면 다음과 같다.

```text
1순위: 최종적으로 RPC가 서버에 도착했는지만 확인
→ ATDPlayerPawn::Server_SetMoveTarget_Implementation

2순위: 이동 RPC만 필터링해서 보고 싶음
→ FObjectReplicator::ReceivedRPC
→ FunctionName == Server_SetMoveTarget 조건부 브레이크

3순위: WorldLocation 값이 어떻게 복원되는지 보고 싶음
→ FRepLayout::ReceivePropertiesForRPC

4순위: ActorChannel에서 Replicator로 넘어가는 구조를 보고 싶음
→ UActorChannel::ProcessBunch / ReadContentBlockPayload

5순위: Packet에서 Bunch를 꺼내 Channel로 분배하는 과정을 보고 싶음
→ UNetConnection::DispatchPacket
→ Bunch.ChIndex, Channels[Bunch.ChIndex], BunchDataBits 확인

6순위: Reliable 순서 / Partial Bunch 조립까지 보고 싶음
→ UChannel::ReceivedRawBunch / ReceivedNextBunch
→ Connection->InReliable[ChIndex], InRec, InPartialBunch 확인
```
### 디버거에서 보면 좋은 값

```text
Connection
Connection->RemoteAddr
Bunch.ChIndex
Bunch.bReliable
Bunch.bPartial
Bunch.ChSequence
Channels[Bunch.ChIndex]
Actor->GetName()
Function->GetName()
```

핵심은 `Bunch.ChIndex`이다.
이 값으로 어떤 Channel이 해당 Bunch를 처리할지 결정된다.
Actor RPC라면 보통 해당 Actor의 `UActorChannel`로 전달된다.

### 그 다음에 보면 좋은 RPC

`Server_SetMoveTarget` 흐름이 이해되면 다음으로 볼 수 있는 RPC:

```text
ATDPlayerController::Server_DoTowerAction
```

이 RPC는 타워 선택, UI 슬롯 클릭, Tower 유효성 검사 등이 포함되어 있어서 처음보다는 두 번째 단계로 보는 것이 좋다.


## 2. 테스트 방법

### PIE 설정

Unreal Editor의 Play 설정에서 다음처럼 설정한다.

```text
Number of Players: 2
Net Mode: Play As Listen Server
Run Under One Process: Off
```

중요한 점:

```text
Run Under One Process = Off
```

이 옵션을 꺼야 Listen Server와 Client가 별도 프로세스로 실행된다.
그래야 Client 송신 흐름과 Server 수신 흐름을 구분해서 보기 쉽다.

### 실행 구성

```text
프로세스 A: Listen Server
프로세스 B: Client 전용
```

확인 기준:

```text
Client 전용 창에서 땅 클릭
→ Client 쪽에서 RPC 송신 흐름 확인

Listen Server 창
→ Server 쪽에서 Packet 수신, Bunch 처리, RPC 실행 확인
```

Listen Server 창에서 직접 클릭하면 서버 권한에서 바로 처리될 수 있으므로,
Packet / Bunch 흐름을 보려면 반드시 Client 전용 창에서 클릭하는 것이 좋다.

### 추천 브레이크포인트

처음에는 아래 정도만 잡는 것을 추천한다.

Client 전용 프로세스:

```text
ATDPlayerController::HandleClick
ATDPlayerPawn::SetMoveTarget
AActor::CallRemoteFunction
UNetDriver::ProcessRemoteFunction
UChannel::SendBunch
UNetConnection::SendRawBunch
UIpConnection::SendToRemote
```

Listen Server 프로세스:

```text
UIpNetDriver::TickDispatch
UIpConnection::ReceivedRawPacket
UNetConnection::ReceivedPacket
UActorChannel::ProcessBunch
FObjectReplicator::ReceivedBunch
ATDPlayerPawn::Server_SetMoveTarget_Implementation
```

### 확인 목표

이번 디버깅의 목표는 아래 흐름을 직접 확인하는 것이다.

```text
Client RPC 호출
↓
RPC 정보가 Bunch로 직렬화됨
↓
Bunch가 Packet에 실림
↓
Server가 Packet 수신
↓
Packet에서 Bunch를 꺼냄
↓
Channel ID로 ActorChannel에 전달
↓
RPC 파라미터를 해석
↓
Server RPC Implementation 실행
```

처음부터 Partial Bunch까지 보려고 하면 복잡해질 수 있다.
먼저 일반 Server RPC 흐름을 확인한 뒤, 큰 데이터 전송이 필요할 때 Partial Bunch를 따로 보는 것이 좋다.

