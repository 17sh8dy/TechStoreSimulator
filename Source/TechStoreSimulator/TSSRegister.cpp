#include "TSSRegister.h"
#include "TSSCustomer.h"
#include "TSSCharacter.h"
#include "TSSProductItem.h"
#include "TSSRegisterDevice.h"
#include "TSSStoreSubsystem.h"
#include "TSSBusinessSubsystem.h"
#include "TSSMoneyVisual.h"
#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

ATSSRegister::ATSSRegister()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Cabinet and counter top: the surface the whole sale happens on.
	Cabinet = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cabinet"));
	Cabinet->SetupAttachment(Root);
	Cabinet->SetStaticMesh(TSSCubeMesh());
	Cabinet->SetRelativeLocation(FVector(0.f, 0.f, 46.f));
	Cabinet->SetRelativeScale3D(FVector(0.66f, 2.56f, 0.92f));

	auto Add = [this, Root](const TCHAR* Name, FVector Loc, FVector Size, FLinearColor Color)
	{
		UStaticMeshComponent* P = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		P->SetupAttachment(Root);
		P->SetStaticMesh(TSSCubeMesh());
		P->SetRelativeLocation(Loc);
		P->SetRelativeScale3D(Size / 100.f);
		Parts.Add(P);
		PartColors.Add(Color);
		return P;
	};
	Add(TEXT("Kick"), FVector(0.f, 0.f, 4.f), FVector(60.f, 250.f, 8.f), FLinearColor(0.05f, 0.05f, 0.06f));
	Add(TEXT("Top"), FVector(-2.f, 0.f, 95.f), FVector(80.f, 270.f, 6.f), FLinearColor(0.9f, 0.9f, 0.92f));
	Add(TEXT("Panel"), FVector(-34.f, 0.f, 48.f), FVector(2.f, 220.f, 64.f), FLinearColor(0.08f, 0.45f, 0.9f));
	Add(TEXT("PanelTrim"), FVector(-34.5f, 0.f, 82.f), FVector(1.f, 230.f, 4.f), FLinearColor(0.95f, 0.95f, 1.f));

	// The customer's cash, lying on the counter while it is being counted.
CashPile = Add(TEXT("CashPile"), FVector(6.f, 76.f, TopZ + 1.5f), FVector(2.f, 2.f, 2.f), FLinearColor(0.25f, 0.6f, 0.3f));
	CashPile->SetVisibility(false);
	CashText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CashText"));
	CashText->SetupAttachment(Root);
CashText->SetRelativeLocation(FVector(6.f, 76.f, TopZ + 14.f));
	CashText->SetWorldSize(7.f);
	CashText->SetHorizontalAlignment(EHTA_Center);
	CashText->SetTextRenderColor(FColor(120, 255, 150));
	CashText->SetVisibility(false);
	TenderLayer = CreateDefaultSubobject<UTSSMoneyLayer>(TEXT("TenderLayer"));
	TenderLayer->SetupAttachment(Root);
	TenderLayer->SetRelativeLocation(FVector(6.f, 56.f, TopZ));
	ChangeLayer = CreateDefaultSubobject<UTSSMoneyLayer>(TEXT("ChangeLayer"));
	ChangeLayer->SetupAttachment(Root);
	ChangeLayer->SetRelativeLocation(FVector(26.f, 8.f, TopZ));

	// The devices: each is its own actor with its own behaviour.
	MonitorComp = CreateDefaultSubobject<UChildActorComponent>(TEXT("MonitorComp"));
	KeyboardComp = CreateDefaultSubobject<UChildActorComponent>(TEXT("KeyboardComp"));
	ScannerComp = CreateDefaultSubobject<UChildActorComponent>(TEXT("ScannerComp"));
	DrawerComp = CreateDefaultSubobject<UChildActorComponent>(TEXT("DrawerComp"));
	TerminalComp = CreateDefaultSubobject<UChildActorComponent>(TEXT("TerminalComp"));
	PrinterComp = CreateDefaultSubobject<UChildActorComponent>(TEXT("PrinterComp"));
	MonitorComp->SetChildActorClass(ATSSRegisterMonitor::StaticClass());
	KeyboardComp->SetChildActorClass(ATSSRegisterKeyboard::StaticClass());
	ScannerComp->SetChildActorClass(ATSSScanner::StaticClass());
	DrawerComp->SetChildActorClass(ATSSCashDrawer::StaticClass());
	TerminalComp->SetChildActorClass(ATSSCardTerminal::StaticClass());
	PrinterComp->SetChildActorClass(ATSSReceiptPrinter::StaticClass());
	UChildActorComponent* All[] = { MonitorComp, KeyboardComp, ScannerComp, DrawerComp, TerminalComp, PrinterComp };
	for (UChildActorComponent* C : All) C->SetupAttachment(Root);
}

