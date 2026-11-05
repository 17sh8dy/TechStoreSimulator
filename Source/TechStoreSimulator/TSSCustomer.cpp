#include "TSSCustomer.h"
#include "TSSRegister.h"
#include "TSSCharacter.h"
#include "TSSHumanComponent.h"
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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// The person: feet on the floor (capsule bottom), facing +X.
	Human = CreateDefaultSubobject<UTSSHumanComponent>(TEXT("Human"));
	Human->SetupAttachment(Capsule);
	Human->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleHalf));
	Human->SetRelativeRotation(FRotator(0.f, -90.f, 0.f)); // the kit's meshes face +Y

	CarriedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Carried"));
	CarriedMesh->SetupAttachment(Capsule);
	CarriedMesh->SetStaticMesh(Cube.Object);
	CarriedMesh->SetRelativeLocation(FVector(34.f, 0.f, 5.f));
	CarriedMesh->SetRelativeScale3D(FVector(0.2f, 0.2f, 0.2f));
	CarriedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CarriedMesh->SetVisibility(false);

	Bubble = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Bubble"));
	Bubble->SetupAttachment(Capsule);
	Bubble->SetRelativeLocation(FVector(0.f, 0.f, 125.f));
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

	// Cast a person: face, clothes and shoes picked at random so a crowd does not look cloned.
	auto Pick = [](const TArray<FString>& V) { return V[FMath::RandRange(0, V.Num() - 1)]; };
	Human->Build(FMath::RandRange(1, UTSSHumanComponent::NumIdentities()),
		Pick(UTSSHumanComponent::Tops()), Pick(UTSSHumanComponent::Bottoms()), Pick(UTSSHumanComponent::Shoes()));

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

	// The basket: this item, plus up to two more from the same shelf if their price is fair too.
	Basket.Reset();
	auto TakeFrom = [&](int32 Slot) -> bool
	{
		const ETSSProduct P = Shelf->GetSlotProduct(Slot);
		if (Shelf->GetSlotQty(Slot) == 0 || P == ETSSProduct::None) return false;
		const int32 ItemPrice = Store->GetPrice(P);
		if (ItemPrice > FMath::RoundToInt(TSSProductDef(P).WholesaleCost * MaxMarkup)) return false;
		ETSSProduct Taken = ETSSProduct::None;
		if (!Shelf->TakeOne(Slot, Taken)) return false;
		Basket.Add({ Taken, ItemPrice, Slot });
		return true;
	};
	if (!TakeFrom(Wanted.Slot)) { WalkOut(TEXT("Sold out?!"), true); return; }

	const float Roll = FMath::FRand();
	const int32 WantCount = Roll < 0.5f ? 1 : (Roll < 0.85f ? 2 : 3);
	for (int32 n = 1; n < WantCount; ++n)
	{
		TArray<int32> Slots;
		for (int32 i = 0; i < ATSSShelf::Tiers * ATSSShelf::Columns; ++i) if (Shelf->GetSlotQty(i) > 0) Slots.Add(i);
		if (Slots.Num() == 0) break;
		TakeFrom(Slots[FMath::RandRange(0, Slots.Num() - 1)]);
	}

	const FTSSProductDef& First = TSSProductDef(Basket[0].Product);
	CarriedMesh->SetRelativeScale3D(First.BoxSize / 100.f);
	TSSApplyColor(CarriedMesh, First.Color);
	CarriedMesh->SetVisibility(true);

	Say(Basket.Num() > 1 ? FString::Printf(TEXT("I'll take these %d!"), Basket.Num()) : FString(TEXT("I'll take this one!")));
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
	// Put anything taken back where it came from if there is still room; otherwise it is simply lost.
	if (Wanted.Shelf.IsValid()) for (const FTSSBasketItem& B : Basket) Wanted.Shelf->PutBack(B.ShelfSlot, B.Product);
	Basket.Reset();
	CarriedMesh->SetVisibility(false);
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

// ---- checkout ---------------------------------------------------------------------------------------

void ATSSCustomer::StartCheckout()
{
	State = ETSSCustomerState::AtCounter;
	CarriedMesh->SetVisibility(false);
	Say(TEXT("Here you go."), 2.5f);
	Register->BeginTransaction(this, Basket);
}

void ATSSCustomer::PayNow(bool bInForceCash)
{
	bForceCash = bInForceCash;
	bPaying = true;
	PayTimer = 1.2f; // a moment to get the wallet out
}

void ATSSCustomer::DecidePayment()
{
	ETSSPayMethod Choice = ForceMethod != ETSSPayMethod::None ? ForceMethod : (FMath::FRand() < 0.55f ? ETSSPayMethod::Card : ETSSPayMethod::Cash);
	if (bForceCash) Choice = ETSSPayMethod::Cash;

	if (Choice == ETSSPayMethod::Card)
	{
		Say(TEXT("Card, please."), 3.f);
		Register->CustomerTapsCard();
		return;
	}

	// Cash: sometimes the exact amount, otherwise rounded up to a friendly note so the owner has change to count.
	const int32 Total = Register->GetTotal();
	int32 Amount = Total;
	if (FMath::FRand() >= 0.3f)
	{
		TArray<int32> Options;
		for (int32 Step : { 5, 10, 20, 50, 100 }) Options.AddUnique(((Total + Step - 1) / Step) * Step);
		Amount = Options[FMath::RandRange(0, Options.Num() - 1)];
	}
	Say(FString::Printf(TEXT("Cash. Here's $%d."), Amount), 3.f);
	Register->CustomerPaysCash(Amount);
}

void ATSSCustomer::FinishCheckout()
{
	Say(TEXT("Thanks, bye!"), 3.f);
	if (Register) Register->Leave(this);
	bQueued = false;
	bPaying = false;
	// They carry their shopping out in a bag.
	CarriedMesh->SetRelativeScale3D(FVector(0.22f, 0.16f, 0.3f));
	TSSApplyColor(CarriedMesh, FLinearColor(0.9f, 0.9f, 0.92f));
	CarriedMesh->SetVisibility(true);
	State = ETSSCustomerState::Leaving;
	StartPath({ Register->HubPoint, Register->DoorPoint, Register->SpawnPoint });
}

FString ATSSCustomer::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	switch (State)
	{
	case ETSSCustomerState::AtCounter: return TEXT("Customer at the register");
	case ETSSCustomerState::Queueing: return TEXT("Customer waiting in line");
	case ETSSCustomerState::Leaving: return FString();
	default: return TEXT("Customer browsing");
	}
}

void ATSSCustomer::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
}

void ATSSCustomer::Animate(float Dt, bool bWalking)
{
	Human->SetWalking(bWalking);
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
		if (IsReadyToPay() && Register->GetPhase() == ETSSCheckoutPhase::Idle)
		{
			StartCheckout();
			break;
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

	case ETSSCustomerState::AtCounter:
		FaceToward(Register->GetActorLocation(), Dt);
		if (bPaying)
		{
			PayTimer -= Dt * SpeedScale;
			if (PayTimer <= 0.f)
			{
				bPaying = false;
				DecidePayment();
			}
		}
		break;

	case ETSSCustomerState::Leaving:
		if (FollowPath(Dt)) Destroy();
		break;
	}

	Animate(Dt, !(GetActorLocation() - Start).IsNearlyZero(0.01f));
}
