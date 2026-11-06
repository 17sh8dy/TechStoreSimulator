#include "TSSRoad.h"
#include "TSSProducts.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"

ATSSRoad::ATSSRoad()
{
	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	SetRootComponent(Spline);
	// A straight 60 m default; the map script and the editor move the points.
	Spline->SetSplinePoints({ FVector(0.f, 0.f, 0.f), FVector(6000.f, 0.f, 0.f) }, ESplineCoordinateSpace::Local);
}

ATSSRoad* ATSSRoad::Find(UWorld* World)
{
	for (TActorIterator<ATSSRoad> It(World); It; ++It) return *It;
	return nullptr;
}

float ATSSRoad::GetLength() const { return Spline->GetSplineLength(); }

FRotator ATSSRoad::LaneRotation(float Distance) const
{
	const FVector T = Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
	return FRotator(0.f, T.Rotation().Yaw, 0.f);
}

FVector ATSSRoad::LaneLocation(float Distance) const
{
	const FVector P = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
	const FVector T = Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World).GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, T);   // X forward, Y right
	return P + Right * LaneOffset;
}

float ATSSRoad::DistanceNearest(const FVector& World) const
{
	const float Key = Spline->FindInputKeyClosestToWorldLocation(World);
	return Spline->GetDistanceAlongSplineAtSplineInputKey(Key);
}

float ATSSRoad::LaneError(const FVector& World) const
{
	const float D = DistanceNearest(World);
	return FVector::Dist2D(LaneLocation(D), World);
}

