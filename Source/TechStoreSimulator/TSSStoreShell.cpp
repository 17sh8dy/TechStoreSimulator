#include "TSSStoreShell.h"
#include "TSSBusinessSubsystem.h"
#include "TSSProducts.h"
#include "TSSStoreLayout.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"

namespace
{
	constexpr int32 MaxLights = 8;
	constexpr float WallT = 20.f;
	constexpr float WallH = 320.f;
	const TSSStoreLayout::EAnchor Anchors[] = { TSSStoreLayout::EAnchor::Back, TSSStoreLayout::EAnchor::Left, TSSStoreLayout::EAnchor::Right };
}

ATSSStoreShell::ATSSStoreShell()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	const FLinearColor Wall(0.88f, 0.88f, 0.9f);
	Floor = Box(TEXT("Floor"), FLinearColor(0.42f, 0.44f, 0.48f));
	Ceiling = Box(TEXT("Ceiling"), FLinearColor(0.95f, 0.95f, 0.97f));
	WallBack = Box(TEXT("WallBack"), Wall);
	WallLeft = Box(TEXT("WallLeft"), Wall);
	WallRight = Box(TEXT("WallRight"), Wall);
	WallFrontA = Box(TEXT("WallFrontA"), Wall);
	WallFrontB = Box(TEXT("WallFrontB"), Wall);
	Lintel = Box(TEXT("Lintel"), Wall);
	FeatureWall = Box(TEXT("FeatureWall"), FLinearColor(0.035f, 0.06f, 0.2f), false);
	DoorMat = Box(TEXT("DoorMat"), FLinearColor(0.05f, 0.05f, 0.07f), false);
	SignBoard = Box(TEXT("SignBoard"), FLinearColor(0.03f, 0.04f, 0.08f), false);

	SignText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SignText"));
	SignText->SetupAttachment(Root);
	SignText->SetWorldSize(34.f);
	SignText->SetHorizontalAlignment(EHTA_Center);
	SignText->SetVerticalAlignment(EVRTA_TextCenter);
	SignText->SetTextRenderColor(FColor(120, 190, 255));
	SignText->SetText(FText::FromString(TEXT("TECH STORE")));
	SignText->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	for (int32 i = 0; i < MaxLights; ++i)
	{
		const FString N = FString::FromInt(i);
		Lights.Add(CreateDefaultSubobject<UPointLightComponent>(*(TEXT("Light") + N)));
		Lights[i]->SetupAttachment(Root);
		Lights[i]->SetMobility(EComponentMobility::Movable); // static lights would need a bake
		Lights[i]->SetIntensityUnits(ELightUnits::Lumens);
		Lights[i]->SetIntensity(300.f);
		Lights[i]->SetAttenuationRadius(700.f);
		Lights[i]->SetLightColor(FLinearColor(1.f, 0.96f, 0.89f));
		LightPanels.Add(Box(*(TEXT("LightPanel") + N), FLinearColor(1.f, 1.f, 0.9f), false));
	}
	for (int32 i = 0; i < 4; ++i)
	{
		Skirting.Add(Box(*(TEXT("Skirting") + FString::FromInt(i)), FLinearColor(0.02f, 0.02f, 0.03f), false));
	}
	const FLinearColor PosterColors[2] = { FLinearColor(0.85f, 0.12f, 0.12f), FLinearColor(0.95f, 0.72f, 0.08f) };
	const TCHAR* PosterLines[2] = { TEXT("SALE\n-20%"), TEXT("NEW\n5G PHONES") };
	for (int32 i = 0; i < 2; ++i)
	{
		const FString N = FString::FromInt(i);
		Posters.Add(Box(*(TEXT("Poster") + N), PosterColors[i], false));
		UTextRenderComponent* T = CreateDefaultSubobject<UTextRenderComponent>(*(TEXT("PosterText") + N));
		T->SetupAttachment(Root);
		T->SetWorldSize(11.f);
		T->SetHorizontalAlignment(EHTA_Center);
		T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetTextRenderColor(FColor(255, 255, 255));
		T->SetText(FText::FromString(PosterLines[i]));
		T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PosterTexts.Add(T);
	}

	Layout(0);
}

UStaticMeshComponent* ATSSStoreShell::Box(const TCHAR* Name, FLinearColor Color, bool bCollide)
{
	UStaticMeshComponent* P = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	P->SetupAttachment(GetRootComponent());
	P->SetStaticMesh(TSSCubeMesh());
	if (!bCollide) P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Parts.Add(P);
	PartColors.Add(Color);
	return P;
}

void ATSSStoreShell::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	for (int32 i = 0; i < Parts.Num() && i < PartColors.Num(); ++i)
	{
		if (Parts[i]) TSSApplyColor(Parts[i], PartColors[i]);
	}
	Layout(CurrentStage);
}

