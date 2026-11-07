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

	// ---- the shop layout the owner has rearranged ----
	/** Called when the owner puts a shelf or the computer down: works out its wall anchor, remembers and saves it. */
	void FinishMove(class ATSSInteractable* Obj);
	/** Puts every moved object back where the owner left it (runs shortly after the level starts). */
	void ApplyLayout();
	int32 GetMovedCount() const { return MovedObjects.Num(); }

	/** Hour of the in-game day, 0 <= h < 24 (8.5 = 8:30 AM). */
	float GetHour() const { return FMath::Fmod(StartHour + SimSeconds / DayLengthSeconds * 24.f + 24.f * 1000.f, 24.f); }
	/** Jumps the clock to this hour of the current day (developer/test helper; it also moves order timers). */
	void SetHour(float Hour);
	/** The shop opens for its first day on this calendar date (a day is one DayLengthSeconds). */
	static FDateTime StartDate() { return FDateTime(2026, 11, 6); }
	/** The calendar date of a shop day. */
	static FDateTime DateForDay(int32 Day) { return StartDate() + FTimespan::FromDays(FMath::Max(0, Day - 1)); }
	/** "11/06/2026" */
	static FString FormatDate(int32 Day) { const FDateTime D = DateForDay(Day); return FString::Printf(TEXT("%02d/%02d/%04d"), D.GetMonth(), D.GetDay(), D.GetYear()); }
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
	// ---- the store's name ------------------------------------------------------------------------
	static constexpr int32 RenameCost = 500;     // dollars
	static constexpr int32 MaxNameLength = 24;
	const FString& GetStoreName() const { return StoreName; }
	/** Checks a proposed name (trimmed, 1-24 characters, different from the current one, affordable). */
	bool CanRenameStore(const FString& NewName, FString& Why) const;
	/** Pays $500 and renames the shop: the sign, the receipts and the screens all follow. */
	bool RenameStore(const FString& NewName, FString& Why);
	bool HasUpgrade(FName Id) const { return Upgrades.Contains(Id); }
	/** 0 = small shop, 1 = medium, 2 = full-size. Each store expansion upgrade adds one. */
	int32 GetStoreStage() const { return (HasUpgrade(TEXT("store_1")) ? 1 : 0) + (HasUpgrade(TEXT("store_1")) && HasUpgrade(TEXT("store_2")) ? 1 : 0); }
	bool CanBuyUpgrade(const FTSSUpgradeDef& Def, FString& Why) const;
	bool BuyUpgrade(FName Id, FString& Why);

	/** Can this product be ordered right now (business level and any upgrade it needs)? */
	bool IsProductAvailable(ETSSProduct Product, FString* Why = nullptr) const;
	/** Most of one product a single order may hold (the improved computer doubles it). */
	int32 MaxCartQty() const { return HasUpgrade(TEXT("equipment")) ? 100 : 50; }

	// ---- back room (storage upgrade) ------------------------------------------------------------
	static constexpr int32 StorageCapacity = 200;
	int32 GetStored(ETSSProduct Product) const;
	int32 GetStoredTotal() const;
	const TArray<FTSSOrderLine>& GetStorage() const { return Storage; }
	/** Moves units into the back room. False if there is no storage upgrade or it would not fit. */
	bool StoreInBackRoom(ETSSProduct Product, int32 Qty);
	/** Moves one unit from the back room onto a shelf slot that is short of it, every few seconds. Returns units moved (test hook). */
	int32 RestockShelvesFromStorage(int32 MaxUnits = 1);

	// ---- staff ----------------------------------------------------------------------------------
	static constexpr int32 CashierHireFee = 150;
	static constexpr int32 CashierWage = 80;   // paid at the start of every shop day
	bool HasCashier() const { return bCashierHired; }
	bool CanHireCashier(FString& Why) const;
	bool HireCashier(FString& Why);
	void DismissCashier();
	/** Test hook: puts a cashier on or off duty without the upgrade, the fee or the Employees page. */
	void SetCashierForTest(bool bOn);

	// ---- contracts ------------------------------------------------------------------------------
	static const TArray<FTSSContractDef>& ContractCatalog();
	bool IsContractActive(FName Id) const;
	int32 ContractDaysLeft(FName Id) const;   // counting today
	bool CanSignContract(const FTSSContractDef& Def, FString& Why) const;
	bool SignContract(FName Id, FString& Why);
	const TArray<FTSSActiveContract>& GetContracts() const { return Contracts; }
	/** Effects of running deals. */
	float WholesaleMultiplier() const { return IsContractActive(TEXT("bulk_deal")) ? 0.85f : 1.f; }
	float PriceToleranceMultiplier() const { return IsContractActive(TEXT("corporate")) ? 1.25f : 1.f; }
	float CustomerRateMultiplier() const { return IsContractActive(TEXT("promo")) ? 2.f : 1.f; }

	// ---- achievements ---------------------------------------------------------------------------
	static const TArray<FTSSAchievementDef>& AchievementCatalog();
	bool HasAchievement(FName Id) const { return Achievements.Contains(Id); }

	// ---- persistence ----------------------------------------------------------------------------
	void SaveNow();
	/** Erases the saved game (cash, stock, upgrades, everything) and reloads the level, so the next start is a fresh $1,000 store. */
	void ResetSaveAndRestart();
	/** The slot in use: the real one, or a throwaway one while running the automated test. */
	const FString& GetSlotName() const { return SlotName; }

	/** Test hooks: scale truck/courier speed and the ordering lead time. */
	float DeliverySpeed = 1.f;
	float EtaScale = 1.f;

	/** A shelf slot changed: save soon (a few seconds later, so a burst of stocking is one write). */
	void NoteStockChanged() { bSaveDirty = true; }

	FSimpleMulticastDelegate OnChanged;

private:
	void RestoreShelves();
	void SpawnEmployee();
	void PayWages();
	void ExpireContracts();
	void ApplyUpgrade(FName Id);
	void CheckAchievements();
	void Unlock(const TCHAR* Id);
	void SendTruck(FTSSOrder& Order);
	void HandleStoreChanged();
	void HandleSale(const FTSSTransaction& Sale);

	UPROPERTY() int32 XP = 0;
	UPROPERTY() float SimSeconds = 0.f;
	UPROPERTY() TArray<FTSSMovedObject> MovedObjects;
	FTimerHandle LayoutTimer;
	UPROPERTY() int32 NextOrderId = 1;
	UPROPERTY() TArray<FTSSOrder> Orders;
	UPROPERTY() TArray<FName> Upgrades;
	UPROPERTY() TArray<FName> Achievements;

	/** Shelf contents read from the save; kept until they are put on the shelves (and re-saved untouched until then). */
	UPROPERTY() TArray<FTSSShelfSave> LoadedShelfStock;
	UPROPERTY() TArray<FTSSOrderLine> Storage;
	UPROPERTY() TArray<FTSSActiveContract> Contracts;
	bool bCashierHired = false;
	float RestockTimer = 0.f;
	TWeakObjectPtr<AActor> Employee;
	bool bShelvesRestored = false;
	bool bSaveDirty = false;
	float DirtyTimer = 0.f;

	FString SlotName;
	FString StoreName = TEXT("Tech Store");
	bool bSavingDisabled = false;   // set while resetting so closing the old level cannot write the old game back
	bool bTruckActive = false;
	float AutosaveTimer = 0.f;
	TWeakObjectPtr<UTSSStoreSubsystem> Store;
	FDelegateHandle StoreHandle;
	FDelegateHandle SaleHandle;
};