void ATSSRegister::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	TSSApplyColor(Cabinet, FLinearColor(0.2f, 0.15f, 0.11f));
	for (int32 i = 0; i < Parts.Num() && i < PartColors.Num(); ++i) if (Parts[i]) TSSApplyColor(Parts[i], PartColors[i]);
}

ATSSScanner* ATSSRegister::GetScanner() const { return Cast<ATSSScanner>(ScannerComp->GetChildActor()); }
ATSSCashDrawer* ATSSRegister::GetDrawer() const { return Cast<ATSSCashDrawer>(DrawerComp->GetChildActor()); }
ATSSCardTerminal* ATSSRegister::GetTerminal() const { return Cast<ATSSCardTerminal>(TerminalComp->GetChildActor()); }
ATSSReceiptPrinter* ATSSRegister::GetPrinter() const { return Cast<ATSSReceiptPrinter>(PrinterComp->GetChildActor()); }
ATSSRegisterKeyboard* ATSSRegister::GetKeyboard() const { return Cast<ATSSRegisterKeyboard>(KeyboardComp->GetChildActor()); }

// ---- the line -------------------------------------------------------------------------------------

ATSSCustomer* ATSSRegister::Front() const
{
	return Queue.Num() > 0 ? Queue[0].Get() : nullptr;
}

FVector ATSSRegister::QueueSpot(int32 Index) const
{
	// Customers stand on the shop side (-X in the register's own frame), a body-width apart.
	return GetActorTransform().TransformPosition(FVector(-80.f - 70.f * Index, 0.f, 0.f)) * FVector(1.f, 1.f, 0.f);
}

int32 ATSSRegister::Join(ATSSCustomer* Who)
{
	Queue.RemoveAll([](const TWeakObjectPtr<ATSSCustomer>& C) { return !C.IsValid(); });
	Queue.AddUnique(Who);
	return Queue.Num() - 1;
}

void ATSSRegister::Leave(ATSSCustomer* Who)
{
	Queue.RemoveAll([Who](const TWeakObjectPtr<ATSSCustomer>& C) { return !C.IsValid() || C.Get() == Who; });
}

int32 ATSSRegister::IndexOf(const ATSSCustomer* Who) const
{
	for (int32 i = 0; i < Queue.Num(); ++i)
	{
		if (Queue[i].Get() == Who) return i;
	}
	return INDEX_NONE;
}

// ---- layout on the counter ------------------------------------------------------------------------

FVector ATSSRegister::StagingSlot(int32 Index) const
{
	return GetActorTransform().TransformPosition(FVector(-24.f, 108.f - 38.f * (Index % 3), TopZ));
}

FVector ATSSRegister::BaggingSlot(int32 Index) const
{
	return GetActorTransform().TransformPosition(FVector(-24.f, -40.f - 30.f * (Index % 3), TopZ));
}

// ---- the sale -------------------------------------------------------------------------------------

