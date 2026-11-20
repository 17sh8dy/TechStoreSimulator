#include "TSSTownDistrict.h"
#include "TSSRoad.h"
#include "TSSProducts.h"
#include "TSSStreetBuilding.h"
#include "TSSStreetProps.h"
#include "TSSTrafficCar.h"
#include "ProceduralMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"

namespace
{
	const FLinearColor AsphaltColor(0.09f, 0.09f, 0.1f);
	const FLinearColor PaveColor(0.58f, 0.58f, 0.6f);
	const FLinearColor GroundColor(0.34f, 0.34f, 0.33f);
	const FLinearColor GrassColor(0.12f, 0.25f, 0.07f);
	const FLinearColor DashColor(0.9f, 0.82f, 0.25f);
	const FLinearColor LineColor(0.9f, 0.9f, 0.88f);
	const FLinearColor WoodColor(0.36f, 0.22f, 0.12f);
	const FLinearColor IronColor(0.08f, 0.09f, 0.1f);

	const FLinearColor ParkedPaints[] = {
		FLinearColor(0.80f, 0.80f, 0.78f), FLinearColor(0.35f, 0.36f, 0.38f), FLinearColor(0.03f, 0.03f, 0.035f), FLinearColor(0.55f, 0.03f, 0.03f),
		FLinearColor(0.03f, 0.1f, 0.35f), FLinearColor(0.02f, 0.18f, 0.1f), FLinearColor(0.55f, 0.45f, 0.3f), FLinearColor(0.5f, 0.52f, 0.55f) };
}

ATSSTownDistrict::ATSSTownDistrict()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ATSSTownDistrict::Inst(UStaticMesh* Mesh, const FLinearColor& Color, const FVector& Pos, const FRotator& Rot, const FVector& SizeCm)
{
	const uint32 Key = GetTypeHash(Mesh) ^ (Color.ToFColor(true).DWColor() * 2654435761u);
	int32* Found = GroupIndex.Find(Key);
	if (!Found)
	{
		UInstancedStaticMeshComponent* G = NewObject<UInstancedStaticMeshComponent>(this);
		G->SetMobility(EComponentMobility::Movable);
		G->SetStaticMesh(Mesh);
		G->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		G->SetCastShadow(false);
		G->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		G->RegisterComponent();
		TSSApplyColor(G, Color);
		Found = &GroupIndex.Add(Key, Groups.Add(G));
	}
	Groups[*Found]->AddInstance(FTransform(Rot, Pos, SizeCm / 100.f), true);
}

UBoxComponent* ATSSTownDistrict::Blocker(const FVector& At, const FRotator& Rot, const FVector& HalfSize, bool bFloor)
{
	UBoxComponent* B = NewObject<UBoxComponent>(this);
	B->SetupAttachment(GetRootComponent());
	B->SetBoxExtent(HalfSize, false);
	B->SetCollisionProfileName(TEXT("BlockAll"));
	B->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (!bFloor)
	{
		// Walls stop people, not the camera or the crosshair (so nothing feels "in the way" near the edge).
		B->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		B->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}
	B->SetHiddenInGame(true);
	B->SetCanEverAffectNavigation(false);
	B->RegisterComponent();
	B->SetWorldLocationAndRotation(At, Rot);
	return B;
}

bool ATSSTownDistrict::IsInside(const FVector& World) const
{
	const FVector D = World - Centre;
	return FMath::Abs(FVector::DotProduct(D, Fwd)) < HalfU && FMath::Abs(FVector::DotProduct(D, Right)) < HalfV;
}

void ATSSTownDistrict::EndPlay(const EEndPlayReason::Type Reason)
{
	for (ATSSStreetBuilding* B : Buildings) if (IsValid(B)) B->Destroy();
	for (ATSSTrafficCar* C : ParkedCars) if (IsValid(C)) C->Destroy();
	Super::EndPlay(Reason);
}

