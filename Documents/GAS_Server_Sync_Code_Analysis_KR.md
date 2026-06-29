# GAS 서버 코드 및 동기화 코드 분석 흐름

이 문서는 GAS를 서버 코드와 동기화 코드 중심으로 따라가기 위한 분석 문서다.

목표는 단순하다.

```txt
함수 흐름을 따라가면서
 -> 서버가 Ability / GameplayEffect 결과를 어떻게 확정하는지 보고
 -> 확정된 결과가 클라이언트로 어떻게 전달되는지 확인한다.
```

현재 문서는 프로젝트 예시보다 엔진 GAS 흐름을 먼저 본다.
현재 프로젝트의 예시는 마지막 섹션에서 별도로 확인한다.

## 1. 선행 문서

먼저 아래 문서를 읽고 들어가는 것을 추천한다.

```txt
TowerDefence/Documents/GAS_Code_Analysis_Flow_KR.md
```

선행하면 좋은 이유:

```txt
1. ASC / AttributeSet / GameplayEffect / GameplayAbility / GameplayTag 역할이 먼저 잡힌다.
2. GiveAbility(), MakeOutgoingSpec(), ApplyGameplayEffectSpecToSelf()의 역할 차이를 알고 시작할 수 있다.
3. 서버에서 준비하는 단계와 서버에서 실행하는 단계를 구분할 수 있다.
4. 동기화 코드를 볼 때 "무엇이 복제되어야 하는 결과인지"를 먼저 알고 들어갈 수 있다.
```

즉 `GAS_Code_Analysis_Flow_KR.md`는 GAS 기본 흐름 문서이고, 이 문서는 그 다음 단계로 서버 권위와 클라이언트 동기화 경로를 추적하기 위한 문서다.

## 2. 표시 규칙

이 문서에서는 함수 옆에 아래 표시를 붙인다.

```txt
[선행 학습]
 -> GAS_Code_Analysis_Flow_KR.md 또는 이전 대화에서 이미 역할을 확인한 함수
 -> 여기서는 서버 권위 / 동기화 관점으로 다시 본다.

[필수 확인]
 -> GAS 서버 코드와 동기화 코드를 이해하기 위해 반드시 확인해야 하는 함수

[프로젝트 예시]
 -> 현재 TowerDefence 프로젝트에서 실제로 연결되는 코드
```

두 표시가 같이 붙을 수 있다.

```txt
ASC::ApplyGameplayEffectSpecToSelf() [선행 학습] [필수 확인]
```

이 경우 의미는 다음과 같다.

```txt
기본 역할은 이미 봤지만,
서버 확정 / Attribute 변경 / Tag 변경 / ActiveGE 등록 / 클라이언트 복제 관점에서는 반드시 다시 봐야 한다.
```

## 3. 전체 큰 흐름

GAS 서버 코드와 동기화 코드는 아래 큰 흐름으로 따라가면 된다.

```txt
0. 네트워크 전제 확인
 -> Actor가 서버에서 생성되는가?
 -> Actor가 복제되는가?
 -> ASC가 복제되는가?
 -> AttributeSet이 복제되는가?

1. GAS 준비
 -> ASC 생성
 -> AttributeSet 생성/등록
 -> ASC::InitAbilityActorInfo()
 -> ASC::GiveAbility()

2. Ability 실행
 -> ASC::TryActivateAbilityByClass()
 -> ASC::TryActivateAbility()
 -> Client/Server RPC 필요 여부 판단
 -> ASC::InternalServerTryActivateAbility()
 -> ASC::InternalTryActivateAbility()
 -> UGameplayAbility::CanActivateAbility()
 -> UGameplayAbility::CallActivateAbility()
 -> UGameplayAbility::ActivateAbility()
 -> UGameplayAbility::CommitAbility()

3. GameplayEffect Spec 생성
 -> ASC::MakeEffectContext()
 -> ASC::MakeOutgoingSpec()
 -> FGameplayEffectSpec::Initialize()
 -> AssignTagSetByCallerMagnitude()

4. GameplayEffect 적용
 -> ASC::ApplyGameplayEffectSpecToSelf()
 -> FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec()
 -> ASC::ExecuteGameplayEffect()
 -> FActiveGameplayEffectsContainer::ExecuteActiveEffectsFrom()

5. 결과 생성
 -> Attribute 변경
 -> Attribute 변경 Delegate 호출
 -> ActiveGameplayEffect 등록
 -> Granted Tag / Cooldown Tag 변경
 -> GameplayCue 실행

6. 클라이언트 동기화
 -> Actor / Component 복제
 -> AbilitySpec 복제
 -> ActiveGameplayEffect 복제
 -> GameplayTag 복제
 -> AttributeSet OnRep
 -> GameplayCue 수신
 -> UI / 연출 갱신
```

