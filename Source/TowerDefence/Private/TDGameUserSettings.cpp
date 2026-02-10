// Fill out your copyright notice in the Description page of Project Settings.


#include "TDGameUserSettings.h"

UTDGameUserSettings::UTDGameUserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTDGameUserSettings::SetMasterVolume(float Volume)
{
	MasterVolume = Volume;
}

void UTDGameUserSettings::SetSoundEffectsVolume(float Volume)
{
	SoundEffectsVolume = Volume;
}

void UTDGameUserSettings::SetMusicVolume(float Volume)
{
	MusicVolume = Volume;
}
