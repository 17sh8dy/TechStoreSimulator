#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TSSProducts.h"
#include "TSSCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UTSSHumanComponent;
class ATSSProductItem;
class UTSSProductBoxComponent;
class UTSSSettingsSubsystem;
class ATSSInteractable;

/** The shop owner. Switches between a first-person and third-person camera from the View settings. */
UCLASS()
class ATSSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATSSCharacter();

	virtual void Tick(float DeltaSeconds) override;

	UTSSHumanComponent* GetHuman() const { return Human; }
	ETSSProduct GetHeld() const { return Held; }
	/** For tests: true if anything is drawn in the player's hands that should not be (nothing held, but a box shows). */
	bool IsStrayBoxDrawn() const;
	/** The specific customer item in the hands (checkout), if any. Cleared when the hands empty. */
	ATSSProductItem* GetHeldItem() const { return HeldItem; }
	void SetHeldItem(ATSSProductItem* Item) { HeldItem = Item; }
	/** Pass None to empty the hands. */
	void SetHeld(ETSSProduct Product);

	/** What the crosshair is on, as a HUD prompt ("" when nothing). */
	const FString& GetFocusPrompt() const { return FocusPrompt; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void ApplyView();
	void UpdateFocus(class ATSSPlayerController* PC);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY() TObjectPtr<UTSSHumanComponent> Human;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTSSProductBoxComponent> HeldMesh;

	ETSSProduct Held = ETSSProduct::None;
	float GestureTimer = 0.f;
	bool bFirstPersonView = true;
	UPROPERTY() TObjectPtr<ATSSProductItem> HeldItem;
	UPROPERTY() TObjectPtr<ATSSInteractable> Focus;
	FHitResult FocusHit;
	FString FocusPrompt;

	FDelegateHandle SettingsHandle;
	float LookSensitivity = 0.12f;
};