#include "TSSSettings.h"
#include "Kismet/GameplayStatics.h"

void UTSSSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Data = Cast<UTSSSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0));
	if (!Data)
	{
		Data = Cast<UTSSSaveGame>(UGameplayStatics::CreateSaveGameObject(UTSSSaveGame::StaticClass()));
	}
	Data->FirstPersonFOV = FMath::Clamp(Data->FirstPersonFOV, TSSMinFOV, TSSMaxFOV);
	Data->ThirdPersonFOV = FMath::Clamp(Data->ThirdPersonFOV, TSSMinFOV, TSSMaxFOV);
	// Saves from before the setting existed load as 0: treat that as the default.
	if (Data->SensitivityX < TSSMinSensitivity) Data->SensitivityX = TSSDefaultSensitivity;
	if (Data->SensitivityY < TSSMinSensitivity) Data->SensitivityY = TSSDefaultSensitivity;
	Data->SensitivityX = FMath::Clamp(Data->SensitivityX, TSSMinSensitivity, TSSMaxSensitivity);
	Data->SensitivityY = FMath::Clamp(Data->SensitivityY, TSSMinSensitivity, TSSMaxSensitivity);
}

void UTSSSettingsSubsystem::SetSensitivity(bool bVertical, int32 Value, bool bPersist)
{
	Value = FMath::Clamp(Value, TSSMinSensitivity, TSSMaxSensitivity);
	int32& Slot = bVertical ? Data->SensitivityY : Data->SensitivityX;
	if (Slot == Value) return;
	Slot = Value;
	if (bPersist) Save();
	OnChanged.Broadcast();
}

float UTSSSettingsSubsystem::LookScaleFor(int32 Value)
{
	// Geometric steps feel even: 1 is about a third of the default, 10 about three times it.
	return 0.12f * FMath::Pow(1.25f, float(FMath::Clamp(Value, TSSMinSensitivity, TSSMaxSensitivity) - TSSDefaultSensitivity));
}

float UTSSSettingsSubsystem::GetLookScale(bool bVertical) const
{
	return LookScaleFor(GetSensitivity(bVertical));
}

float UTSSSettingsSubsystem::GetFOV(ETSSViewMode Mode) const
{
	return Mode == ETSSViewMode::FirstPerson ? Data->FirstPersonFOV : Data->ThirdPersonFOV;
}

void UTSSSettingsSubsystem::SetViewMode(ETSSViewMode Mode, bool bPersist)
{
	if (Data->ViewMode == Mode) return;
	Data->ViewMode = Mode;
	if (bPersist) Save();
	OnChanged.Broadcast();
}

void UTSSSettingsSubsystem::SetFOV(ETSSViewMode Mode, float FOV)
{
	FOV = FMath::Clamp(FMath::RoundToFloat(FOV), TSSMinFOV, TSSMaxFOV);
	float& Slot = Mode == ETSSViewMode::FirstPerson ? Data->FirstPersonFOV : Data->ThirdPersonFOV;
	if (Slot == FOV) return;
	Slot = FOV;
	Save();
	OnChanged.Broadcast();
}

FKey UTSSSettingsSubsystem::GetSprintKey() const
{
	return (Data->SprintKey.IsValid() && Data->SprintKey != EKeys::AnyKey) ? Data->SprintKey : FKey(FName(TEXT("LeftShift")));
}

void UTSSSettingsSubsystem::SetSprintKey(FKey Key, bool bPersist)
{
	bAwaitingSprint = false;
	if (!Key.IsValid() || Key == EKeys::AnyKey) Key = FKey(FName(TEXT("LeftShift")));
	Data->SprintKey = Key;
	if (bPersist) Save();
	OnChanged.Broadcast();
}

void UTSSSettingsSubsystem::MarkViewChosen()
{
	Data->bHasChosenView = true;
	Save();
}

void UTSSSettingsSubsystem::Save()
{
	UGameplayStatics::SaveGameToSlot(Data, SlotName(), 0);
}