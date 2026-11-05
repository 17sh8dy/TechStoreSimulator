#include "TSSPlayerController.h"
#include "TSSMenuWidget.h"
#include "TSSHudWidget.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TSSCharacter.h"
#include "TSSCrate.h"
#include "TSSShelf.h"
#include "TSSStoreSubsystem.h"
#include "TSSCustomer.h"
#include "TSSSettings.h"
#include "TSSCustomerSubsystem.h"
#include "TSSRegister.h"
#include "TSSRegisterDevice.h"
#include "TSSCheckoutTypes.h"
#include "TSSProductItem.h"

void ATSSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return;
	Menu = CreateWidget<UTSSMenuWidget>(this, UTSSMenuWidget::StaticClass());
	Hud = CreateWidget<UTSSHudWidget>(this, UTSSHudWidget::StaticClass());
	Hud->AddToViewport(5);
	Hud->SetVisibility(ESlateVisibility::Collapsed);
	Menu->AddToViewport(10);
	ShowMenu(true);

	// Debug harness: `-TSSAutoTest` runs the interaction self-test and takes screenshots, then quits.
	if (FParse::Param(FCommandLine::Get(), TEXT("TSSAutoTest")))
	{
		FTimerHandle H;
		GetWorldTimerManager().SetTimer(H, [this]()
		{
			TSSSelfTest();
			ATSSShelf* Shelf = nullptr;
			for (TActorIterator<ATSSShelf> It(GetWorld()); It; ++It) { Shelf = *It; break; }
			auto Shot = [this](float Delay, FString Cmd)
			{
				FTimerHandle T;
				GetWorldTimerManager().SetTimer(T, [this, Cmd]() { ConsoleCommand(Cmd); }, Delay, false);
			};
			// 1: overview from the door.
			Shot(0.5f, TEXT("TSSLook 560 0 150 180 -8"));
			Shot(1.5f, TEXT("Shot ShowUI"));
			// 2: stood at the stocked shelf, looking at the first-tier slot with the phones.
			if (Shelf)
			{
				const FVector P = Shelf->GetActorLocation() + Shelf->GetActorForwardVector() * 120.f;
				const FRotator Face = (-Shelf->GetActorForwardVector()).Rotation();
				Shot(2.5f, FString::Printf(TEXT("TSSLook %f %f 40 %f -5"), P.X, P.Y, Face.Yaw));
			}
			Shot(3.5f, TEXT("Shot ShowUI"));
			// 3: the wholesale crates.
			Shot(4.5f, TEXT("TSSLook -100 150 110 90 -18"));
			Shot(5.5f, TEXT("Shot ShowUI"));
			Shot(6.5f, TEXT("TSSCustomerTest"));
		}, 1.5f, false);
	}
}

void ATSSPlayerController::ShowMenu(bool bMainMenu)
{
	bGameActive = false;
	if (UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>()) Store->SetSimPaused(true);
	Menu->SetVisibility(ESlateVisibility::Visible);
	Hud->SetVisibility(ESlateVisibility::Collapsed);
	Menu->SetPage(bMainMenu ? ETSSMenuPage::Main : ETSSMenuPage::Pause);

	// Game+UI so keys the menu ignores (Esc/Tab) still reach us.
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	bShowMouseCursor = true;
}

void ATSSPlayerController::EnterGameInput()
{
	bGameActive = true;
	if (UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>()) Store->SetSimPaused(false);
	Menu->SetVisibility(ESlateVisibility::Collapsed);
	Hud->SetVisibility(ESlateVisibility::HitTestInvisible);
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void ATSSPlayerController::StartNewGame() { EnterGameInput(); }
void ATSSPlayerController::ResumeGame() { EnterGameInput(); }

void ATSSPlayerController::QuitToDesktop()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ATSSPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!Menu) return;

	if (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::Tab))
	{
		if (bGameActive)
		{
			ShowMenu(false);
		}
		else if (!Menu->HandleBack())
		{
			// Esc on the pause page resumes.
			if (Menu->GetPage() == ETSSMenuPage::Pause) ResumeGame();
		}
	}
}

void ATSSPlayerController::TSSStart()
{
	if (Menu) EnterGameInput();
}

void ATSSPlayerController::TSSLook(float X, float Y, float Z, float Yaw, float Pitch)
{
	if (APawn* P = GetPawn()) P->SetActorLocation(FVector(X, Y, Z));
	SetControlRotation(FRotator(Pitch, Yaw, 0.f));
}

