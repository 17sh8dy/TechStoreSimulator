#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TSSPasserby.generated.h"

class UTSSHumanComponent;

/** A person walking along the pavement (not a customer). Walks a short path, sometimes stops to look at the shop window, then is gone. */
UCLASS()
class ATSSPasserby : public AActor
{
	GENERATED_BODY()

public:
	ATSSPasserby();
	virtual void Tick(float DeltaSeconds) override;

	/** Casts a random person and starts them along Path (feet height already in the points). If PauseAt is a valid index they stop there for PauseSeconds, looking at LookTarget. */
	void Begin(const TArray<FVector>& InPath, float InSpeed, int32 PauseAt = INDEX_NONE, float PauseSeconds = 0.f, const FVector& LookTarget = FVector::ZeroVector);

	bool IsPaused() const { return PauseLeft > 0.f; }
	FVector GetGoal() const { return Path.IsValidIndex(Index) ? Path[Index] : GetActorLocation(); }

private:
	UPROPERTY() TObjectPtr<UTSSHumanComponent> Human;
	TArray<FVector> Path;
	int32 Index = 0;
	float Speed = 140.f;
	int32 PauseIndex = INDEX_NONE;
	float PauseLeft = 0.f;
	float PauseTotal = 0.f;
	bool bPauseDone = false;
	FVector Look = FVector::ZeroVector;
};
