#include "TSSCustomerSubsystem.h"
#include "TSSCustomer.h"
#include "TSSCustomerSpawner.h"
#include "TSSStoreSubsystem.h"

bool UTSSCustomerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return Super::ShouldCreateSubsystem(Outer) && World && World->IsGameWorld();
}

ATSSCustomerSpawner* UTSSCustomerSubsystem::GetSpawner()
{
	return ATSSCustomerSpawner::FindOrCreate(GetWorld());
}

ATSSCustomer* UTSSCustomerSubsystem::SpawnCustomer(float SpeedScale)
{
	ATSSCustomerSpawner* S = GetSpawner();
	return S ? S->SpawnCustomer(SpeedScale) : nullptr;
}

bool UTSSCustomerSubsystem::MaySpawn() const
{
	const UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	return bAutoSpawn && Store && Store->IsOpen() && !Store->IsSimPaused();
}

void UTSSCustomerSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!MaySpawn()) return;
	if (ATSSCustomerSpawner* S = GetSpawner()) S->TickSpawn(DeltaTime);
}
