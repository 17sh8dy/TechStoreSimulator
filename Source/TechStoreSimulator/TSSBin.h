#pragma once

#include "CoreMinimal.h"
#include "TSSInteractable.h"
#include "TSSBin.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * A street litter bin. Empty delivery boxes are carried here (E) instead of vanishing. For now it never fills up: it only
 * counts what went in. The lid flips open while a box goes in.
 *
 * Built from plain engine shapes (plinth, body, steel bands, ribs, rim, a hinged lid with a slot and handle, a sign plate).
 */
UCLASS()
class ATSSBin : public ATSSInteractable
{
	GENERATED_BODY()

public:
	ATSSBin();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;

	int32 GetBoxCount() const { return Boxes; }
	/** 0 closed .. 1 fully open (for tests / screenshots). */
	float GetLidOpen() const { return LidOpen; }

private:
	UStaticMeshComponent* Part(const TCHAR* Name, USceneComponent* Parent, class UStaticMesh* Mesh, FVector Loc, FVector SizeCm, FRotator Rot, const FLinearColor& Color, bool bCollide = false);

	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY() TObjectPtr<USceneComponent> LidPivot;
	UPROPERTY() TObjectPtr<UTextRenderComponent> Plate;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	TArray<FLinearColor> PartColors;
	int32 Boxes = 0;
	float OpenTimer = 0.f;
	float LidOpen = 0.f;
};