큰 그림으로 보면 서버와 클라이언트 역할은 다음과 같다.

```txt
[서버]
실행 가능 여부 검증
Ability 실행 확정
GameplayEffect 적용 확정
Attribute / Tag / Cue 결과 확정
복제 대상 상태 변경

[클라이언트]
입력 또는 예측 실행 가능
최종 결과는 서버에서 수신
Attribute / Tag / ActiveGE / Cue 결과를 보고 UI와 연출 갱신
```

## 4. 0단계: 네트워크 전제 확인

GAS 동기화를 보기 전에 먼저 Unreal 복제 전제를 확인한다.

확인할 내용:

```txt
1. Actor가 서버에서 생성되는가?
2. Actor에 bReplicates가 켜져 있는가?
3. ASC가 복제 컴포넌트인가?
4. ASC의 ReplicationMode는 무엇인가?
5. AttributeSet에 ReplicatedUsing / OnRep가 있는가?
6. 클라이언트에도 대상 Actor와 ASC가 존재하는가?
```

꼭 확인할 함수 / 코드:

```txt
AActor::GetLifetimeReplicatedProps() [필수 확인]
 -> Actor의 일반 UPROPERTY 복제 확인

UActorComponent::SetIsReplicated() [필수 확인]
 -> ASC 같은 컴포넌트가 복제되는지 확인

UAbilitySystemComponent::SetReplicationMode() [필수 확인]
 -> ActiveGameplayEffect / Tag / Cue 정보가 어떤 수준으로 복제되는지 확인

UAttributeSet::GetLifetimeReplicatedProps() [필수 확인]
 -> Attribute 값 자체가 클라이언트에 복제되는지 확인

GAMEPLAYATTRIBUTE_REPNOTIFY() [필수 확인]
 -> Attribute OnRep 시 GAS delegate와 예측 보정이 제대로 이어지는지 확인
```

이 단계에서 중요한 판단:

```txt
ASC가 복제되지 않으면 GAS 상태 복제를 기대하기 어렵다.
AttributeSet에 OnRep가 없으면 클라이언트 UI가 Attribute 값을 직접 받는 구조가 아닐 수 있다.
GameplayCue나 Tag는 GE/ASC 설정과 복제 모드에 따라 클라이언트 수신 방식이 달라진다.
```

## 5. 1단계: GAS 준비 흐름

이 단계는 Actor가 GAS를 사용할 준비를 하는 과정이다.

```txt
[서버]
Actor 생성
 -> ASC 생성
 -> AttributeSet 생성/등록
 -> ASC::InitAbilityActorInfo()
 -> ASC::GiveAbility()
 -> AbilitySpec이 ASC에 등록됨
 -> 필요하면 AbilitySpec 복제 대상이 됨

[클라이언트]
복제된 Actor 수신
 -> 복제된 ASC / AttributeSet 상태 확인
 -> AbilitySpec이 복제되는 구조라면 사용 가능한 Ability 목록 수신
```

확인할 함수:

```txt
UAbilitySystemComponent::InitializeComponent() [선행 학습]
 -> ASC 컴포넌트 초기화 시점 확인
 -> AttributeSet 자동 탐색 / 등록 흐름을 확인할 때 본다.

UAbilitySystemComponent::OnRegister() [선행 학습]
 -> 컴포넌트 등록 시점 확인
 -> 생성자보다 먼저 호출되는 함수가 아니라, 컴포넌트 등록 과정에서 호출된다.

UAbilitySystemComponent::InitAbilityActorInfo() [선행 학습] [필수 확인]
 -> ASC가 OwnerActor / AvatarActor를 알게 되는 지점
 -> Ability 실행, EffectContext, 권한 판단의 기준이 된다.

UAbilitySystemComponent::GiveAbility() [선행 학습] [필수 확인]
 -> 서버가 ASC에 AbilitySpec을 등록하는 지점
 -> 일반적으로 서버 권위로 호출해야 한다.

UAbilitySystemComponent::OnGiveAbility() [선행 학습] [필수 확인]
 -> AbilitySpec 등록 후 후처리
 -> InstancedPerActor Ability 인스턴스 생성
 -> GameplayEffect가 부여한 Ability라면 ActiveGE와 AbilitySpec을 연결
 -> Ability Trigger Tag 등록
 -> AbilitySpec 복제 dirty 처리 흐름과 연결된다.
```

이 단계에서 중점 질문:

```txt
1. GiveAbility()가 서버에서만 호출되는가?
2. AbilitySpec이 클라이언트로 복제되는 구조인가?
3. AbilityActorInfo가 서버와 클라이언트 모두에서 유효한가?
4. AvatarActor가 바뀌는 경우 InitAbilityActorInfo()가 다시 호출되는가?
5. Ability가 GameplayEffect에 의해 부여되는 구조가 있는가?
```

