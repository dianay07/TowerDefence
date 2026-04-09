#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "../TD.h"
#include "TDFL_Utility.generated.h"

class ATDGameMode;
class ATDGameState;
class UTDWaveManagerComponent;
//class UTDTowerManagerComponent;
class UTDEventManagerComponent;
class USoundClass;

class ATDPlayerCharacter;
class UTDGameInstance;
class UTDGameUserSettings;
class ATDEnemyActor;
class ATDTowerPawn;
class UStaticMeshComponent;

static TAutoConsoleVariable<int32> CVarDevMode(
	TEXT("TD.Dev.Mode"),
	1,
	TEXT("Dev Mode")
);

UCLASS()
class TOWERDEFENCE_API UTDFL_Utility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ── DevMode /  ──────────────────────────────────────
	// TDFL_Utility.h - 기존 유틸 파일에 추가
	UFUNCTION(BlueprintPure, Category = "TD|Utility")
	static int32 GetCVarDevMode();

	UFUNCTION(BlueprintCallable, Category = "TD|Utility")
	static void SetCVarDevMode(int32 DevMode);

	// ── GameMode / GameState ──────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "TD|Utility", meta = (WorldContext = "WorldContextObject"))
	static ATDGameMode* GetTDGameMode(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "TD|Utility", meta = (WorldContext = "WorldContextObject"))
	static ATDGameState* GetTDGameState(const UObject* WorldContextObject);

	
	// ── Manager (현재 GameMode에 저장됨) ─────────────────────────
	// TODO: Phase 2+ 에서 GameState로 이전

	UFUNCTION(BlueprintPure, Category = "TD|Utility", meta = (WorldContext = "WorldContextObject"))
	static UTDWaveManagerComponent* GetWaveManager(const UObject* WorldContextObject);

	/*UFUNCTION(BlueprintPure, Category = "TD|Utility", meta = (WorldContext = "WorldContextObject"))
	static UTDTowerManagerComponent* GetTowerManager(const UObject* WorldContextObject);*/

	UFUNCTION(BlueprintPure, Category = "TD|Utility", meta = (WorldContext = "WorldContextObject"))
	static UTDEventManagerComponent* GetEventManager(const UObject* WorldContextObject);
	

	// ── 유틸리티 ─────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "TD|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static ATDPlayerCharacter* GetPlayer(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "TD|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static UTDGameInstance* GetTDGameInstance(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "TD|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static UTDGameUserSettings* GetGameUserSetting(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "TD|Utility")
	static void EnemyDamage(ATDEnemyActor* Enemy, float Damage, TSubclassOf<UGameplayEffect> GE_DamageClass);

	UFUNCTION(BlueprintPure, Category = "TD|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static float GetVolume(const UObject* WorldContextObject, USoundClass* SoundClass);

	UFUNCTION(BlueprintCallable, Category = "TD|Utility")
	static void SetVolume(USoundClass* SoundClass, float Volume);

	UFUNCTION(BlueprintPure, Category = "TD|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static int32 GetVideoQuality(const UObject* WorldContextObject, EQualityOptionType QualityOptionType);

	UFUNCTION(BlueprintCallable, Category = "TD|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static void SetVideoQuality(const UObject* WorldContextObject, EQualityOptionType QualityOptionType, int32 QualityLevel);

	UFUNCTION(BlueprintCallable, Category = "TD|Utility", meta = (WorldContext = "WorldContextObject"))
	static ATDTowerPawn* GetTowerUnderMouse(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "TD|Utility", meta = (WorldContext = "WorldContextObject"))
	static UStaticMeshComponent* GetStaticMeshUnderMouse(const UObject* WorldContextObject);
};
