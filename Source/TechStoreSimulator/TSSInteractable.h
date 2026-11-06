#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TSSInteractable.generated.h"

class ATSSCharacter;

/** Anything the shop owner can look at and press E on. */
UCLASS(Abstract)
class ATSSInteractable : public AActor
{
	GENERATED_BODY()

public:
	/** What the HUD says while this is under the crosshair. Empty hides the prompt. */
	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const { return FString(); }
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) {}
	/** Mouse wheel while looking at this: +1 / -1. */
	virtual void Adjust(ATSSCharacter* Player, const FHitResult& Hit, int32 Direction) {}

	/** True if the owner can pick this up and rearrange it (hold E for two seconds). */
	virtual bool IsMovable() const { return false; }
	/** What to call it in the move prompt. */
	virtual FString GetMoveName() const { return TEXT("object"); }
	/** A stable name for saving where the owner put it. */
	virtual FName LayoutKey() const { return GetFName(); }
	/** Called after the owner has put it down in a new place. */
	virtual void OnMoved() {}
};
