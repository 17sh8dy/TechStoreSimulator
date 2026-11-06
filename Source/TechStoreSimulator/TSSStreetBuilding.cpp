#include "TSSStreetBuilding.h"
#include "TSSProducts.h"
#include "Components/StaticMeshComponent.h"
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
	const int32 Pick = FMath::Abs(Style);
	const FLinearColor Wall = SbWalls[Pick % UE_ARRAY_COUNT(SbWalls)];
	const FLinearColor Awning = SbAwnings[(Pick / 2) % UE_ARRAY_COUNT(SbAwnings)];
	const FLinearColor Glass(0.46f, 0.62f, 0.72f);
	const FLinearColor Trim(0.12f, 0.12f, 0.14f);

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

	// Body (front face at x = 0), roof slab and a darker roof edge.
	Add(FVector(-Depth * 0.5f, 0.f, Height * 0.5f), FVector(Depth, Width, Height), Wall, true);
	Add(FVector(-Depth * 0.5f, 0.f, Height + 12.f), FVector(Depth + 20.f, Width + 20.f, 24.f), Trim, false);

	// Window rows above the ground floor: a 300 cm storey, windows every 220 cm.
	const float Storey = 300.f;
	const int32 Floors = FMath::Max(1, FMath::FloorToInt(Height / Storey));
	const int32 Cols = FMath::Max(1, FMath::FloorToInt((Width - 60.f) / 220.f));
	const float Pitch = Width / Cols;
	for (int32 F = (bShopfront ? 1 : 0); F < Floors; ++F)
	{
		for (int32 C = 0; C < Cols; ++C)
		{
			const float Y = -Width * 0.5f + Pitch * (C + 0.5f);
			Add(FVector(1.f, Y, F * Storey + 170.f), FVector(3.f, 120.f, 150.f), Trim, false);
			AddGlass(FVector(2.f, Y, F * Storey + 170.f), FVector(3.f, 108.f, 138.f), Glass, false);
		}
	}

	// Shopfront on the ground floor: a wide window, a door, a sign band and an awning.
	if (bShopfront)
	{
		const float Win = Width * 0.7f;
		Add(FVector(1.f, 0.f, 135.f), FVector(3.f, Win + 12.f, 214.f), Trim, false);
		AddGlass(FVector(2.f, 0.f, 135.f), FVector(3.f, Win, 202.f), Glass, true);
		Add(FVector(3.f, Win * 0.5f - 50.f, 105.f), FVector(3.f, 8.f, 210.f), Trim, false); // door frame post
		Add(FVector(0.f, 0.f, 262.f), FVector(8.f, Width, 40.f), Trim, false);               // sign band
		Add(FVector(45.f, 0.f, 236.f), FVector(90.f, Width * 0.82f, 8.f), Awning, false);   // awning
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
