#include "TSSCustomer.h"
#include "TSSRegister.h"
#include "TSSCharacter.h"
#include "TSSHumanComponent.h"
#include "TSSProductBox.h"
#include "TSSStoreSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float CapsuleHalf = 90.f;
	constexpr ETSSHand RHand = ETSSHand::Right;
	constexpr ETSSHand LHand = ETSSHand::Left;
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

	// The person: feet on the floor (capsule bottom), facing +X.
	Human = CreateDefaultSubobject<UTSSHumanComponent>(TEXT("Human"));
	Human->SetupAttachment(Capsule);
	Human->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleHalf));
	Human->SetRelativeRotation(FRotator(0.f, -90.f, 0.f)); // the kit's meshes face +Y

	CarriedMesh = CreateDefaultSubobject<UTSSProductBoxComponent>(TEXT("Carried"));
	CarriedMesh->SetupAttachment(Capsule);
	CarriedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CarriedMesh->SetVisibility(false, true);

	HandProp = CreateDefaultSubobject<UTSSProductBoxComponent>(TEXT("HandProp"));
	HandProp->SetupAttachment(Capsule);
	HandProp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandProp->SetVisibility(false, true);

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

// ---- the gesture script ---------------------------------------------------------------------------------

ATSSCustomer::FStep ATSSCustomer::Do(TFunction<void()> Action) const
{
	FStep S;
	S.Enter = MoveTemp(Action);
	S.Until = [](float) { return true; };
	return S;
}

ATSSCustomer::FStep ATSSCustomer::Wait(float Seconds) const
{
	FStep S;
	S.Until = [this, Seconds](float) { return StepTime >= Seconds; };
	return S;
}

ATSSCustomer::FStep ATSSCustomer::ReachTo(ETSSHand Hand, TFunction<FVector()> Target) const
{
	FStep S;
	S.Enter = [this, Hand, Target]() { Human->Reach(Hand, Target(), BlendSeconds()); };
	S.Until = [this, Hand, Target](float)
	{
		Human->Reach(Hand, Target(), BlendSeconds());   // keep the target fresh if it moves
		return Human->IsReachSettled(Hand, 0.93f) || StepTime > 6.f;
	};
	return S;
}

ATSSCustomer::FStep ATSSCustomer::WaitFor(TFunction<bool()> Condition) const
{
	FStep S;
	S.Until = [this, Condition](float) { return Condition() || StepTime > 20.f; };
	return S;
}

void ATSSCustomer::SetScript(FScript NewScript)
{
	Script = MoveTemp(NewScript);
	ScriptIndex = 0;
	bStepEntered = false;
	++ScriptEpoch;
}

void ATSSCustomer::ReplaceRestOfScript(FScript NewSteps)
{
	if (Script.Num() == 0) { SetScript(MoveTemp(NewSteps)); return; }
	Script.SetNum(FMath::Min(ScriptIndex + 1, Script.Num()));
	Script.Append(MoveTemp(NewSteps));
}

void ATSSCustomer::ClearScript()
{
	Script.Reset();
	ScriptIndex = 0;
	bStepEntered = false;
	++ScriptEpoch;
}

void ATSSCustomer::RunScript(float Dt)
{
	if (!Script.IsValidIndex(ScriptIndex)) { if (Script.Num() > 0) ClearScript(); return; }
	const FStep Step = Script[ScriptIndex];   // a copy: the step may replace the script while it runs
	const int32 Epoch = ScriptEpoch;
	if (!bStepEntered)
	{
		bStepEntered = true;
		StepTime = 0.f;
		if (Step.Enter) Step.Enter();
		if (Epoch != ScriptEpoch) return;     // the step started a different script
	}
	StepTime += Dt;
	if (!Step.Until || Step.Until(Dt))
	{
		if (Epoch != ScriptEpoch) return;
		++ScriptIndex;
		bStepEntered = false;
		if (ScriptIndex >= Script.Num()) ClearScript();
	}
}

// ---- movement ---------------------------------------------------------------------------------------------

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
	const float Step = WalkSpeed * SpeedScale * (bRunning ? 1.7f : 1.f) * Dt;
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

