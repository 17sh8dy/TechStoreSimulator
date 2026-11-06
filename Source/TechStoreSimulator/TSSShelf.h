#pragma once

#include "CoreMinimal.h"
#include "TSSInteractable.h"
#include "TSSProducts.h"
#include "TSSBusinessTypes.h"
#include "TSSShelf.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UTSSProductBoxComponent;
class ATSSShelf;

/** One shelf slot somewhere in the world (what a customer wants to buy). */
struct FTSSStockRef
{
	TWeakObjectPtr<ATSSShelf> Shelf;
	int32 Slot = 0;
};

/**
 * A shelving unit: 3 tiers x 4 slots. Each slot holds one kind of product, a few boxes deep, with a live price tag.
 * Its front faces local +X.
 */
UCLASS()
class ATSSShelf : public ATSSInteractable
{
	GENERATED_BODY()

public:
	ATSSShelf();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;
	virtual void Adjust(ATSSCharacter* Player, const FHitResult& Hit, int32 Direction) override;
	virtual bool IsMovable() const override { return true; }
	virtual FString GetMoveName() const override { return TEXT("shelf"); }
	virtual FName LayoutKey() const override;

	/** Every slot that currently has something in it, across all shelves in the world. */
	static void CollectStock(UWorld* World, TArray<FTSSStockRef>& Out);
	ETSSProduct GetSlotProduct(int32 Slot) const { return Slots[Slot].Product; }
	int32 GetSlotQty(int32 Slot) const { return Slots[Slot].Qty; }
	/** The product the back room should refill this slot with (what it holds, or held last); None if it never held anything. */
	ETSSProduct GetSlotWanted(int32 Slot) const { return Slots[Slot].Qty > 0 ? Slots[Slot].Product : Slots[Slot].Last; }
	/** Saving: the contents of every slot, and putting them back. Import clamps to what a slot can hold. */
	void ExportSlots(TArray<FTSSShelfSlotSave>& Out) const;
	void ImportSlots(const TArray<FTSSShelfSlotSave>& In);
	/** Customer-side stock handling (the player goes through Interact). */
	bool TakeOne(int32 Slot, ETSSProduct& OutProduct);
	bool PutBack(int32 Slot, ETSSProduct Product);
	/** Floor spot in front of a slot where a customer stands to look at it. */
	FVector GetStandPoint(int32 Slot) const;
	/** World position of the slot, for facing it. */
	FVector GetSlotWorldPos(int32 Slot) const;

	static constexpr int32 Tiers = 3;
	static constexpr int32 Columns = 4;

private:
	struct FSlot
	{
		ETSSProduct Product = ETSSProduct::None;
		int32 Qty = 0;
		ETSSProduct Last = ETSSProduct::None;   // what this slot held most recently (kept after it sells out)
		TArray<TObjectPtr<UTSSProductBoxComponent>> Boxes;
	};

	int32 SlotFromHit(const FHitResult& Hit) const;
	void RefreshSlot(int32 Index);
	void RefreshTag(int32 Index);
	void RefreshAllTags();
	void NotifyStockChanged();
	/** Local position of the slot centre, on the board surface, at the front edge. */
	FVector SlotOrigin(int32 Index) const;

	TArray<FSlot> Slots;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> FrameParts;
	TArray<FLinearColor> FrameColors;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> TagBoards;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> TagTexts;
	FDelegateHandle StoreHandle;
};
