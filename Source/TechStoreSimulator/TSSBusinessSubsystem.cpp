#include "TSSBusinessSubsystem.h"
#include "TSSDeliveryActors.h"
#include "TSSEmployee.h"
#include "TSSRegister.h"
#include "TSSRegisterDevice.h"
#include "TSSBusinessSave.h"
#include "TSSInteractable.h"
#include "TSSShelf.h"
#include "TSSStoreLayout.h"
#include "TSSStoreSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// Where each shelf-unit upgrade appears, in stage-0 coordinates; the anchor says which wall it lives against so it
	// moves with that wall when the store is expanded. Left/right shelves face into the room.
	struct FShelfSpot { const TCHAR* Id; FVector Location; float Yaw; TSSStoreLayout::EAnchor Anchor; };
	const FShelfSpot ShelfSpots[] = {
		{ TEXT("shelf_1"), FVector(150.f, 274.f, 0.f), -90.f, TSSStoreLayout::EAnchor::Right },    // stage 0: right wall
		{ TEXT("shelf_2"), FVector(-60.f, -274.f, 0.f), 90.f, TSSStoreLayout::EAnchor::Left },     // stage 1
		{ TEXT("shelf_3"), FVector(-60.f, 274.f, 0.f), -90.f, TSSStoreLayout::EAnchor::Right },
		{ TEXT("shelf_4"), FVector(-275.f, -274.f, 0.f), 90.f, TSSStoreLayout::EAnchor::Left },    // stage 2
		{ TEXT("shelf_5"), FVector(-275.f, 274.f, 0.f), -90.f, TSSStoreLayout::EAnchor::Right },
		{ TEXT("shelf_6"), FVector(-490.f, -274.f, 0.f), 90.f, TSSStoreLayout::EAnchor::Left },
		{ TEXT("shelf_7"), FVector(-490.f, 274.f, 0.f), -90.f, TSSStoreLayout::EAnchor::Right },
	};
}

bool UTSSBusinessSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return Super::ShouldCreateSubsystem(Outer) && World && World->IsGameWorld();
}

// ---- catalogs -------------------------------------------------------------------------------------

const TArray<FTSSUpgradeDef>& UTSSBusinessSubsystem::UpgradeCatalog()
{
	// Id, name, description, cost, min level, min store stage, implemented.
	static const TArray<FTSSUpgradeDef> Defs = {
		{ TEXT("supplier_fast"), TEXT("Express Supplier Contract"), TEXT("Wholesale orders arrive in half the time."), 400, 1, 0, true },
		{ TEXT("shelf_1"), TEXT("Extra Shelf Unit"), TEXT("A 12-slot shelf unit against the right wall."), 250, 1, 0, true },
		{ TEXT("store_1"), TEXT("Store Expansion I"), TEXT("Knock through to the unit next door: 7 x 6 m becomes 10.5 x 8 m, with room for more shelves."), 1200, 2, 0, true },
		{ TEXT("shelf_2"), TEXT("Shelf Unit: left wall"), TEXT("Another 12 slots along the left wall."), 350, 2, 1, true },
		{ TEXT("shelf_3"), TEXT("Shelf Unit: right wall"), TEXT("Another 12 slots along the right wall."), 350, 2, 1, true },
		{ TEXT("store_2"), TEXT("Store Expansion II"), TEXT("The full unit: 14 x 10 m. Plenty of room to fill."), 3500, 4, 1, true },
		{ TEXT("shelf_4"), TEXT("Shelf Unit: far left wall"), TEXT("12 more slots at the back of the left wall."), 450, 3, 2, true },
		{ TEXT("shelf_5"), TEXT("Shelf Unit: far right wall"), TEXT("12 more slots at the back of the right wall."), 450, 3, 2, true },
		{ TEXT("shelf_6"), TEXT("Shelf Unit: rear left"), TEXT("The last spot on the left wall."), 550, 4, 2, true },
		{ TEXT("shelf_7"), TEXT("Shelf Unit: rear right"), TEXT("The last spot on the right wall."), 550, 4, 2, true },
		{ TEXT("storage"), TEXT("Back-Room Storage"), TEXT("A back room for up to 200 units. Send a parcel there with the mouse wheel and shelves are topped up from it automatically."), 1500, 3, 1, true },
		{ TEXT("equipment"), TEXT("Improved Computer"), TEXT("A faster machine: orders arrive 25% sooner and one order can hold up to 100 of each product."), 2000, 3, 0, true },
		{ TEXT("dept_gaming"), TEXT("Gaming Department"), TEXT("Unlocks the gaming range in the wholesale catalogue: controllers, headsets and racing wheels."), 3000, 4, 2, true },
		{ TEXT("employee"), TEXT("Hire a Cashier"), TEXT("Lets you hire staff from the Employees page. Your first hire is a cashier who runs the register."), 2500, 4, 1, true },
		{ TEXT("supplier_premium"), TEXT("Premium Supplier Access"), TEXT("Unlocks higher-end products: gaming laptops, VR headsets and cameras."), 4000, 6, 2, true },
	};
	return Defs;
}

