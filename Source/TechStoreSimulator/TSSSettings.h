#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TSSSettings.generated.h"

UENUM(BlueprintType)
enum class ETSSViewMode : uint8
{
	FirstPerson,
	ThirdPerson
};

constexpr float TSSMinFOV = 60.f;
constexpr float TSSMaxFOV = 120.f;

UCLASS()
class UTSSSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() ETSSViewMode ViewMode = ETSSViewMode::FirstPerson;
	UPROPERTY() float FirstPersonFOV = 90.f;
	UPROPERTY() float ThirdPersonFOV = 90.f;
	UPROPERTY() bool bHasChosenView = false;
};

DECLARE_MULTICAST_DELEGATE(FTSSSettingsChanged);

/** Player settings (the "View" section for now). Persisted to a save slot and applied live. */
UCLASS()
class UTSSSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	ETSSViewMode GetViewMode() const { return Data->ViewMode; }
	float GetFOV(ETSSViewMode Mode) const;
	bool HasChosenView() const { return Data->bHasChosenView; }
	/** FOV of whichever view is currently active. */
	float GetActiveFOV() const { return GetFOV(Data->ViewMode); }

	void SetViewMode(ETSSViewMode Mode);
	void SetFOV(ETSSViewMode Mode, float FOV);
	void MarkViewChosen();

	FTSSSettingsChanged OnChanged;

private:
	void Save();

	UPROPERTY() TObjectPtr<UTSSSaveGame> Data;

	static const TCHAR* SlotName() { return TEXT("TSSSettings"); }
};