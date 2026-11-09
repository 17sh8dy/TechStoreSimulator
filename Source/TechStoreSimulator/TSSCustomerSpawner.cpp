#include "TSSCustomerSpawner.h"
#include "TSSCustomer.h"
#include "TSSRegister.h"
#include "TSSShelf.h"
#include "TSSBusinessSubsystem.h"
#include "EngineUtils.h"

ATSSCustomerSpawner::ATSSCustomerSpawner()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

ATSSCustomerSpawner* ATSSCustomerSpawner::Find(UWorld* World)
{
	if (!World) return nullptr;
	for (TActorIterator<ATSSCustomerSpawner> It(World); It; ++It) return *It;
	return nullptr;
}

ATSSCustomerSpawner* ATSSCustomerSpawner::FindOrCreate(UWorld* World)
{
	if (ATSSCustomerSpawner* S = Find(World)) return S;
	if (!World) return nullptr;
	FVector At = FVector::ZeroVector;
	for (TActorIterator<ATSSRegister> It(World); It; ++It) { At = It->SpawnPoint; break; }
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ATSSCustomerSpawner>(ATSSCustomerSpawner::StaticClass(), FTransform(At), P);
}

ATSSCustomer* ATSSCustomerSpawner::SpawnCustomer(float SpeedScale)
{
	UWorld* World = GetWorld();
	ATSSRegister* Register = nullptr;
	for (TActorIterator<ATSSRegister> It(World); It; ++It) { Register = *It; break; }
	if (!Register) return nullptr;

	const FVector At = bUseOwnLocation ? GetActorLocation() : Register->SpawnPoint;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATSSCustomer* C = World->SpawnActor<ATSSCustomer>(ATSSCustomer::StaticClass(), FTransform(At + FVector(0, 0, 90)), Params);
	if (C) C->Begin(Register, SpeedScale);
	return C;
}

ATSSCustomer* ATSSCustomerSpawner::TickSpawn(float DeltaTime)
{
	NextSpawnIn -= DeltaTime;
	if (NextSpawnIn > 0.f) return nullptr;
	float RateScale = 1.f;
	if (const UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>()) RateScale = Biz->CustomerRateMultiplier();
	NextSpawnIn = FMath::FRandRange(MinGap, FMath::Max(MinGap, MaxGap)) / FMath::Max(0.1f, RateScale);

	int32 Count = 0;
	for (TActorIterator<ATSSCustomer> It(GetWorld()); It; ++It) ++Count;
	if (Count >= MaxCustomers) return nullptr;

	if (bNeedStock)
	{
		TArray<FTSSStockRef> Stock;
		ATSSShelf::CollectStock(GetWorld(), Stock);
		if (Stock.Num() == 0) return nullptr;
	}
	return SpawnCustomer();
}