void ATSSRegister::SetNotice(const FString& Text, float Seconds)
{
	Notice = Text;
	NoticeTimer = Seconds;
}

void ATSSRegister::RecalculateTotals()
{
	Subtotal = 0;
	for (const FTSSSaleLine& L : Lines) Subtotal += L.Total() * 100;   // prices are whole dollars; the register counts cents
	Tax = TSSMoney::TaxFor(Subtotal);
	Total = Subtotal + Tax;
}

int32 ATSSRegister::ItemsLeftToScan() const
{
	int32 N = 0;
	for (const ATSSProductItem* I : Items) if (I && !I->bScanned) ++N;
	return N;
}

int32 ATSSRegister::GetChangeGiven() const
{
	return TSSMoney::Sum(ChangeBills);
}

void ATSSRegister::ReleaseChange()
{
	if (ATSSCashDrawer* D = GetDrawer()) for (int32 P : ChangeBills) D->ReturnPiece(P);
	ChangeBills.Reset();
}

void ATSSRegister::ShowMoney()
{
	TenderLayer->SetPieces(Tender);
	ChangeLayer->SetPieces(ChangeBills);
	CashText->SetVisibility(Tender.Num() > 0);
	CashText->SetText(FText::FromString(TSSMoney::Format(CashReceived)));
}

void ATSSRegister::BeginTransaction(ATSSCustomer* Who, const TArray<FTSSBasketItem>& Basket)
{
	Reset();
	Customer = Who;
	for (int32 i = 0; i < Basket.Num(); ++i)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ATSSProductItem* Item = GetWorld()->SpawnActor<ATSSProductItem>(ATSSProductItem::StaticClass(), FTransform(StagingSlot(i)), P);
		if (!Item) continue;
		Item->Init(Basket[i].Product, Basket[i].Price, this);
		Item->SetActorLocationAndRotation(StagingSlot(i), GetActorRotation());
		Item->SetCarried(true);   // hidden and untouchable until the customer puts it down
		Items.Add(Item);
	}
	Phase = ETSSCheckoutPhase::Scanning;
	ItemsPlaced = 0;
	SetNotice(TEXT("Customer is putting their items down..."), 5.f);
}

void ATSSRegister::RevealItem(int32 Index)
{
	if (!Items.IsValidIndex(Index) || !Items[Index]) return;
	Items[Index]->PlaceAt(StagingSlot(Index), GetActorRotation());
	++ItemsPlaced;
	if (AllItemsPlaced()) SetNotice(TEXT("Scan the customer's items"), 5.f);
}

FVector ATSSRegister::CashPoint() const
{
	return CashPile->GetComponentLocation();
}

FVector ATSSRegister::TerminalTapPoint() const
{
	const ATSSCardTerminal* T = GetTerminal();
	return T ? T->GetTapPoint() : GetActorLocation() + FVector(0.f, 0.f, TopZ + 25.f);
}

void ATSSRegister::Reset()
{
	ReleaseChange();
	Tender.Reset();
	for (ATSSProductItem* I : Items) if (I) I->Destroy();
	Items.Reset();
	Lines.Reset();
	ChangeBills.Reset();
	Subtotal = Tax = Total = CashReceived = ChangeDue = ScannedCount = ItemsPlaced = 0;
	Method = ETSSPayMethod::None;
	bCardApproved = false;
	CashPile->SetVisibility(false);
	ShowMoney();
}

bool ATSSRegister::ScanItem(ATSSProductItem* Item, FString& Why)
{
	if (Phase != ETSSCheckoutPhase::Scanning) { Why = TEXT("No sale in progress"); return false; }
	if (!Item || !Items.Contains(Item)) { Why = TEXT("That is not part of this sale"); return false; }
	if (Item->bScanned) { Why = TEXT("Already scanned"); return false; }

	Item->bScanned = true;
	FTSSSaleLine* Line = Lines.FindByPredicate([Item](const FTSSSaleLine& L) { return L.Product == Item->Product && L.UnitPrice == Item->UnitPrice; });
	if (Line) ++Line->Qty;
	else Lines.Add({ Item->Product, 1, Item->UnitPrice });
	RecalculateTotals();

	// Scanned items go to the bagging side of the counter.
	Item->PlaceAt(BaggingSlot(ScannedCount++), GetActorRotation());
	Why = FString::Printf(TEXT("Scanned %s"), TSSProductDef(Item->Product).Name);
	return true;
}

