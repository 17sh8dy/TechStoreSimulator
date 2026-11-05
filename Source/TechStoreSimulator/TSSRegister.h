#pragma once

#include "CoreMinimal.h"
#include "TSSInteractable.h"
#include "TSSRegister.generated.h"

class ATSSCustomer;
class UStaticMeshComponent;

/**
 * The checkout counter. Customers queue on its -X side (local); the owner stands on the +X side and presses E.
 * It also holds the shop's entrance geometry so customers know where to come in (move these when the shop
 * moves into the street).
 */
UCLASS()
class ATSSRegister : public ATSSInteractable
{
	GENERATED_BODY()

public:
	ATSSRegister();
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;

	/** Rings up the customer at the front of the line. False if nobody is ready to pay. */
	bool Checkout();

	int32 Join(ATSSCustomer* Customer);
	void Leave(ATSSCustomer* Customer);
	int32 IndexOf(const ATSSCustomer* Customer) const;
	int32 QueueNum() const { return Queue.Num(); }
	ATSSCustomer* Front() const;
	/** World floor position of the i-th place in line. */
	FVector QueueSpot(int32 Index) const;

	/** Where customers appear (outside), the doorway, and a clear point inside the shop they route through (world space, cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customers") FVector SpawnPoint = FVector(1500.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customers") FVector DoorPoint = FVector(760.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customers") FVector HubPoint = FVector(350.f, 0.f, 0.f);

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Counter;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Terminal;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Screen;

	TArray<TWeakObjectPtr<ATSSCustomer>> Queue;
};
