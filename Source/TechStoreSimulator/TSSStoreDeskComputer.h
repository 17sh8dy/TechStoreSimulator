#pragma once

#include "CoreMinimal.h"
#include "TSSInteractable.h"
#include "TSSStoreDeskComputer.generated.h"

class UStaticMeshComponent;
class UWidgetComponent;

/**
 * The owner's business computer: a cheap old desktop with a CRT, beige tower, keyboard and mouse on a plain desk.
 * Press E on any part of it to open StoreDesk. Local frame: +X is where you stand, Y along the desk, origin on the floor.
 * Swapping to better hardware later means replacing this actor (or adding a tier switch here).
 */
UCLASS()
class ATSSStoreDeskComputer : public ATSSInteractable
{
	GENERATED_BODY()

public:
	ATSSStoreDeskComputer();
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;

private:
	UStaticMeshComponent* Box(const TCHAR* Name, FVector Loc, FVector Size, FLinearColor Color, FRotator Rot = FRotator::ZeroRotator, bool bCollide = true);

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	TArray<FLinearColor> PartColors;
	UPROPERTY() TObjectPtr<UWidgetComponent> Screen;
};
