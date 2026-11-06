#pragma once

#include "CoreMinimal.h"
#include "TSSInteractable.h"
#include "TSSProducts.h"
#include "TSSCrate.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/** Wholesale stock: buy one box at a time and carry it to a shelf. E with the same box in hand returns it. */
UCLASS()
class ATSSCrate : public ATSSInteractable
{
	GENERATED_BODY()

public:
	ATSSCrate();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Re-applies colour and label for the current Product (callable from editor scripts). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Tech Store") void Rebuild();

	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) ETSSProduct Product = ETSSProduct::Phone;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
};
