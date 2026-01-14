#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemInterface.h"
#include "TDTowerPawn.generated.h"

UCLASS()
class TOWERDEFENCE_API ATDTowerPawn : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPivateAcess = "true"))
	class UStaticMeshComponent* StaticMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPivateAcess = "true"))
	class UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPivateAcess = "true"))
	TSubclassOf<class UGameplayAbility> DefaultAbility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
	TSubclassOf<class UGameplayEffect> DefaultEffect;

public:
	ATDTowerPawn();

protected:
	virtual void BeginPlay() override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	void InitializeASC();

public:	
	virtual void PostInitializeComponents() override;

	virtual void Tick(float DeltaTime) override;
	
};
