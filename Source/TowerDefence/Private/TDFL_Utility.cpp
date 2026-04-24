#include "TDFL_Utility.h"
#include "TDGameState.h"
#include "TDGameUserSettings.h"
#include "TDEnemyActor.h"
#include "TDTowerPawn.h"
#include "TDPlayerCharacter.h"
#include "TDGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

#include "TDGameMode.h"
#include "TDWaveManagerComponent.h"
#include "GameData/TDEnemyDataTableSubsystem.h"
#include "Server/TDPoolComponent.h"
#include "Player/TDPlayerController.h"

int32 UTDFL_Utility::GetCVarDevMode()
{
	return CVarDevMode.GetValueOnGameThread();
}

void UTDFL_Utility::SetCVarDevMode(int32 DevMode)
{
	CVarDevMode->Set(DevMode);
}

ATDGameMode* UTDFL_Utility::GetTDGameMode(const UObject* WorldContextObject)
{
	return Cast<ATDGameMode>(UGameplayStatics::GetGameMode(WorldContextObject));
}

ATDGameState* UTDFL_Utility::GetTDGameState(const UObject* WorldContextObject)
{
	return Cast<ATDGameState>(UGameplayStatics::GetGameState(WorldContextObject));
}


UTDWaveManagerComponent* UTDFL_Utility::GetWaveManager(const UObject* WorldContextObject)
{
	 ATDGameMode* GM = GetTDGameMode(WorldContextObject);
	 
	 if (GM)
		 return GM->WaveManager;
	 else
		 return nullptr;
}

UTDEnemyDataTableSubsystem* UTDFL_Utility::GetEnemyDataTable(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;
	UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (!GI) return nullptr;
	return GI->GetSubsystem<UTDEnemyDataTableSubsystem>();
}

UTDPoolComponent* UTDFL_Utility::GetPool(const UObject* WorldContextObject)
{
	ATDGameMode* GM = GetTDGameMode(WorldContextObject);
	return GM ? GM->Pool : nullptr;
}

ATDPlayerController* UTDFL_Utility::GetTDPlayerController(const UObject* WorldContextObject)
{
	return Cast<ATDPlayerController>(UGameplayStatics::GetPlayerController(WorldContextObject, 0));
}


ATDPlayerCharacter* UTDFL_Utility::GetPlayer(const UObject* WorldContextObject)
{
	return Cast<ATDPlayerCharacter>(UGameplayStatics::GetPlayerPawn(WorldContextObject, 0));
}

UTDGameInstance* UTDFL_Utility::GetTDGameInstance(const UObject* WorldContextObject)
{
	return Cast<UTDGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject));
}

UTDGameUserSettings* UTDFL_Utility::GetGameUserSetting(const UObject* WorldContextObject)
{
	return Cast<UTDGameUserSettings>(GEngine->GetGameUserSettings());
}

void UTDFL_Utility::EnemyDamage(ATDEnemyActor* Enemy, float Damage, TSubclassOf<UGameplayEffect> GE_DamageClass)
{
	if (!Enemy || !GE_DamageClass) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy);
	if (!TargetASC) return;

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(GE_DamageClass, 0.0f, TargetASC->MakeEffectContext());
	if (!SpecHandle.IsValid()) return;

	FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Damage.SetByCaller"));
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DamageTag, -Damage);
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

float UTDFL_Utility::GetVolume(const UObject* WorldContextObject, USoundClass* SoundClass)
{
	UTDGameUserSettings* UserSettings = GetGameUserSetting(WorldContextObject);
	if (!UserSettings) return 0.f;

	// TODO: SoundClass 비교 후 볼륨 반환하도록 구현, BP에선 SoundClass의 이름으로 구분
	/*if (SoundClass == MasterSoundClass)
		return UserSettings->GetMasterVolume();
	else if (SoundClass == SFXSoundClass)
		return UserSettings->GetSoundEffectsVolume();
	else if (SoundClass == MusicSoundClass)
		return UserSettings->GetMusicVolume();*/

	return 0.f;
}

