#pragma once

#include "CoreMinimal.h"
#include "TSSProducts.h"
#include "TSSMoney.h"
#include "TSSCheckoutTypes.generated.h"

UENUM()
enum class ETSSPayMethod : uint8 { None, Cash, Card };

/** Where the register is in a sale. */
enum class ETSSCheckoutPhase : uint8
{
	Idle,               // no customer at the counter
	Scanning,           // items are on the counter: pick up, scan, then charge
	WaitingForPayment,  // customer is deciding how to pay
	CashDue,            // cash is on the counter: give change, then confirm
	CardTap,            // customer is tapping (or inserting) their card
	CardPin,            // chip card inserted: the customer is typing their PIN
	CardReady,          // terminal wants the owner to process it
	CardProcessing,
	CardResult,         // approved / declined on the terminal screen
	Receipt,            // receipt is printed: hand it over
	Collecting,         // the customer is taking their shopping off the counter
};

/** What a customer is buying: product and the shelf price they agreed to. */
struct FTSSBasketItem
{
	ETSSProduct Product = ETSSProduct::None;
	int32 Price = 0;
	/** Where it came from, so an abandoned basket can be put back. */
	int32 ShelfSlot = 0;
};

/** One line on the register screen (same product + price are merged). */
struct FTSSSaleLine
{
	ETSSProduct Product = ETSSProduct::None;
	int32 Qty = 0;
	int32 UnitPrice = 0;
	int32 Total() const { return Qty * UnitPrice; }
};

/** A finished sale, as recorded in the books (and saved). Every amount is in cents. */
USTRUCT()
struct FTSSTransaction
{
	GENERATED_BODY()

	UPROPERTY() int32 Id = 0;
	UPROPERTY() int32 Items = 0;
	UPROPERTY() int32 Subtotal = 0;
	UPROPERTY() int32 Tax = 0;
	UPROPERTY() int32 Total = 0;
	UPROPERTY() ETSSPayMethod Method = ETSSPayMethod::None;
	UPROPERTY() int32 CashReceived = 0;
	UPROPERTY() int32 Change = 0;
	UPROPERTY() int32 Day = 1;
	/** Card sales: the network, last four digits and authorisation code printed on the receipt. */
	UPROPERTY() FString CardNetwork;
	UPROPERTY() FString CardLast4;
	UPROPERTY() FString AuthCode;
	/** Minutes since midnight when it was rung up (for the receipt). */
	UPROPERTY() int32 TimeMinutes = 0;
};
