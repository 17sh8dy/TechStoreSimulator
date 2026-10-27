#pragma once

#include "CoreMinimal.h"
#include "TSSInteractable.h"
#include "TSSProducts.h"
#include "TSSShelf.generated.h"

class UStaticMeshComponent;

/**
 * A shelving unit: 3 tiers x 4 slots. Each slot holds one kind of product, a few boxes deep.
 * Its front faces local +X.
 */
UCLASS()
class ATSSShelf : public ATSSInteractable
{
	GENERATED_BODY()

public:
	ATSSShelf();
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;
	virtual void Adjust(ATSSCharacter* Player, const FHitResult& Hit, int32 Direction) override;

	static constexpr int32 Tiers = 3;
	static constexpr int32 Columns = 4;

private:
	struct FSlot
	{
		ETSSProduct Product = ETSSProduct::None;
		int32 Qty = 0;
		TArray<TObjectPtr<UStaticMeshComponent>> Boxes;
	};

	int32 SlotFromHit(const FHitResult& Hit) const;
	void RefreshSlot(int32 Index);
	/** Local position of the slot centre, on the board surface, at the front edge. */
	FVector SlotOrigin(int32 Index) const;

	TArray<FSlot> Slots;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> FrameParts;
};
