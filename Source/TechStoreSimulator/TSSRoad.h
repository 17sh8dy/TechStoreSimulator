#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TSSRoad.generated.h"

class USplineComponent;

/**
 * A road: a spline the delivery truck drives along. Edit the spline points to reshape it.
 *
 * The visible road is a simple placeholder (asphalt, kerbs, a dashed centre line) so there is something to drive on.
 * To use a road from Fab instead: place the Fab road meshes in the map along this spline, move the spline points so
 * the line follows the Fab road, and turn OFF "Build Placeholder". The truck only cares about the spline.
 *
 * The truck drives with the spline's direction as "forward" and keeps to the right-hand lane (LaneOffset to the
 * right of the spline). Put the spline so that "right" is the side facing the shop.
 */
UCLASS()
class ATSSRoad : public AActor
{
	GENERATED_BODY()

public:
	ATSSRoad();
	virtual void OnConstruction(const FTransform& Transform) override;

	/** The first road in the world, or null. */
	static ATSSRoad* Find(UWorld* World);

	float GetLength() const;
	/** A point `Offset` cm to the right of the road centre at this distance (right is the shop side). */
	FVector PointAt(float Distance, float Offset) const;
	/** Facing (yaw only) of the road at this distance, in the direction of the spline. */
	FRotator YawAt(float Distance) const { return LaneRotation(Distance); }
	float GetLaneOffset() const { return LaneOffset; }
	/** Distance along the road of the crosswalk. */
	float GetCrosswalkDistance() const;
	/** The truck's lane centre at this distance along the road. */
	FVector LaneLocation(float Distance) const;
	/** Facing (yaw only) of travel at this distance. */
	FRotator LaneRotation(float Distance) const;
	/** Distance along the road of the point closest to a world location. */
	float DistanceNearest(const FVector& World) const;
	/** Sideways distance from the truck's lane centre to a world point (for checks). */
	float LaneError(const FVector& World) const;

	/** Street lamps on or off (the day/night cycle switches them at dusk and dawn). */
	void SetLampsOn(bool bOn);
	bool AreLampsOn() const { return bLampsOn; }

	/** Rebuilds the placeholder (callable from editor scripts). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Road") void Rebuild();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road") bool bBuildPlaceholder = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road") float RoadWidth = 800.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road") float SidewalkWidth = 380.f;
	/** How far right of the spline the truck drives (right-hand traffic). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road") float LaneOffset = 200.f;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USplineComponent> Spline;
	UPROPERTY(Transient) TArray<TObjectPtr<UActorComponent>> Generated;
	UPROPERTY(Transient) TArray<TObjectPtr<class UPointLightComponent>> LampLights;
	bool bLampsOn = false;
};