FVector ATSSCustomer::ChestPoint(ETSSHand Hand) const
{
	return GetActorLocation() + GetActorForwardVector() * 38.f + GetActorRightVector() * (Hand == RHand ? 14.f : -14.f) + FVector(0.f, 0.f, 22.f);
}

FVector ATSSCustomer::HipPoint() const
{
	return GetActorLocation() + GetActorForwardVector() * 6.f + GetActorRightVector() * 20.f + FVector(0.f, 0.f, -6.f);
}

void ATSSCustomer::ShowHeld(UTSSProductBoxComponent* Mesh, bool bShow)
{
	Mesh->SetVisibility(bShow, true);
}

void ATSSCustomer::FollowRightHand(UTSSProductBoxComponent* Mesh, float DepthCm)
{
	if (!Mesh->IsVisible()) return;
	const FRotator Rot(0.f, GetActorRotation().Yaw, 0.f);
	Mesh->SetWorldLocationAndRotation(Human->GetHandWorldLocation(RHand) + Rot.Vector() * (DepthCm * 0.5f + 3.f), Rot);
}

// ---- the shopping trip -------------------------------------------------------------------------------------

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
	const float Roll = FMath::FRand();
	WantCount = Roll < 0.5f ? 1 : (Roll < 0.85f ? 2 : 3);
	State = ETSSCustomerState::Entering;
	StartPath({ Register->DoorPoint, Register->HubPoint, Wanted.Shelf->GetStandPoint(Wanted.Slot) });
}

void ATSSCustomer::BeginBrowsing()
{
	State = ETSSCustomerState::Looking;
	const FVector SlotPos = Wanted.Shelf->GetSlotWorldPos(Wanted.Slot);
	FScript S;
	S.Add(Do([this, SlotPos]() { Human->LookAt(SlotPos); }));
	S.Add(Wait(FMath::FRandRange(0.9f, 1.8f)));                                        // look at what is on the shelf
	S.Add(ReachTo(RHand, [SlotPos]() { return SlotPos; }));                            // reach out
	S.Add(Do([this]() { GrabFirstItem(); }));                                          // take a box
	S.Add(ReachTo(RHand, [this]() { return ChestPoint(RHand); }));                     // bring it up to look at
	S.Add(Do([this]() { Human->LookAt(Human->GetHandWorldLocation(RHand)); }));
	S.Add(Wait(1.0f));                                                                 // read the box
	S.Add(Do([this]() { DecideAfterExamining(); }));
	SetScript(MoveTemp(S));
}

void ATSSCustomer::GrabFirstItem()
{
	UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	ATSSShelf* Shelf = Wanted.Shelf.Get();
	ETSSProduct Taken = ETSSProduct::None;
	if (!Shelf || Shelf->GetSlotQty(Wanted.Slot) == 0 || Shelf->GetSlotProduct(Wanted.Slot) != WantedProduct || !Shelf->TakeOne(Wanted.Slot, Taken))
	{
		WalkOut(TEXT("Sold out?!"), true);
		return;
	}
	Basket.Reset();
	Basket.Add({ Taken, Store->GetPrice(Taken), Wanted.Slot });
	CarriedMesh->SetProduct(Taken);
	CarriedDepth = TSSProductDef(Taken).BoxSize.X;
	ShowHeld(CarriedMesh, true);
	bCarrying = true;
}

