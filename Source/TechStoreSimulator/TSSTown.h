#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TSSTrafficCar.h"
#include "TSSTown.generated.h"

class ATSSRoad;
class ATSSPasserby;

/**
 * Everything that makes the street outside feel lived in, spawned from code at the start of play so the saved map is
 * never touched: traffic on the road, and (as they are added) people on the pavements and the street furniture.
 * One per world. It only exists in a world that has a TSSRoad.
 */
UCLASS()
class UTSSTownSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTSSTownSubsystem, STATGROUP_Tickables); }

	// ---- people on the pavements ----
	/** Someone walking the whole length of a pavement (shop side or far side), optionally stopping to look in the shop window. */
	ATSSPasserby* SpawnPasserby(bool bShopSide, bool bForward, bool bPauseAtShop);
	int32 PasserbyCount() const;
	void SetPeopleEnabled(bool bOn) { bPeople = bOn; }

	// ---- traffic ----
	/** How fast this car may go right now given the traffic ahead, a delivery truck in its lane, and people on the crosswalk. */
	float AllowedSpeed(const ATSSTrafficCar& Car) const;
	const TArray<TWeakObjectPtr<ATSSTrafficCar>>& GetCars() const { return Cars; }
	int32 CarCount() const;
	ATSSTrafficCar* SpawnCar(ETSSCarType Type, bool bAlongRoad, float Distance, float Cruise, const FLinearColor& Paint);
	/** Test / screenshot hooks. */
	void SetTrafficEnabled(bool bOn) { bTraffic = bOn; }
	void ClearTraffic();
	bool IsNight() const { return bNight; }
	/** Set by whatever is crossing the road, so cars wait for it. */
	void SetCrosswalkBusy(bool bBusy) { bCrosswalkBusy = bBusy; }
	bool IsCrosswalkBusy() const { return bCrosswalkBusy; }

private:
	void TickTraffic(float Dt);
	void TickPeople(float Dt);
	void RefreshNight();

	TWeakObjectPtr<ATSSRoad> Road;
	TArray<TWeakObjectPtr<ATSSTrafficCar>> Cars;
	float NextSpawn[2] = { 2.f, 6.f };   // seconds until the next car in each direction
	float NightCheck = 0.f;
	TArray<TWeakObjectPtr<ATSSPasserby>> People;
	float NextPerson = 3.f;
	bool bPeople = true;
	bool bTraffic = true;
	bool bNight = false;
	bool bCrosswalkBusy = false;
};
