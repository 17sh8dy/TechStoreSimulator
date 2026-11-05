#pragma once

#include "CoreMinimal.h"
#include "TSSInteractable.h"
#include "TSSCheckoutTypes.h"
#include "TSSRegister.generated.h"

class ATSSCustomer;
class ATSSProductItem;
class ATSSScanner;
class ATSSCashDrawer;
class ATSSCardTerminal;
class ATSSReceiptPrinter;
class ATSSRegisterMonitor;
class ATSSRegisterKeyboard;
class UChildActorComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * The checkout counter and the brain of a sale. The physical devices (monitor, keyboard, scanner, cash drawer,
 * card terminal, receipt printer) are separate actors attached as child actors; each handles its own
 * interaction and asks this register to do the thing. The register owns the transaction.
 *
 * Local frame: +X is the owner's side, -X is where customers stand, Y runs along the counter, Z up; origin on the floor.
 *
 * A sale: customer reaches the front of the line and puts their items on the counter -> the owner picks each up
 * and scans it -> keyboard "Charge" -> the customer pays cash (owner gives change from the drawer) or card
 * (owner processes it on the terminal) -> the printer prints a receipt -> the owner hands it over -> the customer leaves.
 */
UCLASS()
class ATSSRegister : public ATSSInteractable
{
	GENERATED_BODY()

public:
	ATSSRegister();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	// The counter surface itself: put a picked-up item back down.
	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;

	// ---- the line of customers --------------------------------------------------------------------
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
	/** Chance a card payment is declined (the customer then pays cash). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkout") float CardDeclineChance = 0.1f;

	// ---- the customer starts / drives a sale ------------------------------------------------------
	/** The customer at the front of the line puts their basket on the counter. */
	void BeginTransaction(ATSSCustomer* Customer, const TArray<FTSSBasketItem>& Basket);
	/** The customer hands over `Amount` in cash. */
	void CustomerPaysCash(int32 Amount);
	void CustomerTapsCard();

	// ---- owner actions (called by the devices) ----------------------------------------------------
	bool ScanItem(ATSSProductItem* Item, FString& Why);
	bool TryScanHeld(ATSSCharacter* Player);
	FString ScannerPrompt(const ATSSCharacter* Player) const;
	FString KeyboardPrompt() const;
	void KeyboardPress();
	void AddChange(int32 Denom);
	void RemoveLastChange();
	FString DrawerPrompt() const;
	void ProcessCard();
	FString TerminalPrompt() const;
	void TakeReceipt();
	FString PrinterPrompt() const;

	// ---- read by the screens and tests ------------------------------------------------------------
	ETSSCheckoutPhase GetPhase() const { return Phase; }
	ETSSPayMethod GetMethod() const { return Method; }
	const TArray<FTSSSaleLine>& GetLines() const { return Lines; }
	int32 GetSubtotal() const { return Subtotal; }
	int32 GetTax() const { return Tax; }
	int32 GetTotal() const { return Total; }
	int32 GetCashReceived() const { return CashReceived; }
	int32 GetChangeDue() const { return ChangeDue; }
	int32 GetChangeGiven() const;
	int32 GetLastTotal() const { return LastTotal; }
	const FString& GetNotice() const { return Notice; }
	int32 ItemsLeftToScan() const;
	const TArray<TObjectPtr<ATSSProductItem>>& GetItems() const { return Items; }
	ATSSCustomer* GetCustomer() const { return Customer; }
	void SetNotice(const FString& Text, float Seconds = 2.5f);
	FString TerminalStatus() const;
	FLinearColor TerminalColor() const;
	bool WasCardApproved() const { return bCardApproved; }

	ATSSScanner* GetScanner() const;
	ATSSCashDrawer* GetDrawer() const;
	ATSSCardTerminal* GetTerminal() const;
	ATSSReceiptPrinter* GetPrinter() const;
	ATSSRegisterKeyboard* GetKeyboard() const;

	/** Top of the counter, in the register's local Z. */
	static constexpr float TopZ = 98.f;

private:
	FVector StagingSlot(int32 Index) const;
	FVector BaggingSlot(int32 Index) const;
	void RecalculateTotals();
	void ConfirmChange();
	void Complete(ETSSPayMethod PaidBy);
	void Reset();

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	TArray<FLinearColor> PartColors;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Cabinet;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CashPile;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> CashText;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UChildActorComponent> MonitorComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UChildActorComponent> KeyboardComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UChildActorComponent> ScannerComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UChildActorComponent> DrawerComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UChildActorComponent> TerminalComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UChildActorComponent> PrinterComp;

	TArray<TWeakObjectPtr<ATSSCustomer>> Queue;

	// The current sale.
	ETSSCheckoutPhase Phase = ETSSCheckoutPhase::Idle;
	ETSSPayMethod Method = ETSSPayMethod::None;
	UPROPERTY() TObjectPtr<ATSSCustomer> Customer;
	UPROPERTY() TArray<TObjectPtr<ATSSProductItem>> Items;
	TArray<FTSSSaleLine> Lines;
	TArray<int32> ChangeBills;
	int32 Subtotal = 0, Tax = 0, Total = 0;
	int32 CashReceived = 0, ChangeDue = 0, LastTotal = 0;
	int32 ScannedCount = 0;
	bool bCardApproved = false;
	float PhaseTimer = 0.f;
	FString Notice;
	float NoticeTimer = 0.f;
};
