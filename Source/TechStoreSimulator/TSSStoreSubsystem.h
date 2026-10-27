#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TSSProducts.h"
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

	FTSSStoreChanged OnChanged;

private:
	int32 Money = 1000;
	int32 Prices[static_cast<int32>(ETSSProduct::Count)] = {};
};
