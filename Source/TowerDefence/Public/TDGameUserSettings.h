// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "TDGameUserSettings.generated.h"

/**
 * 
 */
UCLASS(config = GameUserSettings, configdonotcheckdefaults, Blueprintable)
class TOWERDEFENCE_API UTDGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
	
public:
	UTDGameUserSettings(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = Settings)
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintCallable, Category = Settings)
	void SetMasterVolume(float Volume);

	UFUNCTION(BlueprintPure, Category = Settings)
	float GetSoundEffectsVolume() const { return SoundEffectsVolume; }

	UFUNCTION(BlueprintCallable, Category = Settings)
	void SetSoundEffectsVolume(float Volume);

	UFUNCTION(BlueprintPure, Category = Setting)
	float GetMusicVolume() const { return MusicVolume; }

	UFUNCTION(BlueprintCallable, Category = Settings)
	void SetMusicVolume(float Volume);

public:

	UPROPERTY(config)
	float MasterVolume = 1.0f;

	UPROPERTY(config)
	float SoundEffectsVolume = 1.0f;

	UPROPERTY(config)
	float MusicVolume = 1.0f;

};