const TArray<FTSSAchievementDef>& UTSSBusinessSubsystem::AchievementCatalog()
{
	static const TArray<FTSSAchievementDef> Defs = {
		{ TEXT("first_sale"), TEXT("First Sale"), TEXT("Complete your first sale.") },
		{ TEXT("ten_customers"), TEXT("Regulars"), TEXT("Serve 10 customers.") },
		{ TEXT("revenue_1000"), TEXT("Four Figures"), TEXT("Earn $1,000 in lifetime revenue.") },
		{ TEXT("big_sale"), TEXT("Big Spender"), TEXT("Ring up a single sale of $1,000 or more.") },
		{ TEXT("first_delivery"), TEXT("Special Delivery"), TEXT("Receive and unpack a wholesale order.") },
		{ TEXT("level_3"), TEXT("Growing Business"), TEXT("Reach business level 3.") },
	};
	return Defs;
}

const TArray<FTSSContractDef>& UTSSBusinessSubsystem::ContractCatalog()
{
	// Id, name, description, cost, days, min level.
	static const TArray<FTSSContractDef> Defs = {
		{ TEXT("promo"), TEXT("Street Promotion"), TEXT("Flyers and a sign on the pavement: twice as many customers walk in."), 350, 3, 5 },
		{ TEXT("bulk_deal"), TEXT("Bulk Wholesale Deal"), TEXT("A volume agreement with your supplier: every wholesale order costs 15% less."), 500, 3, 5 },
		{ TEXT("corporate"), TEXT("Corporate Accounts"), TEXT("Office buyers who are not shopping on price: customers accept shelf prices 25% higher."), 650, 3, 6 },
	};
	return Defs;
}

// ---- level ----------------------------------------------------------------------------------------

int32 UTSSBusinessSubsystem::XPForLevel(int32 Level)
{
	const int32 L = FMath::Max(1, Level) - 1;
	return 100 * L * L;
}

int32 UTSSBusinessSubsystem::LevelForXP(int32 InXP)
{
	return 1 + FMath::FloorToInt(FMath::Sqrt(FMath::Max(0, InXP) / 100.f));
}

void UTSSBusinessSubsystem::AddXP(int32 Amount)
{
	XP += Amount;
	CheckAchievements();
	OnChanged.Broadcast();
}

// ---- lifecycle ------------------------------------------------------------------------------------

void UTSSBusinessSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UTSSStoreSubsystem>();
	Super::Initialize(Collection);
	Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();

	// The automated test works on a throwaway slot, so it can never touch a real save.
	// -TSSStreetShots (the screenshot helper) moves the clock around, so it must never touch a real save either.
	const bool bTest = FParse::Param(FCommandLine::Get(), TEXT("TSSAutoTest")) || FParse::Param(FCommandLine::Get(), TEXT("TSSStreetShots")) || FParse::Param(FCommandLine::Get(), TEXT("TSSTownShots")) || FParse::Param(FCommandLine::Get(), TEXT("TSSDoorShots")) || FParse::Param(FCommandLine::Get(), TEXT("TSSPayShots")) || FParse::Param(FCommandLine::Get(), TEXT("TSSLightShots")) || FParse::Param(FCommandLine::Get(), TEXT("TSSLifeShots")) || FParse::Param(FCommandLine::Get(), TEXT("TSSPadShots")) || FParse::Param(FCommandLine::Get(), TEXT("TSSWalkShots"));
	SlotName = bTest ? TEXT("TSSAutoTest") : TEXT("TSSBusiness");
	if (bTest) UGameplayStatics::DeleteGameInSlot(SlotName, 0);

	if (UTSSBusinessSave* Save = Cast<UTSSBusinessSave>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
	{
		if (Store.IsValid()) Store->ImportFrom(Save);
		XP = Save->XP;
		SimSeconds = Save->SimSeconds;
		NextOrderId = Save->NextOrderId;
		Orders = Save->Orders;
		Upgrades = Save->Upgrades;
		Achievements = Save->Achievements;
		MovedObjects = Save->MovedObjects;
		LoadedShelfStock = Save->ShelfStock;
		Storage = Save->Storage;
		bCashierHired = Save->bCashierHired;
		Contracts = Save->Contracts;
		if (!Save->StoreName.IsEmpty()) StoreName = Save->StoreName;
		// Parcels are not saved: an order that had already arrived is simply re-delivered.
		for (FTSSOrder& O : Orders)
		{
			if (O.Status == ETSSOrderStatus::Arrived)
			{
				O.Status = ETSSOrderStatus::InTransit;
				O.ArriveAt = SimSeconds;
				O.UnitsLeft = O.TotalUnits();
			}
			O.bTruckSent = false;
		}
	}
	if (Store.IsValid())
	{
		StoreHandle = Store->OnChanged.AddUObject(this, &UTSSBusinessSubsystem::HandleStoreChanged);
		SaleHandle = Store->OnSale.AddUObject(this, &UTSSBusinessSubsystem::HandleSale);
	}
}

