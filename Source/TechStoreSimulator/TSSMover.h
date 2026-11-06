#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TSSMover.generated.h"

class ATSSInteractable;
class ATSSCharacter;
class ATSSPlayerController;
class UStaticMeshComponent;

/**
 * Lets the owner pick up and rearrange furniture. Hold E on a movable object (a shelf, the StoreDesk computer) for
 * two seconds; the object then follows where you look on the floor. Mouse wheel or R rotates it, E puts it down
 * (only where it fits), Q cancels and puts it back. Everything attached to the object (a shelf's stocked boxes, the
 * computer's keyboard and mouse) is part of the same actor, so it all comes along.
 * A quick tap on E still does the object's normal action (stock a shelf, open StoreDesk).
 */
UCLASS()
class UTSSMoverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTSSMoverComponent();

	static constexpr float HoldSeconds = 2.0f;   // hold E this long to pick something up
	static constexpr float TapSeconds = 0.3f;    // released sooner than this counts as a normal press

	/** Called every frame by the character. Returns true while an object is being moved (normal interaction is off). */
	bool Update(ATSSPlayerController* PC, ATSSCharacter* Who, ATSSInteractable* Focus, const FHitResult& Hit, float Dt);

	// ---- the pieces, public so the self-test can drive them ----
	void HoldStep(ATSSCharacter* Who, ATSSInteractable* Target, const FHitResult& Hit, float Dt, bool bDown, bool bPressed);
	void BeginMove(ATSSCharacter* Who, ATSSInteractable* Target);
	/** Puts the object being moved at this floor spot and yaw (degrees) and works out whether it is allowed there. */
	void SetPlacement(const FVector& Pos, float YawDeg);
	bool IsPlacementValid() const { return bValid; }
	/** Puts it down for good. False (and keeps moving) if the spot is not allowed. */
	bool Commit();
	void Cancel();
	bool IsMoving() const { return Moving.IsValid(); }
	ATSSInteractable* GetMoving() const;
	float GetHoldFraction() const { return bArmed ? FMath::Clamp(Hold / HoldSeconds, 0.f, 1.f) : 0.f; }
	/** HUD text while holding or moving; empty otherwise. */
	FString GetPrompt() const;
	int32 GetTapCount() const { return Taps; }
	int32 GetMovesStarted() const { return MovesStarted; }
	const FString& GetInvalidReason() const { return InvalidReason; }

	/** Is the object, where it stands right now, allowed to be there? (Fills InvalidReason.) */
	bool CheckPlacement(const AActor* Obj, const ATSSCharacter* Who);

private:
	void UpdateMarker();
	void HideMarker();

	TWeakObjectPtr<ATSSInteractable> Moving;
	TWeakObjectPtr<ATSSCharacter> Mover;
	FTransform Origin;
	float Yaw = 0.f;
	bool bValid = false;
	FString InvalidReason;

	TWeakObjectPtr<ATSSInteractable> HoldTarget;
	float Hold = 0.f;
	bool bArmed = false;
	int32 Taps = 0;
	int32 MovesStarted = 0;

	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Marker;
};