void UTDFL_Utility::SetVolume(USoundClass* SoundClass, float Volume)
{
	// TODO: SoundClass 비교 후 볼륨 설정하도록 구현, BP에선 SoundClass의 이름으로 구분
}

int32 UTDFL_Utility::GetVideoQuality(const UObject* WorldContextObject, EQualityOptionType QualityOptionType)
{
	UTDGameUserSettings* UserSettings = GetGameUserSetting(WorldContextObject);
	if (!UserSettings) return 0;

	switch (QualityOptionType)
	{
	case EQualityOptionType::Overall:        return UserSettings->GetOverallScalabilityLevel();
	case EQualityOptionType::ViewDistance:   return UserSettings->GetViewDistanceQuality();
	case EQualityOptionType::Shadow:         return UserSettings->GetShadowQuality();
	case EQualityOptionType::GlobalIllumination: return UserSettings->GetGlobalIlluminationQuality();
	case EQualityOptionType::Reflections:    return UserSettings->GetReflectionQuality();
	case EQualityOptionType::AntiAliasing:   return UserSettings->GetAntiAliasingQuality();
	case EQualityOptionType::Texture:        return UserSettings->GetTextureQuality();
	case EQualityOptionType::VisualEffect:   return UserSettings->GetVisualEffectQuality();
	case EQualityOptionType::PostProcessing: return UserSettings->GetPostProcessingQuality();
	case EQualityOptionType::Foliage:        return UserSettings->GetFoliageQuality();
	case EQualityOptionType::Shading:        return UserSettings->GetShadingQuality();
	default:                                 return 0;
	}
}

void UTDFL_Utility::SetVideoQuality(const UObject* WorldContextObject, EQualityOptionType QualityOptionType, int32 QualityLevel)
{
	UTDGameUserSettings* UserSettings = GetGameUserSetting(WorldContextObject);
	if (!UserSettings) return;

	switch (QualityOptionType)
	{
	case EQualityOptionType::Overall:        UserSettings->SetOverallScalabilityLevel(QualityLevel); break;
	case EQualityOptionType::ViewDistance:   UserSettings->SetViewDistanceQuality(QualityLevel);     break;
	case EQualityOptionType::Shadow:         UserSettings->SetShadowQuality(QualityLevel);           break;
	case EQualityOptionType::GlobalIllumination: UserSettings->SetGlobalIlluminationQuality(QualityLevel); break;
	case EQualityOptionType::Reflections:    UserSettings->SetReflectionQuality(QualityLevel);       break;
	case EQualityOptionType::AntiAliasing:   UserSettings->SetAntiAliasingQuality(QualityLevel);     break;
	case EQualityOptionType::Texture:        UserSettings->SetTextureQuality(QualityLevel);          break;
	case EQualityOptionType::VisualEffect:   UserSettings->SetVisualEffectQuality(QualityLevel);     break;
	case EQualityOptionType::PostProcessing: UserSettings->SetPostProcessingQuality(QualityLevel);   break;
	case EQualityOptionType::Foliage:        UserSettings->SetFoliageQuality(QualityLevel);          break;
	case EQualityOptionType::Shading:        UserSettings->SetShadingQuality(QualityLevel);          break;
	default: break;
	}

	UserSettings->ApplySettings(true);
}

ATDTowerPawn* UTDFL_Utility::GetTowerUnderMouse(const UObject* WorldContextObject)
{
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel1)); // Tower 채널

	FHitResult HitResult;
	APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (PC && PC->GetHitResultUnderCursorForObjects(ObjectTypes, true, HitResult))
	{
		return Cast<ATDTowerPawn>(HitResult.GetActor());
	}
	return nullptr;
}

UStaticMeshComponent* UTDFL_Utility::GetStaticMeshUnderMouse(const UObject* WorldContextObject)
{
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

	FHitResult HitResult;
	APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (PC && PC->GetHitResultUnderCursorForObjects(ObjectTypes, true, HitResult))
	{
		return Cast<UStaticMeshComponent>(HitResult.GetComponent());
	}
	return nullptr;
}
