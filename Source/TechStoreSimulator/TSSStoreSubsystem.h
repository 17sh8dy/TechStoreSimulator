#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TSSProducts.h"
#include "TSSCheckoutTypes.h"
#include "TSSStoreSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FTSSStoreChanged);

/** The shop's books: cash and the shelf price of each product. One per world. */
UCLASS()
class UTSSStoreSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	int32 GetMoney() const { return Money; }
	bool TrySpend(int32 Amount);
	void Earn(int32 Amount);

	int32 GetPrice(ETSSProduct Product) const { return Prices[static_cast<int32>(Product)]; }
	void SetPrice(ETSSProduct Product, int32 Price);

	/** A finished checkout: the cash goes into the till and the sale is logged. */
	void RecordSale(const FTSSTransaction& Sale);
	int32 GetCashSales() const { return CashSales; }
	int32 GetCardSales() const { return CardSales; }
	int32 GetTaxCollected() const { return TaxCollected; }
	const TArray<FTSSTransaction>& GetTransactions() const { return Transactions; }
	/** A customer left without buying (too expensive, out of stock, or tired of waiting). */
	void RecordWalkout();
	int32 GetServed() const { return Served; }
	int32 GetLost() const { return Lost; }

	/** True while a menu is up: customers freeze. Set by the player controller. */
	bool IsSimPaused() const { return bSimPaused; }
	void SetSimPaused(bool bPaused) { bSimPaused = bPaused; }

	FTSSStoreChanged OnChanged;

private:
	int32 Money = 1000;
	int32 Served = 0;
	int32 CashSales = 0;
	int32 CardSales = 0;
	int32 TaxCollected = 0;
	TArray<FTSSTransaction> Transactions;
	int32 Lost = 0;
	bool bSimPaused = true;
	int32 Prices[static_cast<int32>(ETSSProduct::Count)] = {};
};
