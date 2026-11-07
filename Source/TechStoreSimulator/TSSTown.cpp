#include "TSSTown.h"
#include "TSSRoad.h"
#include "TSSDayNight.h"
#include "TSSDeliveryActors.h"
#include "TSSStoreSubsystem.h"
#include "EngineUtils.h"

namespace
{
	const FLinearColor CarPaints[] = {
		FLinearColor(0.80f, 0.80f, 0.78f), FLinearColor(0.80f, 0.80f, 0.78f), FLinearColor(0.35f, 0.36f, 0.38f), FLinearColor(0.03f, 0.03f, 0.035f),
		FLinearColor(0.5f, 0.52f, 0.55f), FLinearColor(0.55f, 0.03f, 0.03f), FLinearColor(0.03f, 0.1f, 0.35f), FLinearColor(0.02f, 0.18f, 0.1f),
		FLinearColor(0.55f, 0.45f, 0.3f), FLinearColor(0.8f, 0.3f, 0.04f), FLinearColor(0.12f, 0.2f, 0.4f) };
}

bool UTSSTownSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	if (!Super::ShouldCreateSubsystem(Outer) || !World || !World->IsGameWorld()) return false;
	return !FParse::Param(FCommandLine::Get(), TEXT("TSSNoTown"));
}

void UTSSTownSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	Road = ATSSRoad::Find(&InWorld);
	if (FParse::Param(FCommandLine::Get(), TEXT("TSSNoTraffic"))) bTraffic = false;
	RefreshNight();
}

int32 UTSSTownSubsystem::CarCount() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ATSSTrafficCar>& C : Cars) if (C.IsValid()) ++N;
	return N;
}

ATSSTrafficCar* UTSSTownSubsystem::SpawnCar(ETSSCarType Type, bool bAlongRoad, float Distance, float Cruise, const FLinearColor& Paint)
{
	ATSSRoad* R = Road.Get();
	if (!R) return nullptr;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATSSTrafficCar* Car = GetWorld()->SpawnActor<ATSSTrafficCar>(ATSSTrafficCar::StaticClass(), FTransform::Identity, P);
	if (!Car) return nullptr;
	Car->Build(Type, Paint);
	Car->Drive(R, bAlongRoad, Distance, Cruise);
	Car->SetNight(bNight);
	Cars.Add(Car);
	return Car;
}

void UTSSTownSubsystem::ClearTraffic()
{
	for (TWeakObjectPtr<ATSSTrafficCar>& C : Cars) if (C.IsValid()) C->Destroy();
	Cars.Reset();
}

void UTSSTownSubsystem::RefreshNight()
{
	for (TActorIterator<ATSSDayNight> It(GetWorld()); It; ++It) { bNight = It->IsNight(); break; }
	for (TWeakObjectPtr<ATSSTrafficCar>& C : Cars) if (C.IsValid() && C->IsNight() != bNight) C->SetNight(bNight);
}

float UTSSTownSubsystem::AllowedSpeed(const ATSSTrafficCar& Car) const
{
	const ATSSRoad* R = Road.Get();
	float Allowed = 5000.f;
	if (!R) return Allowed;
	const bool bFwd = Car.GoesAlongRoad();
	const float Dir = bFwd ? 1.f : -1.f;

	// Keep a gap behind whatever is ahead in the same lane.
	auto Follow = [&](float OtherDist, float OtherLen, float OtherSpeed)
	{
		const float Ahead = (OtherDist - Car.GetDistance()) * Dir;
		if (Ahead <= 0.f) return;
		const float Gap = Ahead - 0.5f * (Car.GetLength() + OtherLen) - 140.f;
		if (Gap > 1600.f) return;
		Allowed = FMath::Min(Allowed, Gap <= 0.f ? 0.f : FMath::Min(OtherSpeed + Gap * 0.9f, Gap * 2.f));
	};
	for (const TWeakObjectPtr<ATSSTrafficCar>& O : Cars)
	{
		const ATSSTrafficCar* Other = O.Get();
		if (!Other || Other == &Car || Other->GoesAlongRoad() != bFwd) continue;
		Follow(Other->GetDistance(), Other->GetLength(), Other->GetSpeed());
	}
	if (bFwd)
	{
		// The delivery truck keeps to the same lane; wait for it rather than drive through it.
		for (TActorIterator<ATSSDeliveryTruck> It(GetWorld()); It; ++It)
		{
			Follow(R->DistanceNearest(It->GetActorLocation()), 900.f, 0.f);
		}
	}

	// Stop short of the crosswalk while someone is on it.
	if (bCrosswalkBusy)
	{
		const float Stop = R->GetCrosswalkDistance() - Dir * 160.f;
		const float ToStop = (Stop - Car.GetDistance()) * Dir - Car.GetLength() * 0.5f;
		if (ToStop > -20.f && ToStop < 1200.f) Allowed = FMath::Min(Allowed, FMath::Max(0.f, ToStop * 1.6f));
	}
	return Allowed;
}

void UTSSTownSubsystem::TickTraffic(float Dt)
{
	const ATSSRoad* R = Road.Get();
	if (!R || !bTraffic) return;
	const UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	if (Store && Store->IsSimPaused()) return;

	Cars.RemoveAll([](const TWeakObjectPtr<ATSSTrafficCar>& C) { return !C.IsValid(); });
	const float Len = R->GetLength();
	for (int32 Dir = 0; Dir < 2; ++Dir)
	{
		NextSpawn[Dir] -= Dt;
		if (NextSpawn[Dir] > 0.f) continue;
		NextSpawn[Dir] = FMath::FRandRange(6.f, 15.f);
		if (CarCount() >= 8) continue;
		const bool bAlong = Dir == 0;
		const float Start = bAlong ? -450.f : Len + 450.f;
		// Never spawn on top of another car or a waiting truck.
		bool bClear = true;
		for (const TWeakObjectPtr<ATSSTrafficCar>& C : Cars)
		{
			if (C.IsValid() && C->GoesAlongRoad() == bAlong && FMath::Abs(C->GetDistance() - Start) < 1300.f) bClear = false;
		}
		if (bAlong) for (TActorIterator<ATSSDeliveryTruck> It(GetWorld()); It; ++It) { bClear = false; break; }   // the truck has the lane to itself
		if (!bClear) continue;

		const float Roll = FMath::FRand();
		const ETSSCarType Type = Roll < 0.40f ? ETSSCarType::Sedan : Roll < 0.68f ? ETSSCarType::Hatchback : Roll < 0.92f ? ETSSCarType::Suv : ETSSCarType::Van;
		SpawnCar(Type, bAlong, Start, FMath::FRandRange(760.f, 1150.f), CarPaints[FMath::RandRange(0, UE_ARRAY_COUNT(CarPaints) - 1)]);
	}
}

void UTSSTownSubsystem::Tick(float Dt)
{
	Super::Tick(Dt);
	NightCheck -= Dt;
	if (NightCheck <= 0.f) { NightCheck = 1.f; RefreshNight(); }
	TickTraffic(Dt);
}