void UTSSBusinessSubsystem::Deinitialize()
{
	if (Store.IsValid())
	{
		Store->OnChanged.Remove(StoreHandle);
		Store->OnSale.Remove(SaleHandle);
	}
	SaveNow();
	Super::Deinitialize();
}

void UTSSBusinessSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	for (const FName& U : Upgrades) ApplyUpgrade(U);
	RestoreShelves();
	if (bCashierHired) SpawnEmployee();
	// The shell has finished sliding walls and wall-anchored furniture by now; then put the owner's own layout back.
	InWorld.GetTimerManager().SetTimer(LayoutTimer, FTimerDelegate::CreateUObject(this, &UTSSBusinessSubsystem::ApplyLayout), 0.5f, false);
}

void UTSSBusinessSubsystem::RestoreShelves()
{
	bShelvesRestored = true;
	if (LoadedShelfStock.Num() == 0) return;
	for (TActorIterator<ATSSShelf> It(GetWorld()); It; ++It)
	{
		const FName Key = It->LayoutKey();
		if (const FTSSShelfSave* Saved = LoadedShelfStock.FindByPredicate([Key](const FTSSShelfSave& S) { return S.Key == Key; }))
		{
			It->ImportSlots(Saved->Slots);
		}
	}
	LoadedShelfStock.Reset();
}

void UTSSBusinessSubsystem::FinishMove(ATSSInteractable* Obj)
{
	if (!Obj) return;
	// Which wall is it against now? (Within 30 cm; the nearest one wins.)
	FVector C, E;
	Obj->GetActorBounds(false, C, E);
	const int32 Stage = GetStoreStage();
	const float DBack = (C.X - E.X) - TSSStoreLayout::BackX(Stage);
	const float DLeft = (C.Y - E.Y) + TSSStoreLayout::HalfWidth(Stage);
	const float DRight = TSSStoreLayout::HalfWidth(Stage) - (C.Y + E.Y);
	TSSStoreLayout::EAnchor A = TSSStoreLayout::EAnchor::None;
	float Best = 30.f;
	if (DBack < Best) { Best = DBack; A = TSSStoreLayout::EAnchor::Back; }
	if (DLeft < Best) { Best = DLeft; A = TSSStoreLayout::EAnchor::Left; }
	if (DRight < Best) { Best = DRight; A = TSSStoreLayout::EAnchor::Right; }
	for (const TSSStoreLayout::EAnchor X : { TSSStoreLayout::EAnchor::Back, TSSStoreLayout::EAnchor::Left, TSSStoreLayout::EAnchor::Right })
	{
		Obj->Tags.Remove(FName(TSSStoreLayout::AnchorTag(X)));
	}
	if (A != TSSStoreLayout::EAnchor::None) Obj->Tags.AddUnique(FName(TSSStoreLayout::AnchorTag(A)));

	FTSSMovedObject* Entry = MovedObjects.FindByPredicate([Obj](const FTSSMovedObject& M) { return M.Key == Obj->LayoutKey(); });
	if (!Entry) Entry = &MovedObjects.AddDefaulted_GetRef();
	Entry->Key = Obj->LayoutKey();
	Entry->Transform = Obj->GetActorTransform();
	Entry->Anchor = static_cast<int32>(A);
	Obj->OnMoved();
	SaveNow();
}

void UTSSBusinessSubsystem::ApplyLayout()
{
	if (MovedObjects.Num() == 0) return;
	for (TActorIterator<ATSSInteractable> It(GetWorld()); It; ++It)
	{
		ATSSInteractable* Obj = *It;
		if (!Obj->IsMovable()) continue;
		const FTSSMovedObject* M = MovedObjects.FindByPredicate([Obj](const FTSSMovedObject& X) { return X.Key == Obj->LayoutKey(); });
		if (!M) continue;
		Obj->SetActorTransform(M->Transform);
		for (const TSSStoreLayout::EAnchor X : { TSSStoreLayout::EAnchor::Back, TSSStoreLayout::EAnchor::Left, TSSStoreLayout::EAnchor::Right })
		{
			Obj->Tags.Remove(FName(TSSStoreLayout::AnchorTag(X)));
		}
		const TSSStoreLayout::EAnchor A = static_cast<TSSStoreLayout::EAnchor>(M->Anchor);
		if (A != TSSStoreLayout::EAnchor::None) Obj->Tags.AddUnique(FName(TSSStoreLayout::AnchorTag(A)));
		Obj->OnMoved();
	}
}

