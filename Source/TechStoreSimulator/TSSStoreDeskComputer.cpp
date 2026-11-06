#include "TSSStoreDeskComputer.h"
#include "TSSBusinessSubsystem.h"
#include "TSSCharacter.h"
#include "TSSPlayerController.h"
#include "TSSProducts.h"
#include "TSSStoreDeskWidget.h"
#include "TSSStoreSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"

namespace
{
	const FLinearColor CmpBeige(0.74f, 0.70f, 0.58f);
	const FLinearColor CmpBeigeDark(0.55f, 0.52f, 0.42f);
	const FLinearColor CmpWood(0.34f, 0.23f, 0.14f);
	const FLinearColor CmpDark(0.08f, 0.08f, 0.09f);
	constexpr float CmpDeskTop = 75.f;   // top surface height
}

ATSSStoreDeskComputer::ATSSStoreDeskComputer()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// ---- the desk: top, two side panels, a back panel and a small drawer unit --------------------
	Box(TEXT("CmpDeskTop"), FVector(0.f, 0.f, CmpDeskTop - 2.f), FVector(72.f, 160.f, 4.f), CmpWood);
	Box(TEXT("SideL"), FVector(0.f, 78.f, 36.f), FVector(66.f, 4.f, 72.f), CmpWood);
	Box(TEXT("SideR"), FVector(0.f, -78.f, 36.f), FVector(66.f, 4.f, 72.f), CmpWood);
	Box(TEXT("Back"), FVector(-31.f, 0.f, 48.f), FVector(3.f, 150.f, 50.f), CmpWood);
	Box(TEXT("DrawerUnit"), FVector(2.f, 52.f, 30.f), FVector(58.f, 40.f, 58.f), FLinearColor(0.28f, 0.19f, 0.12f));
	Box(TEXT("DrawerFront1"), FVector(31.5f, 52.f, 44.f), FVector(1.f, 36.f, 24.f), CmpWood);
	Box(TEXT("DrawerFront2"), FVector(31.5f, 52.f, 16.f), FVector(1.f, 36.f, 24.f), CmpWood);
	Box(TEXT("Handle1"), FVector(33.f, 52.f, 44.f), FVector(2.f, 12.f, 2.f), CmpDark);
	Box(TEXT("Handle2"), FVector(33.f, 52.f, 16.f), FVector(2.f, 12.f, 2.f), CmpDark);

	// ---- CRT monitor on a swivel base: deep boxy back, bezel, glass --------------------------------
	const FVector Crt(-6.f, -22.f, CmpDeskTop);
	Box(TEXT("CrtBase"), Crt + FVector(2.f, 0.f, 1.5f), FVector(28.f, 28.f, 3.f), CmpBeige);
	Box(TEXT("CrtBack"), Crt + FVector(-12.f, 0.f, 24.f), FVector(30.f, 32.f, 30.f), CmpBeige);
	Box(TEXT("CrtBody"), Crt + FVector(8.f, 0.f, 24.f), FVector(22.f, 42.f, 38.f), CmpBeige);
	Box(TEXT("CrtBezel"), Crt + FVector(19.5f, 0.f, 24.f), FVector(2.f, 42.f, 38.f), CmpBeigeDark);
	Box(TEXT("CrtGlass"), Crt + FVector(20.6f, 0.f, 25.f), FVector(1.f, 35.f, 29.f), FLinearColor(0.01f, 0.03f, 0.02f));
	Box(TEXT("CrtLed"), Crt + FVector(20.6f, 15.f, 3.f + 2.5f), FVector(1.f, 1.5f, 1.5f), FLinearColor(0.1f, 0.9f, 0.2f));

	Screen = CreateDefaultSubobject<UWidgetComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Root);
	Screen->SetRelativeLocation(Crt + FVector(21.3f, 0.f, 25.f));
	Screen->SetWidgetClass(UTSSDeskIdleWidget::StaticClass());
	Screen->SetDrawSize(FVector2D(560, 440));
	Screen->SetRelativeScale3D(FVector(1.f, 35.f / 560.f, 35.f / 560.f));
	Screen->SetPivot(FVector2D(0.5f, 0.5f));
	Screen->SetWidgetSpace(EWidgetSpace::World);
	Screen->SetBlendMode(EWidgetBlendMode::Opaque);
	Screen->SetTwoSided(false);
	Screen->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ---- keyboard, mouse and mouse mat -------------------------------------------------------------
	Box(TEXT("Keyboard"), FVector(27.f, -22.f, CmpDeskTop + 1.2f), FVector(16.f, 46.f, 2.4f), CmpBeige, FRotator(-4.f, 0.f, 0.f));
	Box(TEXT("Keys"), FVector(27.5f, -22.f, CmpDeskTop + 2.6f), FVector(12.5f, 41.f, 1.2f), FLinearColor(0.62f, 0.6f, 0.52f), FRotator(-4.f, 0.f, 0.f));
	Box(TEXT("Spacebar"), FVector(31.f, -22.f, CmpDeskTop + 3.1f), FVector(2.5f, 20.f, 1.f), CmpBeigeDark, FRotator(-4.f, 0.f, 0.f));
	Box(TEXT("MouseMat"), FVector(26.f, 20.f, CmpDeskTop + 0.2f), FVector(22.f, 24.f, 0.4f), FLinearColor(0.12f, 0.14f, 0.3f));
	Box(TEXT("Mouse"), FVector(26.f, 20.f, CmpDeskTop + 2.2f), FVector(10.f, 6.f, 3.4f), CmpBeige);
	Box(TEXT("MouseButtons"), FVector(29.5f, 20.f, CmpDeskTop + 3.9f), FVector(3.f, 5.f, 0.6f), CmpBeigeDark);

	// ---- the tower, on the floor beside the desk ---------------------------------------------------
	const FVector Tower(2.f, -108.f, 0.f);
	Box(TEXT("Tower"), Tower + FVector(0.f, 0.f, 22.f), FVector(46.f, 21.f, 44.f), CmpBeige);
	Box(TEXT("TowerFront"), Tower + FVector(23.5f, 0.f, 22.f), FVector(1.5f, 20.f, 43.f), CmpBeigeDark);
	Box(TEXT("DriveBay1"), Tower + FVector(24.5f, 0.f, 34.f), FVector(1.f, 16.f, 3.5f), CmpDark);
	Box(TEXT("DriveBay2"), Tower + FVector(24.5f, 0.f, 28.f), FVector(1.f, 16.f, 3.5f), CmpDark);
	Box(TEXT("FloppySlot"), Tower + FVector(24.5f, 0.f, 22.f), FVector(1.f, 10.f, 1.f), CmpDark);
	Box(TEXT("PowerButton"), Tower + FVector(24.6f, -6.f, 10.f), FVector(1.f, 3.5f, 3.5f), CmpDark);
	Box(TEXT("PowerLed"), Tower + FVector(24.6f, 4.f, 10.f), FVector(1.f, 1.5f, 1.5f), FLinearColor(0.1f, 0.9f, 0.2f));

	// ---- a plain office chair (no collision: it never blocks you) -------------------------------------
	Box(TEXT("ChairSeat"), FVector(66.f, -20.f, 45.f), FVector(42.f, 42.f, 7.f), FLinearColor(0.1f, 0.1f, 0.14f), FRotator::ZeroRotator, false);
	Box(TEXT("ChairBack"), FVector(88.f, -20.f, 72.f), FVector(6.f, 40.f, 44.f), FLinearColor(0.1f, 0.1f, 0.14f), FRotator(8.f, 0.f, 0.f), false);
	Box(TEXT("ChairPost"), FVector(66.f, -20.f, 24.f), FVector(6.f, 6.f, 42.f), CmpDark, FRotator::ZeroRotator, false);
	Box(TEXT("ChairBase"), FVector(66.f, -20.f, 4.f), FVector(44.f, 44.f, 4.f), CmpDark, FRotator::ZeroRotator, false);
}

