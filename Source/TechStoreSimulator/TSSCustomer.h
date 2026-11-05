#pragma once

#include "CoreMinimal.h"
#include "TSSInteractable.h"
#include "TSSProducts.h"
#include "TSSShelf.h"
#include "TSSCheckoutTypes.h"
#include "TSSCustomer.generated.h"

class ATSSRegister;
class UCapsuleComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UTSSHumanComponent;

enum class ETSSCustomerState : uint8
{
	Entering,   // walking in and over to the shelf they picked
	Looking,    // stood at the shelf, deciding
	Queueing,   // heading for / standing in the checkout line
	AtCounter,  // at the front: items are on the counter and the sale is being rung up
	Leaving,    // walking out; destroys itself at the door
};

/**
 * A shopper. Picks a stocked shelf slot, looks at it, takes 1-3 items if the price is fair, queues at the register,
 * puts the basket on the counter, and pays (cash or card) once the owner has scanned and charged it.
 * Too expensive, out of stock, or kept waiting in line too long = they leave.
 */
UCLASS()
class ATSSCustomer : public ATSSInteractable
{
	GENERATED_BODY()

public:
	ATSSCustomer();
	virtual void Tick(float DeltaSeconds) override;

	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;

	/** Called by the spawner right after spawning. SpeedScale > 1 hurries walking and browsing (tests). */
	void Begin(ATSSRegister* InRegister, float SpeedScale = 1.f);

	/** At the front of the line and standing at their spot. */
	bool IsReadyToPay() const;

	// ---- the register talks to the customer during a sale -----------------------------------------
	/** The owner has charged them: after a moment they pay (bForceCash after a declined card). */
	void PayNow(bool bForceCash);
	/** Receipt handed over: collect the shopping and leave. */
	void FinishCheckout();
	/** An annoyed remark (wrong change, declined card). */
	void Complain(const FString& Text) { Say(Text, 3.f); }
	void Say(const FString& Text, float Seconds = 2.5f);

	ETSSCustomerState GetState() const { return State; }
	const TArray<FTSSBasketItem>& GetBasket() const { return Basket; }
	/** Tests: make this customer always pay this way (None = random). */
	ETSSPayMethod ForceMethod = ETSSPayMethod::None;

	/** cm per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WalkSpeed = 140.f;
	/** Seconds in line before they give up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Patience = 45.f;

private:
	void StartCheckout();
	void DecidePayment();
	void StartPath(const TArray<FVector>& Points);
	/** Walks the current path; true once the last point is reached. */
	bool FollowPath(float Dt);
	bool MoveToward(const FVector& Target, float Dt);
	void FaceToward(const FVector& Target, float Dt);
	void ChooseItem();
	void DecideToBuy();
	void WalkOut(const FString& Reason, bool bUnhappy);
	void Animate(float Dt, bool bWalking);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> Capsule;
	UPROPERTY() TObjectPtr<UTSSHumanComponent> Human;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> CarriedMesh;
	UPROPERTY() TObjectPtr<UTextRenderComponent> Bubble;

	UPROPERTY() TObjectPtr<ATSSRegister> Register;
	ETSSCustomerState State = ETSSCustomerState::Entering;

	TArray<FVector> Path;
	int32 PathIndex = 0;

	FTSSStockRef Wanted;
	ETSSProduct WantedProduct = ETSSProduct::None;
	TArray<FTSSBasketItem> Basket;
	float PayTimer = 0.f;
	bool bPaying = false;
	bool bForceCash = false;
	/** How much over the going rate this person will pay (a multiple of wholesale). */
	float MaxMarkup = 1.8f;

	float SpeedScale = 1.f;
	float LookTimer = 0.f;
	float WaitedInLine = 0.f;
	float BubbleTimer = 0.f;
	bool bQueued = false;
	bool bSaidHurry = false;
};
