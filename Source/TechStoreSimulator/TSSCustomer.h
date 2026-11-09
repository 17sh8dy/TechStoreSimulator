#pragma once

#include "CoreMinimal.h"
#include "TSSInteractable.h"
#include "TSSProducts.h"
#include "TSSShelf.h"
#include "TSSCheckoutTypes.h"
#include "TSSBankCard.h"
#include "TSSHumanAnimInstance.h"
#include "TSSCustomer.generated.h"

class ATSSRegister;
class UCapsuleComponent;
class UTSSProductBoxComponent;
class UTextRenderComponent;
class UTSSHumanComponent;

enum class ETSSCustomerState : uint8
{
	Entering,    // walking in and over to the shelf they picked
	Looking,     // at the shelf: reach out, pick a box up, examine it
	Queueing,    // heading for / standing in the checkout line
	AtCounter,   // at the front: putting the basket on the counter, then paying
	Collecting,  // taking their shopping off the counter
	Leaving,     // walking out; destroys itself at the door
};

/**
 * A shopper. Picks a stocked shelf slot, walks over, reaches out and takes a box, looks at it, and either puts it back
 * (too expensive) or takes 1-3 items to the register. At the counter they put the basket down item by item, then pay:
 * by card (a card from the pocket, tapped on the terminal) or cash (notes from the pocket, put on the counter). Then they
 * collect their shopping and leave. Too expensive, out of stock, or kept waiting in line too long = they leave unhappy.
 *
 * What they do is a short "script" of steps (reach a hand somewhere, wait, do something) so each sequence stays readable.
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

	/** Optional, before Begin: the customer appears at From (a pavement end), walks to Approach (just outside the door), and leaves back out along the street to To. Without it they start at the register's spawn point. */
	void SetStreetRoute(const FVector& InApproach, const FVector& From, const FVector& To);

	/** Called by the spawner right after spawning. SpeedScale > 1 hurries walking and browsing (tests). */
	void Begin(ATSSRegister* InRegister, float SpeedScale = 1.f);

	/** At the front of the line and standing at their spot. */
	bool IsReadyToPay() const;

	// ---- the register talks to the customer during a sale -----------------------------------------
	/** The owner has charged them: they get their wallet out and pay (bForceCash after a declined card). */
	void PayNow(bool bForceCash);
	/** Receipt handed over: collect the shopping and leave. */
	void FinishCheckout();
	/** An annoyed remark (wrong change, declined card). */
	void Complain(const FString& Text) { Say(Text, 3.f); }
	void Say(const FString& Text, float Seconds = 2.5f);

	ETSSCustomerState GetState() const { return State; }
	/** For tests: has the customer been seen carrying a shopping bag, and did a packaging panel ever show on it? */
	bool WasBagSeen() const { return bBagSeen; }
	bool WasStrayPanelSeen() const { return bStrayPanelSeen; }
	const TArray<FTSSBasketItem>& GetBasket() const { return Basket; }
	/** Tests: is the debit card showing in the customer's hand, and where is it? */
	bool IsCardShown() const;
	FVector GetCardLocation() const;
	/** True while a scripted gesture sequence is running. */
	bool IsBusyGesturing() const { return Script.Num() > 0; }
	/** Tests: make this customer always pay this way (None = random). */
	ETSSPayMethod ForceMethod = ETSSPayMethod::None;

	/** cm per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WalkSpeed = 140.f;
	/** Seconds in line before they give up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Patience = 45.f;

private:
	/** One step of a gesture script: Enter runs once, then Until runs every tick until it returns true. */
	struct FStep
	{
		TFunction<void()> Enter;
		TFunction<bool(float)> Until;
	};
	using FScript = TArray<FStep>;

	// script building blocks
	FStep Do(TFunction<void()> Action) const;
	FStep Wait(float Seconds) const;
	FStep ReachTo(ETSSHand Hand, TFunction<FVector()> Target) const;
	FStep WaitFor(TFunction<bool()> Condition, float MaxSeconds = 20.f) const;
	void SetScript(FScript NewScript);
	void ReplaceRestOfScript(FScript NewSteps);
	void ClearScript();
	void RunScript(float Dt);
	float BlendSeconds() const { return 0.38f / SpeedScale; }

	// the shopping trip
	void ChooseItem();
	void BeginBrowsing();
	void GrabFirstItem();
	void DecideAfterExamining();
	void FinishBrowsing();
	/** The shop closed while this customer was still on the way in or browsing: put things back and go. */
	void TurnAway();
	void WalkOut(const FString& Reason, bool bUnhappy);
	void StartCheckout();
	void BeginPaymentGestures();
	void ShowHeld(UTSSProductBoxComponent* Mesh, bool bShow);
	void FollowRightHand(UTSSProductBoxComponent* Mesh, float DepthCm);
	void FollowCardInHand();
	/** While the customer types their PIN: the index finger hops from key to key and presses each digit as the terminal registers it. */
	void DrivePinTyping();
	/** Hands the card back, straightens up, relaxes the fingers. */
	void EndCardGestures();

	// where the hands go
	FVector ChestPoint(ETSSHand Hand) const;
	FVector HipPoint() const;

	void StartPath(const TArray<FVector>& Points);
	TArray<FVector> LeavePath() const;
	FVector Approach = FVector::ZeroVector, StreetFrom = FVector::ZeroVector, StreetTo = FVector::ZeroVector;
	bool bStreetRoute = false;
	/** Walks the current path; true once the last point is reached. */
	bool FollowPath(float Dt);
	bool MoveToward(const FVector& Target, float Dt);
	void FaceToward(const FVector& Target, float Dt);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> Capsule;
	UPROPERTY() TObjectPtr<UTSSHumanComponent> Human;
	UPROPERTY() TObjectPtr<UTSSProductBoxComponent> CarriedMesh;   // the box (or bag) in the right hand
	UPROPERTY() TObjectPtr<UTSSProductBoxComponent> HandProp;      // banknotes
	UPROPERTY() TObjectPtr<UTSSCardVisual> CardProp;               // their debit card
	FTSSBankCard PayCard;
UPROPERTY() TObjectPtr<UTextRenderComponent> Bubble;

	UPROPERTY() TObjectPtr<ATSSRegister> Register;
	ETSSCustomerState State = ETSSCustomerState::Entering;

	TArray<FVector> Path;
	int32 PathIndex = 0;

	FScript Script;
	int32 ScriptIndex = 0;
	int32 ScriptEpoch = 0;
	bool bStepEntered = false;
	float StepTime = 0.f;

	FTSSStockRef Wanted;
	ETSSProduct WantedProduct = ETSSProduct::None;
	TArray<FTSSBasketItem> Basket;
	int32 WantCount = 1;
	bool bForceCash = false;
	float CardDipT = 0.f;   // 0..1: the card going down into the chip slot
	/** Carrying something in the right hand: it is held at the chest between tasks. */
	bool bCarrying = false;
	bool bBagSeen = false;        // test latches: the carried plain bag was visible at some frame ...
	bool bStrayPanelSeen = false; // ... and a packaging panel was (wrongly) drawn on it
	bool bRunning = false;
	float CarriedDepth = 6.f;
	/** How much over the going rate this person will pay (a multiple of wholesale). */
	float MaxMarkup = 1.8f;

	float SpeedScale = 1.f;
	float WaitedInLine = 0.f;
	float BubbleTimer = 0.f;
	bool bQueued = false;
	bool bSaidHurry = false;
};
