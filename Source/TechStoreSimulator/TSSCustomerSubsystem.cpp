#include "TSSCustomerSubsystem.h"
#include "TSSCustomer.h"
#include "TSSRegister.h"
#include "TSSShelf.h"
#include "TSSStoreSubsystem.h"
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

void UTSSCustomerSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	if (!bAutoSpawn || !Store || Store->IsSimPaused()) return;

	NextSpawnIn -= DeltaTime;
	if (NextSpawnIn > 0.f) return;
	NextSpawnIn = FMath::FRandRange(MinGap, MaxGap);

	int32 Count = 0;
	for (TActorIterator<ATSSCustomer> It(GetWorld()); It; ++It) ++Count;
	if (Count >= MaxCustomers) return;

	// Nobody comes in to an empty shop.
	TArray<FTSSStockRef> Stock;
	ATSSShelf::CollectStock(GetWorld(), Stock);
	if (Stock.Num() == 0) return;

	SpawnCustomer();
}