## 6. 2단계: Ability 실행 흐름

이 단계는 등록된 Ability를 실제로 실행하는 과정이다.

큰 흐름:

```txt
[서버 소유 실행]
서버 코드
 -> ASC::TryActivateAbilityByClass()
 -> ASC::TryActivateAbility()
 -> ASC::InternalTryActivateAbility()
 -> UGameplayAbility::CanActivateAbility()
 -> UGameplayAbility::CallActivateAbility()
 -> UGameplayAbility::ActivateAbility()

[클라이언트 입력 실행]
클라이언트 입력
 -> ASC::TryActivateAbilityByClass()
 -> ASC::TryActivateAbility()
 -> 필요하면 Client Prediction
 -> ASC::CallServerTryActivateAbility()
 -> Server RPC
 -> ASC::InternalServerTryActivateAbility()
 -> 서버 검증 후 실행 확정
 -> 성공/실패 결과 클라이언트 보정
```

확인할 함수:

```txt
UAbilitySystemComponent::TryActivateAbilityByClass() [선행 학습] [필수 확인]
 -> Ability 클래스로 ActivatableAbilities에서 Spec을 찾는다.
 -> 찾은 Spec Handle로 TryActivateAbility()를 호출한다.

UAbilitySystemComponent::TryActivateAbility() [선행 학습] [필수 확인]
 -> AbilitySpecHandle 기준으로 실행을 시도한다.
 -> NetExecutionPolicy, LocalRole, Prediction 가능 여부에 따라 로컬 실행 또는 서버 요청으로 나뉜다.

UAbilitySystemComponent::ClientTryActivateAbility() [필수 확인]
 -> 서버가 클라이언트에게 Ability 실행을 지시했는데 클라이언트에 Spec 복제가 아직 없을 수 있는 상황을 처리한다.
 -> PendingServerActivatedAbilities 흐름을 확인한다.

UAbilitySystemComponent::CallServerTryActivateAbility() [선행 학습] [필수 확인]
 -> 클라이언트가 서버에게 Ability 실행을 요청하는 진입점
 -> PredictionKey와 InputPressed 상태가 함께 전달된다.

UAbilitySystemComponent::InternalServerTryActivateAbility() [선행 학습] [필수 확인]
 -> 서버가 클라이언트 요청을 검증하는 핵심 지점
 -> NetSecurityPolicy, PredictionKey, Spec 유효성, CanActivateAbility() 흐름을 확인한다.

FScopedPredictionWindow [선행 학습] [필수 확인]
 -> 클라이언트 예측과 서버 보정을 연결하는 PredictionKey 범위
 -> Ability 실행 중 생성되는 GE, Cue, TargetData 등이 어떤 예측 키로 묶이는지 본다.

UAbilitySystemComponent::InternalTryActivateAbility() [선행 학습] [필수 확인]
 -> 실제 Ability 실행 직전의 핵심 함수
 -> AbilitySpec 찾기, 인스턴스 준비, CanActivateAbility(), CallActivateAbility() 흐름을 확인한다.

UGameplayAbility::CanActivateAbility() [선행 학습] [필수 확인]
 -> 쿨다운, 비용, 태그 조건, 블로킹 태그 등을 검사한다.

UGameplayAbility::CallActivateAbility() [선행 학습] [필수 확인]
 -> ActivateAbility()를 호출하기 전 PreActivate, Delegate 연결, TriggerData 전달 등을 처리한다.

UGameplayAbility::ActivateAbility() [선행 학습] [필수 확인]
 -> 실제 Ability 행동 로직
 -> C++ Ability라면 자식 클래스에서 override한다.
 -> BP Ability라면 Event ActivateAbility 쪽을 확인한다.

UGameplayAbility::CommitAbility() [선행 학습] [필수 확인]
 -> 비용과 쿨다운을 실제로 적용한다.
 -> Ability를 만들 때 자동 호출되지 않으므로, Ability 구현 쪽에서 호출 여부를 확인해야 한다.

UGameplayAbility::EndAbility() [선행 학습] [필수 확인]
 -> Ability 종료 처리
 -> ActivationOwnedTags 제거
 -> 종료 복제 / Prediction 정리 흐름을 확인한다.
```

이 단계에서 중점 질문:

```txt
1. 이 Ability는 서버에서만 실행되는가?
2. 클라이언트가 실행 요청을 보내는 구조인가?
3. Ability의 NetExecutionPolicy / NetSecurityPolicy는 무엇인가?
4. PredictionKey가 생성되고 서버로 전달되는가?
5. CommitAbility()가 호출되어 쿨다운과 비용이 실제 적용되는가?
6. EndAbility()가 호출되어 ActivationOwnedTags가 제거되는가?
```

