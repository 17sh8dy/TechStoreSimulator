#include "TSSRegister.h"
#include "TSSCustomer.h"
#include "TSSCharacter.h"
#include "TSSProducts.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

ATSSRegister::ATSSRegister()
{
	PrimaryActorTick.bCanEverTick = true;

	// Local frame: X = depth (+X is the owner's side), Y = along the counter, Z up; origin on the floor, centred.
	Counter = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cabinet"));
	SetRootComponent(Counter);
	Counter->SetStaticMesh(TSSCubeMesh());
	Counter->SetRelativeLocation(FVector(0.f, 0.f, 46.f));
	Counter->SetRelativeScale3D(FVector(0.66f, 2.56f, 0.92f));

	auto Add = [this](const TCHAR* Name, FVector Loc, FVector Size, FLinearColor Color, FRotator Rot = FRotator::ZeroRotator)
	{
		UStaticMeshComponent* P = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		P->SetupAttachment(GetRootComponent());
		P->SetStaticMesh(TSSCubeMesh());
		// Sizes are cm in the actor's frame; the root cabinet is non-uniformly scaled, so convert for the child.
		P->SetRelativeLocation(FVector(Loc.X / 0.66f, Loc.Y / 2.56f, (Loc.Z - 46.f) / 0.92f));
		P->SetRelativeRotation(Rot);
		P->SetRelativeScale3D(FVector(Size.X / 66.f, Size.Y / 256.f, Size.Z / 92.f));
		Parts.Add(P);
		PartColors.Add(Color);
		return P;
	};

	const FLinearColor Dark(0.07f, 0.07f, 0.09f);
	const FLinearColor Steel(0.55f, 0.57f, 0.6f);
	Add(TEXT("Kick"), FVector(0.f, 0.f, 4.f), FVector(60.f, 250.f, 8.f), FLinearColor(0.05f, 0.05f, 0.06f));
	Add(TEXT("Top"), FVector(-2.f, 0.f, 95.f), FVector(80.f, 270.f, 6.f), FLinearColor(0.9f, 0.9f, 0.92f));
	Add(TEXT("Panel"), FVector(-34.f, 0.f, 48.f), FVector(2.f, 220.f, 64.f), FLinearColor(0.08f, 0.45f, 0.9f));
	Add(TEXT("PanelTrim"), FVector(-34.5f, 0.f, 82.f), FVector(1.f, 230.f, 4.f), FLinearColor(0.95f, 0.95f, 1.f));
	// Point-of-sale terminal: base with a cash drawer, and an angled customer-facing screen.
	Add(TEXT("PosBase"), FVector(8.f, -70.f, 104.f), FVector(30.f, 46.f, 12.f), Dark);
	Add(TEXT("Drawer"), FVector(23.5f, -70.f, 102.f), FVector(2.f, 38.f, 5.f), Steel);
	Add(TEXT("PosNeck"), FVector(6.f, -70.f, 116.f), FVector(5.f, 6.f, 14.f), Dark);
	Add(TEXT("PosScreen"), FVector(6.f, -70.f, 134.f), FVector(4.f, 40.f, 26.f), Dark, FRotator(-12.f, 0.f, 0.f));
	Add(TEXT("PosGlass"), FVector(8.1f, -70.f, 134.f), FVector(0.5f, 36.f, 22.f), FLinearColor(0.02f, 0.18f, 0.12f), FRotator(-12.f, 0.f, 0.f));
	// Card reader on the customer's side, receipt printer and scanner along the counter.
	Add(TEXT("CardReader"), FVector(-22.f, -20.f, 99.5f), FVector(10.f, 7.f, 2.5f), Dark, FRotator(0.f, 0.f, -10.f));
	Add(TEXT("PrinterBody"), FVector(6.f, -112.f, 103.f), FVector(14.f, 16.f, 12.f), Dark);
	Add(TEXT("PrinterPaper"), FVector(6.f, -112.f, 111.f), FVector(8.f, 8.f, 5.f), FLinearColor(0.96f, 0.96f, 0.94f));
	Add(TEXT("ScannerBase"), FVector(0.f, 40.f, 98.5f), FVector(22.f, 30.f, 2.f), Dark);
	Add(TEXT("ScannerGlass"), FVector(0.f, 40.f, 99.7f), FVector(16.f, 24.f, 0.5f), FLinearColor(0.9f, 0.1f, 0.1f));

	ScreenText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ScreenText"));
	ScreenText->SetupAttachment(GetRootComponent());
	ScreenText->SetRelativeLocation(FVector(9.f / 0.66f, -70.f / 2.56f, (134.f - 46.f) / 0.92f));
	ScreenText->SetRelativeRotation(FRotator(-12.f, 0.f, 0.f));
	ScreenText->SetRelativeScale3D(FVector(1.f / 0.66f, 1.f / 2.56f, 1.f / 0.92f));
	ScreenText->SetWorldSize(7.f);
	ScreenText->SetHorizontalAlignment(EHTA_Center);
	ScreenText->SetVerticalAlignment(EVRTA_TextCenter);
	ScreenText->SetTextRenderColor(FColor(80, 255, 170));
	ScreenText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScreenText->SetText(FText::FromString(TEXT("OPEN")));
}

