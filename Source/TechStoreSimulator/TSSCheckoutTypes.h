#pragma once

#include "CoreMinimal.h"
#include "TSSProducts.h"

enum class ETSSPayMethod : uint8 { None, Cash, Card };

/** Where the register is in a sale. */
enum class ETSSCheckoutPhase : uint8
{
	Idle,               // no customer at the counter
	Scanning,           // items are on the counter: pick up, scan, then charge
	WaitingForPayment,  // customer is deciding how to pay
	CashDue,            // cash is on the counter: give change, then confirm
	CardTap,            // customer is tapping their card
	CardReady,          // terminal wants the owner to process it
	CardProcessing,
	CardResult,         // approved / declined on the terminal screen
	Receipt,            // receipt is printed: hand it over
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

/** A finished sale, as recorded in the store's books. */
struct FTSSTransaction
{
	int32 Id = 0;
	int32 Items = 0;
	int32 Subtotal = 0;
	int32 Tax = 0;
	int32 Total = 0;
	ETSSPayMethod Method = ETSSPayMethod::None;
	int32 CashReceived = 0;
	int32 Change = 0;
};

namespace TSSMoney
{
	/** Bills and coins in the drawer, in the order they sit in it. Whole dollars only. */
	constexpr int32 Denoms[] = { 1, 5, 10, 20, 50, 100 };
	constexpr int32 NumDenoms = 6;
	constexpr float TaxRate = 0.08f;
	inline int32 TaxFor(int32 Subtotal) { return FMath::RoundToInt(Subtotal * TaxRate); }
}