## 7. 3단계: GameplayEffect Spec 생성 흐름

Ability나 일반 게임 로직은 GE를 바로 적용하지 않고, 보통 Spec을 만든 뒤 적용한다.

큰 흐름:

```txt
[서버]
EffectContext 생성
 -> GameplayEffectClass 선택
 -> MakeOutgoingSpec()
 -> FGameplayEffectSpec 생성
 -> SetByCaller 값 주입
 -> ApplyGameplayEffectSpecToSelf() 또는 ApplyGameplayEffectSpecToTarget()
```

확인할 함수:

```txt
UAbilitySystemComponent::MakeEffectContext() [선행 학습] [필수 확인]
 -> Instigator, EffectCauser, SourceObject 등 GE 적용 맥락을 만든다.
 -> 나중에 데미지 출처, GameplayCue Parameters, ExecutionCalculation에서 사용될 수 있다.

UAbilitySystemComponent::MakeOutgoingSpec() [선행 학습] [필수 확인]
 -> GameplayEffectClass를 런타임 적용용 FGameplayEffectSpec으로 만든다.
 -> 아직 적용한 것은 아니다.

FGameplayEffectSpec::Initialize() [선행 학습] [필수 확인]
 -> GE 정의, Level, Context, Captured Attributes, Tag 정보를 Spec에 구성한다.
 -> Level 기반 ScalableFloat 계산이나 Attribute Capture를 볼 때 중요하다.

UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude() [선행 학습] [필수 확인]
 -> Spec 안에 GameplayTag를 키로 숫자 값을 넣는다.
 -> Damage.SetByCaller 같은 태그는 ASC에 붙는 상태 태그가 아니라 Spec 내부 데이터 키다.
```

이 단계에서 중점 질문:

```txt
1. 어떤 GameplayEffectClass를 사용하는가?
2. Level 값은 실제 계산에 쓰이는가?
3. SetByCaller 태그가 GE Modifier와 정확히 맞는가?
4. EffectContext에 SourceObject / Instigator / Causer가 들어가는가?
5. Attribute Capture를 쓰는 GE인가?
```

## 8. 4단계: GameplayEffect 적용 흐름

이 단계가 GAS 서버 코드 분석의 핵심이다.

큰 흐름:

```txt
[서버]
ASC::ApplyGameplayEffectSpecToSelf()
 -> 권한 / 유효성 / Application Query 확인
 -> Instant / Duration / Infinite 분기
 -> Modifier Magnitude 계산
 -> Attribute 변경 또는 ActiveGameplayEffect 등록
 -> Granted Tag / GameplayCue / Delegate 발생
 -> 복제 대상 데이터 dirty 처리

[클라이언트]
서버에서 복제된 ActiveGE / Attribute / Tag / Cue 결과를 수신
 -> UI / 연출 갱신
```

확인할 함수:

```txt
UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf() [선행 학습] [필수 확인]
 -> GE Spec을 자기 ASC에 적용하는 핵심 진입점
 -> 서버 권한, Prediction, Application Query, ActiveGE 등록, Instant 실행 흐름을 본다.

GameplayEffectApplicationQueries [선행 학습] [필수 확인]
 -> GE 적용 가능 여부를 외부 조건으로 막는 쿼리
 -> 어떤 함수나 람다가 등록하는지 확인한다.

FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec() [선행 학습] [필수 확인]
 -> GE 적용의 실제 컨테이너 처리
 -> Duration / Infinite GE는 ActiveGameplayEffect로 등록된다.
 -> Instant GE는 바로 실행 경로로 들어간다.

FGameplayEffectSpec::CalculateModifierMagnitudes() [선행 학습] [필수 확인]
 -> Modifier 크기를 계산한다.
 -> SetByCaller, ScalableFloat, Attribute Capture 값이 여기서 사용된다.

UAbilitySystemComponent::ExecuteGameplayEffect() [선행 학습] [필수 확인]
 -> Instant GE 또는 실행형 GE가 Attribute에 실제 영향을 주는 경로
 -> Attribute 값이 어디서 바뀌는지 볼 때 중요하다.

FActiveGameplayEffectsContainer::ExecuteActiveEffectsFrom() [선행 학습] [필수 확인]
 -> Active GE에서 주기 실행 또는 누적 효과를 처리하는 흐름
 -> 이미 등록된 ActiveGameplayEffect가 다른 시점에 Attribute에 영향을 주는 경우 확인한다.
```

이 단계에서 중점 질문:

```txt
1. 이 GE는 Instant인가, Duration인가, Infinite인가?
2. Attribute 값을 즉시 바꾸는가?
3. ActiveGameplayEffect로 남아서 주기적으로 실행되는가?
4. Modifier Magnitude는 어떤 방식으로 계산되는가?
5. GE 적용 후 Attribute / Tag / Cue 중 무엇이 결과로 생기는가?
6. 결과가 클라이언트 복제 대상으로 들어가는가?
```

