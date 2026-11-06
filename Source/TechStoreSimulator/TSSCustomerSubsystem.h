#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TSSCustomerSubsystem.generated.h"

class ATSSCustomer;

/** Sends shoppers through the door while the shop has something on its shelves. */
UCLASS()
class UTSSCustomerSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTSSCustomerSubsystem, STATGROUP_Tickables); }

	/** Spawns one customer at the register's door right now. Null if the map has no register. */
	ATSSCustomer* SpawnCustomer(float SpeedScale = 1.f);

	/** True when the spawner is allowed to send someone in: automatic spawning on, shop open, no menu up. */
	bool MaySpawn() const;

	/** Off in the self-test so it can control exactly who walks in. */
	bool bAutoSpawn = true;
	int32 MaxCustomers = 3;
	float MinGap = 9.f;
	float MaxGap = 20.f;

private:
	float NextSpawnIn = 5.f;
};
