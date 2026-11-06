#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TSSEmployee.generated.h"

class UTSSHumanComponent;
class ATSSRegister;

/**
 * The hired cashier: a person who stands beside the register and runs it (the register's own timers do the work;
 * this actor only gives them a body, so the sale visibly happens). Spawned and removed by the business subsystem.
 */
UCLASS()
class ATSSEmployee : public AActor
{
	GENERATED_BODY()

public:
	ATSSEmployee();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Where the cashier stands, in the register's local frame (Y is along the counter; +X is the owner's side). */
	static FVector StandOffset() { return FVector(105.f, -95.f, 0.f); }

private:
	UPROPERTY() TObjectPtr<UTSSHumanComponent> Human;
	TWeakObjectPtr<ATSSRegister> Register;
};