## 9. 5단계: 결과 생성 흐름

GE 적용 후 실제로 생기는 결과는 크게 네 가지다.

```txt
1. Attribute 값 변경
2. ActiveGameplayEffect 등록 / 제거
3. GameplayTag Count 변경
4. GameplayCue 실행
```

### 9.1 Attribute 변경

확인할 함수 / 코드:

```txt
UAttributeSet::PreAttributeBaseChange() [선행 학습] [필수 확인]
 -> BaseValue 변경 전 보정

UAttributeSet::PreAttributeChange() [선행 학습] [필수 확인]
 -> CurrentValue 변경 전 보정

UAttributeSet::PostGameplayEffectExecute() [필수 확인]
 -> Instant GE 실행 후 후처리
 -> 데미지 계산 후 Health Clamp, 사망 판정 트리거 등을 둘 수 있다.

UAbilitySystemComponent::GetGameplayAttributeValueChangeDelegate() [선행 학습] [필수 확인]
 -> Attribute 변경을 감지하는 delegate
 -> 서버에서 호출되는지, 클라이언트에서도 복제 후 호출되는지 확인한다.
```

중점 질문:

```txt
1. Attribute 값은 서버에서 바뀌는가?
2. 클라이언트도 같은 Attribute 값을 받는가?
3. AttributeSet에 OnRep가 있는가?
4. Delegate가 서버와 클라이언트 중 어디서 호출되는가?
```

### 9.2 ActiveGameplayEffect 등록

확인할 함수:

```txt
FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec() [선행 학습] [필수 확인]
 -> Duration / Infinite GE가 ActiveGameplayEffect로 등록되는 위치

FActiveGameplayEffectsContainer::AddActiveGameplayEffectGrantedTagsAndModifiers() [필수 확인]
 -> ActiveGE가 붙으면서 GrantedTag와 Modifier를 ASC에 반영한다.

FActiveGameplayEffectsContainer::RemoveActiveGameplayEffectGrantedTagsAndModifiers() [필수 확인]
 -> ActiveGE가 제거될 때 GrantedTag와 Modifier를 제거한다.

FActiveGameplayEffectsContainer::MarkItemDirty() [필수 확인]
 -> ActiveGameplayEffect FastArray 복제 대상 변경 표시
```

중점 질문:

```txt
1. GE가 ActiveGameplayEffect로 남는가?
2. 남는다면 클라이언트에 복제되어야 하는가?
3. ActiveGE가 제거될 때 Tag / Modifier가 같이 제거되는가?
```

### 9.3 GameplayTag 변경

태그는 두 종류를 구분해서 봐야 한다.

```txt
SetByCaller Tag
 -> Spec 안의 데이터 키
 -> ASC에 붙는 상태 태그가 아니다.

Owned GameplayTag
 -> ASC가 현재 가지고 있는 상태 태그
 -> GE GrantedTag, LooseTag, ActivationOwnedTags 등으로 생긴다.
```

확인할 함수:

```txt
UAbilitySystemComponent::UpdateTagMap() [필수 확인]
 -> ASC의 Owned Tag Count를 증가/감소시킨다.

UAbilitySystemComponent::RegisterGameplayTagEvent() [필수 확인]
 -> 특정 태그 Count 변경을 감지한다.

UGameplayAbility::PreActivate() [필수 확인]
 -> ActivationOwnedTags가 붙는 위치를 확인한다.

UGameplayAbility::EndAbility() [선행 학습] [필수 확인]
 -> ActivationOwnedTags가 제거되는 위치를 확인한다.

UGameplayAbility::CheckCooldown() [선행 학습] [필수 확인]
 -> ASC에 Cooldown Tag가 있는지 확인한다.

UGameplayAbility::ApplyCooldown() [선행 학습] [필수 확인]
 -> Cooldown GE를 적용해 Cooldown Tag를 붙인다.
```

중점 질문:

```txt
1. GE가 GrantedTag를 주는가?
2. Cooldown Tag는 어떤 GE가 부여하는가?
3. ActivationOwnedTags는 Ability 실행 중에만 붙는가?
4. 클라이언트에서 태그 변화를 감지하는 코드가 있는가?
```

### 9.4 GameplayCue 실행

확인할 함수:

```txt
UAbilitySystemComponent::ExecuteGameplayCue() [필수 확인]
 -> 순간성 Cue 실행

UAbilitySystemComponent::AddGameplayCue() [필수 확인]
 -> 지속성 Cue 시작

UAbilitySystemComponent::RemoveGameplayCue() [필수 확인]
 -> 지속성 Cue 종료

UAbilitySystemComponent::InvokeGameplayCueEvent() [필수 확인]
 -> 실제 GameplayCue 이벤트 발생 경로
```