void UTSSBusinessSubsystem::HandleSale(const FTSSTransaction& Sale)
{
	// Experience: 5 per product sold plus a tenth of the takings.
XP += Sale.Items * 5 + Sale.Total / 1000;   // 5 per product plus a tenth of the takings (the total is in cents)
	CheckAchievements();
	SaveNow();
	OnChanged.Broadcast();
}

void UTSSBusinessSubsystem::HandleStoreChanged()
{
	bSaveDirty = true;   // covers shelf price changes, which only go through the store
	CheckAchievements();
	OnChanged.Broadcast();
}

void UTSSBusinessSubsystem::ResetSaveAndRestart()
{
	bSavingDisabled = true;
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void UTSSBusinessSubsystem::SaveNow()
{
	if (!Store.IsValid() || bSavingDisabled) return;
	UTSSBusinessSave* Save = Cast<UTSSBusinessSave>(UGameplayStatics::CreateSaveGameObject(UTSSBusinessSave::StaticClass()));
	Store->ExportTo(Save);
	Save->XP = XP;
	Save->SimSeconds = SimSeconds;
	Save->NextOrderId = NextOrderId;
	Save->Orders = Orders;
	Save->Upgrades = Upgrades;
	Save->Achievements = Achievements;
	Save->MovedObjects = MovedObjects;
	Save->Storage = Storage;
	Save->bCashierHired = bCashierHired;
	Save->Contracts = Contracts;
	Save->StoreName = StoreName;
	if (!bShelvesRestored)
	{
		Save->ShelfStock = LoadedShelfStock;   // the shelves are not back yet: do not overwrite the saved stock with empty ones
	}
	else if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ATSSShelf> It(World); It; ++It)
		{
			FTSSShelfSave& Entry = Save->ShelfStock.AddDefaulted_GetRef();
			Entry.Key = It->LayoutKey();
			It->ExportSlots(Entry.Slots);
		}
	}
	bSaveDirty = false;
	DirtyTimer = 0.f;
	UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
}

// ---- dashboard ------------------------------------------------------------------------------------

FTSSDashboard UTSSBusinessSubsystem::GetDashboard() const
{
	FTSSDashboard D;
	if (!Store.IsValid()) return D;
	D.Money = Store->GetMoney();
	D.TodayRevenue = Store->GetTodayRevenue();
	D.LifetimeRevenue = Store->GetLifetimeRevenue();
	D.ProductsSold = Store->GetProductsSold();
	D.CustomersServed = Store->GetServed();
	D.WalkOuts = Store->GetLost();
	D.Day = Store->GetDay();
	D.CashSales = Store->GetCashSales();
	D.CardSales = Store->GetCardSales();
	D.XP = XP;
	D.Level = LevelForXP(XP);
	D.XPIntoLevel = XP - XPForLevel(D.Level);
	D.XPForNextLevel = XPForLevel(D.Level + 1) - XPForLevel(D.Level);
	D.ActiveOrders = ActiveOrderCount();
	D.AverageSale = D.CustomersServed > 0 ? D.LifetimeRevenue / D.CustomersServed : 0;
	return D;
}

// ---- achievements ---------------------------------------------------------------------------------

void UTSSBusinessSubsystem::Unlock(const TCHAR* Id)
{
	const FName Name(Id);
	if (Achievements.Contains(Name)) return;
	Achievements.Add(Name);
	UE_LOG(LogTemp, Display, TEXT("TSS achievement unlocked: %s"), Id);
}

void UTSSBusinessSubsystem::CheckAchievements()
{
	if (!Store.IsValid()) return;
	if (Store->GetServed() >= 1) Unlock(TEXT("first_sale"));
	if (Store->GetServed() >= 10) Unlock(TEXT("ten_customers"));
	if (Store->GetLifetimeRevenue() >= 1000) Unlock(TEXT("revenue_1000"));
	if (GetLevel() >= 3) Unlock(TEXT("level_3"));
	for (const FTSSTransaction& T : Store->GetTransactions())
	{
if (T.Total >= 100000) { Unlock(TEXT("big_sale")); break; }
	}
	for (const FTSSOrder& O : Orders)
	{
		if (O.Status == ETSSOrderStatus::Received) { Unlock(TEXT("first_delivery")); break; }
	}
}

