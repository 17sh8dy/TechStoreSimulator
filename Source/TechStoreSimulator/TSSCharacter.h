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
class UTSSMoverComponent;

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
	/** Walking and sprinting speeds (cm/s). */
	static float SpeedFor(bool bSprint) { return bSprint ? 400.f : 210.f; }
	/** The specific customer item in the hands (checkout), if any. Cleared when the hands empty. */
	ATSSProductItem* GetHeldItem() const { return HeldItem; }
	void SetHeldItem(ATSSProductItem* Item) { HeldItem = Item; }
	/** Pass None to empty the hands. */
	void SetHeld(ETSSProduct Product);

	/** An emptied delivery box carried to a bin: the hands are full of it until it is thrown away. */
	bool IsCarryingEmptyBox() const { return bEmptyBox; }
	void SetCarryingEmptyBox(bool bCarry);

	/** Presses E on something: its action plus the little reach gesture. */
	void InteractWith(ATSSInteractable* Target, const FHitResult& Hit);
	UTSSMoverComponent* GetMover() const { return Mover; }

	// ---- getting hit by a car ----
	/** A moving car ran into you: you are thrown along PushDir (horizontal), fall over, lie there a moment and get back up. */
	void HitByCar(const FVector& PushDir, float CarSpeed);
	/** False while down, getting up, or just after (so one car does not hit you over and over). */
	bool CanBeHitByCar() const { return KnockTime < 0.f && HitCooldown <= 0.f; }
	bool IsKnockedDown() const { return KnockTime >= 0.f; }
	/** Tests: skip the rest of the fall and stand up now. */
	void RecoverNow();
	/** How long a knock-down lasts, falling to standing (seconds). */
	static constexpr float KnockDuration = 3.2f;

	/** What the crosshair is on, as a HUD prompt ("" when nothing). */
	const FString& GetFocusPrompt() const { return FocusPrompt; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void ApplyView();
	void UpdateFocus(class ATSSPlayerController* PC);
	/** Poses the body / first-person camera for the knock-down; returns false once it is over. */
	bool TickKnockDown(float DeltaSeconds);
	void ApplyKnockPose(float FallAlpha, float Bounce);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY() TObjectPtr<UTSSHumanComponent> Human;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTSSProductBoxComponent> HeldMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTSSMoverComponent> Mover;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> EmptyBoxMesh;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> EmptyBoxParts;
	bool bEmptyBox = false;

	ETSSProduct Held = ETSSProduct::None;
	float GestureTimer = 0.f;
	/** Seconds the game has been in control since the last menu / StoreDesk (a click that closed one must not also act in the shop). */
	float ActiveFor = 0.f;
bool bFirstPersonView = true;
	UPROPERTY() TObjectPtr<ATSSProductItem> HeldItem;
	UPROPERTY() TObjectPtr<ATSSInteractable> Focus;
	FHitResult FocusHit;
	FString FocusPrompt;

	FDelegateHandle SettingsHandle;

	/** Seconds into the current knock-down, or -1 when standing. */
	float KnockTime = -1.f;
	float HitCooldown = 0.f;
	/** Which way the body falls, in the capsule's own frame (horizontal, unit). */
	FVector KnockDirLocal = FVector::ForwardVector;
};