void ATSSTownDistrict::Build(ATSSRoad* Road)
{
	if (!Road) return;
	UWorld* World = GetWorld();
	static UStaticMesh* Cube = TSSCubeMesh();
	static UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (!Cube || !World) return;

	// ---- the road frame: U along the main street from its middle, V to the right (the shop side) ----
	const float Len = Road->GetLength();
	const float Half = Len * 0.5f;
	Centre = Road->PointAt(Half, 0.f);
	Centre.Z = 0.f;
	Right = (Road->PointAt(Half, 100.f) - Road->PointAt(Half, 0.f)).GetSafeNormal2D();
	Fwd = FVector::CrossProduct(Right, FVector::UpVector).GetSafeNormal();
	const FRotator Yaw(0.f, Fwd.Rotation().Yaw, 0.f);
	auto W = [&](float U, float V, float Z) { return Centre + Fwd * U + Right * V + FVector(0.f, 0.f, Z); };
	// An axis-aligned (in the road frame) slab between two U and two V values.
	auto Rect = [&](const FLinearColor& Color, float U0, float U1, float V0, float V1, float Zc, float Thick)
	{
		if (U1 < U0) Swap(U0, U1);
		if (V1 < V0) Swap(V0, V1);
		if (U1 - U0 < 1.f || V1 - V0 < 1.f) return;
		Inst(Cube, Color, W((U0 + U1) * 0.5f, (V0 + V1) * 0.5f, Zc), Yaw, FVector(U1 - U0, V1 - V0, Thick));
	};
	auto Asphalt = [&](float U0, float U1, float V0, float V1) { Rect(AsphaltColor, U0, U1, V0, V1, 0.f, 4.f); };        // top at 2, like the real road
	auto Pave = [&](float U0, float U1, float V0, float V1) { Rect(PaveColor, U0, U1, V0, V1, 5.f, 14.f); };              // top at 12, like its pavements

	const float RW2 = Road->RoadWidth * 0.5f;          // 400
	const float Kerb = RW2 + Road->SidewalkWidth;      // 780: back of the main street's pavement
	const float CW2 = 400.f, CS = 300.f;               // cross streets: half width, pavement
	const float BW2 = 350.f, BS = 300.f;               // back streets
	const float XU = CrossU, BV = BackV;
	FRandomStream Rnd(7311);

	// ---- ground under the whole town (just below the map's own floor), then the invisible floor and walls ----
	Rect(GroundColor, -HalfU - 400.f, HalfU + 400.f, -HalfV - 400.f, HalfV + 400.f, -6.f, 10.f);
	Solids.Add(Blocker(W(0.f, 0.f, -50.f), Yaw, FVector(HalfU + 600.f, HalfV + 600.f, 50.f), true));
	for (const float S : { -1.f, 1.f })
	{
		Walls.Add(Blocker(W(S * (HalfU + 50.f), 0.f, 400.f), Yaw, FVector(50.f, HalfV + 100.f, 700.f), false));
		Walls.Add(Blocker(W(0.f, S * (HalfV + 50.f), 400.f), Yaw, FVector(HalfU + 100.f, 50.f, 700.f), false));
	}

	// ---- the main street, carried on from both ends of the real road to the town edge (and a little past it) ----
	for (const float Dir : { -1.f, 1.f })
	{
		const float U0 = Dir * Half, U1 = Dir * (HalfU + 400.f);
		Asphalt(U0, U1, -RW2, RW2);
		for (const float S : { -1.f, 1.f })
		{
			// The pavement stops at the cross street (the crossing is asphalt).
			Pave(U0, Dir * (XU - CW2), S * RW2, S * Kerb);
			Pave(Dir * (XU + CW2), U1, S * RW2, S * Kerb);
		}
		// Zebra crossing over the main street just before each junction.
		const float Zu = Dir * (XU - CW2 - 260.f);
		for (float V = -RW2 + 40.f; V < RW2 - 20.f; V += 85.f) Rect(LineColor, Zu - 160.f, Zu + 160.f, V, V + 45.f, 2.3f, 0.5f);
	}
	// Centre dashes on the same 7 m rhythm as the real road's.
	for (float D = 200.f - 700.f * 20.f; D < Len + 700.f * 20.f; D += 700.f)
	{
		const float U = D - Half;
		if (FMath::Abs(U) <= Half || FMath::Abs(U) > HalfU - 150.f) continue;   // the backdrop dashes on from the edge
		if (FMath::Abs(FMath::Abs(U) - XU) < CW2 + 160.f) continue;   // not across the junction
		Rect(DashColor, U - 150.f, U + 150.f, -8.f, 8.f, 2.4f, 0.4f);
	}

	// ---- cross streets (one at each end, across the main street) and back streets (behind each row) ----
	for (const float SU : { -1.f, 1.f })
	{
		const float U = SU * XU;
		Asphalt(U - CW2, U + CW2, -BV - BW2, BV + BW2);
		for (float V = -BV + 300.f; V < BV - 300.f; V += 700.f)
		{
			if (FMath::Abs(V) < RW2 + 200.f) continue;
			Rect(DashColor, U - 8.f, U + 8.f, V - 150.f, V + 150.f, 2.4f, 0.4f);
		}
		for (const float SV : { -1.f, 1.f })
		{
			// Pavement on the inner side (between main and back street) and the outer side (round the corner as well).
			Pave(U - SU * (CW2 + CS), U - SU * CW2, SV * Kerb, SV * (BV - BW2));
			Pave(U + SU * CW2, U + SU * (CW2 + CS), SV * Kerb, SV * (BV + BW2 + BS));
		}
	}
	for (const float SV : { -1.f, 1.f })
	{
		const float V = SV * BV;
		Asphalt(-XU - CW2, XU + CW2, V - BW2, V + BW2);
		for (float U = -XU + 500.f; U < XU - 450.f; U += 700.f) Rect(DashColor, U - 150.f, U + 150.f, V - 8.f, V + 8.f, 2.4f, 0.4f);
		Pave(-XU + CW2, XU - CW2, SV * (BV - BW2 - BS), SV * (BV - BW2));          // inner side
		Pave(-XU - CW2 - CS, XU + CW2 + CS, SV * (BV + BW2), SV * (BV + BW2 + BS)); // outer side
	}

	// ---- buildings ----
	// FaceU/FaceV: which way the front looks, in the road frame (exactly one is +-1). Front point at (U, V).
	int32 StyleSeed = 7;
	auto Bld = [&](float U, float V, float FaceU, float FaceV, float Width, float Depth, float Height)
	{
		const FVector Face = (Fwd * FaceU + Right * FaceV).GetSafeNormal();
		const FTransform T(FRotator(0.f, Face.Rotation().Yaw, 0.f), W(U, V, 0.f));
		ATSSStreetBuilding* B = World->SpawnActorDeferred<ATSSStreetBuilding>(ATSSStreetBuilding::StaticClass(), T, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!B) return;
		B->Width = Width;
		B->Depth = Depth;
		B->Height = Height;
		B->Style = StyleSeed++;
		B->FinishSpawning(T);
		Buildings.Add(B);
	};
	const float Front = Kerb + 20.f;   // 800: the line the real buildings stand on
	// Main street, filling the gaps left and right of the real buildings.
	Bld(2850.f, -Front, 0.f, 1.f, 1100.f, 600.f, 1000.f);
	Bld(-3000.f, -Front, 0.f, 1.f, 900.f, 600.f, 1300.f);
	for (const float SU : { -1.f, 1.f }) Bld(SU * 2650.f, Front, 0.f, -1.f, 1300.f, 600.f, SU > 0.f ? 900.f : 1100.f);
	// Back rows, facing the back streets.
	const float BackFront = BV - BW2 - BS - 20.f;   // 4030
	for (const float U : { -3150.f, -1850.f, 2000.f, 3150.f }) Bld(U, -BackFront, 0.f, -1.f, FMath::Abs(U) > 3000.f ? 900.f : 1100.f, 650.f, Rnd.FRandRange(700.f, 1300.f));
	for (const float U : { -3150.f, -1850.f, 1850.f, 3150.f }) Bld(U, BackFront, 0.f, 1.f, FMath::Abs(U) > 3000.f ? 900.f : 1100.f, 600.f, Rnd.FRandRange(700.f, 1200.f));
	// Beyond the cross streets: two along the main street and one facing the cross street, in each corner.
	const float OuterFront = XU + CW2 + CS + 20.f;   // 5020
	for (const float SU : { -1.f, 1.f })
		for (const float SV : { -1.f, 1.f })
		{
			Bld(SU * 5650.f, SV * Front, 0.f, -SV, 1200.f, 600.f, Rnd.FRandRange(800.f, 1400.f));
			Bld(SU * 6900.f, SV * Front, 0.f, -SV, 1000.f, 600.f, Rnd.FRandRange(700.f, 1200.f));
			Bld(SU * OuterFront, SV * 2600.f, -SU, 0.f, 1400.f, 800.f, Rnd.FRandRange(650.f, 900.f));
		}
	// Beyond the back streets, facing them.
	const float FarFront = BV + BW2 + BS + 20.f;   // 5370
	for (const float SV : { -1.f, 1.f })
		for (const float U : { -2800.f, 0.f, 2800.f })
			Bld(U, SV * FarFront, 0.f, -SV, U == 0.f ? 1500.f : 1300.f, 600.f, Rnd.FRandRange(800.f, 1500.f));

	// ---- trees (the detailed kind, like the real street's) ----
	int32 TreeSeed = 400;
	auto Tree = [&](float U, float V, float TrunkH)
	{
		UProceduralMeshComponent* T = NewObject<UProceduralMeshComponent>(this);
		T->SetupAttachment(GetRootComponent());
		T->RegisterComponent();
		T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		T->SetWorldLocationAndRotation(W(U, V, 12.f), FRotator(0.f, TreeSeed * 53.f, 0.f));
		TSSStreet::BuildTree(T, TreeSeed++, TrunkH);
		// A solid trunk so you cannot walk through it.
		Solids.Add(Blocker(W(U, V, 150.f), Yaw, FVector(14.f, 14.f, 150.f), false));
	};
	for (const float Dir : { -1.f, 1.f })
		for (const float S : { -1.f, 1.f })
			for (float U = Half + 650.f; U < HalfU - 300.f; U += 1300.f)
			{
				if (FMath::Abs(U - XU) < CW2 + CS + 250.f) continue;
				Tree(Dir * U, S * (RW2 + 70.f), 330.f + 55.f * (TreeSeed % 3));
			}

	// ---- a small park behind the shop, open to the back street ----
	{
		const float P0 = -1300.f, P1 = 1300.f, Q0 = 2500.f, Q1 = BackFront;
		Rect(GrassColor, P0, P1, Q0, Q1, -0.5f, 3.f);
		Rect(PaveColor, -110.f, 110.f, Q0, Q1, 0.5f, 3.f);              // path from the back street
		Rect(PaveColor, P0, P1, 3150.f, 3350.f, 0.5f, 3.f);              // and across
		for (const float U : { -850.f, 850.f })
			for (const float V : { 2850.f, 3700.f })
				Tree(U, V, 300.f + 60.f * (TreeSeed % 3));
		// Benches along the cross path.
		for (const float U : { -450.f, 450.f })
			for (const float S : { -1.f, 1.f })
			{
				const float V = 3250.f + S * 150.f;
				Rect(WoodColor, U - 80.f, U + 80.f, V - 22.f, V + 22.f, 45.f, 6.f);                      // seat
				Rect(WoodColor, U - 80.f, U + 80.f, V + S * 22.f - 3.f, V + S * 22.f + 3.f, 70.f, 40.f);  // back
				for (const float L : { -70.f, 70.f }) Rect(IronColor, U + L - 3.f, U + L + 3.f, V - 20.f, V + 20.f, 22.f, 44.f);
				Solids.Add(Blocker(W(U, V, 40.f), Yaw, FVector(80.f, 25.f, 40.f), false));
			}
		// Lamp posts at the corners of the cross path (no light: just the look).
		for (const float U : { -1150.f, 1150.f })
		{
			Inst(Cyl, IronColor, W(U, 3250.f, 200.f), FRotator::ZeroRotator, FVector(12.f, 12.f, 400.f));
			Inst(Cube, FLinearColor(0.9f, 0.86f, 0.7f), W(U, 3250.f, 405.f), Yaw, FVector(30.f, 30.f, 22.f));
		}
	}

	// ---- parked cars: a car park across the street (behind the far row) and kerbside on the cross streets ----
	auto Park = [&](float U, float V, float FaceU, float FaceV, ETSSCarType Type)
	{
		const FVector Face = (Fwd * FaceU + Right * FaceV).GetSafeNormal();
		const FRotator Rot(0.f, Face.Rotation().Yaw, 0.f);
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		P.Owner = this;
		ATSSTrafficCar* Car = World->SpawnActor<ATSSTrafficCar>(ATSSTrafficCar::StaticClass(), FTransform(Rot, W(U, V, 2.f)), P);
		if (!Car) return;
		Car->Build(Type, ParkedPaints[Rnd.RandRange(0, UE_ARRAY_COUNT(ParkedPaints) - 1)]);
		ParkedCars.Add(Car);
		float L, Wd, H;
		ATSSTrafficCar::Dimensions(Type, L, Wd, H);
		Solids.Add(Blocker(W(U, V, H * 0.5f), Rot, FVector(L * 0.5f, Wd * 0.5f, H * 0.5f), false));
	};
	auto RandomType = [&]() { const float R = Rnd.FRand(); return R < 0.4f ? ETSSCarType::Sedan : R < 0.68f ? ETSSCarType::Hatchback : R < 0.92f ? ETSSCarType::Suv : ETSSCarType::Van; };
	{
		const float L0 = -1150.f, L1 = 1300.f, V0 = -BackFront, V1 = -3250.f;
		Asphalt(L0, L1, V0, V1);
		for (float U = L0 + 50.f; U <= L1 - 50.f; U += 350.f) Rect(LineColor, U - 6.f, U + 6.f, V1 - 20.f, V1 - 560.f, 2.3f, 0.5f);   // bay lines
		int32 Bay = 0;
		for (float U = L0 + 225.f; U < L1 - 100.f; U += 350.f, ++Bay)
			if (Bay != 2 && Bay != 5) Park(U, (V0 + V1) * 0.5f + 30.f, 0.f, 1.f, RandomType());   // nose in, two bays free
	}
	for (const float SU : { -1.f, 1.f })
	{
		// Kerbside on the outer lane of each cross street, both ways (no traffic runs here).
		Park(SU * (XU + 260.f), 1700.f, 0.f, 1.f, RandomType());
		Park(SU * (XU + 260.f), 2950.f, 0.f, 1.f, RandomType());
		Park(SU * (XU - 260.f), -1800.f, 0.f, -1.f, RandomType());
		Park(SU * (XU - 260.f), -3100.f, 0.f, -1.f, RandomType());
	}
	UE_LOG(LogTemp, Display, TEXT("TSS town district: %d buildings, %d parked cars, walls at +-%.0f / +-%.0f"), Buildings.Num(), ParkedCars.Num(), HalfU, HalfV);
}