bool UTSSBusinessSubsystem::IsProductAvailable(ETSSProduct Product, FString* Why) const
{
	const FTSSProductDef& Def = TSSProductDef(Product);
	if (Def.MinLevel > GetLevel())
	{
		if (Why) *Why = FString::Printf(TEXT("%s is not available yet: reach business level %d."), Def.Name, Def.MinLevel);
		return false;
	}
	if (Def.RequiresUpgrade[0] != 0 && !HasUpgrade(Def.RequiresUpgrade))
	{
		if (Why)
		{
			FString UpgradeName = Def.RequiresUpgrade;
			for (const FTSSUpgradeDef& U : UpgradeCatalog()) if (FName(U.Id) == FName(Def.RequiresUpgrade)) UpgradeName = U.Name;
			*Why = FString::Printf(TEXT("%s needs the upgrade \"%s\"."), Def.Name, *UpgradeName);
		}
		return false;
	}
	return true;
}

// ---- orders ---------------------------------------------------------------------------------------

int32 UTSSBusinessSubsystem::OrderCost(const TArray<FTSSOrderLine>& Lines) const
{
	int32 Cost = 0;
	for (const FTSSOrderLine& L : Lines) Cost += TSSProductDef(L.Product).WholesaleCost * L.Qty;
	return FMath::RoundToInt(Cost * WholesaleMultiplier());
}

float UTSSBusinessSubsystem::EtaFor(int32 Units) const
{
	float Eta = FMath::Min(40.f + 6.f * Units, 120.f);
	if (HasUpgrade(TEXT("supplier_fast"))) Eta *= 0.5f;
	if (HasUpgrade(TEXT("equipment"))) Eta *= 0.75f;
	return Eta * EtaScale;
}

int32 UTSSBusinessSubsystem::ActiveOrderCount() const
{
	int32 N = 0;
	for (const FTSSOrder& O : Orders) if (O.Status != ETSSOrderStatus::Received) ++N;
	return N;
}

const FTSSOrder* UTSSBusinessSubsystem::FindOrder(int32 Id) const
{
	return Orders.FindByPredicate([Id](const FTSSOrder& O) { return O.Id == Id; });
}

bool UTSSBusinessSubsystem::PlaceOrder(const TArray<FTSSOrderLine>& InLines, FString& Why, int32* OutOrderId)
{
	FTSSOrder Order;
	for (const FTSSOrderLine& L : InLines)
	{
		if (L.Product == ETSSProduct::None || L.Qty <= 0) continue;
		Order.Lines.Add(L);
	}
	if (Order.Lines.Num() == 0) { Why = TEXT("Your cart is empty."); return false; }
	for (const FTSSOrderLine& L : Order.Lines)
	{
		if (!IsProductAvailable(L.Product, &Why)) return false;
		if (L.Qty > MaxCartQty()) { Why = FString::Printf(TEXT("One order can hold at most %d of each product."), MaxCartQty()); return false; }
	}
	if (!Store.IsValid()) { Why = TEXT("No store."); return false; }

	Order.Cost = OrderCost(Order.Lines);
	if (!Store->TrySpend(Order.Cost))
	{
		Why = FString::Printf(TEXT("Not enough money: the order costs $%d."), Order.Cost);
		return false;
	}
	Store->NoteOrderSpend(Order.Cost);

	Order.Id = NextOrderId++;
	Order.UnitsLeft = Order.TotalUnits();
	Order.PlacedAt = SimSeconds;
	Order.ArriveAt = SimSeconds + EtaFor(Order.UnitsLeft);
	Order.Status = ETSSOrderStatus::InTransit;
	Orders.Add(Order);
	if (OutOrderId) *OutOrderId = Order.Id;
	Why = FString::Printf(TEXT("Order #%d placed ($%d). Estimated delivery: %d seconds."), Order.Id, Order.Cost, FMath::RoundToInt(Order.ArriveAt - SimSeconds));
	SaveNow();
	OnChanged.Broadcast();
	return true;
}

void UTSSBusinessSubsystem::SendTruck(FTSSOrder& Order)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FTSSDockInfo Dock = ATSSDeliveryDock::Resolve(GetWorld());
	ATSSDeliveryTruck* Truck = GetWorld()->SpawnActor<ATSSDeliveryTruck>(ATSSDeliveryTruck::StaticClass(), FTransform(FRotator(0.f, 180.f, 0.f), Dock.TruckStart), Params);
	if (!Truck) return;
	Truck->Setup(Order, Dock, DeliverySpeed);
	Order.bTruckSent = true;
	bTruckActive = true;
}