bool ATSSRegister::TryScanHeld(ATSSCharacter* Player)
{
	FString Why;
	ATSSProductItem* Item = Player->GetHeldItem();
	if (!Item)
	{
		SetNotice(Player->GetHeld() == ETSSProduct::None ? TEXT("Pick up one of the customer's items") : TEXT("That is not the customer's item"));
		return false;
	}
	const bool bOk = ScanItem(Item, Why);
	SetNotice(Why);
	if (bOk) Player->SetHeld(ETSSProduct::None);
	if (ATSSScanner* S = GetScanner()) S->Beep(bOk);
	return bOk;
}

FString ATSSRegister::ScannerPrompt(const ATSSCharacter* Player) const
{
	if (Phase != ETSSCheckoutPhase::Scanning) return TEXT("Barcode scanner");
	if (const ATSSProductItem* I = Player->GetHeldItem())
	{
		return I->bScanned ? FString(TEXT("Already scanned")) : FString::Printf(TEXT("[E / Click] Scan %s  ($%d)"), TSSProductDef(I->Product).Name, I->UnitPrice);
	}
	return TEXT("Hold an item here to scan it");
}

FString ATSSRegister::KeyboardPrompt() const
{
	switch (Phase)
	{
	case ETSSCheckoutPhase::Idle: return TEXT("Register keyboard\nNo customer yet");
	case ETSSCheckoutPhase::Scanning:
	{
		const int32 Left = ItemsLeftToScan();
		return Left > 0 ? FString::Printf(TEXT("Scan all items first  (%d left)"), Left) :FString::Printf(TEXT("[E / Click] Charge  %s"), *TSSMoney::Format(Total));
	}
	case ETSSCheckoutPhase::WaitingForPayment: return TEXT("Waiting for the customer to pay...");
	case ETSSCheckoutPhase::CashDue:
		return FString::Printf(TEXT("[E / Click] Confirm change\nGiving %s of %s due"), *TSSMoney::Format(GetChangeGiven()), *TSSMoney::Format(ChangeDue));
	case ETSSCheckoutPhase::Receipt: return TEXT("Hand over the receipt");
	case ETSSCheckoutPhase::Collecting: return TEXT("The customer is collecting their shopping");
	default: return TEXT("Waiting for the card payment...");
	}
}

void ATSSRegister::KeyboardPress()
{
	if (Phase == ETSSCheckoutPhase::Scanning && Items.Num() > 0 && ItemsLeftToScan() == 0)
	{
		Phase = ETSSCheckoutPhase::WaitingForPayment;
		SetNotice(FString::Printf(TEXT("Total %s  -  waiting for payment"), *TSSMoney::Format(Total)), 6.f);
		if (Customer) Customer->PayNow(false);
	}
	else if (Phase == ETSSCheckoutPhase::CashDue)
	{
		ConfirmChange();
	}
}

// ---- cash -----------------------------------------------------------------------------------------