void ATSSStoreShell::Layout(int32 Stage)
{
	using namespace TSSStoreLayout;
	const float Bx = BackX(Stage), Hw = HalfWidth(Stage), L = Length(Stage);
	const float Cx = (Bx + FrontX) * 0.5f;
	auto Place = [](UStaticMeshComponent* C, FVector Loc, FVector Size)
	{
		C->SetRelativeLocation(Loc);
		C->SetRelativeScale3D(Size / 100.f);
	};

	Place(Floor, FVector(Cx, 0.f, -3.f), FVector(L, 2.f * Hw, 6.f));
	Place(Ceiling, FVector(Cx, 0.f, WallH + 10.f), FVector(L + 2.f * WallT, 2.f * Hw + 2.f * WallT, 20.f));
	Place(WallBack, FVector(Bx - WallT * 0.5f, 0.f, WallH * 0.5f), FVector(WallT, 2.f * Hw + 2.f * WallT, WallH));
	Place(WallLeft, FVector(Cx, -(Hw + WallT * 0.5f), WallH * 0.5f), FVector(L + 2.f * WallT, WallT, WallH));
	Place(WallRight, FVector(Cx, Hw + WallT * 0.5f, WallH * 0.5f), FVector(L + 2.f * WallT, WallT, WallH));
	const float SegLen = Hw + WallT - DoorHalfGap;
	const float SegY = DoorHalfGap + SegLen * 0.5f;
	Place(WallFrontA, FVector(FrontX + WallT * 0.5f, -SegY, WallH * 0.5f), FVector(WallT, SegLen, WallH));
	Place(WallFrontB, FVector(FrontX + WallT * 0.5f, SegY, WallH * 0.5f), FVector(WallT, SegLen, WallH));
	Place(Lintel, FVector(FrontX + WallT * 0.5f, 0.f, WallH - 40.f), FVector(WallT, 2.f * DoorHalfGap, 80.f));

	// Decor that makes it feel like a shop: a painted feature wall, skirting, a door mat, a sign, posters.
	Place(FeatureWall, FVector(Bx + 1.5f, 0.f, 125.f), FVector(3.f, 2.f * Hw, 250.f));
	Place(DoorMat, FVector(FrontX - 60.f, 0.f, 0.8f), FVector(90.f, 2.f * DoorHalfGap, 1.6f));
	Place(Skirting[0], FVector(Bx + 1.f, 0.f, 6.f), FVector(2.f, 2.f * Hw, 12.f));
	Place(Skirting[1], FVector(Cx, -Hw + 1.f, 6.f), FVector(L, 2.f, 12.f));
	Place(Skirting[2], FVector(Cx, Hw - 1.f, 6.f), FVector(L, 2.f, 12.f));
	Skirting[3]->SetVisibility(false);
	Place(SignBoard, FVector(FrontX + WallT + 3.f, 0.f, 285.f), FVector(6.f, 2.f * DoorHalfGap + 160.f, 50.f));
	SignText->SetRelativeLocation(FVector(FrontX + WallT + 6.6f, 0.f, 285.f));
	for (int32 i = 0; i < Posters.Num(); ++i)
	{
		const float Y = (i == 0 ? -1.f : 1.f) * Hw * 0.55f;
		Place(Posters[i], FVector(Bx + 4.f, Y, 190.f), FVector(1.5f, 56.f, 76.f));
		PosterTexts[i]->SetRelativeLocation(FVector(Bx + 5.2f, Y, 190.f));
	}

	// Ceiling lights: more of them as the shop grows.
	const int32 Cols = LightColumns(Stage);
	for (int32 i = 0; i < MaxLights; ++i)
	{
		const bool bOn = i < Cols * 2;
		Lights[i]->SetVisibility(bOn);
		LightPanels[i]->SetVisibility(bOn);
		if (!bOn) continue;
		const float X = Bx + (i % Cols + 0.5f) * (L / Cols);
		const float Y = (i / Cols == 0 ? -1.f : 1.f) * Hw * 0.5f;
		Lights[i]->SetRelativeLocation(FVector(X, Y, WallH - 40.f));
		Place(LightPanels[i], FVector(X, Y, WallH - 1.f), FVector(120.f, 60.f, 3.f));
	}
}

void ATSSStoreShell::ApplyStage(int32 Stage, bool bInitial)
{
	Stage = TSSStoreLayout::ClampStage(Stage);
	if (Stage != CurrentStage)
	{
		// Slide everything that lives against a wall by how far that wall moved.
		for (const TSSStoreLayout::EAnchor A : Anchors)
		{
			const FVector Delta = TSSStoreLayout::AnchorDelta(A, Stage) - TSSStoreLayout::AnchorDelta(A, CurrentStage);
			if (Delta.IsNearlyZero()) continue;
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			{
				AActor* Actor = *It;
				if (!Actor->ActorHasTag(TSSStoreLayout::AnchorTag(A))) continue;
				// Shelves bought as upgrades are spawned already at the right stage position at load time.
				if (bInitial && Actor->ActorHasTag(TEXT("TSSUpgrade"))) continue;
				Actor->AddActorWorldOffset(Delta);
			}
		}
		CurrentStage = Stage;
	}
	Layout(CurrentStage);
}

void ATSSStoreShell::BeginPlay()
{
	Super::BeginPlay();
	UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>();
	if (!Biz) return;
	BizHandle = Biz->OnChanged.AddUObject(this, &ATSSStoreShell::OnBusinessChanged);
	ApplyStage(Biz->GetStoreStage(), true);
}

void ATSSStoreShell::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* World = GetWorld())
	{
		if (UTSSBusinessSubsystem* Biz = World->GetSubsystem<UTSSBusinessSubsystem>()) Biz->OnChanged.Remove(BizHandle);
	}
	Super::EndPlay(Reason);
}

void ATSSStoreShell::OnBusinessChanged()
{
	if (UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>())
	{
		if (Biz->GetStoreStage() != CurrentStage) ApplyStage(Biz->GetStoreStage(), false);
	}
}
