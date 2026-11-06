#include "TSSStoreSubsystem.h"
#include "TSSBusinessSave.h"

void UTSSStoreSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	for (int32 i = 0; i < static_cast<int32>(ETSSProduct::Count); ++i)
	{
		Prices[i] = TSSProductDef(static_cast<ETSSProduct>(i)).DefaultPrice();
	}
}

bool UTSSStoreSubsystem::TrySpendCents(int32 Amount)
{
	if (Amount > MoneyCents) return false;
	MoneyCents -= Amount;
	OnChanged.Broadcast();
	return true;
}

void UTSSStoreSubsystem::EarnCents(int32 Amount)
{
	MoneyCents += Amount;
	OnChanged.Broadcast();
}

void UTSSStoreSubsystem::SetPrice(ETSSProduct Product, int32 Price)
{
	if (Product == ETSSProduct::None) return;
	Prices[static_cast<int32>(Product)] = FMath::Clamp(Price, 1, 99999);
	OnChanged.Broadcast();
}

void UTSSStoreSubsystem::RecordSale(const FTSSTransaction& Sale)
{
	++Served;
	FTSSTransaction Logged = Sale;
	Logged.Id = (Transactions.Num() > 0 ? Transactions.Last().Id : 0) + 1;
	Logged.Day = Day;
	Transactions.Add(Logged);
	if (Transactions.Num() > 60) Transactions.RemoveAt(0, Transactions.Num() - 60);
	LifetimeRevenue += Sale.Subtotal;
	TodayRevenue += Sale.Subtotal;
	ProductsSold += Sale.Items;
	(Sale.Method == ETSSPayMethod::Cash ? CashSales : CardSales) += Sale.Total;
	TaxCollected += Sale.Tax; // collected, but not yet owed to anyone: tax liability is not modelled
	EarnCents(Sale.Total);
OnSale.Broadcast(Logged);
}

void UTSSStoreSubsystem::RecordWalkout()
{
	++Lost;
	OnChanged.Broadcast();
}

void UTSSStoreSubsystem::StartDay(int32 NewDay)
{
	Day = NewDay;
	TodayRevenue = 0;
	OnChanged.Broadcast();
}

void UTSSStoreSubsystem::ExportTo(UTSSBusinessSave* S) const
{
S->Version = 2; S->Money = MoneyCents; S->Served = Served; S->Lost = Lost; S->CashSales = CashSales; S->CardSales = CardSales;
	S->TaxCollected = TaxCollected; S->LifetimeRevenue = LifetimeRevenue; S->TodayRevenue = TodayRevenue;
	S->ProductsSold = ProductsSold; S->OrderSpend = OrderSpend; S->UpgradeSpend = UpgradeSpend; S->Day = Day;
	S->Transactions = Transactions;
	S->bStoreOpen = bOpen;
	S->WageSpend = WageSpend; S->ContractSpend = ContractSpend;
	S->Prices.Reset();
	for (int32 i = 0; i < static_cast<int32>(ETSSProduct::Count); ++i) S->Prices.Add(Prices[i]);
}

void UTSSStoreSubsystem::ImportFrom(const UTSSBusinessSave* S)
{
	MoneyCents = S->Money; Served = S->Served; Lost = S->Lost; CashSales = S->CashSales; CardSales = S->CardSales;
	TaxCollected = S->TaxCollected; LifetimeRevenue = S->LifetimeRevenue; TodayRevenue = S->TodayRevenue;
	ProductsSold = S->ProductsSold; OrderSpend = S->OrderSpend; UpgradeSpend = S->UpgradeSpend; Day = FMath::Max(1, S->Day);
	Transactions = S->Transactions;
	bOpen = S->bStoreOpen;
	if (S->Version < 2)
	{
		// Saved before cents: every money field was whole dollars.
		MoneyCents *= 100; CashSales *= 100; CardSales *= 100; TaxCollected *= 100; LifetimeRevenue *= 100; TodayRevenue *= 100;
		for (FTSSTransaction& T : Transactions) { T.Subtotal *= 100; T.Tax *= 100; T.Total *= 100; T.CashReceived *= 100; T.Change *= 100; }
	}
	WageSpend = S->WageSpend; ContractSpend = S->ContractSpend;
	// An older save may predate newer products: restore the prices it knows, leave the rest at their defaults.
	for (int32 i = 0; i < S->Prices.Num() && i < static_cast<int32>(ETSSProduct::Count); ++i) Prices[i] = FMath::Clamp(S->Prices[i], 1, 99999);
	OnChanged.Broadcast();
}
