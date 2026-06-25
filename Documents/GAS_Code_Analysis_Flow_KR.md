# GAS 코드 분석 기준 흐름

이 문서는 내일 GAS 코드 분석을 시작할 때 기준으로 볼 흐름이다.

처음에는 GAS를 너무 세부적으로 보지 말고, 아래 흐름을 큰 기준선으로 잡는다.

## 1. 전체 기준 흐름

```txt
[서버]
Actor 생성
 -> ASC 생성
 -> AttributeSet 생성/등록
 -> ASC::InitAbilityActorInfo()
 -> ASC::GiveAbility()
 -> ASC::MakeOutgoingSpec(GameplayEffect)
 -> AssignTagSetByCallerMagnitude()
 -> ASC::ApplyGameplayEffectSpecToSelf()
 -> AttributeSet 값 변경
 -> Attribute 변경 Delegate / OnRep / GameplayCue / Tag 변화

[클라이언트]
복제된 Actor 수신
 -> ASC / AttributeSet 상태 수신
 -> Attribute 값, Tag, Effect 상태를 보고 UI/연출 갱신
```

위 흐름은 GAS를 처음 따라갈 때 좋은 큰 그림이다.
다만 실제 코드 분석에서는 이 흐름을 두 단계로 나누어 보는 것이 좋다.

```txt
1. GAS 준비 단계
2. GAS 실행 단계
```

## 2. GAS 준비 단계

```txt
[서버]
Actor 생성
 -> ASC 생성
 -> AttributeSet 생성/등록
 -> ASC::InitAbilityActorInfo()
 -> ASC::GiveAbility()
```

이 단계는 Actor가 GAS를 사용할 준비를 하는 과정이다.

각 객체의 역할은 다음과 같다.

```txt
Actor
 -> 게임 월드에 존재하는 실제 대상

ASC
 -> Ability, GameplayEffect, GameplayTag, AttributeSet을 관리하는 GAS 중심 컴포넌트

AttributeSet
 -> Health, Damage, Range 같은 실제 스탯 값을 저장하는 객체

GameplayAbility
 -> 공격, 스킬, 발사 같은 행동 로직

GameplayEffect
 -> Attribute나 GameplayTag를 변경하는 규칙

GameplayTag
 -> 상태, 조건, 분류, SetByCaller 값 이름을 표현하는 키
```

핵심 함수:

```txt
ASC::InitAbilityActorInfo()
 -> ASC에게 OwnerActor와 AvatarActor를 알려준다.
 -> Ability 실행, 권한 판단, Targeting, EffectContext에서 기준이 된다.

ASC::GiveAbility()
 -> ASC에 사용할 수 있는 Ability를 등록한다.
 -> 등록만 하는 것이고, 즉시 실행한다는 뜻은 아니다.
```

예를 들어 RPG 게임에서 캐릭터가 스킬을 배우면 보통 서버에서 `GiveAbility()`를 호출한다.

```txt
[서버]
스킬북 획득
 -> ASC::GiveAbility(GA_Fireball)
 -> 캐릭터가 Fireball Ability를 사용할 수 있게 됨
```

스킬을 잊거나 장비를 해제하면 `ClearAbility()` 같은 함수로 제거한다.

## 3. GAS 실행 단계

```txt
[서버]
Ability 실행 또는 게임 로직 발생
 -> ASC::MakeOutgoingSpec(GameplayEffect)
 -> AssignTagSetByCallerMagnitude()
 -> ASC::ApplyGameplayEffectSpecToSelf()
 -> AttributeSet 값 변경
 -> Attribute 변경 Delegate / GameplayCue / GameplayTag 변화
 -> 복제 대상 상태 변경
```

이 단계는 실제 효과가 발생하는 과정이다.

예를 들어 데미지, 힐, 버프, 디버프, 쿨다운, 비용 지불 등이 여기에 해당한다.

핵심 함수:

