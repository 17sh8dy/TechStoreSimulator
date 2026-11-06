#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TSSProducts.h"
#include "TSSCheckoutTypes.h"
#include "TSSBusinessTypes.h"
#include "TSSStoreSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FTSSStoreChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FTSSSaleRecorded, const FTSSTransaction&);

class UTSSBusinessSave;

/** The shop's books: cash and the shelf price of each product. One per world. */
UCLASS()
class UTSSStoreSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Cash in the shop, in whole dollars (cents dropped): what most screens show. */
	int32 GetMoney() const { return MoneyCents / 100; }
	/** Exact cash, in cents. */
	int32 GetMoneyCents() const { return MoneyCents; }
	/** Spending and earning in whole dollars (orders, upgrades, wages...) or exact cents (the register). */
	bool TrySpend(int32 Dollars) { return TrySpendCents(Dollars * 100); }
	void Earn(int32 Dollars) { EarnCents(Dollars * 100); }
	bool TrySpendCents(int32 Cents);
	void EarnCents(int32 Cents);

	int32 GetPrice(ETSSProduct Product) const { return Prices[static_cast<int32>(Product)]; }
	void SetPrice(ETSSProduct Product, int32 Price);

	/** A finished checkout: the cash goes into the till and the sale is logged. */
	void RecordSale(const FTSSTransaction& Sale);
	/** Sales takings including tax, split by how they were paid. Whole dollars, or exact cents. */
	int32 GetCashSales() const { return CashSales / 100; }
	int32 GetCardSales() const { return CardSales / 100; }
	int32 GetTaxCollected() const { return TaxCollected / 100; }
	int32 GetCashSalesCents() const { return CashSales; }
	int32 GetCardSalesCents() const { return CardSales; }
	int32 GetTaxCollectedCents() const { return TaxCollected; }
	const TArray<FTSSTransaction>& GetTransactions() const { return Transactions; }
	/** A customer left without buying (too expensive, out of stock, or tired of waiting). */
	void RecordWalkout();
	int32 GetServed() const { return Served; }
	int32 GetLost() const { return Lost; }

	/** The Open/Closed sign. While closed, no new customers come in (the ones already inside finish shopping). */
	bool IsOpen() const { return bOpen; }
	void SetOpen(bool bNewOpen) { if (bOpen != bNewOpen) { bOpen = bNewOpen; OnChanged.Broadcast(); } }

	/** True while a menu is up: customers freeze. Set by the player controller. */
	bool IsSimPaused() const { return bSimPaused; }
	void SetSimPaused(bool bPaused) { bSimPaused = bPaused; }

	// ---- business statistics (every StoreDesk number comes from here) -----------------------------
	/** Revenue is the pre-tax amount of finished sales. */
	int32 GetLifetimeRevenue() const { return LifetimeRevenue / 100; }
	int32 GetTodayRevenue() const { return TodayRevenue / 100; }
	int32 GetLifetimeRevenueCents() const { return LifetimeRevenue; }
	int32 GetTodayRevenueCents() const { return TodayRevenue; }
	int32 GetProductsSold() const { return ProductsSold; }
	int32 GetDay() const { return Day; }
	int32 GetOrderSpend() const { return OrderSpend; }
	int32 GetUpgradeSpend() const { return UpgradeSpend; }
	/** Called by the business clock when a new shop day starts: today's revenue resets. */
	void StartDay(int32 NewDay);
	/** Spending that StoreDesk does goes through TrySpend, then is noted here for the finance page. */
	void NoteOrderSpend(int32 Amount) { OrderSpend += Amount; OnChanged.Broadcast(); }
	void NoteUpgradeSpend(int32 Amount) { UpgradeSpend += Amount; OnChanged.Broadcast(); }
	void NoteWageSpend(int32 Amount) { WageSpend += Amount; OnChanged.Broadcast(); }
	void NoteContractSpend(int32 Amount) { ContractSpend += Amount; OnChanged.Broadcast(); }
	int32 GetWageSpend() const { return WageSpend; }
	int32 GetContractSpend() const { return ContractSpend; }

	void ExportTo(UTSSBusinessSave* Save) const;
	void ImportFrom(const UTSSBusinessSave* Save);

	FTSSStoreChanged OnChanged;
	/** Fired once per finished sale (the business uses it for experience). */
	FTSSSaleRecorded OnSale;

private:
	int32 MoneyCents = 100000;
int32 Served = 0;
	int32 CashSales = 0;
	int32 CardSales = 0;
	int32 TaxCollected = 0;
	TArray<FTSSTransaction> Transactions;
	int32 Lost = 0;
	int32 LifetimeRevenue = 0;
	int32 TodayRevenue = 0;
	int32 ProductsSold = 0;
	int32 OrderSpend = 0;
	int32 UpgradeSpend = 0;
	int32 WageSpend = 0;
	int32 ContractSpend = 0;
	int32 Day = 1;
	bool bOpen = true;
	bool bSimPaused = true;
	int32 Prices[static_cast<int32>(ETSSProduct::Count)] = {};
};
