#include "TSSCustomer.h"
#include "TSSRegister.h"
#include "TSSCharacter.h"
#include "TSSStoreSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CapsuleHalf = 90.f;
}

ATSSCustomer::ATSSCustomer()
{
	PrimaryActorTick.bCanEverTick = true;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	SetRootComponent(Capsule);
	Capsule->InitCapsuleSize(30.f, CapsuleHalf);
	// Only the owner's crosshair sees customers; they never block the player or each other.
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Capsule);

	auto Part = [this](const TCHAR* Name, UStaticMesh* Mesh, USceneComponent* Parent, FVector Loc, FVector Scale)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Parent);
		C->SetStaticMesh(Mesh);
		C->SetRelativeLocation(Loc);
		C->SetRelativeScale3D(Scale);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return C;
	};

	// Origin is the capsule centre (waist height); feet are at -90.
	Torso = Part(TEXT("Torso"), Cylinder.Object, Visual, FVector(0.f, 0.f, 17.f), FVector(0.5f, 0.5f, 0.95f));
	Head = Part(TEXT("Head"), Sphere.Object, Visual, FVector(0.f, 0.f, 75.f), FVector(0.28f, 0.28f, 0.28f));
	HipL = CreateDefaultSubobject<USceneComponent>(TEXT("HipL"));
	HipL->SetupAttachment(Visual);
	HipL->SetRelativeLocation(FVector(0.f, -10.f, -30.f));
	HipR = CreateDefaultSubobject<USceneComponent>(TEXT("HipR"));
	HipR->SetupAttachment(Visual);
	HipR->SetRelativeLocation(FVector(0.f, 10.f, -30.f));
	LegL = Part(TEXT("LegL"), Cylinder.Object, HipL, FVector(0.f, 0.f, -30.f), FVector(0.16f, 0.16f, 0.6f));
	LegR = Part(TEXT("LegR"), Cylinder.Object, HipR, FVector(0.f, 0.f, -30.f), FVector(0.16f, 0.16f, 0.6f));
	CarriedMesh = Part(TEXT("Carried"), Cube.Object, Visual, FVector(34.f, 0.f, 5.f), FVector(0.2f, 0.2f, 0.2f));
	CarriedMesh->SetVisibility(false);

	Bubble = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Bubble"));
	Bubble->SetupAttachment(Capsule);
	Bubble->SetRelativeLocation(FVector(0.f, 0.f, 135.f));
	Bubble->SetWorldSize(13.f);
	Bubble->SetHorizontalAlignment(EHTA_Center);
	Bubble->SetTextRenderColor(FColor::White);
	Bubble->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bubble->SetText(FText::GetEmpty());
}

void ATSSCustomer::Begin(ATSSRegister* InRegister, float InSpeedScale)
{
	Register = InRegister;
	SpeedScale = InSpeedScale;
	MaxMarkup = FMath::FRandRange(1.45f, 2.1f);

	// A little variety so a crowd does not look cloned.
	const FLinearColor Shirt = FLinearColor::MakeFromHSV8(static_cast<uint8>(FMath::RandRange(0, 255)), 170, 215);
	static const FLinearColor Skin[] = {
		FLinearColor(0.80f, 0.60f, 0.46f), FLinearColor(0.60f, 0.40f, 0.28f),
		FLinearColor(0.36f, 0.23f, 0.16f), FLinearColor(0.92f, 0.76f, 0.64f) };
	TSSApplyColor(Torso, Shirt);
	TSSApplyColor(Head, Skin[FMath::RandRange(0, 3)]);
	TSSApplyColor(LegL, FLinearColor(0.08f, 0.1f, 0.2f));
	TSSApplyColor(LegR, FLinearColor(0.08f, 0.1f, 0.2f));

	SetActorLocation(FVector(Register->SpawnPoint.X, Register->SpawnPoint.Y, CapsuleHalf));
	ChooseItem();
}

void ATSSCustomer::Say(const FString& Text, float Seconds)
{
	Bubble->SetText(FText::FromString(Text));
	BubbleTimer = Seconds;
}

void ATSSCustomer::StartPath(const TArray<FVector>& Points)
{
	Path = Points;
	PathIndex = 0;
}