void UTSSBusinessSubsystem::OnParcelsDelivered(int32 OrderId)
{
	for (FTSSOrder& O : Orders) if (O.Id == OrderId) O.Status = ETSSOrderStatus::Arrived;
	OnChanged.Broadcast();
}

void UTSSBusinessSubsystem::OnUnitUnloaded(int32 OrderId)
{
	for (FTSSOrder& O : Orders)
	{
		if (O.Id != OrderId) continue;
		if (--O.UnitsLeft <= 0)
		{
			O.UnitsLeft = 0;
			O.Status = ETSSOrderStatus::Received;
			AddXP(25 + O.TotalUnits());
			SaveNow();
		}
		OnChanged.Broadcast();
		return;
	}
}

// ---- upgrades -------------------------------------------------------------------------------------

bool UTSSBusinessSubsystem::CanBuyUpgrade(const FTSSUpgradeDef& Def, FString& Why) const
{
	if (!Def.bImplemented) { Why = TEXT("Not available yet."); return false; }
	if (HasUpgrade(Def.Id)) { Why = TEXT("Already owned."); return false; }
	if (GetLevel() < Def.MinLevel) { Why = FString::Printf(TEXT("Requires business level %d."), Def.MinLevel); return false; }
	if (GetStoreStage() < Def.MinStage) { Why = FString::Printf(TEXT("Needs a bigger store first (%s)."), TSSStoreLayout::StageName(Def.MinStage)); return false; }
	if (!Store.IsValid() || Store->GetMoney() < Def.Cost) { Why = FString::Printf(TEXT("Costs $%d."), Def.Cost); return false; }
	return true;
}

bool UTSSBusinessSubsystem::CanRenameStore(const FString& NewName, FString& Why) const
{
	const FString Name = NewName.TrimStartAndEnd();
	if (Name.IsEmpty()) { Why = TEXT("Type a name first."); return false; }
	if (Name.Len() > MaxNameLength) { Why = FString::Printf(TEXT("Names can be up to %d characters."), MaxNameLength); return false; }
	if (Name == StoreName) { Why = TEXT("That is already the store's name."); return false; }
	if (!Store.IsValid() || Store->GetMoney() < RenameCost * 100) { Why = FString::Printf(TEXT("Renaming costs $%d."), RenameCost); return false; }
	return true;
}

bool UTSSBusinessSubsystem::RenameStore(const FString& NewName, FString& Why)
{
	if (!CanRenameStore(NewName, Why)) return false;
	if (!Store->TrySpend(RenameCost)) { Why = TEXT("Not enough money."); return false; }
	Store->NoteUpgradeSpend(RenameCost);
	StoreName = NewName.TrimStartAndEnd();
	SaveNow();
	OnChanged.Broadcast();
	Why = FString::Printf(TEXT("Your store is now called \"%s\"."), *StoreName);
	return true;
}

bool UTSSBusinessSubsystem::BuyUpgrade(FName Id, FString& Why)
{
	for (const FTSSUpgradeDef& Def : UpgradeCatalog())
	{
		if (Id != FName(Def.Id)) continue;
		if (!CanBuyUpgrade(Def, Why)) return false;
		if (!Store->TrySpend(Def.Cost)) { Why = TEXT("Not enough money."); return false; }
		Store->NoteUpgradeSpend(Def.Cost);
		Upgrades.Add(Id);
		ApplyUpgrade(Id);
		AddXP(50);
		SaveNow();
		Why = FString::Printf(TEXT("Purchased %s."), Def.Name);
		return true;
	}
	Why = TEXT("Unknown upgrade.");
	return false;
}

