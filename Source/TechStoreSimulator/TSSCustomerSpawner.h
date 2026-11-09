#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TSSCustomerSpawner.generated.h"

class ATSSCustomer;
class ATSSRegister;

/**
 * Sends shoppers to the shop. Quick, basic version: every few seconds (a random gap, shortened by the business's
 * customer-rate upgrades) it may spawn one customer at the spawn point, as long as the shop is stocked, fewer than
 * MaxCustomers are inside, and the caller says spawning is allowed (shop open, not paused).
 *
 * Place one in a map to move the spawn point or change the numbers; if the map has none, UTSSCustomerSubsystem
 * creates one at the register's SpawnPoint. The subsystem ticks it (`TickSpawn`) so it obeys the same open/closed rules.
 */
UCLASS()
class ATSSCustomerSpawner : public AActor
{
	GENERATED_BODY()

public:
	ATSSCustomerSpawner();

	/** Counts the timer down and spawns when it reaches zero (and the checks pass). Returns the new customer or null. */
	ATSSCustomer* TickSpawn(float DeltaTime);
	/** Spawns one customer now, ignoring gaps/limits. Null if the map has no register. */
	ATSSCustomer* SpawnCustomer(float SpeedScale = 1.f);

	/** Use the spawner's own location instead of the register's SpawnPoint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner") bool bUseOwnLocation = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner") int32 MaxCustomers = 3;
	/** Seconds between spawn attempts, random between these two. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner") float MinGap = 9.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner") float MaxGap = 20.f;
	/** Nobody comes in to an empty shop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner") bool bNeedStock = true;

	/** The spawner for this world, or null. */
	static ATSSCustomerSpawner* Find(UWorld* World);
	/** The spawner for this world, creating one at the register's SpawnPoint if there is none. */
	static ATSSCustomerSpawner* FindOrCreate(UWorld* World);

private:
	float NextSpawnIn = 5.f;
};
