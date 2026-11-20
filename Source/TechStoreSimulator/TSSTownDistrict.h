#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TSSTownDistrict.generated.h"

class ATSSRoad;
class ATSSStreetBuilding;
class ATSSTrafficCar;
class UBoxComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * Layer 3 of the town: the few blocks you can actually walk around. Spawned from code by the town subsystem at the start
 * of play (the saved map is not touched; `-TSSNoDistrict` skips it).
 *
 * Layout, in the road's frame (U along the main street from its middle, V to the right = the shop side):
 *   - the main street carried on to both ends of the town (asphalt, pavements, centre line, trees),
 *   - a cross street at each end (U = +-CrossU) and a back street on each side (V = +-BackV), so the shop sits in a
 *     ring of streets,
 *   - buildings facing every street, a small park behind the shop and a car park across the street, parked cars,
 *   - an invisible floor under the whole town and invisible walls round its edge, so the player can never fall off
 *     the map or wander into the backdrop.
 * The cheap backdrop (ATSSTownBackdrop) takes over beyond the walls.
 */
UCLASS()
class ATSSTownDistrict : public AActor
{
	GENERATED_BODY()

public:
	ATSSTownDistrict();

	/** Half the town's size along the main street and across it (cm, from the middle of the road). The walls stand here. */
	static constexpr float HalfU = 7600.f;
	static constexpr float HalfV = 6400.f;
	/** Where the cross streets and back streets run (centre lines). */
	static constexpr float CrossU = 4300.f;
	static constexpr float BackV = 4700.f;

	void Build(ATSSRoad* Road);

	int32 NumBuildings() const { return Buildings.Num(); }
	int32 NumParkedCars() const { return ParkedCars.Num(); }
	/** True if a world point is inside the walls (on the town side). */
	bool IsInside(const FVector& World) const;
	/** The invisible walls (for tests). */
	const TArray<TObjectPtr<UBoxComponent>>& GetWalls() const { return Walls; }

	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void Inst(UStaticMesh* Mesh, const FLinearColor& Color, const FVector& Pos, const FRotator& Rot, const FVector& SizeCm);
	UBoxComponent* Blocker(const FVector& Centre, const FRotator& Rot, const FVector& HalfSize, bool bFloor);

	UPROPERTY(Transient) TArray<TObjectPtr<UInstancedStaticMeshComponent>> Groups;
	UPROPERTY(Transient) TArray<TObjectPtr<UBoxComponent>> Walls;
	UPROPERTY(Transient) TArray<TObjectPtr<UBoxComponent>> Solids;
	UPROPERTY(Transient) TArray<TObjectPtr<ATSSStreetBuilding>> Buildings;
	UPROPERTY(Transient) TArray<TObjectPtr<ATSSTrafficCar>> ParkedCars;
	TMap<uint32, int32> GroupIndex;

	// The road frame, kept for IsInside().
	FVector Centre = FVector::ZeroVector;
	FVector Fwd = FVector::ForwardVector;
	FVector Right = FVector::RightVector;
};
