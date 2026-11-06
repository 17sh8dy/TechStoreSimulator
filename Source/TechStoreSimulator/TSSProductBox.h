#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "TSSProducts.h"
#include "TSSProductBox.generated.h"

/**
 * A retail box for one product: a body in the brand colour with the packaging art on its front (+X) face.
 * Used by shelves, checkout items, hands and baskets so a product looks the same everywhere.
 * The front panel is a child component created on registration; materials are shared per product.
 */
UCLASS()
class UTSSProductBoxComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UTSSProductBoxComponent();

	/** Shows this product's box (sets the mesh scale to the box size). */
	void SetProduct(ETSSProduct InProduct);
	/** A plain coloured block with no packaging (e.g. a shopping bag). */
	void SetPlain(const FVector& SizeCm, const FLinearColor& Color);
	ETSSProduct GetProduct() const { return Product; }

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

private:
	void Apply();

	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Panel;
	ETSSProduct Product = ETSSProduct::None;
	FVector PlainSize = FVector::ZeroVector;
	FLinearColor PlainColor = FLinearColor::White;
};
