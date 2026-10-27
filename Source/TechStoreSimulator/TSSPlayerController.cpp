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
			Shot(7.0f, TEXT("QUIT_EDITOR"));
		}, 1.5f, false);
	}
}

void ATSSPlayerController::ShowMenu(bool bMainMenu)
{
	bGameActive = false;
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