void UTSSBusinessSubsystem::ApplyUpgrade(FName Id)
{
	UWorld* World = GetWorld();
	for (const FShelfSpot& Spot : ShelfSpots)
	{
		if (Id != FName(Spot.Id)) continue;
		// Already in the world (e.g. applied once this session)?
		for (TActorIterator<ATSSShelf> It(World); It; ++It)
		{
			if (It->ActorHasTag(Id)) return;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector At = Spot.Location + TSSStoreLayout::AnchorDelta(Spot.Anchor, GetStoreStage());
		if (ATSSShelf* Shelf = World->SpawnActor<ATSSShelf>(ATSSShelf::StaticClass(), FTransform(FRotator(0.f, Spot.Yaw, 0.f), At), Params))
		{
			Shelf->Tags.Add(Id);
			Shelf->Tags.Add(TEXT("TSSUpgrade"));
			Shelf->Tags.Add(TSSStoreLayout::AnchorTag(Spot.Anchor));
		}
	}
	// store_1 / store_2 are applied by the StoreShell (it watches GetStoreStage()); supplier_fast is read by EtaFor().
}

// ---- back room ------------------------------------------------------------------------------------

int32 UTSSBusinessSubsystem::GetStored(ETSSProduct Product) const
{
	const FTSSOrderLine* L = Storage.FindByPredicate([Product](const FTSSOrderLine& X) { return X.Product == Product; });
	return L ? L->Qty : 0;
}

int32 UTSSBusinessSubsystem::GetStoredTotal() const
{
	int32 N = 0;
	for (const FTSSOrderLine& L : Storage) N += L.Qty;
	return N;
}

bool UTSSBusinessSubsystem::StoreInBackRoom(ETSSProduct Product, int32 Qty)
{
	if (!HasUpgrade(TEXT("storage")) || Product == ETSSProduct::None || Qty <= 0) return false;
	if (GetStoredTotal() + Qty > StorageCapacity) return false;
	FTSSOrderLine* L = Storage.FindByPredicate([Product](const FTSSOrderLine& X) { return X.Product == Product; });
	if (!L) { L = &Storage.AddDefaulted_GetRef(); L->Product = Product; }
	L->Qty += Qty;
	bSaveDirty = true;
	OnChanged.Broadcast();
	return true;
}

int32 UTSSBusinessSubsystem::RestockShelvesFromStorage(int32 MaxUnits)
{
	int32 Moved = 0;
	if (!HasUpgrade(TEXT("storage")) || GetStoredTotal() == 0) return 0;
	for (TActorIterator<ATSSShelf> It(GetWorld()); It && Moved < MaxUnits; ++It)
	{
		for (int32 s = 0; s < ATSSShelf::Tiers * ATSSShelf::Columns && Moved < MaxUnits; ++s)
		{
			const ETSSProduct Wants = It->GetSlotWanted(s);
			if (Wants == ETSSProduct::None || GetStored(Wants) <= 0) continue;
			if (!It->PutBack(s, Wants)) continue;   // full, or holds something else
			FTSSOrderLine* L = Storage.FindByPredicate([Wants](const FTSSOrderLine& X) { return X.Product == Wants; });
			if (L && --L->Qty <= 0) Storage.RemoveAll([Wants](const FTSSOrderLine& X) { return X.Product == Wants; });
			++Moved;
		}
	}
	if (Moved > 0) OnChanged.Broadcast();
	return Moved;
}

// ---- staff ----------------------------------------------------------------------------------------

bool UTSSBusinessSubsystem::CanHireCashier(FString& Why) const
{
	if (!HasUpgrade(TEXT("employee"))) { Why = TEXT("Buy the \"Hire a Cashier\" upgrade in Store Management first."); return false; }
	if (bCashierHired) { Why = TEXT("You already have a cashier."); return false; }
	if (!Store.IsValid() || Store->GetMoney() < CashierHireFee) { Why = FString::Printf(TEXT("Hiring costs $%d."), CashierHireFee); return false; }
	return true;
}

bool UTSSBusinessSubsystem::HireCashier(FString& Why)
{
	if (!CanHireCashier(Why)) return false;
	if (!Store->TrySpend(CashierHireFee)) { Why = TEXT("Not enough money."); return false; }
	Store->NoteWageSpend(CashierHireFee);
	bCashierHired = true;
	SpawnEmployee();
	AddXP(30);
	SaveNow();
	Why = FString::Printf(TEXT("You hired a cashier. Wage: $%d per day."), CashierWage);
	return true;
}

void UTSSBusinessSubsystem::DismissCashier()
{
	bCashierHired = false;
	if (Employee.IsValid()) Employee->Destroy();
	Employee.Reset();
	SaveNow();
	OnChanged.Broadcast();
}

void UTSSBusinessSubsystem::SetCashierForTest(bool bOn)
{
	if (bOn) { bCashierHired = true; SpawnEmployee(); }
	else DismissCashier();
}

void UTSSBusinessSubsystem::SpawnEmployee()
{
	if (Employee.IsValid()) return;
	ATSSRegister* Reg = nullptr;
	for (TActorIterator<ATSSRegister> It(GetWorld()); It; ++It) { Reg = *It; break; }
	if (!Reg) return;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector At = Reg->GetActorTransform().TransformPosition(ATSSEmployee::StandOffset());
	Employee = GetWorld()->SpawnActor<ATSSEmployee>(ATSSEmployee::StaticClass(), FTransform(FRotator(0.f, Reg->GetActorRotation().Yaw + 180.f, 0.f), At), Params);
}

void UTSSBusinessSubsystem::PayWages()
{
	if (!bCashierHired || !Store.IsValid()) return;
	if (Store->TrySpend(CashierWage))
	{
		Store->NoteWageSpend(CashierWage);
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("TSS the cashier quit: wages could not be paid"));
		DismissCashier();
	}
}