void ATSSCustomer::FaceToward(const FVector& Target, float Dt)
{
	FVector D = Target - GetActorLocation();
	D.Z = 0.f;
	if (D.SizeSquared() < 1.f) return;
	const FRotator Want(0.f, D.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Want, Dt, 540.f));
}

bool ATSSCustomer::MoveToward(const FVector& Target, float Dt)
{
	const FVector Goal(Target.X, Target.Y, CapsuleHalf);
	const FVector Delta = Goal - GetActorLocation();
	const float Dist = Delta.Size();
	const float Step = WalkSpeed * SpeedScale * Dt;
	if (Dist <= Step)
	{
		SetActorLocation(Goal);
		return true;
	}
	SetActorLocation(GetActorLocation() + Delta / Dist * Step);
	FaceToward(Goal, Dt);
	return false;
}

bool ATSSCustomer::FollowPath(float Dt)
{
	if (PathIndex >= Path.Num()) return true;
	if (MoveToward(Path[PathIndex], Dt)) ++PathIndex;
	return PathIndex >= Path.Num();
}

void ATSSCustomer::ChooseItem()
{
	TArray<FTSSStockRef> Stock;
	ATSSShelf::CollectStock(GetWorld(), Stock);
	if (Stock.Num() == 0)
	{
		WalkOut(TEXT("Nothing in stock..."), true);
		return;
	}
	Wanted = Stock[FMath::RandRange(0, Stock.Num() - 1)];
	WantedProduct = Wanted.Shelf->GetSlotProduct(Wanted.Slot);
	State = ETSSCustomerState::Entering;
	StartPath({ Register->DoorPoint, Register->HubPoint, Wanted.Shelf->GetStandPoint(Wanted.Slot) });
}

void ATSSCustomer::DecideToBuy()
{
	UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	ATSSShelf* Shelf = Wanted.Shelf.Get();
	if (!Shelf || Shelf->GetSlotQty(Wanted.Slot) == 0 || Shelf->GetSlotProduct(Wanted.Slot) != WantedProduct)
	{
		WalkOut(TEXT("Sold out?!"), true);
		return;
	}

	const FTSSProductDef& Def = TSSProductDef(WantedProduct);
	const int32 Price = Store->GetPrice(WantedProduct);
	if (Price > FMath::RoundToInt(Def.WholesaleCost * MaxMarkup))
	{
		WalkOut(FString::Printf(TEXT("$%d? Too expensive!"), Price), true);
		return;
	}

	ETSSProduct Taken = ETSSProduct::None;
	if (!Shelf->TakeOne(Wanted.Slot, Taken)) { WalkOut(TEXT("Sold out?!"), true); return; }
	Carried = Taken;
	AgreedPrice = Price;
	CarriedMesh->SetRelativeScale3D(TSSProductDef(Carried).BoxSize / 100.f);
	TSSApplyColor(CarriedMesh, TSSProductDef(Carried).Color);
	CarriedMesh->SetVisibility(true);

	Say(TEXT("I'll take this one!"));
	Register->Join(this);
	bQueued = true;
	State = ETSSCustomerState::Queueing;
	WaitedInLine = 0.f;
	StartPath({ Register->HubPoint });
}

void ATSSCustomer::WalkOut(const FString& Reason, bool bUnhappy)
{
	if (Register) Register->Leave(this);
	bQueued = false;
	if (Carried != ETSSProduct::None)
	{
		// Put it back where it came from if there is still room; otherwise it is simply lost.
		if (Wanted.Shelf.IsValid()) Wanted.Shelf->PutBack(Wanted.Slot, Carried);
		Carried = ETSSProduct::None;
		CarriedMesh->SetVisibility(false);
	}
	if (bUnhappy)
	{
		if (UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>()) Store->RecordWalkout();
	}
	Say(Reason, 3.f);
	State = ETSSCustomerState::Leaving;
	StartPath({ Register->HubPoint, Register->DoorPoint, Register->SpawnPoint });
}

bool ATSSCustomer::IsReadyToPay() const
{
	if (State != ETSSCustomerState::Queueing || !Register || Register->IndexOf(this) != 0) return false;
	return FVector::DistSquared2D(GetActorLocation(), Register->QueueSpot(0)) < 15.f * 15.f;
}

