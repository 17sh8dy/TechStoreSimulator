#pragma once

#include "CoreMinimal.h"
#include "TSSInteractable.h"
#include "TSSCheckoutTypes.h"
#include "TSSMoney.h"
#include "TSSRegisterDevice.generated.h"

class ATSSRegister;
class UStaticMeshComponent;
class UTextRenderComponent;
class UWidgetComponent;

/**
 * One piece of checkout hardware. Devices sit on a register as child actors (all positioned in the register's own
 * frame: +X owner side, Y along the counter, Z up). A device handles its own look, animation and interaction, and
 * asks the register to change the sale; it never edits the transaction itself.
 */
UCLASS(Abstract)
class ATSSRegisterDevice : public ATSSInteractable
{
	GENERATED_BODY()

public:
	ATSSRegisterDevice();
	virtual void OnConstruction(const FTransform& Transform) override;

	ATSSRegister* GetRegister() const;

protected:
	/** A painted box (cm, in the register's frame, or in Parent's frame). Call from constructors only. */
	UStaticMeshComponent* Box(const TCHAR* Name, FVector Loc, FVector Size, FLinearColor Color, FRotator Rot = FRotator::ZeroRotator, USceneComponent* Parent = nullptr);
	UTextRenderComponent* Label(const TCHAR* Name, FVector Loc, FRotator Rot, float WorldSize, FColor Color, USceneComponent* Parent = nullptr);

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	TArray<FLinearColor> PartColors;
};

/** The register screen: product list, subtotal, tax, total and payment status. Display only. */
UCLASS()
class ATSSRegisterMonitor : public ATSSRegisterDevice
{
	GENERATED_BODY()
public:
	ATSSRegisterMonitor();
	virtual void BeginPlay() override;
	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override { return FString(); }

private:
	UPROPERTY() TObjectPtr<UWidgetComponent> Screen;
};

/** The keyboard: "Charge" once everything is scanned, and "Confirm change" when paying cash. */
UCLASS()
class ATSSRegisterKeyboard : public ATSSRegisterDevice
{
	GENERATED_BODY()
public:
	ATSSRegisterKeyboard();
	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;
};

/** The barcode scanner: scans the item in your hands (each box only once). */
UCLASS()
class ATSSScanner : public ATSSRegisterDevice
{
	GENERATED_BODY()
public:
	ATSSScanner();
	virtual void Tick(float DeltaSeconds) override;
	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;
	/** Flash the glass green (good scan) or red (rejected). */
	void Beep(bool bOk);

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Glass;
	float FlashTimer = 0.f;
	bool bFlashOk = true;
	bool bGlassIsIdle = true;
};

/**
 * The cash drawer: slides open when cash is paid. It holds real US notes and coins in eleven slots (six notes, five coins);
 * click a slot to put that piece into the change. The customer's payment goes into the drawer when the sale completes, and
 * the shelf-stocker tops it back up to its float at the start of every day.
 */
UCLASS()
class ATSSCashDrawer : public ATSSRegisterDevice
{
	GENERATED_BODY()
public:
	ATSSCashDrawer();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;
	virtual void Adjust(ATSSCharacter* Player, const FHitResult& Hit, int32 Direction) override;

	void SetOpen(bool bOpen) { bWantOpen = bOpen; }
	bool IsOpen() const { return OpenAlpha > 0.5f; }

	const TSSMoney::FStock& GetStock() const { return Stock; }
	int32 GetCashOnHandCents() const { return Stock.TotalCents(); }
	/** Takes one piece (a TSSMoney table index) out for change. False if the slot is empty. */
	bool TakePiece(int32 Piece);
	/** Puts a piece back (change that was taken but not given). */
	void ReturnPiece(int32 Piece);
	/** The customer's payment goes into the slots. */
	void Receive(const TArray<int32>& Pieces);
	/** Tops every slot up to the starting float (a trip to the bank, every morning). */
	void RefillFloat();
	/** Which slot (a TSSMoney table index) a trace hit lands on. */
	int32 SlotAt(const FHitResult& Hit) const;
	/** Test hook: add one piece to the change as a click on its slot would. */
	void ClickPiece(int32 Piece);
	/** Test hook: set exactly what the drawer holds. */
	void SetStockForTest(const TSSMoney::FStock& S) { Stock = S; Refresh(); }

private:
	void Refresh();

	UPROPERTY() TObjectPtr<USceneComponent> Tray;
	UPROPERTY() TObjectPtr<UTextRenderComponent> CashLabel;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Tops;      // what shows in each slot
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> SlotLabels;
	TSSMoney::FStock Stock;
	bool bWantOpen = false;
	float OpenAlpha = 0.f;
};

/** The card terminal: shows the amount, takes the customer's card, and processes the payment when you press it. */
UCLASS()
class ATSSCardTerminal : public ATSSRegisterDevice
{
	GENERATED_BODY()
public:
	ATSSCardTerminal();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;

	/** The customer's card flies in to tap the terminal / is taken away again. */
	void SetCardPresent(bool bPresent) { bCardWanted = bPresent; }
	/** World position the customer taps their card on. */
	FVector GetTapPoint() const;

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Card;
	UPROPERTY() TObjectPtr<UWidgetComponent> Screen;
	bool bCardWanted = false;
	float CardAlpha = 0.f;
};

/** The receipt printer: prints the receipt after a sale; handing it over lets the customer go. */
UCLASS()
class ATSSReceiptPrinter : public ATSSRegisterDevice
{
	GENERATED_BODY()
public:
	ATSSReceiptPrinter();
	virtual void Tick(float DeltaSeconds) override;
	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;

	/** Prints the receipt for a finished sale (its lines are the register screen's lines). */
	void Print(const FTSSTransaction& Sale, const TArray<FTSSSaleLine>& Lines);
	void Tear();
	bool HasReceipt() const { return bPrinting || PrintAlpha > 0.f; }

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Paper;
	UPROPERTY() TObjectPtr<UTextRenderComponent> PaperText;
	bool bPrinting = false;
	float PrintAlpha = 0.f;
};