중점 질문:

```txt
1. GE에 GameplayCue가 설정되어 있는가?
2. Instant GE라면 Executed Cue가 발생하는가?
3. Duration / Infinite GE라면 OnActive / WhileActive / Removed Cue가 발생하는가?
4. 클라이언트에서 GameplayCueNotify가 실행되는가?
```

## 10. 6단계: 클라이언트 동기화 흐름

서버에서 결과가 생긴 뒤 클라이언트가 받는 경로는 여러 개다.

```txt
1. Actor 일반 복제
2. ASC 복제
3. AbilitySpec 복제
4. ActiveGameplayEffect 복제
5. GameplayTag 복제
6. AttributeSet 복제
7. GameplayCue RPC / Cue 복제
```

확인할 함수 / 구조:

```txt
AActor::GetLifetimeReplicatedProps() [필수 확인]
 -> 일반 Actor 변수 복제

UAbilitySystemComponent::GetLifetimeReplicatedProps() [필수 확인]
 -> ASC가 어떤 GAS 상태를 복제하는지 확인

FGameplayAbilitySpecContainer::NetDeltaSerialize() [필수 확인]
 -> AbilitySpec FastArray 복제 흐름

FActiveGameplayEffectsContainer::NetDeltaSerialize() [필수 확인]
 -> ActiveGameplayEffect FastArray 복제 흐름

FMinimalReplicationTagCountMap::NetSerialize() [필수 확인]
 -> Minimal Replication Tag Count 복제 흐름

UAttributeSet::GetLifetimeReplicatedProps() [필수 확인]
 -> AttributeSet 값 복제

OnRep_AttributeName() + GAMEPLAYATTRIBUTE_REPNOTIFY() [필수 확인]
 -> 클라이언트 Attribute 수신 후 GAS delegate / 예측 보정 연결

GameplayCue Notify / GameplayCueManager [필수 확인]
 -> 클라이언트 연출 수신 경로
```

중점 질문:

```txt
1. 서버에서 바뀐 값이 어떤 복제 컨테이너에 들어가는가?
2. 해당 컨테이너가 FastArray로 복제되는가?
3. 클라이언트 수신 후 OnRep 또는 PostReplicatedAdd/Change/Remove가 호출되는가?
4. UI는 Attribute / Tag / ActiveGE / Cue 중 무엇을 보고 갱신되는가?
```

## 11. 함수 흐름별 분석 순서

실제로 코드를 볼 때는 아래 순서대로 따라가면 된다.

```txt
1. ASC 복제 전제 확인 [필수 확인]
   -> SetIsReplicated()
   -> SetReplicationMode()
   -> GetLifetimeReplicatedProps()

2. Ability 등록 확인 [선행 학습] [필수 확인]
   -> InitAbilityActorInfo()
   -> GiveAbility()
   -> OnGiveAbility()
   -> AbilitySpec 복제 여부

3. Ability 실행 확인 [선행 학습] [필수 확인]
   -> TryActivateAbilityByClass()
   -> TryActivateAbility()
   -> CallServerTryActivateAbility()
   -> InternalServerTryActivateAbility()
   -> InternalTryActivateAbility()
   -> CanActivateAbility()
   -> CallActivateAbility()
   -> ActivateAbility()
   -> CommitAbility()
   -> EndAbility()

4. GE Spec 생성 확인 [선행 학습] [필수 확인]
   -> MakeEffectContext()
   -> MakeOutgoingSpec()
   -> FGameplayEffectSpec::Initialize()
   -> AssignTagSetByCallerMagnitude()

5. GE 적용 확인 [선행 학습] [필수 확인]
   -> ApplyGameplayEffectSpecToSelf()
   -> ApplyGameplayEffectSpec()
   -> CalculateModifierMagnitudes()
   -> ExecuteGameplayEffect()
   -> ExecuteActiveEffectsFrom()

6. 결과 확인 [필수 확인]
   -> Attribute 변경
   -> ActiveGameplayEffect 등록
   -> GrantedTag / CooldownTag / ActivationOwnedTags 변화
   -> GameplayCue 발생

7. 클라이언트 수신 확인 [필수 확인]
   -> AbilitySpec 복제
   -> ActiveGE 복제
   -> Tag 복제
   -> Attribute OnRep
   -> GameplayCue 실행
   -> UI / 연출 갱신
```

## 12. 현재 프로젝트 예시

이 섹션은 엔진 GAS 흐름을 본 뒤, 현재 프로젝트에서 어디에 연결되는지 확인하기 위한 예시다.

### 12.1 적 초기 스탯 적용

파일:

```txt
Source/TowerDefence/Private/Enemy/TDEnemyActor.cpp
```

흐름:

