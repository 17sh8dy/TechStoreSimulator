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
};