// ---- contracts ------------------------------------------------------------------------------------

bool UTSSBusinessSubsystem::IsContractActive(FName Id) const
{
	return Contracts.ContainsByPredicate([Id](const FTSSActiveContract& C) { return C.Id == Id; });
}

int32 UTSSBusinessSubsystem::ContractDaysLeft(FName Id) const
{
	const FTSSActiveContract* C = Contracts.FindByPredicate([Id](const FTSSActiveContract& X) { return X.Id == Id; });
	return (C && Store.IsValid()) ? FMath::Max(0, C->EndDay - Store->GetDay() + 1) : 0;
}

bool UTSSBusinessSubsystem::CanSignContract(const FTSSContractDef& Def, FString& Why) const
{
	if (IsContractActive(Def.Id)) { Why = TEXT("Already running."); return false; }
	if (GetLevel() < Def.MinLevel) { Why = FString::Printf(TEXT("Requires business level %d."), Def.MinLevel); return false; }
	if (!Store.IsValid() || Store->GetMoney() < Def.Cost) { Why = FString::Printf(TEXT("Costs $%d."), Def.Cost); return false; }
	return true;
}

bool UTSSBusinessSubsystem::SignContract(FName Id, FString& Why)
{
	for (const FTSSContractDef& Def : ContractCatalog())
	{
		if (Id != FName(Def.Id)) continue;
		if (!CanSignContract(Def, Why)) return false;
		if (!Store->TrySpend(Def.Cost)) { Why = TEXT("Not enough money."); return false; }
		Store->NoteContractSpend(Def.Cost);
		FTSSActiveContract& C = Contracts.AddDefaulted_GetRef();
		C.Id = Id;
		C.EndDay = Store->GetDay() + Def.Days - 1;
		AddXP(40);
		SaveNow();
		Why = FString::Printf(TEXT("Signed: %s (%d days)."), Def.Name, Def.Days);
		return true;
	}
	Why = TEXT("Unknown contract.");
	return false;
}

void UTSSBusinessSubsystem::ExpireContracts()
{
	if (!Store.IsValid()) return;
	const int32 Today = Store->GetDay();
	Contracts.RemoveAll([Today](const FTSSActiveContract& C) { return C.EndDay < Today; });
}

// ---- clock ----------------------------------------------------------------------------------------

FString UTSSBusinessSubsystem::FormatClock(float Hour)
{
	int32 Minutes = FMath::FloorToInt(FMath::Fmod(Hour + 24.f, 24.f) * 60.f);
	const int32 H24 = (Minutes / 60) % 24;
	const int32 M = Minutes % 60;
	const int32 H12 = (H24 % 12 == 0) ? 12 : (H24 % 12);
	return FString::Printf(TEXT("%d:%02d %s"), H12, M, H24 < 12 ? TEXT("AM") : TEXT("PM"));
}

void UTSSBusinessSubsystem::SetHour(float Hour)
{
	const float Offset = StartHour / 24.f * DayLengthSeconds;
	const int32 DayIndex = FMath::FloorToInt((SimSeconds + Offset) / DayLengthSeconds);
	float NewSim = (DayIndex + FMath::Fmod(Hour + 24.f, 24.f) / 24.f) * DayLengthSeconds - Offset;
	if (NewSim < 0.f) NewSim += DayLengthSeconds;
	SimSeconds = NewSim;
}

void UTSSBusinessSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!Store.IsValid() || Store->IsSimPaused()) return;

	SimSeconds += DeltaTime;
	const int32 NewDay = DayForSeconds(SimSeconds);
	if (NewDay > Store->GetDay())
	{
		Store->StartDay(NewDay);
		ExpireContracts();
		PayWages();
		// A trip to the bank every morning: any note or coin the drawer ran short of is topped back up to the float.
		for (TActorIterator<ATSSRegister> RegIt(GetWorld()); RegIt; ++RegIt) if (ATSSCashDrawer* Drawer = RegIt->GetDrawer()) Drawer->RefillFloat();
}

	RestockTimer += DeltaTime;
	if (RestockTimer > 4.f)
	{
		RestockTimer = 0.f;
		RestockShelvesFromStorage(2);
	}

	if (!bTruckActive)
	{
		for (FTSSOrder& O : Orders)
		{
			if (O.Status == ETSSOrderStatus::InTransit && !O.bTruckSent && SimSeconds >= O.ArriveAt)
			{
				SendTruck(O);
				break;
			}
		}
	}

	if (bSaveDirty && (DirtyTimer += DeltaTime) > 3.f) SaveNow();

	AutosaveTimer += DeltaTime;
	if (AutosaveTimer > 30.f)
	{
		AutosaveTimer = 0.f;
		SaveNow();
	}
}