void ATSSCustomer::DecideAfterExamining()
{
	UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	ATSSShelf* Shelf = Wanted.Shelf.Get();
	if (!Shelf || Basket.Num() == 0) { WalkOut(TEXT("Hmm."), true); return; }

	const FTSSProductDef& Def = TSSProductDef(Basket[0].Product);
	const int32 Price = Basket[0].Price;
	if (Price > FMath::RoundToInt(Def.WholesaleCost * MaxMarkup))
	{
		// Too dear: put it back where it was, then leave.
		const FVector SlotPos = Shelf->GetSlotWorldPos(Wanted.Slot);
		FScript S;
		S.Add(Do([this, Price]() { Say(FString::Printf(TEXT("$%d? Hmm..."), Price), 2.f); }));
		S.Add(ReachTo(RHand, [SlotPos]() { return SlotPos; }));
		S.Add(Do([this]()
		{
			if (Wanted.Shelf.IsValid() && Basket.Num() > 0) Wanted.Shelf->PutBack(Basket[0].ShelfSlot, Basket[0].Product);
			Basket.Reset();
			ShowHeld(CarriedMesh, false);
			bCarrying = false;
		}));
		S.Add(Wait(0.3f));
		S.Add(Do([this, Price]() { WalkOut(FString::Printf(TEXT("$%d? Too expensive!"), Price), true); }));
		ReplaceRestOfScript(MoveTemp(S));
		return;
	}

	// Fair price. Maybe a couple more things from the same shelf, picked with the other hand.
	FScript S;
	for (int32 n = 1; n < WantCount; ++n)
	{
		TArray<int32> Slots;
		for (int32 i = 0; i < ATSSShelf::Tiers * ATSSShelf::Columns; ++i)
		{
			const ETSSProduct P = Shelf->GetSlotProduct(i);
			if (Shelf->GetSlotQty(i) > 0 && Store->GetPrice(P) <= FMath::RoundToInt(TSSProductDef(P).WholesaleCost * MaxMarkup)) Slots.Add(i);
		}
		if (Slots.Num() == 0) break;
		const int32 Slot = Slots[FMath::RandRange(0, Slots.Num() - 1)];
		const FVector Pos = Shelf->GetSlotWorldPos(Slot);
		S.Add(Do([this, Pos]() { Human->LookAt(Pos); }));
		S.Add(ReachTo(LHand, [Pos]() { return Pos; }));
		S.Add(Do([this, Slot]()
		{
			ATSSShelf* Sh = Wanted.Shelf.Get();
			UTSSStoreSubsystem* St = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
			ETSSProduct Taken = ETSSProduct::None;
			if (Sh && Sh->GetSlotQty(Slot) > 0 && Sh->TakeOne(Slot, Taken)) Basket.Add({ Taken, St->GetPrice(Taken), Slot });
		}));
		S.Add(Wait(0.15f));
	}
	S.Add(Do([this]() { FinishBrowsing(); }));
	ReplaceRestOfScript(MoveTemp(S));
}

void ATSSCustomer::FinishBrowsing()
{
	Human->Relax(LHand, BlendSeconds());
	Human->StopLooking();
	Say(Basket.Num() > 1 ? FString::Printf(TEXT("I'll take these %d!"), Basket.Num()) : FString(TEXT("I'll take this one!")));
	Register->Join(this);
	bQueued = true;
	State = ETSSCustomerState::Queueing;
	WaitedInLine = 0.f;
	StartPath({ Register->HubPoint });
}

void ATSSCustomer::WalkOut(const FString& Reason, bool bUnhappy)
{
	ClearScript();
	Human->Relax(RHand, BlendSeconds());
	Human->Relax(LHand, BlendSeconds());
	Human->StopLooking();
	if (Register) Register->Leave(this);
	bQueued = false;
	// Put anything taken back where it came from if there is still room; otherwise it is simply lost.
	if (Wanted.Shelf.IsValid()) for (const FTSSBasketItem& B : Basket) Wanted.Shelf->PutBack(B.ShelfSlot, B.Product);
	Basket.Reset();
	ShowHeld(CarriedMesh, false);
	ShowHeld(HandProp, false);
	bCarrying = false;
	if (bUnhappy)
	{
		if (UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>()) Store->RecordWalkout();
	}
	bRunning = bUnhappy;   // storming out
	Say(Reason, 3.f);
	State = ETSSCustomerState::Leaving;
	StartPath({ Register->HubPoint, Register->DoorPoint, Register->SpawnPoint });
}

bool ATSSCustomer::IsReadyToPay() const
{
	if (State != ETSSCustomerState::Queueing || !Register || Register->IndexOf(this) != 0) return false;
	return FVector::DistSquared2D(GetActorLocation(), Register->QueueSpot(0)) < 15.f * 15.f;
}

// ---- checkout ---------------------------------------------------------------------------------------------

