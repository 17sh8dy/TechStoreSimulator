#include "TSSCustomerSpawner.h"
#include "TSSCustomer.h"
#include "TSSRegister.h"
#include "TSSShelf.h"
#include "TSSRoad.h"
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

ATSSCustomer* ATSSCustomerSpawner::SpawnCustomer(float SpeedScale, bool bFromStreet)
{
	UWorld* World = GetWorld();
	ATSSRegister* Register = nullptr;
	for (TActorIterator<ATSSRegister> It(World); It; ++It) { Register = *It; break; }
	if (!Register) return nullptr;

	const FVector At = bUseOwnLocation ? GetActorLocation() : Register->SpawnPoint;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FRotator Facing = bUseOwnLocation ? FRotator(0.f, GetActorRotation().Yaw, 0.f) : FRotator::ZeroRotator;   // a placed spawner decides which way shoppers face
	ATSSCustomer* C = World->SpawnActor<ATSSCustomer>(ATSSCustomer::StaticClass(), FTransform(Facing, At + FVector(0, 0, 90)), Params);
	if (!C) return nullptr;
	// Shoppers walk in from along the street instead of appearing at the door: they start at one end of the shop-side pavement,
	// come up to the spawner's spot just outside, and (when they leave) walk off to either end.
	if (const ATSSRoad* Road = bFromStreet ? ATSSRoad::Find(World) : nullptr)
	{
		const float Len = Road->GetLength();
		auto EndPoint = [&](bool bFar)
		{
			FVector V = Road->PointAt(bFar ? Len - 80.f : 80.f, Road->RoadWidth * 0.5f + FMath::FRandRange(130.f, 250.f));
			V.Z = 12.f;
			return V;
		};
		const bool bFrom = FMath::FRand() < 0.5f;
		const bool bTo = FMath::FRand() < 0.5f;
		C->SetStreetRoute(At, EndPoint(bFrom), EndPoint(bTo));
	}
	if (!bFromStreet) C->WalkSpeed = 140.f;   // test / debug spawns keep the old slow pace: the self-test runs them at 8x and times door swings against it
	C->Begin(Register, SpeedScale);
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
	return SpawnCustomer(1.f, true);
}