```txt
ASC::MakeOutgoingSpec(GameplayEffect)
 -> 이번에 적용할 GameplayEffect의 런타임 적용 정보를 만든다.
 -> 아직 적용한 것은 아니다.

AssignTagSetByCallerMagnitude()
 -> GameplayEffectSpec 안에 GameplayTag를 키로 해서 숫자 값을 넣는다.
 -> 예: Damage.SetByCaller = -10

ASC::ApplyGameplayEffectSpecToSelf()
 -> 준비된 GameplayEffectSpec을 실제 ASC에 적용한다.
 -> 이때 AttributeSet 값이 바뀌거나 GameplayTag가 붙거나 GameplayCue가 실행될 수 있다.
```

`GiveAbility()`와 `MakeOutgoingSpec()`은 역할이 다르다.

```txt
GiveAbility()
 -> 이 캐릭터가 어떤 Ability를 사용할 수 있는지 등록

MakeOutgoingSpec()
 -> 특정 GameplayEffect를 이번에 적용하기 위한 데이터 생성
```

따라서 두 함수가 항상 바로 이어서 호출되는 것은 아니다.

예:

```txt
[서버]
캐릭터 생성
 -> GiveAbility(GA_Fireball)

몇 초 뒤

[클라이언트]
플레이어가 스킬 키 입력

[서버]
Ability 실행 검증
 -> Ability 내부에서 MakeOutgoingSpec(GE_FireballDamage)
 -> ApplyGameplayEffectSpecToSelf()
 -> 대상 Health 감소
```

## 4. 클라이언트 관점

```txt
[클라이언트]
복제된 Actor 수신
 -> ASC / AttributeSet 상태 수신
 -> Attribute 값, GameplayTag, ActiveGameplayEffect 상태 확인
 -> UI / 연출 / 상태 표시 갱신
```

클라이언트는 보통 게임 결과를 직접 확정하지 않는다.

```txt
[서버]
데미지 적용
체력 변경
사망 판정
보상 지급
태그 부여
쿨다운 적용

[클라이언트]
복제된 결과를 보고 체력바, 이펙트, 아이콘, 애니메이션 갱신
```

GAS Prediction을 사용하는 경우 클라이언트가 먼저 예측 실행할 수도 있지만, 최종 권위는 서버가 가진다.
현재 프로젝트 분석에서는 우선 서버 권위 흐름부터 보는 것이 좋다.

## 5. 현재 프로젝트 기준 흐름

현재 프로젝트의 타워 발사와 적 데미지 흐름은 다음처럼 보면 된다.

```txt
[서버: 타워 초기화]
ATDTowerPawn::InitializeASC()
 -> ASC::GiveAbility(DefaultAbility)

[서버: 타워 Tick]
ATDTowerBase::Tick()
 -> ASC::TryActivateAbilityByClass(DefaultAbility, true)

[서버: Ability 실행 결과]
GA_FireWeapon 실행
 -> Weapon::FireAtEnemy()
 -> MulticastFireProjectile()

[서버: 투사체 명중]
ATDProjectile::OnHitTarget()
 -> UTDFL_Utility::EnemyDamage()
 -> ASC::MakeOutgoingSpec(GE_Damage)
 -> AssignTagSetByCallerMagnitude(Damage.SetByCaller, -Damage)
 -> ASC::ApplyGameplayEffectSpecToSelf()
 -> UTDEnemySet::Health 변경
 -> ATDEnemyActor::OnHealthAttributeChanged()
 -> Health <= 0 이면 사망 처리

[클라이언트]
MulticastFireProjectile()로 투사체 연출
복제된 Actor / 상태를 보고 화면 갱신
```

타워 스탯 적용은 다음처럼 보면 된다.

```txt
[서버]
타워 생성 또는 업그레이드
 -> TowerData 갱신
 -> ATDTowerBase::SetTowerAttributes()
 -> ASC::MakeOutgoingSpec(DefaultEffect)
 -> Tower.Range.SetByCaller = Range
 -> Tower.FireRate.SetByCaller = FireRate
 -> Tower.Damage.SetByCaller = Damage
 -> ASC::ApplyGameplayEffectSpecToSelf()
 -> UTDTowerSet 값 변경
```

## 6. `TryActivateAbilityByClass()` 정리

