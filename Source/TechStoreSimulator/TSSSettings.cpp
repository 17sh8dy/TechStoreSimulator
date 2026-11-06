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
	return Data->SprintKey.IsValid() ? Data->SprintKey : FKey(FName(TEXT("LeftShift")));
}

void UTSSSettingsSubsystem::SetSprintKey(FKey Key, bool bPersist)
{
	bAwaitingSprint = false;
	if (!Key.IsValid()) Key = FKey(FName(TEXT("LeftShift")));
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