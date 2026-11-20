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
/** Mouse sensitivity steps shown in Settings (5 is the old fixed speed). */
constexpr int32 TSSMinSensitivity = 1;
constexpr int32 TSSMaxSensitivity = 10;
constexpr int32 TSSDefaultSensitivity = 5;

UCLASS()
class UTSSSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() ETSSViewMode ViewMode = ETSSViewMode::FirstPerson;
	UPROPERTY() float FirstPersonFOV = 90.f;
	UPROPERTY() float ThirdPersonFOV = 90.f;
	UPROPERTY() bool bHasChosenView = false;
	UPROPERTY() FKey SprintKey = FKey(FName(TEXT("LeftShift")));
	/** Mouse look sensitivity, 1..10, separately for left/right (X) and up/down (Y). */
	UPROPERTY() int32 SensitivityX = TSSDefaultSensitivity;
	UPROPERTY() int32 SensitivityY = TSSDefaultSensitivity;
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

	/** bPersist = false changes the view for this session only (the self-test uses it so it never rewrites your settings). */
	void SetViewMode(ETSSViewMode Mode, bool bPersist = true);
	void SetFOV(ETSSViewMode Mode, float FOV);
	void MarkViewChosen();

	// ---- controls ----
	FKey GetSprintKey() const;
	/** bPersist = false keeps the change to this session (tests). */
	void SetSprintKey(FKey Key, bool bPersist = true);
	void ResetSprintKey(bool bPersist = true) { SetSprintKey(FKey(FName(TEXT("LeftShift"))), bPersist); }
	/** True while the Settings screen is waiting for the next key press to use as the sprint key. */
	bool IsAwaitingSprintKey() const { return bAwaitingSprint; }
	void BeginAwaitSprintKey() { bAwaitingSprint = true; OnChanged.Broadcast(); }
	void CancelAwaitSprintKey() { bAwaitingSprint = false; OnChanged.Broadcast(); }

	// ---- mouse ----
	/** The 1..10 setting for one axis (bVertical = Y, up/down). */
	int32 GetSensitivity(bool bVertical) const { return bVertical ? Data->SensitivityY : Data->SensitivityX; }
	void SetSensitivity(bool bVertical, int32 Value, bool bPersist = true);
	/** Degrees of turn per unit of mouse movement for that axis (5 = 0.12, the original speed; each step is about x1.25). */
	float GetLookScale(bool bVertical) const;
	static float LookScaleFor(int32 Value);

	FTSSSettingsChanged OnChanged;

private:
	void Save();

	UPROPERTY() TObjectPtr<UTSSSaveGame> Data;
	bool bAwaitingSprint = false;

	static const TCHAR* SlotName() { return TEXT("TSSSettings"); }
};