#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TSSCheckoutTypes.h"
#include "TSSBusinessTypes.h"
#include "TSSBusinessSave.generated.h"

/** Everything about the business that survives quitting the game. (Shelf stock is not saved yet.) */
UCLASS()
class UTSSBusinessSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 Version = 1;

	// The books
	UPROPERTY() int32 Money = 1000;
	UPROPERTY() int32 Served = 0;
	UPROPERTY() int32 Lost = 0;
	UPROPERTY() int32 CashSales = 0;
	UPROPERTY() int32 CardSales = 0;
	UPROPERTY() int32 TaxCollected = 0;
	UPROPERTY() int32 LifetimeRevenue = 0;
	UPROPERTY() int32 TodayRevenue = 0;
	UPROPERTY() int32 ProductsSold = 0;
	UPROPERTY() int32 OrderSpend = 0;
	UPROPERTY() int32 UpgradeSpend = 0;
	UPROPERTY() int32 Day = 1;
	UPROPERTY() bool bStoreOpen = true;   // the Open/Closed sign
	UPROPERTY() TArray<FTSSTransaction> Transactions;

	// Progression
	UPROPERTY() int32 XP = 0;
	UPROPERTY() float SimSeconds = 0.f;
	UPROPERTY() int32 NextOrderId = 1;
	UPROPERTY() TArray<FTSSOrder> Orders;
	UPROPERTY() TArray<FName> Upgrades;
	UPROPERTY() TArray<FName> Achievements;
};