```txt
[서버] [프로젝트 예시]
ATDEnemyActor 생성
 -> AbilitySystemComponent 생성
 -> EnemySet 생성
 -> PostInitializeComponents()
 -> InitAbilityActorInfo(this, this) [선행 학습] [필수 확인]
 -> BeginPlay()
 -> GetGameplayAttributeValueChangeDelegate(Health) 바인딩 [선행 학습] [필수 확인]
 -> InitializeASC()
 -> MakeOutgoingSpec(DefaultEffect) [선행 학습] [필수 확인]
 -> AssignTagSetByCallerMagnitude(Enemy.Health.SetByCaller, InitialHealth) [선행 학습] [필수 확인]
 -> AssignTagSetByCallerMagnitude(Enemy.MoveSpeed.SetByCaller, InitialMoveSpeed) [선행 학습] [필수 확인]
 -> AssignTagSetByCallerMagnitude(Enemy.Damage.SetByCaller, InitialDamage) [선행 학습] [필수 확인]
 -> ApplyGameplayEffectSpecToSelf() [선행 학습] [필수 확인]
 -> UTDEnemySet 값 변경
```

현재 확인 포인트:

```txt
1. ATDEnemyActor는 bReplicates = true이다.
2. CurrentPath / Distance / IsDead는 일반 Actor Replication으로 복제된다.
3. UTDEnemySet의 Attribute에는 현재 ReplicatedUsing이 보이지 않는다.
4. AbilitySystemComponent->SetIsReplicated(true)는 현재 C++ 검색 기준으로 보이지 않는다.
```

### 12.2 타워 Ability 등록

파일:

```txt
Source/TowerDefence/Private/Tower/TDTowerPawn.cpp
```

흐름:

```txt
[서버] [프로젝트 예시]
ATDTowerPawn 생성
 -> AbilitySystemComponent 생성
 -> TowerSet 생성
 -> PostInitializeComponents()
 -> InitAbilityActorInfo(this, this) [선행 학습] [필수 확인]
 -> BeginPlay()
 -> InitializeASC()
 -> HasAuthority() && DefaultAbility
 -> GiveAbility(FGameplayAbilitySpec(DefaultAbility, 1, -1, this)) [선행 학습] [필수 확인]
```

현재 확인 포인트:

```txt
1. GiveAbility()는 HasAuthority() 안에서 호출된다.
2. Ability 등록은 서버 권위 흐름으로 보인다.
3. AbilitySpec이 클라이언트로 복제되는지는 ASC 복제 설정을 확인해야 한다.
```

### 12.3 타워 Ability 실행

파일:

```txt
Source/TowerDefence/Private/TDTowerBase.cpp
```

흐름:

```txt
[서버 / 클라이언트 가능성 있음] [프로젝트 예시]
ATDTowerBase::Tick()
 -> AbilitySystemComponent->TryActivateAbilityByClass(DefaultAbility, true) [선행 학습] [필수 확인]
 -> GA_FireWeapon 실행
 -> ATD_Weapon::FireAtEnemy() [선행 학습] [필수 확인]
 -> MulticastFireProjectile()
```

현재 확인 포인트:

```txt
1. ATDTowerBase::Tick()에는 HasAuthority() 조건이 없다.
2. 서버와 클라이언트 모두 Tick을 돌 수 있다.
3. 실제 Ability 실행이 서버에서만 되는지 TryActivateAbilityByClass() 내부와 Ability NetPolicy를 확인해야 한다.
4. 타워가 클라이언트 소유 Actor가 아니면 Client -> Server 원격 Ability 실행이 일반 플레이어 Pawn처럼 동작하지 않을 수 있다.
```

### 12.4 Projectile 명중 후 데미지 적용

파일:

```txt
Source/TowerDefence/Private/Tower/TDProjectile.cpp
Source/TowerDefence/Private/TDFL_Utility.cpp
```

흐름:

```txt
[서버] [프로젝트 예시]
ATDProjectile::OnHitTarget() [선행 학습] [필수 확인]
 -> HasAuthority()
 -> UTDFL_Utility::EnemyDamage(Target, Damage, BP_GE_DamageClass) [선행 학습] [필수 확인]
 -> TargetASC = GetAbilitySystemComponent(Enemy)
 -> MakeOutgoingSpec(GE_DamageClass, 0.0f, MakeEffectContext()) [선행 학습] [필수 확인]
 -> AssignTagSetByCallerMagnitude(Damage.SetByCaller, -Damage) [선행 학습] [필수 확인]
 -> ApplyGameplayEffectSpecToSelf() [선행 학습] [필수 확인]
 -> Enemy Health 변경
 -> ATDEnemyActor::OnHealthAttributeChanged() [선행 학습] [필수 확인]
 -> Health <= 0 이면 사망 처리
```

현재 확인 포인트:

