#include "TSSTownBackdrop.h"
#include "TSSRoad.h"
#include "TSSProducts.h"
#include "TSSStreetProps.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

ATSSTownBackdrop::ATSSTownBackdrop()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ATSSTownBackdrop::Inst(UStaticMesh* Mesh, const FLinearColor& Color, const FVector& Pos, const FRotator& Rot, const FVector& SizeCm)
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
	++Instances;
}

void ATSSTownBackdrop::Build(const ATSSRoad* Road)
{
	if (!Road) return;
	static UStaticMesh* Cube = TSSCubeMesh();
	static UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static UStaticMesh* Ball = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!Cube) return;

	// Road frame: u along the road from its middle, v to the right (the shop side).
	const float Len = Road->GetLength();
	const FVector C = Road->PointAt(Len * 0.5f, 0.f);
	const FVector Right = (Road->PointAt(Len * 0.5f, 100.f) - C).GetSafeNormal();
	const FVector Fwd = FVector::CrossProduct(Right, FVector::UpVector).GetSafeNormal();
	const FRotator Yaw(0.f, Fwd.Rotation().Yaw, 0.f);
	auto W = [&](float U, float V, float Z) { return FVector(C.X, C.Y, 0.f) + Fwd * U + Right * V + FVector(0.f, 0.f, Z); };
	const float Half = Len * 0.5f;
	const float RoadW = Road->RoadWidth, WalkW = Road->SidewalkWidth;
	FRandomStream Rnd(4242);

	// Ground out to the horizon, just under the map's own ground so the join is invisible.
	Inst(Cube, FLinearColor(0.16f, 0.2f, 0.17f), FVector(C.X, C.Y, -13.f), FRotator::ZeroRotator, FVector(90000.f, 90000.f, 20.f));

	// The street carries on past both ends: asphalt, pavements, dashed centre line.
	const float FarEnd = 12000.f;
	for (float Dir : { -1.f, 1.f })
	{
		const float Mid = Dir * (Half + FarEnd * 0.5f);
		Inst(Cube, FLinearColor(0.09f, 0.09f, 0.1f), W(Mid, 0.f, 1.f), Yaw, FVector(FarEnd, RoadW, 4.f));
		for (float S : { -1.f, 1.f })
			Inst(Cube, FLinearColor(0.58f, 0.58f, 0.6f), W(Mid, S * (RoadW * 0.5f + WalkW * 0.5f), 6.f), Yaw, FVector(FarEnd, WalkW, 14.f));
		for (float U = 200.f; U < FarEnd; U += 700.f)
			Inst(Cube, FLinearColor(0.9f, 0.82f, 0.25f), W(Dir * (Half + U), 0.f, 2.4f), Yaw, FVector(300.f, 16.f, 0.4f));
	}

	// Blocks lining the carried-on street, on the same front line as the real buildings.
	static const FLinearColor Walls[] = { FLinearColor(0.38f, 0.2f, 0.16f), FLinearColor(0.58f, 0.5f, 0.4f), FLinearColor(0.42f, 0.43f, 0.46f),
		FLinearColor(0.3f, 0.35f, 0.42f), FLinearColor(0.62f, 0.6f, 0.52f), FLinearColor(0.28f, 0.33f, 0.28f), FLinearColor(0.5f, 0.28f, 0.2f) };
	TArray<FBox2D> Footprints;   // (u, v) rectangles of every block, so trees keep out of them
	auto Block = [&](float U, float V, float Wd, float Dp, float H, bool bFaceRoad, float FaceSign)
	{
		const FLinearColor Wall = Walls[Rnd.RandRange(0, UE_ARRAY_COUNT(Walls) - 1)];
		Inst(Cube, Wall, W(U, V, H * 0.5f), Yaw, FVector(Wd, Dp, H));
		Inst(Cube, Wall * 0.45f, W(U, V, H + 8.f), Yaw, FVector(Wd + 16.f, Dp + 16.f, 16.f));   // roof cap
		if (bFaceRoad)   // dark window bands on the road-facing wall
		{
			const int32 Floors = FMath::Max(1, FMath::FloorToInt(H / 300.f));
			for (int32 F = (Rnd.FRand() < 0.5f ? 1 : 0); F < Floors; ++F)
				Inst(Cube, FLinearColor(0.1f, 0.14f, 0.2f), W(U, V + FaceSign * (Dp * 0.5f + 1.f), F * 300.f + 170.f), Yaw, FVector(Wd * 0.8f, 3.f, 120.f));
		}
		Footprints.Add(FBox2D(FVector2D(U - Wd * 0.5f - 60.f, V - Dp * 0.5f - 60.f), FVector2D(U + Wd * 0.5f + 60.f, V + Dp * 0.5f + 60.f)));
	};
	for (float Dir : { -1.f, 1.f })
		for (float S : { -1.f, 1.f })
		{
			float U = Half + 40.f;
			while (U < Half + FarEnd)
			{
				const float Wd = Rnd.FRandRange(620.f, 900.f), Dp = Rnd.FRandRange(600.f, 900.f);
				const float Far = (U - Half) / FarEnd;
				const float H = Rnd.FRandRange(650.f, 1500.f) * (1.f + Far * 1.2f);
				Block(Dir * (U + Wd * 0.5f), S * (RoadW * 0.5f + WalkW + 20.f + Dp * 0.5f), Wd, Dp, H, true, -S);
				U += Wd + Rnd.FRandRange(0.f, 120.f);
			}
		}

	// Skyline and second rows: taller blocks scattered around, kept off the real street and the carried-on street.
	const float NearU = Half + 400.f;
	for (int32 I = 0; I < 130; ++I)
	{
		const float Ang = Rnd.FRand() * 2.f * PI, Rad = FMath::Lerp(3600.f, 17000.f, FMath::Pow(Rnd.FRand(), 0.8f));
		const float U = FMath::Cos(Ang) * Rad, V = FMath::Sin(Ang) * Rad;
		const float Wd = Rnd.FRandRange(700.f, 1300.f), Dp = Rnd.FRandRange(700.f, 1300.f);
		if (FMath::Abs(U) < NearU + Wd * 0.5f && FMath::Abs(V) < 3300.f) continue;               // around the real street and shop
		if (FMath::Abs(U) > Half && FMath::Abs(V) < RoadW * 0.5f + WalkW + 1400.f) continue;     // beside the carried-on street
		bool bHit = false;
		const FBox2D Mine(FVector2D(U - Wd * 0.5f, V - Dp * 0.5f), FVector2D(U + Wd * 0.5f, V + Dp * 0.5f));
		for (const FBox2D& B : Footprints) if (B.Intersect(Mine)) { bHit = true; break; }
		if (bHit) continue;
		const float H = Rnd.FRandRange(800.f, 2200.f) + Rad * Rnd.FRandRange(0.05f, 0.30f);
		Block(U, V, Wd, Dp, H, false, 0.f);
	}

	// Trees: along the carried-on pavements, and clumps in the gaps further out.
	static const FLinearColor Leaves[] = { FLinearColor(0.1f, 0.3f, 0.07f), FLinearColor(0.16f, 0.36f, 0.08f), FLinearColor(0.34f, 0.4f, 0.07f), FLinearColor(0.12f, 0.27f, 0.1f) };
	// Far trees are cheap instanced clumps (several leaf blobs in different shades on a slim trunk); the ones you can walk up to are the detailed procedural trees.
	auto Tree = [&](const FVector& Base, float Scale)
	{
		const float H = 360.f * Scale;
		const FLinearColor Leaf = Leaves[Rnd.RandRange(0, UE_ARRAY_COUNT(Leaves) - 1)];
		Inst(Cyl, FLinearColor(0.18f, 0.125f, 0.08f), Base + FVector(0.f, 0.f, H * 0.5f), FRotator::ZeroRotator, FVector(20.f, 20.f, H) * Scale);
		for (int32 K = 0; K < 6; ++K)
		{
			const float A = Rnd.FRand() * 6.28f, Rd = (K == 0 ? 0.f : 80.f) * Scale;
			const float Dia = Rnd.FRandRange(150.f, 210.f) * Scale;
			Inst(Ball, Leaf * Rnd.FRandRange(0.75f, 1.2f), Base + FVector(FMath::Cos(A) * Rd, FMath::Sin(A) * Rd, H + (K == 0 ? 90.f : Rnd.FRandRange(10.f, 150.f)) * Scale),
				FRotator::ZeroRotator, FVector(Dia, Dia, Dia * 0.85f));
		}
	};
	int32 NearSeed = 0;
	auto NearTree = [&](const FVector& Base)
	{
		UProceduralMeshComponent* T = NewObject<UProceduralMeshComponent>(this);
		T->SetupAttachment(GetRootComponent());
		T->RegisterComponent();
		T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		T->SetWorldLocationAndRotation(Base, FRotator(0.f, NearSeed * 47.f, 0.f));
		TSSStreet::BuildTree(T, 200 + NearSeed, 330.f + 55.f * (NearSeed % 3));
		++NearSeed;
	};
	for (float Dir : { -1.f, 1.f })
		for (float S : { -1.f, 1.f })
			for (float U = 300.f; U < FarEnd * 0.75f; U += 1300.f)
			{
				if (U < 4200.f) NearTree(W(Dir * (Half + U), S * (RoadW * 0.5f + 70.f), 12.f));
				else Tree(W(Dir * (Half + U), S * (RoadW * 0.5f + 70.f), 12.f), 1.f);
			}
	for (int32 I = 0, Placed = 0; I < 600 && Placed < 90; ++I)
	{
		const float Ang = Rnd.FRand() * 2.f * PI, Rad = FMath::Lerp(3300.f, 14000.f, Rnd.FRand());
		const float U = FMath::Cos(Ang) * Rad, V = FMath::Sin(Ang) * Rad;
		if (FMath::Abs(U) < Half + 300.f && FMath::Abs(V) < 3300.f) continue;
		if (FMath::Abs(U) > Half - 200.f && FMath::Abs(V) < RoadW * 0.5f + WalkW + 40.f) continue;
		bool bHit = false;
		for (const FBox2D& B : Footprints) if (B.IsInside(FVector2D(U, V))) { bHit = true; break; }
		if (bHit) continue;
		Tree(W(U, V, 0.f), Rnd.FRandRange(0.9f, 1.5f));
		++Placed;
	}
}