void ATSSRegister::CustomerPaysCash(const TArray<int32>& Pieces)
{
	Phase = ETSSCheckoutPhase::CashDue;
	Method = ETSSPayMethod::Cash;
	Tender = Pieces;
	CashReceived = TSSMoney::Sum(Pieces);
	ChangeDue = CashReceived - Total;
	ReleaseChange();
	bool bBank = false;
	if (ATSSCashDrawer* D = GetDrawer())
	{
		D->SetOpen(true);
		// If the drawer cannot make this change (a $100 note for a small sale and no small notes left), the owner
		// pops next door to the bank and comes back with a fresh float.
		TArray<int32> Probe;
		if (ChangeDue > 0 && !TSSMoney::MakeChange(ChangeDue, D->GetStock(), Probe)) { D->RefillFloat(); bBank = true; }
	}
	ShowMoney();
	if (bBank) SetNotice(TEXT("Short of change: you topped the drawer up from the bank"), 5.f);
	else SetNotice(ChangeDue > 0 ? FString::Printf(TEXT("Give %s change"), *TSSMoney::Format(ChangeDue)) : FString(TEXT("Exact amount - confirm")), 6.f);
}

bool ATSSRegister::AddChange(int32 Piece)
{
	if (Phase != ETSSCheckoutPhase::CashDue) return false;
	ATSSCashDrawer* D = GetDrawer();
	if (!D || !D->TakePiece(Piece)) { SetNotice(FString::Printf(TEXT("The drawer is out of %s"), TSSMoney::Denom(Piece).Name), 2.5f); return false; }
	ChangeBills.Add(Piece);
	ShowMoney();
	return true;
}

void ATSSRegister::RemoveLastChange()
{
	if (Phase != ETSSCheckoutPhase::CashDue || ChangeBills.Num() == 0) return;
	if (ATSSCashDrawer* D = GetDrawer()) D->ReturnPiece(ChangeBills.Last());
	ChangeBills.Pop();
	ShowMoney();
}

FString ATSSRegister::DrawerPrompt() const
{
	if (Phase != ETSSCheckoutPhase::CashDue) return TEXT("Cash drawer");
	return FString::Printf(TEXT("[E / Click] Add this bill to the change   |   [Wheel] take the last one back\nGiving $%d of $%d"), GetChangeGiven(), ChangeDue);
}

void ATSSRegister::ConfirmChange()
{
	if (GetChangeGiven() == ChangeDue)
	{
		Complete(ETSSPayMethod::Cash);
		return;
	}
	// Wrong change: the customer sends it back and the tray is cleared.
	SetNotice(GetChangeGiven() < ChangeDue ? TEXT("Not enough change!") : TEXT("Too much change!"), 3.f);
	ReleaseChange();
	ShowMoney();
	if (Customer) Customer->Complain(TEXT("That's not the right change!"));
}

// ---- card -----------------------------------------------------------------------------------------

void ATSSRegister::CustomerTapsCard()
{
	Phase = ETSSCheckoutPhase::CardTap;
	Method = ETSSPayMethod::Card;
	PhaseTimer = 1.0f;
	if (ATSSCardTerminal* T = GetTerminal()) T->SetCardPresent(true);
}

void ATSSRegister::ProcessCard()
{
	if (Phase != ETSSCheckoutPhase::CardReady) return;
	Phase = ETSSCheckoutPhase::CardProcessing;
	PhaseTimer = 1.6f;
}

FString ATSSRegister::TerminalPrompt() const
{
	if (Phase == ETSSCheckoutPhase::CardReady) return FString::Printf(TEXT("[E / Click] Process card payment  (%s)"), *TSSMoney::Format(Total));
	return TEXT("Card terminal");
}

FString ATSSRegister::TerminalStatus() const
{
	switch (Phase)
	{
	case ETSSCheckoutPhase::CardTap: return TEXT("TAP CARD");
	case ETSSCheckoutPhase::CardReady: return TEXT("PRESS TO PAY");
	case ETSSCheckoutPhase::CardProcessing:
		return FString(TEXT("PROCESSING")) + FString::ChrN(1 + (static_cast<int32>(GetWorld()->GetTimeSeconds() * 3.f) % 3), TEXT('.'));
	case ETSSCheckoutPhase::CardResult: return bCardApproved ? TEXT("APPROVED") : TEXT("DECLINED");
	case ETSSCheckoutPhase::Receipt: return Method == ETSSPayMethod::Card ? TEXT("THANK YOU") : TEXT("STANDBY");
	default: return TEXT("STANDBY");
	}
}