void ATSSCustomer::StartCheckout()
{
	State = ETSSCustomerState::AtCounter;
	Say(TEXT("Here you go."), 2.5f);
	Register->BeginTransaction(this, Basket);   // the boxes are made but stay hidden until put down

	FScript S;
	for (int32 i = 0; i < Basket.Num(); ++i)
	{
		S.Add(ReachTo(RHand, [this, i]()
		{
			const float H = Basket.IsValidIndex(i) ? TSSProductDef(Basket[i].Product).BoxSize.Z * 0.5f : 8.f;
			return Register->StagingPoint(i) + FVector(0.f, 0.f, H + 12.f);
		}));
		S.Add(Do([this, i]()
		{
			Register->RevealItem(i);
			if (i == 0) { ShowHeld(CarriedMesh, false); bCarrying = false; }
		}));
		S.Add(Wait(0.25f));
	}
	S.Add(Do([this]() { Human->Relax(RHand, BlendSeconds()); }));
	SetScript(MoveTemp(S));
}

void ATSSCustomer::PayNow(bool bInForceCash)
{
	bForceCash = bInForceCash;
	FScript S;
	S.Add(Do([this]() { ShowHeld(HandProp, false); Human->Relax(RHand, BlendSeconds()); }));
	S.Add(Wait(0.8f));   // a moment to get the wallet out
	S.Add(Do([this]() { BeginPaymentGestures(); }));
	SetScript(MoveTemp(S));
}

void ATSSCustomer::BeginPaymentGestures()
{
	ETSSPayMethod Choice = ForceMethod != ETSSPayMethod::None ? ForceMethod : (FMath::FRand() < 0.55f ? ETSSPayMethod::Card : ETSSPayMethod::Cash);
	if (bForceCash) Choice = ETSSPayMethod::Cash;

	FScript S;
	if (Choice == ETSSPayMethod::Card)
	{
		S.Add(Do([this]() { Say(TEXT("Card, please."), 3.f); }));
		S.Add(ReachTo(RHand, [this]() { return HipPoint(); }));
		S.Add(Do([this]() { HandProp->SetPlain(FVector(8.5f, 5.4f, 0.4f), FLinearColor(0.08f, 0.25f, 0.8f)); ShowHeld(HandProp, true); }));
		S.Add(Wait(0.2f));
		S.Add(ReachTo(RHand, [this]() { return Register->TerminalTapPoint(); }));
		S.Add(Do([this]() { Register->CustomerTapsCard(); }));
		// Hold the card to the terminal until the payment has gone through (or been declined).
		S.Add(WaitFor([this]()
		{
			const ETSSCheckoutPhase P = Register->GetPhase();
			return !(P == ETSSCheckoutPhase::CardTap || P == ETSSCheckoutPhase::CardReady || P == ETSSCheckoutPhase::CardProcessing || P == ETSSCheckoutPhase::CardResult);
		}));
		S.Add(Do([this]() { ShowHeld(HandProp, false); Human->Relax(RHand, BlendSeconds()); }));
	}
	else
	{
		// Cash: sometimes the exact amount, otherwise rounded up to a friendly note so the owner has change to count.
		const int32 Total = Register->GetTotal();
		int32 Amount = Total;
		if (FMath::FRand() >= 0.3f)
		{
			TArray<int32> Options;
			for (int32 Step : { 5, 10, 20, 50, 100 }) Options.AddUnique(((Total + Step - 1) / Step) * Step);
			Amount = Options[FMath::RandRange(0, Options.Num() - 1)];
		}
		S.Add(Do([this, Amount]() { Say(FString::Printf(TEXT("Cash. Here's $%d."), Amount), 3.f); }));
		S.Add(ReachTo(RHand, [this]() { return HipPoint(); }));
		S.Add(Do([this]() { HandProp->SetPlain(FVector(16.f, 7.f, 1.6f), FLinearColor(0.25f, 0.6f, 0.3f)); ShowHeld(HandProp, true); }));
		S.Add(Wait(0.2f));
		S.Add(ReachTo(RHand, [this]() { return Register->CashPoint() + FVector(0.f, 0.f, 8.f); }));
		S.Add(Do([this, Amount]() { ShowHeld(HandProp, false); Register->CustomerPaysCash(Amount); }));
		S.Add(Wait(0.25f));
		S.Add(Do([this]() { Human->Relax(RHand, BlendSeconds()); }));
	}
	ReplaceRestOfScript(MoveTemp(S));
}

