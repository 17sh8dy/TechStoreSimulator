#include "TSSCustomerSubsystem.h"
#include "TSSCustomer.h"
#include "TSSRegister.h"
#include "TSSShelf.h"
#include "TSSStoreSubsystem.h"
#include "TSSBusinessSubsystem.h"
#include "EngineUtils.h"

bool UTSSCustomerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return Super::ShouldCreateSubsystem(Outer) && World && World->IsGameWorld();
}

ATSSCustomer* UTSSCustomerSubsystem::SpawnCustomer(float SpeedScale)
{
	UWorld* World = GetWorld();
	ATSSRegister* Register = nullptr;
	for (TActorIterator<ATSSRegister> It(World); It; ++It) { Register = *It; break; }
	if (!Register) return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATSSCustomer* C = World->SpawnActor<ATSSCustomer>(ATSSCustomer::StaticClass(), FTransform(Register->SpawnPoint + FVector(0, 0, 90)), Params);
	if (C) C->Begin(Register, SpeedScale);
	return C;
}

bool UTSSCustomerSubsystem::MaySpawn() const
{
	const UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	return bAutoSpawn && Store && Store->IsOpen() && !Store->IsSimPaused();
}

void UTSSCustomerSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	if (!MaySpawn()) return;

	NextSpawnIn -= DeltaTime;
	if (NextSpawnIn > 0.f) return;
	float RateScale = 1.f;
	if (const UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>()) RateScale = Biz->CustomerRateMultiplier();
	NextSpawnIn = FMath::FRandRange(MinGap, MaxGap) / FMath::Max(0.1f, RateScale);

	int32 Count = 0;
	for (TActorIterator<ATSSCustomer> It(GetWorld()); It; ++It) ++Count;
	if (Count >= MaxCustomers) return;

	// Nobody comes in to an empty shop.
	TArray<FTSSStockRef> Stock;
	ATSSShelf::CollectStock(GetWorld(), Stock);
	if (Stock.Num() == 0) return;

	SpawnCustomer();
}
