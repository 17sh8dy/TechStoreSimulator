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

bool UTSSStoreSubsystem::TrySpend(int32 Amount)
{
	if (Amount > Money) return false;
	Money -= Amount;
	OnChanged.Broadcast();
	return true;
}

void UTSSStoreSubsystem::Earn(int32 Amount)
{
	Money += Amount;
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
	Earn(Sale.Total);
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
	S->Money = Money; S->Served = Served; S->Lost = Lost; S->CashSales = CashSales; S->CardSales = CardSales;
	S->TaxCollected = TaxCollected; S->LifetimeRevenue = LifetimeRevenue; S->TodayRevenue = TodayRevenue;
	S->ProductsSold = ProductsSold; S->OrderSpend = OrderSpend; S->UpgradeSpend = UpgradeSpend; S->Day = Day;
	S->Transactions = Transactions;
	S->bStoreOpen = bOpen;
	S->Prices.Reset();
	for (int32 i = 0; i < static_cast<int32>(ETSSProduct::Count); ++i) S->Prices.Add(Prices[i]);
}

void UTSSStoreSubsystem::ImportFrom(const UTSSBusinessSave* S)
{
	Money = S->Money; Served = S->Served; Lost = S->Lost; CashSales = S->CashSales; CardSales = S->CardSales;
	TaxCollected = S->TaxCollected; LifetimeRevenue = S->LifetimeRevenue; TodayRevenue = S->TodayRevenue;
	ProductsSold = S->ProductsSold; OrderSpend = S->OrderSpend; UpgradeSpend = S->UpgradeSpend; Day = FMath::Max(1, S->Day);
	Transactions = S->Transactions;
	bOpen = S->bStoreOpen;
	if (S->Prices.Num() == static_cast<int32>(ETSSProduct::Count))
	{
		for (int32 i = 0; i < S->Prices.Num(); ++i) Prices[i] = FMath::Clamp(S->Prices[i], 1, 99999);
	}
	OnChanged.Broadcast();
}