UStaticMeshComponent* ATSSStoreDeskComputer::Box(const TCHAR* Name, FVector Loc, FVector Size, FLinearColor Color, FRotator Rot, bool bCollide)
{
	UStaticMeshComponent* P = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	P->SetupAttachment(GetRootComponent());
	P->SetStaticMesh(TSSCubeMesh());
	P->SetRelativeLocation(Loc);
	P->SetRelativeRotation(Rot);
	P->SetRelativeScale3D(Size / 100.f);
	if (!bCollide) P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Parts.Add(P);
	PartColors.Add(Color);
	return P;
}

void ATSSStoreDeskComputer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	for (int32 i = 0; i < Parts.Num() && i < PartColors.Num(); ++i) if (Parts[i]) TSSApplyColor(Parts[i], PartColors[i]);
}

void ATSSStoreDeskComputer::BeginPlay()
{
	Super::BeginPlay();
	Screen->InitWidget();
	if (UTSSDeskIdleWidget* W = Cast<UTSSDeskIdleWidget>(Screen->GetUserWidgetObject()))
	{
		W->SetSystems(GetWorld()->GetSubsystem<UTSSStoreSubsystem>(), GetWorld()->GetSubsystem<UTSSBusinessSubsystem>());
	}
}

FString ATSSStoreDeskComputer::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	if (Player->GetHeldItem()) return TEXT("Put the customer's item down first");
	return TEXT("[E / Click] Use StoreDesk");
}

void ATSSStoreDeskComputer::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	if (Player->GetHeldItem()) return;
	if (ATSSPlayerController* PC = Cast<ATSSPlayerController>(Player->GetController())) PC->OpenStoreDesk();
}