void ATSSRoad::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void ATSSRoad::Rebuild()
{
	for (UActorComponent* C : Generated) if (C) C->DestroyComponent();
	Generated.Reset();
	if (!bBuildPlaceholder) return;

	UStaticMesh* Cube = TSSCubeMesh();
	const int32 Points = Spline->GetNumberOfSplinePoints();

	// A strip along the whole spline, `Offset` to the right of it, `Width` wide, `Thick` thick, centred at height `Z`.
	auto Strip = [&](float Offset, float Width, float Thick, float Z, const FLinearColor& Color)
	{
		for (int32 i = 0; i + 1 < Points; ++i)
		{
			USplineMeshComponent* M = NewObject<USplineMeshComponent>(this);
			M->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			M->SetMobility(EComponentMobility::Movable);
			M->SetStaticMesh(Cube);
			M->SetForwardAxis(ESplineMeshAxis::X);
			M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			M->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);

			auto At = [&](int32 Index, FVector& Pos, FVector& Tan)
			{
				const FVector P = Spline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::Local);
				FVector T = Spline->GetTangentAtSplinePoint(Index, ESplineCoordinateSpace::Local);
				const FVector Dir = T.GetSafeNormal2D();
				Pos = P + FVector::CrossProduct(FVector::UpVector, Dir) * Offset + FVector(0.f, 0.f, Z);
				Tan = (i == Index) ? Spline->GetLeaveTangentAtSplinePoint(Index, ESplineCoordinateSpace::Local)
				                   : Spline->GetArriveTangentAtSplinePoint(Index, ESplineCoordinateSpace::Local);
			};
			FVector P0, T0, P1, T1;
			At(i, P0, T0);
			At(i + 1, P1, T1);
			M->SetStartAndEnd(P0, T0, P1, T1);
			M->SetStartScale(FVector2D(Width / 100.f, Thick / 100.f));
			M->SetEndScale(FVector2D(Width / 100.f, Thick / 100.f));
			M->RegisterComponent();
			TSSApplyColor(M, Color);
			Generated.Add(M);
		}
	};

	Strip(0.f, RoadWidth, 4.f, 0.f, FLinearColor(0.09f, 0.09f, 0.1f));                                        // asphalt
	const float Walk = RoadWidth * 0.5f + SidewalkWidth * 0.5f;
	Strip(Walk, SidewalkWidth, 14.f, 5.f, FLinearColor(0.58f, 0.58f, 0.6f));                                    // sidewalks
	Strip(-Walk, SidewalkWidth, 14.f, 5.f, FLinearColor(0.58f, 0.58f, 0.6f));

	// Dashed centre line.
	const float Len = Spline->GetSplineLength();
	for (float D = 200.f; D + 150.f < Len; D += 700.f)
	{
		UStaticMeshComponent* Dash = NewObject<UStaticMeshComponent>(this);
		Dash->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		Dash->SetMobility(EComponentMobility::Movable);
		Dash->SetStaticMesh(Cube);
		Dash->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Dash->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);
		Dash->SetWorldLocation(Spline->GetLocationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World) + FVector(0.f, 0.f, 2.4f));
		Dash->SetWorldRotation(FRotator(0.f, Spline->GetDirectionAtDistanceAlongSpline(D, ESplineCoordinateSpace::World).Rotation().Yaw, 0.f));
		Dash->SetWorldScale3D(FVector(3.f, 0.16f, 0.004f));
		Dash->RegisterComponent();
		TSSApplyColor(Dash, FLinearColor(0.9f, 0.82f, 0.25f));
		Generated.Add(Dash);
	}

	// ---- crosswalk (zebra stripes across the road) and street lamps ----
	auto AddBox = [&](FVector WorldCentre, FRotator Rot, FVector SizeCm, const FLinearColor& Color)
	{
		UStaticMeshComponent* P = NewObject<UStaticMeshComponent>(this);
		P->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		P->SetMobility(EComponentMobility::Movable);
		P->SetStaticMesh(Cube);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);
		P->SetWorldLocation(WorldCentre);
		P->SetWorldRotation(Rot);
		P->SetWorldScale3D(SizeCm / 100.f);
		P->RegisterComponent();
		TSSApplyColor(P, Color);
		Generated.Add(P);
	};
	static UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// The crosswalk sits a little before the shop so the parked truck never covers it.
	const float CrossAt = FMath::Clamp(Len * 0.5f - 500.f, 300.f, Len - 300.f);
	{
		const FVector Mid = Spline->GetLocationAtDistanceAlongSpline(CrossAt, ESplineCoordinateSpace::World);
		const FVector Fwd = Spline->GetDirectionAtDistanceAlongSpline(CrossAt, ESplineCoordinateSpace::World).GetSafeNormal2D();
		const FVector Rgt = FVector::CrossProduct(FVector::UpVector, Fwd);
		const FRotator Rot(0.f, Fwd.Rotation().Yaw, 0.f);
		for (float Off = -RoadWidth * 0.5f + 40.f; Off < RoadWidth * 0.5f - 20.f; Off += 85.f)
		{
			AddBox(Mid + Rgt * Off + FVector(0.f, 0.f, 2.3f), Rot, FVector(320.f, 45.f, 0.5f), FLinearColor(0.9f, 0.9f, 0.88f));
		}
	}

	const FLinearColor Pole(0.14f, 0.15f, 0.17f), Lamp(1.f, 0.93f, 0.7f);
	int32 Count = 0;
	for (float D = 500.f; D < Len - 200.f; D += 1300.f, ++Count)
	{
		const FVector Pt = Spline->GetLocationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
		const FVector Fwd = Spline->GetDirectionAtDistanceAlongSpline(D, ESplineCoordinateSpace::World).GetSafeNormal2D();
		const FVector Rgt = FVector::CrossProduct(FVector::UpVector, Fwd);
		const float Side = (Count % 2 == 0) ? 1.f : -1.f;   // alternate sides of the road
		const FVector Base = Pt + Rgt * Side * (RoadWidth * 0.5f + 50.f);
		const FRotator Rot(0.f, Fwd.Rotation().Yaw, 0.f);

		UStaticMeshComponent* Post = NewObject<UStaticMeshComponent>(this);
		Post->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		Post->SetMobility(EComponentMobility::Movable);
		Post->SetStaticMesh(Cylinder);
		Post->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Post->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);
		Post->SetWorldLocation(Base + FVector(0.f, 0.f, 300.f));
		Post->SetWorldScale3D(FVector(0.16f, 0.16f, 6.f));
		Post->RegisterComponent();
		TSSApplyColor(Post, Pole);
		Generated.Add(Post);
		// Arm reaching over the road, with the lamp head at its end.
		AddBox(Base + FVector(0.f, 0.f, 598.f) - Rgt * Side * 70.f, Rot, FVector(10.f, 170.f, 10.f), Pole);
		AddBox(Base + FVector(0.f, 0.f, 590.f) - Rgt * Side * 150.f, Rot, FVector(60.f, 28.f, 12.f), Lamp);
	}
}
