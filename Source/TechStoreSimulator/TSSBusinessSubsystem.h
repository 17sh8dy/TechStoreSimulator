#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TSSBusinessTypes.h"
#include "TSSCheckoutTypes.h"
#include "TSSBusinessSubsystem.generated.h"

class UTSSStoreSubsystem;
class UTSSBusinessSave;

/** The numbers on the StoreDesk dashboard, gathered in one place from the real gameplay data. */
struct FTSSDashboard
{
	int32 Money = 0;
	int32 TodayRevenue = 0;
	int32 LifetimeRevenue = 0;
	int32 ProductsSold = 0;
	int32 CustomersServed = 0;
	int32 WalkOuts = 0;
	int32 Level = 1;
	int32 XP = 0;
	int32 XPIntoLevel = 0;
	int32 XPForNextLevel = 100;
	int32 ActiveOrders = 0;
	int32 Day = 1;
	int32 AverageSale = 0;
	int32 CashSales = 0;
	int32 CardSales = 0;
};

struct FTSSAchievementDef
{
	const TCHAR* Id;
	const TCHAR* Name;
	const TCHAR* Description;
};

/**
 * Everything StoreDesk manages that is not the cash register's books (those live in UTSSStoreSubsystem):
 * wholesale orders and their delivery, experience / level, upgrades, achievements, the shop-day clock,
 * and saving all of it. The StoreDesk interface only ever reads and calls this.
 */
UCLASS()
class UTSSBusinessSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTSSBusinessSubsystem, STATGROUP_Tickables); }

	// ---- stats ----------------------------------------------------------------------------------
	FTSSDashboard GetDashboard() const;
	int32 GetXP() const { return XP; }
	int32 GetLevel() const { return LevelForXP(XP); }
	void AddXP(int32 Amount);
	static int32 LevelForXP(int32 XP);
	/** Total XP needed to reach `Level`. */
	static int32 XPForLevel(int32 Level);
	float GetSimSeconds() const { return SimSeconds; }
	bool IsPrestigeUnlocked() const { return GetLevel() >= PrestigeLevel; }
	static constexpr int32 PrestigeLevel = 10;
	static constexpr float DayLengthSeconds = 900.f;   // one in-game day (24 hours) lasts this many real seconds
	/** The shop day starts at this hour on a new game. */
	static constexpr float StartHour = 8.f;

	/** Hour of the in-game day, 0 <= h < 24 (8.5 = 8:30 AM). */
	float GetHour() const { return FMath::Fmod(StartHour + SimSeconds / DayLengthSeconds * 24.f + 24.f * 1000.f, 24.f); }
	/** Jumps the clock to this hour of the current day (developer/test helper; it also moves order timers). */
	void SetHour(float Hour);
	/** "5:44 PM" */
	static FString FormatClock(float Hour);
	/** The calendar day number for a given sim time: a new day begins at midnight. */
	static int32 DayForSeconds(float Seconds) { return 1 + FMath::FloorToInt((Seconds + StartHour / 24.f * DayLengthSeconds) / DayLengthSeconds); }

	// ---- orders & deliveries ------------------------------------------------------------------
	int32 OrderCost(const TArray<FTSSOrderLine>& Lines) const;
	/** Pays and records an order. False (with a reason) if it cannot be placed. */
	bool PlaceOrder(const TArray<FTSSOrderLine>& Lines, FString& Why, int32* OutOrderId = nullptr);
	const TArray<FTSSOrder>& GetOrders() const { return Orders; }
	const FTSSOrder* FindOrder(int32 Id) const;
	int32 ActiveOrderCount() const;
	float SecondsUntilArrival(const FTSSOrder& Order) const { return FMath::Max(0.f, Order.ArriveAt - SimSeconds); }
	/** Estimated delivery time for an order of this many units (seconds). */
	float EtaFor(int32 Units) const;
	/** Truck/courier callbacks. */
	void OnParcelsDelivered(int32 OrderId);
	void OnUnitUnloaded(int32 OrderId);
	void OnTruckFinished() { bTruckActive = false; }

	// ---- upgrades -------------------------------------------------------------------------------
	static const TArray<FTSSUpgradeDef>& UpgradeCatalog();
	bool HasUpgrade(FName Id) const { return Upgrades.Contains(Id); }
	/** 0 = small shop, 1 = medium, 2 = full-size. Each store expansion upgrade adds one. */
	int32 GetStoreStage() const { return (HasUpgrade(TEXT("store_1")) ? 1 : 0) + (HasUpgrade(TEXT("store_1")) && HasUpgrade(TEXT("store_2")) ? 1 : 0); }
	bool CanBuyUpgrade(const FTSSUpgradeDef& Def, FString& Why) const;
	bool BuyUpgrade(FName Id, FString& Why);

	// ---- achievements ---------------------------------------------------------------------------
	static const TArray<FTSSAchievementDef>& AchievementCatalog();
	bool HasAchievement(FName Id) const { return Achievements.Contains(Id); }

	// ---- persistence ----------------------------------------------------------------------------
	void SaveNow();
	/** The slot in use: the real one, or a throwaway one while running the automated test. */
	const FString& GetSlotName() const { return SlotName; }

	/** Test hooks: scale truck/courier speed and the ordering lead time. */
	float DeliverySpeed = 1.f;
	float EtaScale = 1.f;

	FSimpleMulticastDelegate OnChanged;

private:
	void ApplyUpgrade(FName Id);
	void CheckAchievements();
	void Unlock(const TCHAR* Id);
	void SendTruck(FTSSOrder& Order);
	void HandleStoreChanged();
	void HandleSale(const FTSSTransaction& Sale);

	UPROPERTY() int32 XP = 0;
	UPROPERTY() float SimSeconds = 0.f;
	UPROPERTY() int32 NextOrderId = 1;
	UPROPERTY() TArray<FTSSOrder> Orders;
	UPROPERTY() TArray<FName> Upgrades;
	UPROPERTY() TArray<FName> Achievements;

	FString SlotName;
	bool bTruckActive = false;
	float AutosaveTimer = 0.f;
	TWeakObjectPtr<UTSSStoreSubsystem> Store;
	FDelegateHandle StoreHandle;
	FDelegateHandle SaleHandle;
};