FLinearColor ATSSRegister::TerminalColor() const
{
	switch (Phase)
	{
	case ETSSCheckoutPhase::CardTap: return FLinearColor(0.1f, 0.4f, 0.85f);
	case ETSSCheckoutPhase::CardReady: return FLinearColor(0.85f, 0.55f, 0.05f);
	case ETSSCheckoutPhase::CardProcessing: return FLinearColor(0.2f, 0.3f, 0.5f);
	case ETSSCheckoutPhase::CardResult: return bCardApproved ? FLinearColor(0.1f, 0.65f, 0.3f) : FLinearColor(0.8f, 0.15f, 0.15f);
	default: return FLinearColor(0.1f, 0.12f, 0.16f);
	}
}

// ---- finishing ------------------------------------------------------------------------------------

void ATSSRegister::Complete(ETSSPayMethod PaidBy)
{
	FTSSTransaction T;
	T.Items = Items.Num();
	T.Subtotal = Subtotal;
	T.Tax = Tax;
	T.Total = Total;
	T.Method = PaidBy;
	T.CashReceived = PaidBy == ETSSPayMethod::Cash ? CashReceived : 0;
	T.Change = PaidBy == ETSSPayMethod::Cash ? ChangeDue : 0;
	if (const UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>()) T.TimeMinutes = FMath::FloorToInt(Biz->GetHour() * 60.f);
	if (UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>()) Store->RecordSale(T);

	if (ATSSCashDrawer* D = GetDrawer())
	{
		if (PaidBy == ETSSPayMethod::Cash) D->Receive(Tender);   // the change was already counted out of the drawer
		D->SetOpen(false);
	}
	ChangeBills.Reset();   // handed to the customer
	Tender.Reset();
	ShowMoney();
	LastTotal = Total;
if (ATSSReceiptPrinter* P = GetPrinter())
	{
		const UTSSStoreSubsystem* St = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
		P->Print(St && St->GetTransactions().Num() > 0 ? St->GetTransactions().Last() : T, Lines);
	}
	if (Customer) Customer->Say(TEXT("Thank you!"), 3.f);
	Phase = ETSSCheckoutPhase::Receipt;
	PhaseTimer = 9.f; // handed over automatically if the owner is busy
	SetNotice(TEXT("Sale complete  -  hand over the receipt"), 6.f);
}

FString ATSSRegister::PrinterPrompt() const
{
	return Phase == ETSSCheckoutPhase::Receipt ? FString(TEXT("[E / Click] Hand the receipt to the customer")) : FString(TEXT("Receipt printer"));
}

void ATSSRegister::TakeReceipt()
{
	if (Phase != ETSSCheckoutPhase::Receipt) return;
	if (ATSSReceiptPrinter* P = GetPrinter()) P->Tear();
	if (ATSSCardTerminal* T = GetTerminal()) T->SetCardPresent(false);
	Phase = ETSSCheckoutPhase::Collecting;
	if (Customer) Customer->FinishCheckout();
	else ReleaseSale();
}

void ATSSRegister::ReleaseSale()
{
	Customer = nullptr;
	Reset();
	Phase = ETSSCheckoutPhase::Idle;
}

// ---- the counter surface --------------------------------------------------------------------------

FString ATSSRegister::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	if (Player->GetHeldItem())
	{
		return Phase == ETSSCheckoutPhase::Scanning ? FString(TEXT("[E / Click] Put the item down")) : FString(TEXT("Scan it, or put it down on the counter"));
	}
	return Phase == ETSSCheckoutPhase::Idle ? FString(TEXT("Checkout counter")) : FString();
}

