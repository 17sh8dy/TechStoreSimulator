#include "TSSStoreSubsystem.h"

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

void UTSSStoreSubsystem::RecordSale(int32 Amount)
{
	++Served;
	Earn(Amount);
}

void UTSSStoreSubsystem::RecordWalkout()
{
	++Lost;
	OnChanged.Broadcast();
}