void ATSSPlayerController::TSSSelfTest()
{
	TSSStart();
	int32 Fails = 0;
	auto Check = [&Fails](const TCHAR* Name, bool bOk)
	{
		UE_LOG(LogTemp, Display, TEXT("TSSTEST %s %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), Name);
		if (!bOk) ++Fails;
	};

	ATSSCharacter* Me = Cast<ATSSCharacter>(GetPawn());
	UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	ATSSCrate* PhoneCrate = nullptr;
	ATSSCrate* LaptopCrate = nullptr;
	ATSSShelf* Shelf = nullptr;
	for (TActorIterator<ATSSCrate> It(GetWorld()); It; ++It)
	{
		if (It->Product == ETSSProduct::Phone) PhoneCrate = *It;
		if (It->Product == ETSSProduct::Laptop) LaptopCrate = *It;
	}
	for (TActorIterator<ATSSShelf> It(GetWorld()); It; ++It) { Shelf = *It; break; }
	Check(TEXT("found player, store, crates and a shelf"), Me && Store && PhoneCrate && LaptopCrate && Shelf);
	if (!(Me && Store && PhoneCrate && LaptopCrate && Shelf)) return;

	// A hit on the shelf's first-tier, second-column slot, from the shelf's own frame.
	FHitResult Hit;
	Hit.ImpactPoint = Shelf->GetActorTransform().TransformPosition(FVector(24.f, -25.f, 30.f));
	FHitResult CrateHit;

	Store->Earn(3000); // enough to buy a full slot's worth; the game itself starts you with $1000
	const int32 Start = Store->GetMoney();
	PhoneCrate->Interact(Me, CrateHit);
	Check(TEXT("buying a phone takes $300 and fills the hands"), Me->GetHeld() == ETSSProduct::Phone && Store->GetMoney() == Start - 300);
	LaptopCrate->Interact(Me, CrateHit);
	Check(TEXT("hands full: a second crate does nothing"), Me->GetHeld() == ETSSProduct::Phone && Store->GetMoney() == Start - 300);

	Shelf->Interact(Me, Hit);
	Check(TEXT("placing empties the hands"), Me->GetHeld() == ETSSProduct::None);
	Check(TEXT("prompt on a stocked slot offers Take"), Shelf->GetPrompt(Me, Hit).Contains(TEXT("[E] Take Smartphone")));

	Shelf->Interact(Me, Hit);
	Check(TEXT("taking returns the phone to the hands"), Me->GetHeld() == ETSSProduct::Phone);
	Shelf->Interact(Me, Hit);
	for (int32 i = 0; i < 3; ++i) { PhoneCrate->Interact(Me, CrateHit); Shelf->Interact(Me, Hit); }
	PhoneCrate->Interact(Me, CrateHit);
	Shelf->Interact(Me, Hit);
	Check(TEXT("a full slot refuses a fifth phone"), Me->GetHeld() == ETSSProduct::Phone && Shelf->GetPrompt(Me, Hit).Contains(TEXT("full")));
	const int32 BeforeReturn = Store->GetMoney();
	PhoneCrate->Interact(Me, CrateHit);
	Check(TEXT("returning to the crate refunds $300"), Me->GetHeld() == ETSSProduct::None && Store->GetMoney() == BeforeReturn + 300);

	LaptopCrate->Interact(Me, CrateHit);
	Shelf->Interact(Me, Hit);
	Check(TEXT("a slot holding phones refuses a laptop"), Me->GetHeld() == ETSSProduct::Laptop);
	LaptopCrate->Interact(Me, CrateHit);

	const int32 Price = Store->GetPrice(ETSSProduct::Phone);
	Shelf->Adjust(Me, Hit, +1);
	Shelf->Adjust(Me, Hit, +1);
	Check(TEXT("wheel up twice raises the phone price by $2"), Store->GetPrice(ETSSProduct::Phone) == Price + 2);

	// Leave the shop looking stocked for screenshots.
	const ETSSProduct Fill[] = { ETSSProduct::Laptop, ETSSProduct::Headphones, ETSSProduct::Console, ETSSProduct::Monitor, ETSSProduct::Keyboard };
	TArray<ATSSCrate*> Crates;
	for (TActorIterator<ATSSCrate> It(GetWorld()); It; ++It) Crates.Add(*It);
	Store->Earn(5000);
	int32 Slot = 0;
	for (ETSSProduct P : Fill)
	{
		for (ATSSCrate* C : Crates) if (C->Product == P)
		{
			FHitResult H;
			// Tiers 1 and 2 (the first tier already has the phones).
			const float Z = 90.f + 60.f * (Slot / ATSSShelf::Columns);
			H.ImpactPoint = Shelf->GetActorTransform().TransformPosition(FVector(24.f, -75.f + 50.f * (Slot % ATSSShelf::Columns), Z));
			for (int32 i = 0; i < FMath::Min(3, TSSProductDef(P).SlotCapacity()); ++i) { C->Interact(Me, CrateHit); Shelf->Interact(Me, H); }
			break;
		}
		++Slot;
	}
	UE_LOG(LogTemp, Display, TEXT("TSSTEST DONE fails=%d"), Fails);
}

void ATSSPlayerController::TSSCustomer()
{
	if (UTSSCustomerSubsystem* CS = GetWorld()->GetSubsystem<UTSSCustomerSubsystem>()) CS->SpawnCustomer();
}

void ATSSPlayerController::TSSCustomerTest()
{
	// Drives three customers through the real shop with the real device code:
	//   run 0: pays CASH (wrong change first, then right), run 1: pays by CARD, run 2: balks at the price.
	struct FState
	{
		int32 Run = 0;
		int32 Step = 0;
		TWeakObjectPtr<ATSSCustomer> Customer;
		int32 Money0 = 0, Served0 = 0, Lost0 = 0, Cash0 = 0, Card0 = 0, Tax0 = 0, Txns0 = 0;
		int32 ExpectedTotal = 0, ExpectedSub = 0;
		float Hold = 0.f;      // wait before the next action (lets a screenshot happen)
		float Elapsed = 0.f;   // time in the current step
		int32 Fails = 0;
		TMap<ETSSProduct, int32> OldPrices;
	};
	TSharedRef<FState> St = MakeShared<FState>();

	UTSSCustomerSubsystem* CS = GetWorld()->GetSubsystem<UTSSCustomerSubsystem>();
	UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	ATSSRegister* Reg = nullptr;
	for (TActorIterator<ATSSRegister> It(GetWorld()); It; ++It) { Reg = *It; break; }
	ATSSCharacter* Me = Cast<ATSSCharacter>(GetPawn());
	auto Check = [St](const TCHAR* Name, bool bOk)
	{
		UE_LOG(LogTemp, Display, TEXT("TSSTEST %s %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), Name);
		if (!bOk) ++St->Fails;
	};
	Check(TEXT("customer subsystem, store, register and player exist"), CS && Store && Reg && Me);
	if (!(CS && Store && Reg && Me)) return;
	Check(TEXT("register has all six devices"), Reg->GetScanner() && Reg->GetDrawer() && Reg->GetTerminal() && Reg->GetPrinter() && Reg->GetKeyboard());
	if (!(Reg->GetScanner() && Reg->GetDrawer() && Reg->GetTerminal() && Reg->GetPrinter() && Reg->GetKeyboard())) return;

	TSSStart();
	CS->bAutoSpawn = false;
	Reg->CardDeclineChance = 0.f; // deterministic; the decline path has its own check below
	if (UTSSSettingsSubsystem* Cfg = GetGameInstance()->GetSubsystem<UTSSSettingsSubsystem>()) Cfg->SetViewMode(ETSSViewMode::FirstPerson);

	auto Spawn = [this, St, CS, Store](ETSSPayMethod Method)
	{
		St->Step = 0; St->Elapsed = 0.f; St->Hold = 0.f;
		St->Money0 = Store->GetMoney(); St->Served0 = Store->GetServed(); St->Lost0 = Store->GetLost();
		St->Cash0 = Store->GetCashSales(); St->Card0 = Store->GetCardSales(); St->Tax0 = Store->GetTaxCollected();
		St->Txns0 = Store->GetTransactions().Num();
		ATSSCustomer* C = CS->SpawnCustomer(8.f);
		St->Customer = C;
		if (C) { C->Patience = 300.f; C->ForceMethod = Method; }
	};
	// Camera helper: stand on the owner's side at register-local P, look at register-local T, then screenshot.
	auto Snap = [this, Reg](FVector P, FVector T)
	{
		const FVector Wp = Reg->GetActorTransform().TransformPosition(P);
		const FVector Wt = Reg->GetActorTransform().TransformPosition(T);
		const FRotator R = (Wt - (Wp + FVector(0.f, 0.f, 154.f))).Rotation();
		ConsoleCommand(FString::Printf(TEXT("TSSLook %f %f 90 %f %f"), Wp.X, Wp.Y, R.Yaw, R.Pitch));
		FTimerHandle T2;
		GetWorldTimerManager().SetTimer(T2, [this]() { ConsoleCommand(TEXT("Shot ShowUI")); }, 0.6f, false);
	};

	Spawn(ETSSPayMethod::Cash);
	Check(TEXT("a customer spawns at the door"), St->Customer.IsValid());

	FTimerHandle* Handle = new FTimerHandle();
	GetWorldTimerManager().SetTimer(*Handle, [this, St, CS, Store, Reg, Me, Check, Spawn, Snap, Handle]()
	{
		const float Dt = 0.2f;
		St->Elapsed += Dt;
		if (St->Hold > 0.f) { St->Hold -= Dt; return; }
		const bool bTimeout = St->Elapsed > 40.f;
		auto Fail = [&](const TCHAR* What) { Check(What, false); St->Run = 99; };

		if (St->Run == 0 || St->Run == 1)
		{
			const bool bCash = St->Run == 0;
			switch (St->Step)
			{
			case 0: // customer reaches the counter and puts the basket down
				if (Reg->GetPhase() != ETSSCheckoutPhase::Scanning) { if (bTimeout) Fail(TEXT("customer reached the counter with a basket")); return; }
				{
					const TArray<TObjectPtr<ATSSProductItem>>& Items = Reg->GetItems();
					Check(TEXT("the basket is on the counter as real product items"), Items.Num() >= 1);
					St->ExpectedSub = 0;
					for (ATSSProductItem* I : Items) St->ExpectedSub += I->UnitPrice;

					Reg->GetKeyboard()->Interact(Me, FHitResult());
					Check(TEXT("charging is refused while items are unscanned"), Reg->GetPhase() == ETSSCheckoutPhase::Scanning);

					for (ATSSProductItem* I : Items)
					{
						I->Interact(Me, FHitResult());
						Check(TEXT("picking up an item puts it in the player's hands"), Me->GetHeldItem() == I && Me->GetHeld() == I->Product);
						Check(TEXT("scanning the held item works"), Reg->TryScanHeld(Me) && I->bScanned && Me->GetHeld() == ETSSProduct::None);
						FString Why;
						Check(TEXT("the same box cannot be scanned twice"), !Reg->ScanItem(I, Why));
						I->Interact(Me, FHitResult());
						Check(TEXT("a scanned box cannot be picked up again"), Me->GetHeld() == ETSSProduct::None);
					}
					Check(TEXT("subtotal is the sum of the shelf prices"), Reg->GetSubtotal() == St->ExpectedSub);
					Check(TEXT("tax is 8% and total = subtotal + tax"), Reg->GetTax() == TSSMoney::TaxFor(St->ExpectedSub) && Reg->GetTotal() == St->ExpectedSub + Reg->GetTax());
					Check(TEXT("monitor lines add up to the subtotal"), [&]() { int32 S = 0; for (const FTSSSaleLine& L : Reg->GetLines()) S += L.Total(); return S == Reg->GetSubtotal(); }());
					St->ExpectedTotal = Reg->GetTotal();
					Snap(FVector(110.f, -85.f, 0.f), FVector(17.f, -85.f, 125.f));
					St->Hold = 1.4f;
					St->Step = 1; St->Elapsed = 0.f;
				}
				break;
			case 1: // charge
				Reg->GetKeyboard()->Interact(Me, FHitResult());
				Check(TEXT("keyboard Charge moves the sale to awaiting payment"), Reg->GetPhase() == ETSSCheckoutPhase::WaitingForPayment);
				St->Step = 2; St->Elapsed = 0.f;
				break;
			case 2: // the customer pays
				if (bCash)
				{
					if (Reg->GetPhase() != ETSSCheckoutPhase::CashDue || !Reg->GetDrawer()->IsOpen()) { if (bTimeout) Fail(TEXT("customer paid cash and the drawer opened")); return; }
					Check(TEXT("customer chose cash"), Reg->GetMethod() == ETSSPayMethod::Cash);
					Check(TEXT("change due = cash received - total"), Reg->GetChangeDue() == Reg->GetCashReceived() - Reg->GetTotal() && Reg->GetChangeDue() >= 0);
					if (Reg->GetChangeDue() > 0)
					{
						Reg->GetKeyboard()->Interact(Me, FHitResult());
						Check(TEXT("confirming with no change given is rejected"), Reg->GetPhase() == ETSSCheckoutPhase::CashDue);
					}
					int32 Left = Reg->GetChangeDue();
					for (int32 d = TSSMoney::NumDenoms - 1; d >= 0; --d)
					{
						while (Left >= TSSMoney::Denoms[d]) { Reg->GetDrawer()->ClickDenom(TSSMoney::Denoms[d]); Left -= TSSMoney::Denoms[d]; }
					}
					Check(TEXT("bills clicked out of the drawer add up to the change due"), Reg->GetChangeGiven() == Reg->GetChangeDue());
					Snap(FVector(115.f, -85.f, 0.f), FVector(54.f, -85.f, 90.f));
					St->Hold = 1.4f;
					St->Step = 3; St->Elapsed = 0.f;
				}
				else
				{
					if (Reg->GetPhase() != ETSSCheckoutPhase::CardReady) { if (bTimeout) Fail(TEXT("customer tapped a card and the terminal is ready")); return; }
					Check(TEXT("customer chose card"), Reg->GetMethod() == ETSSPayMethod::Card);
					Snap(FVector(100.f, 55.f, 0.f), FVector(2.f, 55.f, 118.f));
					St->Hold = 1.4f;
					St->Step = 3; St->Elapsed = 0.f;
				}
				break;
			case 3: // finish paying
				if (bCash)
				{
					Reg->GetKeyboard()->Interact(Me, FHitResult());
				}
				else
				{
					Reg->GetTerminal()->Interact(Me, FHitResult());
					Check(TEXT("pressing the terminal starts processing"), Reg->GetPhase() == ETSSCheckoutPhase::CardProcessing);
				}
				St->Step = 4; St->Elapsed = 0.f;
				break;
			case 4: // sale recorded, receipt printed
				if (Reg->GetPhase() != ETSSCheckoutPhase::Receipt) { if (bTimeout) Fail(TEXT("payment completed and a receipt printed")); return; }
				Check(TEXT("the store's money went up by exactly the total"), Store->GetMoney() == St->Money0 + St->ExpectedTotal);
				Check(TEXT("served counter and transaction log updated"), Store->GetServed() == St->Served0 + 1 && Store->GetTransactions().Num() == St->Txns0 + 1);
				Check(TEXT("sale recorded under the right payment method"),
					bCash ? (Store->GetCashSales() == St->Cash0 + St->ExpectedTotal && Store->GetCardSales() == St->Card0)
					      : (Store->GetCardSales() == St->Card0 + St->ExpectedTotal && Store->GetCashSales() == St->Cash0));
				Check(TEXT("tax collected is recorded"), Store->GetTaxCollected() == St->Tax0 + TSSMoney::TaxFor(St->ExpectedSub));
				Check(TEXT("the receipt printer has a receipt"), Reg->GetPrinter()->HasReceipt());
				St->Hold = 2.0f; // let the receipt finish printing
				St->Step = 5; St->Elapsed = 0.f;
				break;
			case 5: // hand over the receipt; customer leaves
				Reg->GetPrinter()->Interact(Me, FHitResult());
				Check(TEXT("handing over the receipt frees the register"), Reg->GetPhase() == ETSSCheckoutPhase::Idle);
				St->Step = 6; St->Elapsed = 0.f;
				break;
			case 6:
				if (St->Customer.IsValid()) { if (bTimeout) Fail(TEXT("the customer left the shop")); return; }
				Check(TEXT("the customer walked out and is gone"), true);
				++St->Run;
				if (St->Run == 1) Spawn(ETSSPayMethod::Card);
				else
				{
					// Run 2: absurd prices, the customer should balk and never reach the counter.
					for (int32 i = 1; i < static_cast<int32>(ETSSProduct::Count); ++i)
					{
						const ETSSProduct P = static_cast<ETSSProduct>(i);
						St->OldPrices.Add(P, Store->GetPrice(P));
						Store->SetPrice(P, 90000);
					}
					Spawn(ETSSPayMethod::None);
				}
				break;
			}
		}
		else if (St->Run == 2)
		{
			if (Store->GetLost() > St->Lost0)
			{
				Check(TEXT("an overpriced shop loses the customer"), true);
				for (const TPair<ETSSProduct, int32>& KV : St->OldPrices) Store->SetPrice(KV.Key, KV.Value);
				St->Run = 99;
			}
			else if (bTimeout) { Check(TEXT("an overpriced shop loses the customer"), false); St->Run = 99; }
		}

		if (St->Run == 99)
		{
			UE_LOG(LogTemp, Display, TEXT("TSSTEST CUSTOMERS DONE fails=%d"), St->Fails);
			GetWorldTimerManager().ClearTimer(*Handle);
			if (FParse::Param(FCommandLine::Get(), TEXT("TSSAutoTest"))) ConsoleCommand(TEXT("QUIT_EDITOR"));
		}
	}, 0.2f, true);
}