void ATSSRegister::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	ATSSProductItem* Item = Player->GetHeldItem();
	if (!Item) return;
	// Only onto the flat top of the counter.
	if (Hit.ImpactNormal.Z < 0.7f) return;
	Item->PlaceAt(Hit.ImpactPoint, GetActorRotation());
	Player->SetHeld(ETSSProduct::None);
}

// ---- tick: card timers, notice, auto receipt ------------------------------------------------------

void ATSSRegister::Tick(float Dt)
{
	Super::Tick(Dt);
	if (NoticeTimer > 0.f)
	{
		NoticeTimer -= Dt;
		if (NoticeTimer <= 0.f) Notice.Reset();
	}

	switch (Phase)
	{
	case ETSSCheckoutPhase::CardTap:
		PhaseTimer -= Dt;
		if (PhaseTimer <= 0.f)
		{
			Phase = ETSSCheckoutPhase::CardReady;
			SetNotice(TEXT("Card ready  -  process the payment on the terminal"), 5.f);
		}
		break;
	case ETSSCheckoutPhase::CardProcessing:
		PhaseTimer -= Dt;
		if (PhaseTimer <= 0.f)
		{
			bCardApproved = FMath::FRand() >= CardDeclineChance;
			Phase = ETSSCheckoutPhase::CardResult;
			PhaseTimer = 1.4f;
		}
		break;
	case ETSSCheckoutPhase::CardResult:
		PhaseTimer -= Dt;
		if (PhaseTimer <= 0.f)
		{
			if (bCardApproved)
			{
				Complete(ETSSPayMethod::Card);
			}
			else
			{
				// Declined: the customer tries cash instead.
				if (ATSSCardTerminal* T = GetTerminal()) T->SetCardPresent(false);
				Phase = ETSSCheckoutPhase::WaitingForPayment;
				Method = ETSSPayMethod::None;
				SetNotice(TEXT("Card declined  -  customer will pay cash"), 4.f);
				if (Customer) { Customer->Complain(TEXT("Declined?! Let me pay cash.")); Customer->PayNow(true); }
			}
		}
		break;
	case ETSSCheckoutPhase::Receipt:
		PhaseTimer -= Dt;
		if (PhaseTimer <= 0.f) TakeReceipt();
		break;
	default:
		break;
	}

	if (const UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>())
	{
		if (Biz->HasCashier()) RunCashier(Dt);
	}
}

void ATSSRegister::RunCashier(float Dt)
{
	// Each new step starts after a pause; the owner can always get in first.
	if (Phase != CashierSeenPhase)
	{
		CashierSeenPhase = Phase;
		CashierTimer = (Phase == ETSSCheckoutPhase::Scanning) ? 1.8f : 1.2f;
		return;
	}
	CashierTimer -= Dt;
	if (CashierTimer > 0.f) return;

	switch (Phase)
	{
	case ETSSCheckoutPhase::Scanning:
		if (!AllItemsPlaced()) return;
		for (ATSSProductItem* I : Items)
		{
			if (I && !I->bScanned)
			{
				FString Why;
				const bool bOk = ScanItem(I, Why);
				if (ATSSScanner* S = GetScanner()) S->Beep(bOk);
				CashierTimer = 0.8f;
				return;
			}
		}
		KeyboardPress();   // everything is scanned: charge
		break;
	case ETSSCheckoutPhase::CardReady:
		ProcessCard();
		break;
	case ETSSCheckoutPhase::CashDue:
	{
		// Count out the change from what the drawer really holds, biggest pieces first, then confirm.
		ReleaseChange();
		TArray<int32> Pieces;
		if (ATSSCashDrawer* D = GetDrawer())
		{
			if (TSSMoney::MakeChange(ChangeDue, D->GetStock(), Pieces)) for (int32 P : Pieces) AddChange(P);
		}
		ConfirmChange();
		break;
	}
	case ETSSCheckoutPhase::Receipt:
		TakeReceipt();
		break;
	default:
		break;
	}
}