void ATSSCustomer::FinishCheckout()
{
	Say(TEXT("Thanks, bye!"), 3.f);
	State = ETSSCustomerState::Collecting;
	FScript S;
	S.Add(ReachTo(RHand, [this]() { return Register->BaggingPoint(0) + FVector(0.f, 0.f, 18.f); }));   // sweep the shopping up
	S.Add(Do([this]()
	{
		Register->ReleaseSale();
		Register->Leave(this);
		bQueued = false;
		// They carry their shopping out in a bag.
		CarriedMesh->SetPlain(FVector(22.f, 16.f, 30.f), FLinearColor(0.9f, 0.9f, 0.92f));
		CarriedDepth = 16.f;
		ShowHeld(CarriedMesh, true);
		bCarrying = true;
	}));
	S.Add(Wait(0.3f));
	S.Add(Do([this]()
	{
		State = ETSSCustomerState::Leaving;
		StartPath({ Register->HubPoint, Register->DoorPoint, Register->SpawnPoint });
	}));
	SetScript(MoveTemp(S));
}

FString ATSSCustomer::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	switch (State)
	{
	case ETSSCustomerState::AtCounter:
	case ETSSCustomerState::Collecting: return TEXT("Customer at the register");
	case ETSSCustomerState::Queueing: return TEXT("Customer waiting in line");
	case ETSSCustomerState::Leaving: return FString();
	default: return TEXT("Customer browsing");
	}
}

void ATSSCustomer::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
}

// ---- per frame ----------------------------------------------------------------------------------------------

void ATSSCustomer::Tick(float Dt)
{
	Super::Tick(Dt);

	// Speech bubble: fades out and always reads from the player's side.
	if (BubbleTimer > 0.f)
	{
		BubbleTimer -= Dt;
		if (BubbleTimer <= 0.f) Bubble->SetText(FText::GetEmpty());
	}
	FVector CameraPos = GetActorLocation() + FVector(200.f, 0.f, 60.f);
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		CameraPos = Cam->GetCameraLocation();
		FRotator Face = (CameraPos - Bubble->GetComponentLocation()).Rotation();
		Face.Pitch = 0.f;
		Bubble->SetWorldRotation(Face);
	}

	UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	if (!Register || !Store || Store->IsSimPaused())
	{
		Human->SetSpeed(0.f);
		return;
	}

	const FVector Start = GetActorLocation();

	switch (State)
	{
	case ETSSCustomerState::Entering:
		if (FollowPath(Dt)) BeginBrowsing();
		break;

	case ETSSCustomerState::Looking:
		if (Wanted.Shelf.IsValid()) FaceToward(Wanted.Shelf->GetSlotWorldPos(Wanted.Slot), Dt);
		RunScript(Dt * SpeedScale);
		break;

	case ETSSCustomerState::Queueing:
	{
		if (FollowPath(Dt))
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
	case ETSSCustomerState::Collecting:
		FaceToward(Register->GetActorLocation(), Dt);
		RunScript(Dt * SpeedScale);
		break;

	case ETSSCustomerState::Leaving:
		if (FollowPath(Dt)) Destroy();
		break;
	}

	// ---- body language ------------------------------------------------------------------------------------------
	const float Moved = Dt > 0.f ? (GetActorLocation() - Start).Size2D() / Dt : 0.f;
	Human->SetSpeed(Moved);

	// Look at the owner when talking business; otherwise ahead.
	const bool bStanding = Moved < 5.f;
	if (State == ETSSCustomerState::AtCounter || State == ETSSCustomerState::Collecting || (State == ETSSCustomerState::Queueing && bStanding))
	{
		if (!IsBusyGesturing()) Human->LookAt(CameraPos);
	}
	else if (State != ETSSCustomerState::Looking)
	{
		Human->StopLooking();
	}

	// Between tasks a held box rests against the chest; empty hands drop to the side.
	if (!IsBusyGesturing())
	{
		if (bCarrying) Human->Reach(RHand, ChestPoint(RHand), BlendSeconds());
		else Human->Relax(RHand, BlendSeconds());
	}

	if (CarriedMesh->IsVisible() && CarriedMesh->IsPlain())
	{
		bBagSeen = true;
		if (CarriedMesh->IsPanelDrawn()) bStrayPanelSeen = true;
	}
	FollowRightHand(CarriedMesh, CarriedDepth);
	FollowRightHand(HandProp, 8.f);
}
