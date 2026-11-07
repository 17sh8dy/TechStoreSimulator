#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TSSTrafficCar.generated.h"

class UProceduralMeshComponent;
class USpotLightComponent;
class ATSSRoad;

/** The shape a car is built in. */
UENUM()
enum class ETSSCarType : uint8 { Sedan, Hatchback, Suv, Van };

/**
 * A passing car. The whole model is built from code (a lofted body, glass, pillars, grille, lights, mirrors and four separate
 * wheels with rims and spokes), so every colour and shape is free and nothing is imported. It drives along the road in its
 * lane, slows for the car in front, stops for people on the crosswalk, and puts its headlights on at night.
 *
 * Local frame: +X forward, +Y right, +Z up, origin on the ground in the middle.
 */
UCLASS()
class ATSSTrafficCar : public AActor
{
	GENERATED_BODY()

public:
	ATSSTrafficCar();
	virtual void Tick(float DeltaSeconds) override;

	/** Builds the model. Call once, right after spawning. */
	void Build(ETSSCarType InType, const FLinearColor& Paint);
	/** Puts the car on the road: which way it drives, where it starts, and how fast it likes to go (cm/s). */
	void Drive(ATSSRoad* InRoad, bool bAlongRoad, float StartDistance, float Cruise);

	void SetNight(bool bNight);
	bool IsNight() const { return bNightNow; }

	/** Distance travelled along the road, and which way. */
	float GetDistance() const { return Distance; }
	bool GoesAlongRoad() const { return bForward; }
	float GetSpeed() const { return Speed; }
	float GetLength() const { return Length; }
	ETSSCarType GetCarType() const { return Type; }
	int32 NumTriangles() const { return Triangles; }
	/** A stopped car (waiting at the crosswalk, in a queue). */
	bool IsStopped() const { return Speed < 5.f; }

	/** Where this type of car's wheels sit, for the model and for tests. */
	static void Dimensions(ETSSCarType Type, float& L, float& W, float& H);

private:
	void BuildWheel(UProceduralMeshComponent* Wheel, float Radius, float Width, bool bRight);
	void UpdatePose(float Dt);

	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Body;
	UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Wheels;
	UPROPERTY() TObjectPtr<USpotLightComponent> Headlight;

	UPROPERTY() TObjectPtr<ATSSRoad> Road;
	ETSSCarType Type = ETSSCarType::Sedan;
	FLinearColor PaintColor = FLinearColor::White;
	float Length = 470.f, Width = 182.f, Height = 146.f, WheelRadius = 33.f, WheelX = 150.f;
	int32 Triangles = 0;

	bool bForward = true;
	float Distance = 0.f;
	float Speed = 0.f;
	float Cruise = 900.f;
	float WheelSpin = 0.f;
	bool bNightNow = false;
	bool bBraking = false;
	bool bBuilt = false;
};
