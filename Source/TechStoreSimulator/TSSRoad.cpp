#include "TSSRoad.h"
#include "TSSProducts.h"
#include "Components/SplineComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
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

FVector ATSSRoad::PointAt(float Distance, float Offset) const
{
	const FVector P = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
	const FVector T = Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World).GetSafeNormal2D();
	return P + FVector::CrossProduct(FVector::UpVector, T) * Offset;
}

float ATSSRoad::GetCrosswalkDistance() const
{
	const float Len = Spline->GetSplineLength();
	return FMath::Clamp(Len * 0.5f - 500.f, 300.f, Len - 300.f);   // the same spot Rebuild() paints the zebra stripes
}

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

void ATSSRoad::SetLampsOn(bool bOn)
{
	bLampsOn = bOn;
	for (UPointLightComponent* L : LampLights) if (L) L->SetVisibility(bOn);
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
	LampLights.Reset();
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
const float CrossAt = GetCrosswalkDistance();
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

		// The lamp's light, off until the day/night cycle calls for it.
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensityUnits(ELightUnits::Lumens);
		Light->SetIntensity(9000.f);
		Light->SetAttenuationRadius(1300.f);
		Light->SetLightColor(FLinearColor(1.f, 0.82f, 0.55f));
		Light->SetCastShadows(false);
		Light->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);
		Light->SetWorldLocation(Base + FVector(0.f, 0.f, 570.f) - Rgt * Side * 150.f);
		Light->SetVisibility(bLampsOn);
		Light->RegisterComponent();
		Generated.Add(Light);
		LampLights.Add(Light);
	}

	// ---- street kit (layer 2): paving joints, kerb, trees, hydrants, bins, benches, signs. One instanced component per colour. ----
	{
		static UStaticMesh* Ball = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		TMap<uint32, UInstancedStaticMeshComponent*> Groups;
		auto Inst = [&](UStaticMesh* Mesh, const FLinearColor& Color, bool bCollide, const FVector& World, const FRotator& Rot, const FVector& SizeCm)
		{
			const uint32 Key = GetTypeHash(Mesh) ^ (Color.ToFColor(true).DWColor() * 2654435761u) ^ (bCollide ? 0x9e3779b9u : 0u);
			UInstancedStaticMeshComponent*& G = Groups.FindOrAdd(Key);
			if (!G)
			{
				G = NewObject<UInstancedStaticMeshComponent>(this);
				G->CreationMethod = EComponentCreationMethod::UserConstructionScript;
				G->SetMobility(EComponentMobility::Movable);
				G->SetStaticMesh(Mesh);
				G->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
				G->SetCastShadow(true);
				G->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);
				G->RegisterComponent();
				TSSApplyColor(G, Color);
				Generated.Add(G);
			}
			G->AddInstance(FTransform(Rot, World, SizeCm / 100.f), true);
		};
		auto Frame = [&](float D, float Off, FVector& Pos, FVector& Rgt, FRotator& Rot)
		{
			const FVector Pt = Spline->GetLocationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
			const FVector Fwd = Spline->GetDirectionAtDistanceAlongSpline(D, ESplineCoordinateSpace::World).GetSafeNormal2D();
			Rgt = FVector::CrossProduct(FVector::UpVector, Fwd);
			Pos = Pt + Rgt * Off;
			Rot = FRotator(0.f, Fwd.Rotation().Yaw, 0.f);
		};
		const float KerbOff = RoadWidth * 0.5f;
		const float SideMid = KerbOff + SidewalkWidth * 0.5f;

		// Kerb stone along both pavements, and paving joints (slab lines) across and along them.
		const FLinearColor KerbCol(0.72f, 0.72f, 0.7f), Joint(0.33f, 0.33f, 0.35f);
		for (float D = 0.f; D < Len; D += 200.f)
		{
			for (float Side : { 1.f, -1.f })
			{
				FVector P, R; FRotator Rot;
				Frame(D + 100.f, Side * (KerbOff + 6.f), P, R, Rot);
				Inst(Cube, KerbCol, false, P + FVector(0.f, 0.f, 11.f), Rot, FVector(200.f, 14.f, 18.f));
			}
		}
		for (float D = 70.f; D < Len; D += 140.f)
		{
			for (float Side : { 1.f, -1.f })
			{
				FVector P, R; FRotator Rot;
				Frame(D, Side * SideMid, P, R, Rot);
				Inst(Cube, Joint, false, P + FVector(0.f, 0.f, 12.3f), Rot, FVector(2.5f, SidewalkWidth - 20.f, 0.5f));
			}
		}
		for (float Side : { 1.f, -1.f })
			for (float Frac : { -0.25f, 0.25f })
			{
				FVector P, R; FRotator Rot;
				Frame(Len * 0.5f, Side * (SideMid + Frac * SidewalkWidth), P, R, Rot);
				Inst(Cube, Joint, false, P + FVector(0.f, 0.f, 12.3f), Rot, FVector(Len, 2.5f, 0.5f));
			}

		// Trees: a pit, a trunk and a clump of leaves, on both pavements, clear of the lamps and the crosswalk.
		static const FLinearColor Leaves[] = { FLinearColor(0.1f, 0.3f, 0.07f), FLinearColor(0.16f, 0.36f, 0.08f), FLinearColor(0.34f, 0.4f, 0.07f), FLinearColor(0.12f, 0.27f, 0.1f) };
		int32 Idx = 0;
		for (float D = 1150.f; D < Len - 500.f; D += 1300.f, ++Idx)
		{
			if (FMath::Abs(D - CrossAt) < 400.f) continue;
			for (float Side : { 1.f, -1.f })
			{
				FVector P, R; FRotator Rot;
				Frame(D + (Side < 0.f ? 260.f : 0.f), Side * (KerbOff + 70.f), P, R, Rot);
				const FVector Base = P + FVector(0.f, 0.f, 12.f);
				const float H = 360.f + 60.f * ((Idx + (Side < 0.f ? 1 : 0)) % 3);
				Inst(Cube, FLinearColor(0.16f, 0.13f, 0.11f), false, Base + FVector(0.f, 0.f, 1.f), Rot, FVector(110.f, 110.f, 3.f));
				Inst(Cylinder, FLinearColor(0.2f, 0.14f, 0.09f), true, Base + FVector(0.f, 0.f, H * 0.5f), Rot, FVector(22.f, 22.f, H));
				const FLinearColor Leaf = Leaves[(Idx * 2 + (Side < 0.f ? 1 : 0)) % UE_ARRAY_COUNT(Leaves)];
				Inst(Ball, Leaf, false, Base + FVector(0.f, 0.f, H + 80.f), Rot, FVector(300.f, 300.f, 260.f));
				Inst(Ball, Leaf * 0.85f, false, Base + R * 70.f + FVector(0.f, 60.f, H + 20.f), Rot, FVector(210.f, 210.f, 190.f));
				Inst(Ball, Leaf * 1.1f, false, Base - R * 70.f + FVector(0.f, -60.f, H + 140.f), Rot, FVector(200.f, 200.f, 190.f));
			}
		}

		// Hydrants (red), litter bins (dark green) and benches on the building side of the far pavement.
		for (float D : { 900.f, 4100.f })
		{
			FVector P, R; FRotator Rot;
			Frame(D, KerbOff + 45.f, P, R, Rot);
			const FLinearColor Red(0.7f, 0.08f, 0.06f);
			Inst(Cylinder, Red, true, P + FVector(0.f, 0.f, 12.f + 32.f), Rot, FVector(24.f, 24.f, 64.f));
			Inst(Cylinder, Red, false, P + FVector(0.f, 0.f, 12.f + 70.f), Rot, FVector(30.f, 30.f, 8.f));
			Inst(Cylinder, Red, false, P + FVector(0.f, 0.f, 12.f + 42.f) + R * 0.f, Rot + FRotator(0.f, 0.f, 90.f), FVector(12.f, 12.f, 48.f));
		}
		for (float D : { 1650.f, 3300.f, 5000.f })
		{
			for (float Side : { 1.f, -1.f })
			{
				FVector P, R; FRotator Rot;
				Frame(D, Side * (KerbOff + 40.f), P, R, Rot);
				Inst(Cylinder, FLinearColor(0.06f, 0.14f, 0.1f), true, P + FVector(0.f, 0.f, 12.f + 45.f), Rot, FVector(46.f, 46.f, 90.f));
				Inst(Cylinder, FLinearColor(0.1f, 0.1f, 0.11f), false, P + FVector(0.f, 0.f, 12.f + 92.f), Rot, FVector(50.f, 50.f, 5.f));
			}
		}
		for (float D : { 2150.f, 4600.f })
		{
			FVector P, R; FRotator Rot;
			Frame(D, -(KerbOff + SidewalkWidth - 70.f), P, R, Rot);   // far side, near the building wall, facing the road
			const FLinearColor Wood(0.42f, 0.27f, 0.14f), Steel(0.1f, 0.1f, 0.12f);
			Inst(Cube, Wood, true, P + FVector(0.f, 0.f, 12.f + 45.f), Rot, FVector(170.f, 45.f, 6.f));            // seat (long side along the road)
			Inst(Cube, Wood, false, P - R * 20.f + FVector(0.f, 0.f, 12.f + 72.f), Rot, FVector(170.f, 5.f, 38.f)); // back, on the building side
			Inst(Cube, Steel, false, P + Rot.RotateVector(FVector(70.f, 0.f, 22.f)), Rot, FVector(6.f, 40.f, 44.f));
			Inst(Cube, Steel, false, P + Rot.RotateVector(FVector(-70.f, 0.f, 22.f)), Rot, FVector(6.f, 40.f, 44.f));
		}

		// Crosswalk signs (yellow diamonds on poles) either side of the zebra stripes.
		for (float Side : { 1.f, -1.f })
		{
			FVector P, R; FRotator Rot;
			Frame(CrossAt - 380.f, Side * (KerbOff + 45.f), P, R, Rot);
			Inst(Cylinder, FLinearColor(0.14f, 0.15f, 0.17f), true, P + FVector(0.f, 0.f, 12.f + 130.f), Rot, FVector(7.f, 7.f, 260.f));
			Inst(Cube, FLinearColor(0.95f, 0.78f, 0.1f), false, P + FVector(0.f, 0.f, 12.f + 255.f), Rot + FRotator(0.f, 0.f, 45.f), FVector(3.f, 60.f, 60.f));
		}

		// Bollards guarding the kerb at the crosswalk (posts with a yellow band).
		for (float Side : { 1.f, -1.f })
			for (float Dd : { -230.f, -150.f, 150.f, 230.f })
			{
				FVector P, R; FRotator Rot;
				Frame(CrossAt + Dd, Side * (KerbOff + 28.f), P, R, Rot);
				Inst(Cylinder, FLinearColor(0.12f, 0.13f, 0.15f), true, P + FVector(0.f, 0.f, 12.f + 38.f), Rot, FVector(14.f, 14.f, 76.f));
				Inst(Cylinder, FLinearColor(0.95f, 0.78f, 0.1f), false, P + FVector(0.f, 0.f, 12.f + 60.f), Rot, FVector(15.f, 15.f, 8.f));
				Inst(Ball, FLinearColor(0.12f, 0.13f, 0.15f), false, P + FVector(0.f, 0.f, 12.f + 76.f), Rot, FVector(14.f, 14.f, 10.f));
			}
	}
}
