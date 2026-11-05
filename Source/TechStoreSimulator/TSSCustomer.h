#pragma once

#include "CoreMinimal.h"
#include "TSSInteractable.h"
#include "TSSProducts.h"
#include "TSSShelf.h"
#include "TSSCustomer.generated.h"

class ATSSRegister;
class UCapsuleComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

enum class ETSSCustomerState : uint8
{
	Entering,   // walking in and over to the shelf they picked
	Looking,    // stood at the shelf, deciding
	Queueing,   // heading for / standing in the checkout line
	Leaving,    // walking out; destroys itself at the door
};

/**
 * A shopper. Picks a stocked shelf slot, looks at it, buys one if the price is fair, queues at the register
 * and waits for the owner to ring them up. Too expensive, out of stock, or kept waiting too long = they leave.
 * Looking at them and pressing E rings them up too.
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

	bool IsReadyToPay() const;
	/** Called by the register: pays the agreed price and heads out. */
	void CompleteSale();

	ETSSCustomerState GetState() const { return State; }
	ETSSProduct GetCarried() const { return Carried; }
	int32 GetAgreedPrice() const { return AgreedPrice; }

	/** cm per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WalkSpeed = 140.f;
	/** Seconds in line before they give up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Patience = 45.f;

private:
	void Say(const FString& Text, float Seconds = 2.5f);
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
	UPROPERTY() TObjectPtr<USceneComponent> Visual;
	UPROPERTY() TObjectPtr<USceneComponent> HipL;
	UPROPERTY() TObjectPtr<USceneComponent> HipR;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Torso;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Head;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LegL;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LegR;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> CarriedMesh;
	UPROPERTY() TObjectPtr<UTextRenderComponent> Bubble;

	UPROPERTY() TObjectPtr<ATSSRegister> Register;
	ETSSCustomerState State = ETSSCustomerState::Entering;

	TArray<FVector> Path;
	int32 PathIndex = 0;

	FTSSStockRef Wanted;
	ETSSProduct WantedProduct = ETSSProduct::None;
	ETSSProduct Carried = ETSSProduct::None;
	int32 AgreedPrice = 0;
	/** How much over the going rate this person will pay (a multiple of wholesale). */
	float MaxMarkup = 1.8f;

	float SpeedScale = 1.f;
	float LookTimer = 0.f;
	float WaitedInLine = 0.f;
	float BubbleTimer = 0.f;
	float WalkPhase = 0.f;
	bool bQueued = false;
	bool bSaidHurry = false;
};
