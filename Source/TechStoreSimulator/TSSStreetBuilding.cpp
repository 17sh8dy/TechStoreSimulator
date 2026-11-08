#include "TSSStreetBuilding.h"
#include "TSSProducts.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ATSSStreetBuilding::ATSSStreetBuilding()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void ATSSStreetBuilding::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void ATSSStreetBuilding::Rebuild()
{
	for (UActorComponent* C : Generated) if (C) C->DestroyComponent();
	Generated.Reset();
	Panes.Reset();
	PaneAlwaysLit.Reset();
	LitCount = 0;

	static const FLinearColor SbWalls[] = {
		FLinearColor(0.42f, 0.19f, 0.15f), FLinearColor(0.66f, 0.58f, 0.44f), FLinearColor(0.46f, 0.47f, 0.5f),
		FLinearColor(0.34f, 0.4f, 0.48f), FLinearColor(0.72f, 0.69f, 0.6f), FLinearColor(0.3f, 0.36f, 0.3f) };
	static const FLinearColor SbAwnings[] = {
		FLinearColor(0.75f, 0.12f, 0.12f), FLinearColor(0.1f, 0.35f, 0.7f), FLinearColor(0.1f, 0.5f, 0.25f),
		FLinearColor(0.85f, 0.6f, 0.1f), FLinearColor(0.5f, 0.15f, 0.55f) };
	static const TCHAR* SbNames[] = { TEXT("CAFE"), TEXT("PIZZA"), TEXT("BOOKS"), TEXT("PHARMACY"), TEXT("BARBER"), TEXT("DELI"), TEXT("BAKERY"), TEXT("DINER") };
	const int32 Pick = FMath::Abs(Style);
	const FLinearColor Wall = SbWalls[Pick % UE_ARRAY_COUNT(SbWalls)];
	const FLinearColor Awning = SbAwnings[(Pick / 2) % UE_ARRAY_COUNT(SbAwnings)];
	const FLinearColor Glass(0.46f, 0.62f, 0.72f);
	const FLinearColor Trim(0.12f, 0.12f, 0.14f);
	auto Mix = [](const FLinearColor& A, const FLinearColor& B2, float T) { return FLinearColor(FMath::Lerp(A.R, B2.R, T), FMath::Lerp(A.G, B2.G, T), FMath::Lerp(A.B, B2.B, T), 1.f); };
	const FLinearColor Stone = Mix(Wall, FLinearColor(0.86f, 0.84f, 0.78f), 0.6f);      // sills, lintels, cornice
	const FLinearColor StoneDark = Mix(Wall, FLinearColor(0.2f, 0.2f, 0.2f), 0.25f);
	const FLinearColor Iron(0.04f, 0.04f, 0.05f);
	FRandomStream Rnd(Pick * 7919 + 13);   // same building, same details every time

	static UStaticMesh* Cube = TSSCubeMesh();
	static UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static UStaticMesh* Ball = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	// A full cube with its own component: used for the body and the window panes (panes change material at night).
	auto Add = [this](FVector Centre, FVector Size, const FLinearColor& Color, bool bCollide) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* P = NewObject<UStaticMeshComponent>(this);
		P->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		P->SetMobility(EComponentMobility::Movable);
		P->SetStaticMesh(TSSCubeMesh());
		P->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		P->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
		P->SetRelativeLocation(Centre);
		P->SetRelativeScale3D(Size / 100.f);
		P->RegisterComponent();
		TSSApplyColor(P, Color);
		Generated.Add(P);
		return P;
	};
	auto AddGlass = [this, &Add](FVector Centre, FVector Size, const FLinearColor& Color, bool bAlwaysLit)
	{
		Panes.Add(Add(Centre, Size, Color, false));
		PaneAlwaysLit.Add(bAlwaysLit);
	};
	// Every other detail is an instance in one shared component per (mesh, colour): a dozen draw calls per building, not hundreds.
	TMap<uint32, UInstancedStaticMeshComponent*> Groups;
	auto B = [&](UStaticMesh* Mesh, const FLinearColor& Color, FVector Pos, FVector Size, FRotator Rot = FRotator::ZeroRotator)
	{
		const uint32 Key = GetTypeHash(Mesh) ^ (Color.ToFColor(true).DWColor() * 2654435761u);
		UInstancedStaticMeshComponent*& G = Groups.FindOrAdd(Key);
		if (!G)
		{
			G = NewObject<UInstancedStaticMeshComponent>(this);
			G->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			G->SetMobility(EComponentMobility::Movable);
			G->SetStaticMesh(Mesh);
			G->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			G->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
			G->RegisterComponent();
			TSSApplyColor(G, Color);
			Generated.Add(G);
		}
		G->AddInstance(FTransform(Rot, Pos, Size / 100.f), false);
	};
	// A beam between two local points (fire-escape stringers).
	auto Beam = [&](const FLinearColor& Color, FVector A, FVector Bp, float W, float T)
	{
		const FVector D = Bp - A;
		B(Cube, Color, (A + Bp) * 0.5f, FVector(D.Size(), W, T), D.Rotation());
	};

	// ---- body ----
	Add(FVector(-Depth * 0.5f, 0.f, Height * 0.5f), FVector(Depth, Width, Height), Wall, true);

	const float Storey = 300.f;
	const int32 Floors = FMath::Max(1, FMath::FloorToInt(Height / Storey));
	const int32 Cols = FMath::Max(1, FMath::FloorToInt((Width - 60.f) / 220.f));
	const float Pitch = Width / Cols;
	const int32 FirstFloor = bShopfront ? 1 : 0;

	// ---- cornice, parapet and roof edge. Three profiles by style. ----
	{
		const int32 Prof = Pick % 3;
		const float Proj = Prof == 2 ? 46.f : 28.f, Thick = Prof == 2 ? 24.f : 15.f;
		B(Cube, Stone, FVector((Proj - 10.f) * 0.5f, 0.f, Height + Thick * 0.5f - 4.f), FVector(Proj + 10.f, Width + 14.f, Thick));            // main cornice slab
		B(Cube, Stone, FVector(2.f, 0.f, Height - 18.f), FVector(8.f, Width + 2.f, 28.f));                                                    // frieze band under it
		if (Prof == 1)   // dentils: a row of little blocks under the slab
			for (float Y = -Width * 0.5f + 18.f; Y < Width * 0.5f - 10.f; Y += 30.f)
				B(Cube, Stone, FVector(Proj * 0.5f - 6.f, Y, Height - 8.f), FVector(Proj - 10.f, 14.f, 12.f));
		if (Prof == 2)   // deeper cove under a tall cornice
			B(Cube, StoneDark, FVector(8.f, 0.f, Height - 8.f), FVector(20.f, Width, 14.f));
		const float RoofZ = Height + Thick - 4.f;
		B(Cube, Trim, FVector(-Depth * 0.5f, 0.f, Height + 4.f), FVector(Depth, Width, 8.f));                                                 // roof surface
		B(Cube, Wall, FVector(-8.f, 0.f, RoofZ + 24.f), FVector(16.f, Width, 48.f));                                                           // front parapet
		B(Cube, Stone, FVector(-4.f, 0.f, RoofZ + 50.f), FVector(24.f, Width + 6.f, 6.f));                                                     // coping
		for (float S : { -1.f, 1.f })
			B(Cube, Wall, FVector(-Depth * 0.5f, S * (Width * 0.5f - 8.f), RoofZ + 20.f), FVector(Depth, 16.f, 40.f));                            // side parapets
	}

	// ---- storey bands and corner pilasters ----
	for (int32 F = 1; F < Floors; ++F)
		B(Cube, Stone, FVector(3.f, 0.f, F * Storey + 7.f), FVector(6.f, Width + 2.f, 14.f));
	if (Pick % 2 == 1)
		for (float S : { -1.f, 1.f })
			B(Cube, Stone, FVector(3.f, S * (Width * 0.5f - 16.f), Height * 0.5f), FVector(6.f, 32.f, Height));

	// ---- windows: frame, glass, sill, lintel, and sometimes shutters or an air-conditioner ----
	const int32 ShutterPick = Pick % 3;
	for (int32 F = FirstFloor; F < Floors; ++F)
	{
		for (int32 C = 0; C < Cols; ++C)
		{
			const float Y = -Width * 0.5f + Pitch * (C + 0.5f);
			const float Zc = F * Storey + 170.f;
			B(Cube, Trim, FVector(1.f, Y, Zc), FVector(3.f, 120.f, 150.f));
			AddGlass(FVector(2.f, Y, Zc), FVector(3.f, 108.f, 138.f), Glass, false);
			B(Cube, Stone, FVector(8.f, Y, Zc - 80.f), FVector(16.f, 142.f, 7.f));                           // sill
			B(Cube, Stone, FVector(5.f, Y, Zc + 82.f), FVector(10.f, 138.f, 12.f));                          // lintel
			if (Pick % 2 == 0) B(Cube, Stone, FVector(7.f, Y, Zc + 92.f), FVector(14.f, 150.f, 5.f));        // cap on the lintel
			if (ShutterPick == 2)
			{
				for (float S : { -1.f, 1.f })
					B(Cube, Mix(Awning, Iron, 0.35f), FVector(4.f, Y + S * 74.f, Zc), FVector(3.f, 28.f, 140.f));
			}
			else if (F >= 1 && Rnd.FRand() < 0.2f)   // window air-conditioner sitting on the sill
			{
				B(Cube, FLinearColor(0.78f, 0.78f, 0.8f), FVector(26.f, Y, Zc - 56.f), FVector(48.f, 70.f, 38.f));
				B(Cube, FLinearColor(0.25f, 0.25f, 0.28f), FVector(50.f, Y, Zc - 60.f), FVector(2.f, 58.f, 18.f));
			}
		}
	}

	// ---- fire escape: a stack of balconies joined by stairs, up one window column ----
	if (Floors >= 3 && Pick % 3 != 1)
	{
		const float Y = -Width * 0.5f + Pitch * ((Pick % Cols) + 0.5f);
		const float HW = 80.f, Proj = 110.f;
		for (int32 F = 1; F < Floors; ++F)
		{
			const float Z = F * Storey + 92.f;
			B(Cube, Iron, FVector(Proj * 0.5f, Y, Z), FVector(Proj, HW * 2.f, 4.f));                           // platform
			B(Cube, Iron, FVector(Proj, Y, Z + 95.f), FVector(4.f, HW * 2.f, 4.f));                            // front top rail
			B(Cube, Iron, FVector(Proj, Y, Z + 50.f), FVector(3.f, HW * 2.f, 3.f));                            // front mid rail
			for (float S : { -1.f, 1.f })
			{
				B(Cube, Iron, FVector(Proj * 0.5f, Y + S * HW, Z + 95.f), FVector(Proj, 4.f, 4.f));            // side rails
				B(Cube, Iron, FVector(Proj * 0.5f, Y + S * HW, Z + 50.f), FVector(Proj, 3.f, 3.f));
			}
			for (int32 P = 0; P < 5; ++P)                                                                      // balusters
				B(Cube, Iron, FVector(Proj, Y - HW + P * HW * 0.5f, Z + 47.f), FVector(3.f, 3.f, 94.f));
			if (F + 1 < Floors)   // stairs up to the next balcony, zig-zagging
			{
				const bool bFlip = (F % 2) == 0;
				const FVector Lo(bFlip ? 15.f : Proj - 5.f, Y, Z), Hi(bFlip ? Proj - 5.f : 15.f, Y, Z + Storey);
				for (float S : { -1.f, 1.f })
					Beam(Iron, Lo + FVector(0.f, S * 38.f, 0.f), Hi + FVector(0.f, S * 38.f, 0.f), 5.f, 12.f);
				for (int32 T = 1; T < 9; ++T)
					B(Cube, Iron, FMath::Lerp(Lo, Hi, T / 9.f) + FVector(0.f, 0.f, 3.f), FVector(16.f, 76.f, 3.f));
			}
		}
		// A drop ladder from the lowest balcony.
		const float Z1 = Storey + 92.f;
		for (float S : { -1.f, 1.f })
			B(Cube, Iron, FVector(Proj - 6.f, Y + S * 22.f, Z1 - 90.f), FVector(4.f, 4.f, 180.f));
		for (int32 R = 0; R < 6; ++R)
			B(Cube, Iron, FVector(Proj - 6.f, Y, Z1 - 20.f - R * 30.f), FVector(3.f, 44.f, 3.f));
	}

	// ---- shopfront: window, plinth, sign board with the shop's name, striped sloped awning, sometimes a blade sign ----
	if (bShopfront)
	{
		const float Win = Width * 0.7f;
		B(Cube, Trim, FVector(1.f, 0.f, 135.f), FVector(3.f, Win + 12.f, 214.f));
		AddGlass(FVector(2.f, 0.f, 135.f), FVector(3.f, Win, 202.f), Glass, true);
		B(Cube, StoneDark, FVector(4.f, 0.f, 14.f), FVector(8.f, Win + 20.f, 28.f));                                  // plinth under the window
		B(Cube, Trim, FVector(3.f, Win * 0.5f - 50.f, 105.f), FVector(3.f, 8.f, 210.f));                              // door frame post
		for (float S : { -1.f, 1.f })
			B(Cube, Stone, FVector(5.f, S * (Win * 0.5f + 20.f), 140.f), FVector(10.f, 24.f, 280.f));                 // pilasters either side
		B(Cube, Trim, FVector(0.f, 0.f, 262.f), FVector(8.f, Width, 40.f));                                           // sign band

		const float SignW = Width * 0.82f;
		const FLinearColor SignCol = Mix(Awning, Iron, 0.55f);
		B(Cube, SignCol, FVector(7.f, 0.f, 262.f), FVector(6.f, SignW, 34.f));                                        // sign board
		if (UTextRenderComponent* T = NewObject<UTextRenderComponent>(this))
		{
			T->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			T->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
			T->SetRelativeLocation(FVector(11.f, 0.f, 262.f));
			T->SetText(FText::FromString(SbNames[Pick % UE_ARRAY_COUNT(SbNames)]));
			T->SetWorldSize(26.f);
			T->SetTextRenderColor(FColor(244, 238, 220));
			T->SetHorizontalAlignment(EHTA_Center);
			T->SetVerticalAlignment(EVRTA_TextCenter);
			T->RegisterComponent();
			Generated.Add(T);
		}

		// Awning: slats sloping down away from the wall, plain or striped, with a hanging valance.
		const bool bStripe = Pick % 2 == 0;
		const int32 Slats = FMath::Max(2, FMath::RoundToInt(SignW / 40.f));
		const float SlatW = SignW / Slats;
		const FLinearColor Cream(0.9f, 0.88f, 0.8f);
		for (int32 S = 0; S < Slats; ++S)
		{
			const FLinearColor Col = (bStripe && (S % 2)) ? Cream : Awning;
			const float Y = -SignW * 0.5f + SlatW * (S + 0.5f);
			B(Cube, Col, FVector(50.f, Y, 226.f), FVector(99.f, SlatW, 3.f), FRotator(-16.f, 0.f, 0.f));
			B(Cube, Col, FVector(97.f, Y, 205.f), FVector(3.f, SlatW, 18.f));
		}
		if ((Pick + 1) % 3 == 0)   // projecting blade sign
		{
			const float Y = -Width * 0.5f + 55.f;
			B(Cube, Iron, FVector(30.f, Y, 340.f), FVector(60.f, 4.f, 4.f));
			B(Cube, Mix(Awning, Cream, 0.2f), FVector(36.f, Y, 312.f), FVector(48.f, 5.f, 48.f));
		}
	}

	// ---- roof: whatever a flat roof has on it, chosen per building ----
	{
		const float Z0 = Height + 16.f;
		const FLinearColor Metal(0.55f, 0.56f, 0.58f), MetalDark(0.2f, 0.21f, 0.23f);
		const int32 Hvac = 1 + Rnd.RandRange(0, 2);
		for (int32 H = 0; H < Hvac; ++H)
		{
			const FVector P(-Depth * (0.3f + 0.45f * Rnd.FRand()), (Rnd.FRand() - 0.5f) * (Width - 260.f), Z0);
			B(Cube, Metal, P + FVector(0.f, 0.f, 35.f), FVector(150.f, 100.f, 70.f));
			B(Cyl, MetalDark, P + FVector(0.f, 0.f, 73.f), FVector(70.f, 70.f, 6.f));
		}
		if (Floors >= 3)   // stairwell bulkhead
		{
			const float Y = (Rnd.FRand() - 0.5f) * (Width - 300.f);
			B(Cube, Wall, FVector(-Depth + 110.f, Y, Z0 + 55.f), FVector(150.f, 170.f, 110.f));
			B(Cube, Trim, FVector(-Depth + 35.f, Y, Z0 + 45.f), FVector(4.f, 56.f, 90.f));
			B(Cube, Stone, FVector(-Depth + 110.f, Y, Z0 + 113.f), FVector(160.f, 180.f, 6.f));
		}
		if (Floors >= 3 && Pick % 3 == 0)   // water tank on legs
		{
			const FVector P(-Depth * 0.45f, Width * 0.18f, Z0);
			for (float X : { -45.f, 45.f })
				for (float Y : { -45.f, 45.f })
					B(Cube, Iron, P + FVector(X, Y, 70.f), FVector(6.f, 6.f, 140.f));
			B(Cyl, FLinearColor(0.32f, 0.2f, 0.12f), P + FVector(0.f, 0.f, 215.f), FVector(130.f, 130.f, 150.f));
			B(Ball, FLinearColor(0.28f, 0.17f, 0.1f), P + FVector(0.f, 0.f, 290.f), FVector(130.f, 130.f, 56.f));
			for (float Zr : { 175.f, 230.f }) B(Cyl, Iron, P + FVector(0.f, 0.f, Zr), FVector(134.f, 134.f, 6.f));
		}
		if (Pick % 2 == 1)   // TV aerial
		{
			const FVector P(-Depth * 0.7f, -Width * 0.3f, Z0);
			B(Cyl, Iron, P + FVector(0.f, 0.f, 130.f), FVector(4.f, 4.f, 260.f));
			for (int32 A = 0; A < 3; ++A) B(Cube, Iron, P + FVector(0.f, 0.f, 120.f + A * 45.f), FVector(4.f, 90.f - A * 18.f, 3.f));
		}
		for (int32 V = 0; V < 3; ++V)   // vent pipes
		{
			const FVector P(-Depth * (0.2f + 0.6f * Rnd.FRand()), (Rnd.FRand() - 0.5f) * (Width - 160.f), Z0);
			B(Cyl, MetalDark, P + FVector(0.f, 0.f, 35.f), FVector(14.f, 14.f, 70.f));
			B(Cyl, Metal, P + FVector(0.f, 0.f, 74.f), FVector(24.f, 24.f, 6.f));
		}
	}
}

