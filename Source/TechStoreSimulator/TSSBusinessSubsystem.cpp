#include "TSSBusinessSubsystem.h"
#include "TSSDeliveryActors.h"
#include "TSSBusinessSave.h"
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
		{ TEXT("storage"), TEXT("Back-Room Storage"), TEXT("Stock held in the back room for the shelves."), 1500, 3, 1, false },
		{ TEXT("equipment"), TEXT("Improved Computer"), TEXT("A faster machine and a newer StoreDesk."), 2000, 3, 0, false },
		{ TEXT("dept_gaming"), TEXT("Gaming Department"), TEXT("A dedicated area for consoles and accessories."), 3000, 4, 2, false },
		{ TEXT("employee"), TEXT("Hire a Cashier"), TEXT("An employee to run the register."), 2500, 4, 1, false },
		{ TEXT("supplier_premium"), TEXT("Premium Supplier Access"), TEXT("Unlock higher-end products."), 4000, 6, 2, false },
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
	const bool bTest = FParse::Param(FCommandLine::Get(), TEXT("TSSAutoTest"));
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
}

void UTSSBusinessSubsystem::HandleSale(const FTSSTransaction& Sale)
{
	// Experience: 5 per product sold plus a tenth of the takings.
	XP += Sale.Items * 5 + Sale.Total / 10;
	CheckAchievements();
	SaveNow();
	OnChanged.Broadcast();
}

void UTSSBusinessSubsystem::HandleStoreChanged()
{
	CheckAchievements();
	OnChanged.Broadcast();
}

void UTSSBusinessSubsystem::SaveNow()
{
	if (!Store.IsValid()) return;
	UTSSBusinessSave* Save = Cast<UTSSBusinessSave>(UGameplayStatics::CreateSaveGameObject(UTSSBusinessSave::StaticClass()));
	Store->ExportTo(Save);
	Save->XP = XP;
	Save->SimSeconds = SimSeconds;
	Save->NextOrderId = NextOrderId;
	Save->Orders = Orders;
	Save->Upgrades = Upgrades;
	Save->Achievements = Achievements;
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
		if (T.Total >= 1000) { Unlock(TEXT("big_sale")); break; }
	}
	for (const FTSSOrder& O : Orders)
	{
		if (O.Status == ETSSOrderStatus::Received) { Unlock(TEXT("first_delivery")); break; }
	}
}

// ---- orders ---------------------------------------------------------------------------------------

int32 UTSSBusinessSubsystem::OrderCost(const TArray<FTSSOrderLine>& Lines) const
{
	int32 Cost = 0;
	for (const FTSSOrderLine& L : Lines) Cost += TSSProductDef(L.Product).WholesaleCost * L.Qty;
	return Cost;
}

float UTSSBusinessSubsystem::EtaFor(int32 Units) const
{
	float Eta = FMath::Min(40.f + 6.f * Units, 120.f);
	if (HasUpgrade(TEXT("supplier_fast"))) Eta *= 0.5f;
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
		const FTSSProductDef& Def = TSSProductDef(L.Product);
		if (Def.MinLevel > GetLevel())
		{
			Why = FString::Printf(TEXT("%s is not available yet: reach business level %d."), Def.Name, Def.MinLevel);
			return false;
		}
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

// ---- clock ----------------------------------------------------------------------------------------

void UTSSBusinessSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!Store.IsValid() || Store->IsSimPaused()) return;

	SimSeconds += DeltaTime;
	const int32 NewDay = 1 + FMath::FloorToInt(SimSeconds / DayLengthSeconds);
	if (NewDay > Store->GetDay()) Store->StartDay(NewDay);

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

	AutosaveTimer += DeltaTime;
	if (AutosaveTimer > 30.f)
	{
		AutosaveTimer = 0.f;
		SaveNow();
	}
}