현재 프로젝트에서 Tick에서 처리하는 GAS 핵심 함수는 다음이다.

```cpp
AbilitySystemComponent->TryActivateAbilityByClass(DefaultAbility, true);
```

의미:

```txt
ASC 안에 등록된 Ability 중 DefaultAbility 클래스의 Ability를 찾아 실행 시도한다.
```

전제:

```txt
먼저 ASC::GiveAbility()로 해당 Ability가 등록되어 있어야 한다.
```

내부 흐름:

```txt
TryActivateAbilityByClass()
 -> ASC의 ActivatableAbilities 목록에서 해당 Ability 클래스 검색
 -> 찾으면 TryActivateAbility(AbilityHandle) 호출
 -> 쿨다운, 비용, 태그 조건, 네트워크 정책 등을 검사
 -> 조건이 맞으면 Ability 실행
```

현재 프로젝트 의미:

```txt
[서버]
타워가 매 Tick마다 발사 Ability 실행 가능 여부를 확인한다.
가능하면 GA_FireWeapon이 실행되고, 발사 흐름으로 이어진다.
```

## 7. 내일 코드 분석 순서

내일 코드를 볼 때는 다음 순서로 들어가면 된다.

```txt
1. Actor가 ASC와 AttributeSet을 어디서 생성하는지 확인
2. ASC::InitAbilityActorInfo()가 어디서 호출되는지 확인
3. AttributeSet이 ASC에 등록되는지 확인
4. ASC::GiveAbility()로 어떤 Ability가 등록되는지 확인
5. Tick 또는 입력에서 TryActivateAbility 계열 함수가 호출되는지 확인
6. Ability 실행 후 어떤 GameplayEffect를 만드는지 확인
7. AssignTagSetByCallerMagnitude()로 어떤 값을 넣는지 확인
8. ApplyGameplayEffectSpecToSelf() 후 어떤 Attribute가 바뀌는지 확인
9. Attribute 변경 Delegate / OnRep / GameplayCue / GameplayTag 변화가 어떻게 이어지는지 확인
10. 서버에서 바뀐 결과가 클라이언트에 어떻게 보이는지 확인
```

분석할 때 가장 중요한 구분:

```txt
초기화
 -> ASC / AttributeSet / Ability 등록

실행
 -> Ability 또는 게임 로직이 GameplayEffect 생성/적용

결과
 -> Attribute / GameplayTag / GameplayCue 변화

동기화
 -> 서버 결과가 클라이언트 UI와 연출로 반영
```

## 8. 앞으로 GAS 함수 분석 답변 형식

GAS 코드 분석 중 특정 함수를 물어보면, 답변은 아래 형식을 기준으로 작성한다.

```txt
1. 묻는 함수의 핵심 코드를 먼저 제시한다.
   - 실제 프로젝트나 엔진 코드에 주석을 직접 추가하는 것이 아니다.
   - 답변 안에서 코드 블록으로 보여주고, 각 코드 줄 또는 주요 블록마다 주석으로 설명한다.

2. 클라이언트와 서버가 다른 동작을 할 때만 서버/클라이언트 관점을 나누어 설명한다.
   - 동작이 동일하면 굳이 서버/클라이언트로 나누지 않는다.
   - 권한 체크, 복제, Prediction, RPC, 서버 전용 로직이 있으면 구분한다.

3. 흐름은 큰 흐름만 간단히 제시한다.
   - 너무 깊은 엔진 내부까지 한 번에 파고들지 않는다.
   - 필요하면 다음 질문에서 세부 함수로 확장한다.

4. 묻는 함수 자체의 역할을 설명한다.
   - 함수가 언제 호출되는지
   - 어떤 데이터를 준비하거나 변경하는지
   - 어떤 다음 흐름으로 이어지는지 설명한다.

5. 마지막에는 짧게 정리한다.
   - 한 줄 또는 짧은 문단으로 핵심만 요약한다.
```

답변 구조 예시:

```txt
1. 코드
2. 함수 역할
3. 큰 흐름
4. 서버/클라이언트 차이점이 있을 경우에만 분리 설명
5. 정리
```
