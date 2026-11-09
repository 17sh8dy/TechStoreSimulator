#include "TSSBin.h"
#include "TSSCharacter.h"
#include "TSSProducts.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float BinLidOpenDeg = 105.f;
}

UStaticMeshComponent* ATSSBin::Part(const TCHAR* Name, USceneComponent* Parent, UStaticMesh* Mesh, FVector Loc, FVector SizeCm, FRotator Rot, const FLinearColor& Color, bool bCollide)
{
	UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	C->SetupAttachment(Parent);
	C->SetStaticMesh(Mesh);
	C->SetRelativeLocation(Loc);
	C->SetRelativeRotation(Rot);
	C->SetRelativeScale3D(SizeCm / 100.f);   // the engine cube / cylinder are 100 cm
	if (bCollide)
	{
		C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		C->SetCollisionResponseToAllChannels(ECR_Block);
	}
	else
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	Parts.Add(C);
	PartColors.Add(Color);
	return C;
}

ATSSBin::ATSSBin()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Cyl = CylFinder.Object;
	UStaticMesh* Cube = TSSCubeMesh();

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	const FLinearColor Green(0.05f, 0.3f, 0.14f), Steel(0.34f, 0.35f, 0.38f), Dark(0.012f, 0.012f, 0.015f), Grey(0.2f, 0.2f, 0.21f);
	Part(TEXT("Plinth"), Root, Cyl, FVector(0.f, 0.f, 2.5f), FVector(46.f, 46.f, 5.f), FRotator::ZeroRotator, Grey);
	Body = Part(TEXT("Body"), Root, Cyl, FVector(0.f, 0.f, 43.f), FVector(48.f, 48.f, 76.f), FRotator::ZeroRotator, Green, true);
	Part(TEXT("BandLow"), Root, Cyl, FVector(0.f, 0.f, 27.5f), FVector(50.4f, 50.4f, 3.f), FRotator::ZeroRotator, Steel);
	Part(TEXT("BandHigh"), Root, Cyl, FVector(0.f, 0.f, 59.5f), FVector(50.4f, 50.4f, 3.f), FRotator::ZeroRotator, Steel);
	Part(TEXT("Rim"), Root, Cyl, FVector(0.f, 0.f, 81.5f), FVector(52.f, 52.f, 5.f), FRotator::ZeroRotator, Steel);
	Part(TEXT("Liner"), Root, Cyl, FVector(0.f, 0.f, 84.2f), FVector(43.f, 43.f, 0.6f), FRotator::ZeroRotator, Dark);
	for (int32 i = 0; i < 8; ++i)   // vertical ribs around the body
	{
		const float A = i * 45.f, R = 24.3f;
		Part(*FString::Printf(TEXT("Rib%d"), i), Root, Cube, FVector(FMath::Cos(FMath::DegreesToRadians(A)) * R, FMath::Sin(FMath::DegreesToRadians(A)) * R, 44.f),
			FVector(2.6f, 3.6f, 34.f), FRotator(0.f, A, 0.f), FLinearColor(0.04f, 0.24f, 0.11f));
	}
	Part(TEXT("PlateFrame"), Root, Cube, FVector(24.6f, 0.f, 50.f), FVector(1.2f, 22.f, 13.f), FRotator::ZeroRotator, Steel);
	Part(TEXT("PlateFace"), Root, Cube, FVector(25.3f, 0.f, 50.f), FVector(0.6f, 20.f, 11.f), FRotator::ZeroRotator, FLinearColor(0.85f, 0.86f, 0.82f));

	// Lid: hinged at the back edge, a flat dome with a slot and a front handle.
	LidPivot = CreateDefaultSubobject<USceneComponent>(TEXT("LidPivot"));
	LidPivot->SetupAttachment(Root);
	LidPivot->SetRelativeLocation(FVector(-24.f, 0.f, 84.5f));
	Part(TEXT("Lid"), LidPivot, Cyl, FVector(24.f, 0.f, 2.5f), FVector(53.f, 53.f, 5.f), FRotator::ZeroRotator, Green);
	Part(TEXT("LidCrown"), LidPivot, Cyl, FVector(24.f, 0.f, 6.f), FVector(34.f, 34.f, 3.f), FRotator::ZeroRotator, FLinearColor(0.06f, 0.34f, 0.16f));
	Part(TEXT("LidSlot"), LidPivot, Cube, FVector(24.f + 4.f, 0.f, 7.7f), FVector(9.f, 24.f, 0.8f), FRotator::ZeroRotator, Dark);
	Part(TEXT("LidHandle"), LidPivot, Cube, FVector(24.f + 24.f, 0.f, 5.5f), FVector(5.f, 14.f, 2.5f), FRotator::ZeroRotator, Steel);
	Part(TEXT("LidHinge"), LidPivot, Cube, FVector(1.f, 0.f, 2.5f), FVector(3.f, 12.f, 4.f), FRotator::ZeroRotator, Steel);

	Plate = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Plate"));
	Plate->SetupAttachment(Root);
	Plate->SetRelativeLocation(FVector(25.8f, 0.f, 50.f));
	Plate->SetWorldSize(5.f);
	Plate->SetHorizontalAlignment(EHTA_Center);
	Plate->SetVerticalAlignment(EVRTA_TextCenter);
	Plate->SetTextRenderColor(FColor(20, 70, 45));
	Plate->SetText(FText::FromString(TEXT("LITTER")));
	Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATSSBin::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	for (int32 i = 0; i < Parts.Num() && i < PartColors.Num(); ++i)
	{
		if (Parts[i]) TSSApplyColor(Parts[i], PartColors[i]);
	}
}

void ATSSBin::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	OpenTimer = FMath::Max(0.f, OpenTimer - DeltaSeconds);
	const float Target = OpenTimer > 0.f ? 1.f : 0.f;
	LidOpen = FMath::FInterpConstantTo(LidOpen, Target, DeltaSeconds, 3.2f);
	LidPivot->SetRelativeRotation(FRotator(BinLidOpenDeg * FMath::InterpEaseInOut(0.f, 1.f, LidOpen, 2.f), 0.f, 0.f));
}

FString ATSSBin::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	if (Player && Player->IsCarryingEmptyBox()) return TEXT("[E / Click] Put the empty box in the bin");
	if (Boxes > 0) return FString::Printf(TEXT("Litter bin  (%d empty %s inside)"), Boxes, Boxes == 1 ? TEXT("box") : TEXT("boxes"));
	return TEXT("Litter bin");
}

void ATSSBin::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	if (!Player || !Player->IsCarryingEmptyBox()) return;
	Player->SetCarryingEmptyBox(false);
	++Boxes;           // no capacity yet: it never fills
	OpenTimer = 1.1f;  // lid flips up while the box drops in
}