void ATSSRegister::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	TSSApplyColor(Counter, FLinearColor(0.2f, 0.15f, 0.11f));
	for (int32 i = 0; i < Parts.Num(); ++i) TSSApplyColor(Parts[i], PartColors[i]);
}

void ATSSRegister::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TextTimer -= DeltaSeconds;
	if (TextTimer > 0.f) return;
	TextTimer = 0.2f;
	const ATSSCustomer* C = Front();
	FString Text = TEXT("OPEN");
	if (C && C->IsReadyToPay()) Text = FString::Printf(TEXT("TOTAL\n$%d"), C->GetAgreedPrice());
	else if (C) Text = TEXT("...");
	ScreenText->SetText(FText::FromString(Text));
}

ATSSCustomer* ATSSRegister::Front() const
{
	return Queue.Num() > 0 ? Queue[0].Get() : nullptr;
}

FVector ATSSRegister::QueueSpot(int32 Index) const
{
	// Customers stand on the shop side (-X in the register's own frame), a body-width apart.
	return GetActorTransform().TransformPosition(FVector(-80.f - 70.f * Index, 0.f, 0.f)) * FVector(1.f, 1.f, 0.f);
}

int32 ATSSRegister::Join(ATSSCustomer* Customer)
{
	Queue.RemoveAll([](const TWeakObjectPtr<ATSSCustomer>& C) { return !C.IsValid(); });
	Queue.AddUnique(Customer);
	return Queue.Num() - 1;
}

void ATSSRegister::Leave(ATSSCustomer* Customer)
{
	Queue.RemoveAll([Customer](const TWeakObjectPtr<ATSSCustomer>& C) { return !C.IsValid() || C.Get() == Customer; });
}

int32 ATSSRegister::IndexOf(const ATSSCustomer* Customer) const
{
	for (int32 i = 0; i < Queue.Num(); ++i)
	{
		if (Queue[i].Get() == Customer) return i;
	}
	return INDEX_NONE;
}

bool ATSSRegister::Checkout()
{
	ATSSCustomer* C = Front();
	if (!C || !C->IsReadyToPay()) return false;
	C->CompleteSale();
	return true;
}

FString ATSSRegister::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	const ATSSCustomer* C = Front();
	if (!C) return TEXT("Register\nNo customers");
	if (!C->IsReadyToPay()) return TEXT("Register\nA customer is walking up...");
	return FString::Printf(TEXT("[E] Ring up %s  ($%d)"), TSSProductDef(C->GetCarried()).Name, C->GetAgreedPrice());
}

void ATSSRegister::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	Checkout();
}