// ---- night: lit windows ---------------------------------------------------------------------------------

static UMaterialInstanceDynamic* SbNightWindow(int32 Variant)
{
	static TMap<int32, UMaterialInstanceDynamic*> Cache;
	if (UMaterialInstanceDynamic** F = Cache.Find(Variant)) { if (*F && (*F)->IsValidLowLevel()) return *F; }
	static const FLinearColor Warm[3] = { FLinearColor(1.f, 0.78f, 0.4f), FLinearColor(0.95f, 0.86f, 0.62f), FLinearColor(0.72f, 0.86f, 1.f) };
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TSS/Materials/M_TSSEmissive.M_TSSEmissive"));
	const bool bEmissive = Base != nullptr;
	if (!Base) Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Base) return nullptr;
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, GetTransientPackage());
	M->AddToRoot();
	M->SetVectorParameterValue(TEXT("Color"), bEmissive ? Warm[Variant % 3] : Warm[Variant % 3] * 2.5f);   // fallback: just very bright
	Cache.Add(Variant, M);
	return M;
}

void ATSSStreetBuilding::SetNight(bool bNight)
{
	bNightNow = bNight;
	LitCount = 0;
	const FLinearColor Glass(0.46f, 0.62f, 0.72f);
	for (int32 i = 0; i < Panes.Num(); ++i)
	{
		UStaticMeshComponent* P = Panes[i];
		if (!P) continue;
		const bool bLit = bNight && (PaneAlwaysLit[i] || ((i * 7 + FMath::Abs(Style) * 3) % 10) < 6);
		if (bLit)
		{
			if (UMaterialInstanceDynamic* M = SbNightWindow((i + FMath::Abs(Style)) % 3)) P->SetMaterial(0, M);
			++LitCount;
		}
		else
		{
			TSSApplyColor(P, Glass);
		}
	}
}
