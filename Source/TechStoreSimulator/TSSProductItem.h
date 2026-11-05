#pragma once

#include "CoreMinimal.h"
#include "TSSInteractable.h"
#include "TSSProducts.h"
#include "TSSProductItem.generated.h"

class ATSSRegister;
class UStaticMeshComponent;

/**
 * One physical product box a customer has put on the counter. The owner picks it up, scans it, and it goes to
 * the bagging area. `bScanned` is what stops the same box being charged twice.
 */
UCLASS()
class ATSSProductItem : public ATSSInteractable
{
	GENERATED_BODY()

public:
	ATSSProductItem();

	void Init(ETSSProduct InProduct, int32 InPrice, ATSSRegister* InRegister);

	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;

	/** Puts it on the counter at a world position (origin = the surface the box sits on). */
	void PlaceAt(const FVector& SurfacePoint, const FRotator& Rotation);
	/** In the player's hands: invisible and untraceable until it is placed again. */
	void SetCarried(bool bCarried);

	ETSSProduct Product = ETSSProduct::None;
	int32 UnitPrice = 0;
	bool bScanned = false;
	TWeakObjectPtr<ATSSRegister> Register;

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
};