```txt
1. 데미지 확정은 HasAuthority() 안에서 실행되므로 서버 권위 흐름으로 보인다.
2. Projectile 연출은 MulticastFireProjectile() 흐름과 별도로 봐야 한다.
3. Health 변경 결과가 클라이언트 UI에 어떻게 보이는지는 AttributeSet 복제 또는 별도 UI 갱신 경로를 확인해야 한다.
```

### 12.5 현재 프로젝트에서 우선 확인할 것

```txt
1. ASC 복제 설정 [필수 확인]
   -> AbilitySystemComponent->SetIsReplicated(true)가 있는가?
   -> SetReplicationMode()가 있는가?

2. AttributeSet 복제 [필수 확인]
   -> UTDEnemySet / UTDTowerSet에 ReplicatedUsing이 있는가?
   -> GetLifetimeReplicatedProps()가 있는가?
   -> GAMEPLAYATTRIBUTE_REPNOTIFY()를 쓰는가?

3. Ability 실행 권한 [필수 확인]
   -> ATDTowerBase::Tick()에서 클라이언트도 TryActivateAbilityByClass()를 호출하는가?
   -> GA_FireWeapon의 NetExecutionPolicy는 무엇인가?
   -> CommitAbility()가 호출되는가?

4. GE 에셋 설정 [필수 확인]
   -> GE_Damage의 Modifier가 Damage.SetByCaller를 읽는가?
   -> GE_FireWeaponCooldown이 Weapon.Cooldown 태그를 부여하는가?
   -> GE_StickyMud가 Debuff.StickyMud 태그를 부여하는가?
   -> GameplayCue가 설정되어 있는 GE가 있는가?

5. 클라이언트 수신 경로 [필수 확인]
   -> Attribute OnRep가 호출되는가?
   -> Tag Event가 클라이언트에서 바인딩되어 있는가?
   -> GameplayCueNotify가 실행되는가?
   -> UI는 어떤 값으로 갱신되는가?
```

## 13. 로그 추천 위치

서버 확인용:

```txt
ATDTowerPawn::InitializeASC() [프로젝트 예시]
ATDTowerBase::Tick() [프로젝트 예시]
ATD_Weapon::FireAtEnemy() [프로젝트 예시]
ATDProjectile::OnHitTarget() [프로젝트 예시]
UTDFL_Utility::EnemyDamage() [프로젝트 예시]
ATDEnemyActor::OnHealthAttributeChanged() [프로젝트 예시]
```

클라이언트 확인용:

```txt
ATDEnemyActor::OnRep_CurrentPath() [프로젝트 예시]
ATDEnemyActor::OnRep_Distance() [프로젝트 예시]
ATDEnemyActor::OnHealthAttributeChanged() [프로젝트 예시]
GameplayCueNotify 실행 여부
UI 체력바 갱신 함수
```

로그에 같이 찍을 값:

```txt
HasAuthority()
GetNetMode()
GetLocalRole()
GetRemoteRole()
Actor 이름
ASC 포인터
Health OldValue / NewValue
Owned GameplayTag 목록
ActiveGameplayEffect 개수
PredictionKey
```

## 14. 최종 요약

이 문서는 아래 순서대로 보면 된다.

```txt
1. 네트워크 전제 확인
2. GAS 준비 확인
3. Ability 실행 확인
4. GameplayEffect Spec 생성 확인
5. GameplayEffect 적용 확인
6. Attribute / Tag / Cue 결과 확인
7. 클라이언트 복제 수신 확인
8. 마지막에 현재 프로젝트 예시로 연결
```

가장 중요한 함수는 다음이다.

```txt
UAbilitySystemComponent::TryActivateAbility() [선행 학습] [필수 확인]
UAbilitySystemComponent::CallServerTryActivateAbility() [필수 확인]
UAbilitySystemComponent::InternalServerTryActivateAbility() [필수 확인]
UAbilitySystemComponent::InternalTryActivateAbility() [선행 학습] [필수 확인]
UGameplayAbility::CommitAbility() [선행 학습] [필수 확인]
UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf() [선행 학습] [필수 확인]
FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec() [선행 학습] [필수 확인]
UAbilitySystemComponent::ExecuteGameplayEffect() [선행 학습] [필수 확인]
UAbilitySystemComponent::GetLifetimeReplicatedProps() [필수 확인]
UAttributeSet::GetLifetimeReplicatedProps() [필수 확인]
GAMEPLAYATTRIBUTE_REPNOTIFY() [필수 확인]
```

이번 분석의 핵심 문장은 이것이다.

```txt
서버가 Ability와 GameplayEffect 결과를 확정하고,
클라이언트는 ASC / AttributeSet / ActiveGE / Tag / GameplayCue 복제를 통해 그 결과를 수신한다.
```