void ATSSCustomer::CompleteSale()
{
	if (UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>()) Store->RecordSale(AgreedPrice);
	Register->Leave(this);
	bQueued = false;
	Say(TEXT("Thanks!"), 3.f);
	State = ETSSCustomerState::Leaving;
	StartPath({ Register->HubPoint, Register->DoorPoint, Register->SpawnPoint });
}

FString ATSSCustomer::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	if (IsReadyToPay())
	{
		return FString::Printf(TEXT("[E] Ring up %s  ($%d)"), TSSProductDef(Carried).Name, AgreedPrice);
	}
	switch (State)
	{
	case ETSSCustomerState::Queueing: return TEXT("Customer waiting in line");
	case ETSSCustomerState::Leaving: return FString();
	default: return TEXT("Customer browsing");
	}
}

void ATSSCustomer::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	if (IsReadyToPay()) CompleteSale();
}

void ATSSCustomer::Animate(float Dt, bool bWalking)
{
	if (bWalking)
	{
		WalkPhase += Dt * SpeedScale * 9.f;
		const float Swing = FMath::Sin(WalkPhase) * 30.f;
		HipL->SetRelativeRotation(FRotator(Swing, 0.f, 0.f));
		HipR->SetRelativeRotation(FRotator(-Swing, 0.f, 0.f));
		Visual->SetRelativeLocation(FVector(0.f, 0.f, FMath::Abs(FMath::Sin(WalkPhase)) * 2.f));
	}
	else
	{
		HipL->SetRelativeRotation(FMath::RInterpTo(HipL->GetRelativeRotation(), FRotator::ZeroRotator, Dt, 12.f));
		HipR->SetRelativeRotation(FMath::RInterpTo(HipR->GetRelativeRotation(), FRotator::ZeroRotator, Dt, 12.f));
		Visual->SetRelativeLocation(FVector::ZeroVector);
	}
}

void ATSSCustomer::Tick(float Dt)
{
	Super::Tick(Dt);

	// Speech bubble: fades out and always reads from the player's side.
	if (BubbleTimer > 0.f)
	{
		BubbleTimer -= Dt;
		if (BubbleTimer <= 0.f) Bubble->SetText(FText::GetEmpty());
	}
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		FRotator Face = (Cam->GetCameraLocation() - Bubble->GetComponentLocation()).Rotation();
		Face.Pitch = 0.f;
		Bubble->SetWorldRotation(Face);
	}

	UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	if (!Register || !Store || Store->IsSimPaused()) return;

	const FVector Start = GetActorLocation();

	switch (State)
	{
	case ETSSCustomerState::Entering:
		if (FollowPath(Dt))
		{
			State = ETSSCustomerState::Looking;
			LookTimer = FMath::FRandRange(2.f, 4.f);
		}
		break;

	case ETSSCustomerState::Looking:
		if (Wanted.Shelf.IsValid()) FaceToward(Wanted.Shelf->GetSlotWorldPos(Wanted.Slot), Dt);
		LookTimer -= Dt * SpeedScale;
		if (LookTimer <= 0.f) DecideToBuy();
		break;

	case ETSSCustomerState::Queueing:
	{
		if (!FollowPath(Dt))
		{
			// Still on the way to the hub.
		}
		else
		{
			const int32 Index = Register->IndexOf(this);
			if (Index != INDEX_NONE) MoveToward(Register->QueueSpot(Index), Dt);
			if (IsReadyToPay() || FVector::DistSquared2D(GetActorLocation(), Register->QueueSpot(FMath::Max(Index, 0))) < 15.f * 15.f)
			{
				FaceToward(Register->GetActorLocation(), Dt);
			}
		}
		WaitedInLine += Dt;
		if (!bSaidHurry && WaitedInLine > Patience * 0.55f)
		{
			bSaidHurry = true;
			Say(TEXT("Hello?? I'm waiting!"));
		}
		if (WaitedInLine > Patience) WalkOut(TEXT("Forget it, I'm leaving!"), true);
		break;
	}

	case ETSSCustomerState::Leaving:
		if (FollowPath(Dt)) Destroy();
		break;
	}

	Animate(Dt, !(GetActorLocation() - Start).IsNearlyZero(0.01f));
}
