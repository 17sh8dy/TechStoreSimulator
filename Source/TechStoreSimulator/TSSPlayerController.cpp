#include "TSSPlayerController.h"
#include "TSSMenuWidget.h"
#include "TSSHudWidget.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TSSCharacter.h"
#include "TSSShopDoor.h"
#include "TSSRoad.h"
#include "TSSDayNight.h"
#include "TSSStreetBuilding.h"
#include "TSSMover.h"
#include "TSSShelf.h"
#include "TSSStoreSubsystem.h"
#include "TSSBin.h"
#include "TSSPasserby.h"
#include "TSSTown.h"
#include "TSSCustomerSpawner.h"
#include "TSSCustomer.h"
#include "TSSSettings.h"
#include "TSSCustomerSubsystem.h"
#include "TSSRegister.h"
#include "TSSStoreLayout.h"
#include "TSSStoreShell.h"
#include "TSSBusinessSave.h"
#include "TSSDeliveryActors.h"
#include "TSSBusinessSubsystem.h"
#include "TSSTown.h"
#include "TSSEmployee.h"
#include "TSSStoreDeskComputer.h"
#include "TSSStoreDeskWidget.h"
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

		// `-TSSTownShots`: parks one of each car on the road and photographs them, then quits (for looking at the cars).
		if (FParse::Param(FCommandLine::Get(), TEXT("TSSTownShots")))
		{
			FTimerHandle H;
			GetWorldTimerManager().SetTimer(H, [this]()
			{
				TSSStart();
				auto At = [this](float Delay, FString Cmd)
				{
					FTimerHandle T;
					GetWorldTimerManager().SetTimer(T, [this, Cmd]() { ConsoleCommand(Cmd); }, Delay, false);
				};
				float T = 0.3f;
				At(T, TEXT("TSSTownDemo")); T += 0.8f;
				const int32 Shots[][2] = { { 0, 0 }, { 0, 1 }, { 0, 2 }, { 1, 0 }, { 2, 0 }, { 2, 1 }, { 3, 0 }, { 1, 3 }, { 2, 4 } };
				for (const int32* S : Shots)
				{
					At(T, FString::Printf(TEXT("TSSTownCam %d %d"), S[0], S[1])); T += 0.7f;
					At(T, TEXT("Shot ShowUI")); T += 1.1f;
				}
				At(T, TEXT("TSSTime 21.5")); T += 0.6f;
				for (const int32* S : { Shots[0], Shots[3], Shots[6] })
				{
					At(T, FString::Printf(TEXT("TSSTownCam %d %d"), S[0], S[1])); T += 0.7f;
					At(T, TEXT("Shot ShowUI")); T += 1.1f;
				}
				At(T + 0.5f, TEXT("quit"));
			}, 1.5f, false);
		}

		// `-TSSPayShots`: one customer pays by chip and PIN; photographs the typing and the receipt, then quits.
		if (FParse::Param(FCommandLine::Get(), TEXT("TSSPayShots")))
		{
			FTimerHandle H;
			GetWorldTimerManager().SetTimer(H, [this]() { ConsoleCommand(TEXT("TSSPayDemo")); }, 1.0f, false);
		}
		// `-TSSDoorShots`: photographs the shop door from outside and inside, then quits.
		if (FParse::Param(FCommandLine::Get(), TEXT("TSSDoorShots")))
		{
			FTimerHandle H;
			GetWorldTimerManager().SetTimer(H, [this]()
			{
				TSSStart();
				auto At = [this](float Delay, FString Cmd)
				{
					FTimerHandle T;
					GetWorldTimerManager().SetTimer(T, [this, Cmd]() { ConsoleCommand(Cmd); }, Delay, false);
				};
				At(0.5f, TEXT("TSSLook 950 0 140 180 0"));  At(1.8f, TEXT("Shot ShowUI"));
				At(2.4f, TEXT("TSSLook 560 0 140 0 0"));    At(3.7f, TEXT("Shot ShowUI"));
				At(4.3f, TEXT("TSSLook 900 -120 150 150 0")); At(5.6f, TEXT("Shot ShowUI"));
				At(6.5f, TEXT("QUIT_EDITOR"));
			}, 1.0f, false);
		}
		// `-TSSLightShots`: photographs the shop interior at noon, dusk and night (for tuning the lighting), then quits.
		if (FParse::Param(FCommandLine::Get(), TEXT("TSSLightShots")))
		{
			FTimerHandle H;
			GetWorldTimerManager().SetTimer(H, [this]()
			{
				TSSStart();
				auto At = [this](float Delay, FString Cmd)
				{
					FTimerHandle T;
					GetWorldTimerManager().SetTimer(T, [this, Cmd]() { ConsoleCommand(Cmd); }, Delay, false);
				};
				float T = 0.5f;
				for (float Hour : { 12.f, 2.f, 21.5f })
				{
					At(T, FString::Printf(TEXT("TSSTime %.1f"), Hour)); T += 1.0f;
					At(T, TEXT("TSSLook 330 0 150 180 -8")); T += 1.3f;  At(T, TEXT("Shot ShowUI")); T += 1.0f;
					At(T, TEXT("TSSLook 380 0 150 0 -8")); T += 1.3f;   At(T, TEXT("Shot ShowUI")); T += 1.0f;
				}
				At(T + 0.5f, TEXT("QUIT_EDITOR"));
			}, 1.0f, false);
		}
		// `-TSSLifeShots`: trees, a bin, people on the pavement and the empty box in hand, then quits.
		if (FParse::Param(FCommandLine::Get(), TEXT("TSSLifeShots")))
		{
			FTimerHandle H;
			GetWorldTimerManager().SetTimer(H, [this]()
			{
				TSSStart();
				auto At = [this](float Delay, FString Cmd)
				{
					FTimerHandle T;
					GetWorldTimerManager().SetTimer(T, [this, Cmd]() { ConsoleCommand(Cmd); }, Delay, false);
				};
				At(0.2f, TEXT("TSSTime 12.4"));
				At(0.3f, TEXT("TSSCrowd 6"));
				At(1.0f, TEXT("TSSLook 840 -1000 165 90 -3"));   At(3.0f, TEXT("Shot ShowUI"));
				At(3.6f, TEXT("TSSLook 900 -1560 170 -35 -2"));  At(5.2f, TEXT("Shot ShowUI"));
				At(5.8f, TEXT("TSSLook 1000 215 190 8 -32"));    At(7.2f, TEXT("Shot ShowUI"));
				At(7.25f, TEXT("TSSLook 1790 140 175 40 -14"));  At(8.1f, TEXT("Shot ShowUI"));
				At(8.3f, TEXT("TSSBox 1"));
				At(8.4f, TEXT("TSSLook 900 -80 165 0 -12"));     At(9.7f, TEXT("Shot ShowUI"));
				At(10.1f, TEXT("TSSTime 21.5"));
				At(10.7f, TEXT("TSSBox 0"));
				At(10.9f, TEXT("TSSLook 840 -1000 165 90 -3"));  At(12.3f, TEXT("Shot ShowUI"));
				At(13.1f, TEXT("QUIT_EDITOR"));
			}, 1.0f, false);
		}
		// `-TSSStreetShots`: sends a truck down the road and photographs it, then quits (for looking at the street).
	if (FParse::Param(FCommandLine::Get(), TEXT("TSSStreetShots")))
	{
		FTimerHandle H;
		GetWorldTimerManager().SetTimer(H, [this]()
		{
			TSSStart();
			auto At = [this](float Delay, FString Cmd)
			{
				FTimerHandle T;
				GetWorldTimerManager().SetTimer(T, [this, Cmd]() { ConsoleCommand(Cmd); }, Delay, false);
			};
			At(0.2f, TEXT("TSSTruck"));
			At(0.5f, TEXT("TSSLook 760 -900 140 25 -5"));
			At(2.2f, TEXT("Shot ShowUI"));
			At(3.0f, TEXT("TSSLook 800 -250 150 15 -6"));
			At(4.6f, TEXT("Shot ShowUI"));
			At(5.4f, TEXT("TSSLook 830 120 150 -15 -6"));
			At(7.0f, TEXT("Shot ShowUI"));
			At(7.6f, TEXT("TSSLook 760 0 160 0 -2"));
			At(8.6f, TEXT("Shot ShowUI"));
			At(9.2f, TEXT("TSSLook 790 -650 170 -35 -3"));
			At(10.2f, TEXT("Shot ShowUI"));
			At(10.8f, TEXT("TSSTime 17.8"));
			At(11.0f, TEXT("TSSLook 760 0 160 0 -2"));
			At(12.2f, TEXT("Shot ShowUI"));
			At(12.6f, TEXT("TSSTime 21.5"));
			At(13.6f, TEXT("Shot ShowUI"));
			At(14.0f, TEXT("TSSLook 790 -650 170 -35 -3"));
			At(15.0f, TEXT("Shot ShowUI"));
			At(15.4f, TEXT("TSSTime 12"));
			At(16.4f, TEXT("Shot ShowUI"));
			At(17.2f, TEXT("TSSSettingsPage"));
			At(18.2f, TEXT("Shot ShowUI"));
			At(19.2f, TEXT("quit"));
		}, 1.5f, false);
	}

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
			// 3: the shop door and its sign, from inside and from the street.
			Shot(4.5f, TEXT("TSSLook 430 0 100 0 -4"));
			Shot(5.5f, TEXT("Shot ShowUI"));
			Shot(6.5f, TEXT("TSSLook 780 -150 130 20 -6"));
			Shot(7.5f, TEXT("Shot ShowUI"));
			Shot(8.5f, TEXT("TSSCustomerTest"));
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

void ATSSPlayerController::OpenStoreDesk()
{
	if (!bGameActive || bDeskOpen || !Hud) return;
	if (!Desk)
	{
		Desk = CreateWidget<UTSSStoreDeskWidget>(this, UTSSStoreDeskWidget::StaticClass());
		Desk->SetOnClose([this]() { CloseStoreDesk(); });
	}
	bDeskOpen = true;
	bGameActive = false; // the character stops reading input; the shop itself keeps running
	Hud->SetVisibility(ESlateVisibility::Collapsed);
	Desk->AddToViewport(20);
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	bShowMouseCursor = true;
}

void ATSSPlayerController::CloseStoreDesk()
{
	if (!bDeskOpen) return;
	bDeskOpen = false;
	if (Desk) Desk->RemoveFromParent();
	if (UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>()) Biz->SaveNow();
	EnterGameInput();
}

void ATSSPlayerController::TSSDeskPage(int32 Page)
{
	TSSStart();
	if (!bDeskOpen) OpenStoreDesk();
	if (Desk) Desk->SetPage(Page);
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

	// While the Settings screen waits for a sprint key, the next key press is the new binding (Esc cancels).
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTSSSettingsSubsystem* Cfg = GI->GetSubsystem<UTSSSettingsSubsystem>())
		{
			if (Cfg->IsAwaitingSprintKey())
			{
				if (WasInputKeyJustPressed(EKeys::Escape)) { Cfg->CancelAwaitSprintKey(); return; }
				TArray<FKey> All;
				EKeys::GetAllKeys(All);
				for (const FKey& K : All)
				{
					if (K == EKeys::AnyKey || !K.IsValid() || K.IsGamepadKey() || K.IsTouch() || K.IsAxis1D() || K.IsAxis2D() || K.IsAxis3D() || K == EKeys::LeftMouseButton || K == EKeys::MouseScrollUp || K == EKeys::MouseScrollDown) continue;
					if (K == EKeys::Tab || K == EKeys::W || K == EKeys::A || K == EKeys::S || K == EKeys::D || K == EKeys::E) continue;   // used for other things
					if (WasInputKeyJustPressed(K)) { Cfg->SetSprintKey(K); break; }
				}
				return;
			}
		}
	}

	if (bDeskOpen)
	{
		if (WasInputKeyJustPressed(EKeys::Escape)) CloseStoreDesk();
		return;
	}

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

void ATSSPlayerController::TSSPayDemo()
{
	UTSSCustomerSubsystem* CS = GetWorld()->GetSubsystem<UTSSCustomerSubsystem>();
	ATSSRegister* Reg = nullptr;
	for (TActorIterator<ATSSRegister> It(GetWorld()); It; ++It) { Reg = *It; break; }
	ATSSCharacter* Me = Cast<ATSSCharacter>(GetPawn());
	if (!CS || !Reg || !Me) return;
	TSSStart();
	CS->bAutoSpawn = false;
	for (TActorIterator<ATSSCustomer> It(GetWorld()); It; ++It) It->Destroy();
	Reg->ReleaseSale();
	Reg->bForceChip = true; Reg->ExpiredCardChance = 0.f; Reg->CardDeclineChance = 0.f; Reg->WrongPinChance = 0.f;
	ATSSCustomer* C = CS->SpawnCustomer(8.f);
	if (!C) return;
	C->Patience = 300.f; C->ForceMethod = ETSSPayMethod::Card;
	struct FS { int32 Step = 0; float T = 0.f; int32 Shots = 0; float Next = 0.f; };
	TSharedRef<FS> St = MakeShared<FS>();
	auto Cam = [this, Reg](FVector P, FVector T)
	{
		const FVector Wp = Reg->GetActorTransform().TransformPosition(P);
		const FVector Wt = Reg->GetActorTransform().TransformPosition(T);
		const FRotator R = (Wt - (Wp + FVector(0.f, 0.f, 154.f))).Rotation();
		ConsoleCommand(FString::Printf(TEXT("TSSLook %f %f 90 %f %f"), Wp.X, Wp.Y, R.Yaw, R.Pitch));
	};
	FTimerHandle* H = new FTimerHandle();
	GetWorldTimerManager().SetTimer(*H, [this, St, Reg, Me, Cam, H]()
	{
		St->T += 0.1f;
		switch (St->Step)
		{
		case 0:
			if (Reg->GetPhase() == ETSSCheckoutPhase::Scanning && Reg->AllItemsPlaced())
			{
				for (ATSSProductItem* I : Reg->GetItems()) { I->Interact(Me, FHitResult()); Reg->TryScanHeld(Me); }
				Reg->GetKeyboard()->Interact(Me, FHitResult());
				St->Step = 1; St->T = 0.f;
			}
			break;
		case 1: // the customer is at the pad: photograph the card going in and the typing from the customer's side
			if (Reg->GetPhase() == ETSSCheckoutPhase::CardTap || Reg->GetPhase() == ETSSCheckoutPhase::CardPin)
			{
				if (St->T >= St->Next && St->Shots < 8)
				{
					Cam(FVector(-135.f, -25.f, 0.f), FVector(-40.f, 40.f, 100.f));
					FTimerHandle T2; GetWorldTimerManager().SetTimer(T2, [this]() { ConsoleCommand(TEXT("Shot ShowUI")); }, 0.12f, false);
					++St->Shots; St->Next = St->T + 0.55f;
				}
			}
			else if (Reg->GetPhase() == ETSSCheckoutPhase::CardReady)
			{
				Reg->GetTerminal()->Interact(Me, FHitResult());
				St->Step = 2; St->T = 0.f;
			}
			if (St->T > 60.f) ConsoleCommand(TEXT("QUIT_EDITOR"));
			break;
		case 2: // receipt
			if (Reg->GetPhase() == ETSSCheckoutPhase::Receipt && St->T > 2.2f)
			{
				Cam(FVector(70.f, -105.f, 0.f), FVector(28.f, -122.f, 30.f));
				FTimerHandle T2; GetWorldTimerManager().SetTimer(T2, [this]() { ConsoleCommand(TEXT("Shot ShowUI")); }, 0.5f, false);
				St->Step = 3; St->T = 0.f;
			}
			if (St->T > 30.f) ConsoleCommand(TEXT("QUIT_EDITOR"));
			break;
		case 3:
			if (St->T > 1.5f) ConsoleCommand(TEXT("QUIT_EDITOR"));
			break;
		}
	}, 0.1f, true);
}

void ATSSPlayerController::TSSStart()
{
	if (Menu) EnterGameInput();
}

void ATSSPlayerController::TSSBox(int32 On)
{
	if (ATSSCharacter* Me = Cast<ATSSCharacter>(GetPawn())) Me->SetCarryingEmptyBox(On != 0);
}

void ATSSPlayerController::TSSCrowd(int32 Count)
{
	UTSSTownSubsystem* Town = GetWorld()->GetSubsystem<UTSSTownSubsystem>();
	if (!Town) return;
	for (int32 i = 0; i < Count; ++i) Town->SpawnPasserby(i % 3 != 2, i % 2 == 0, false);
}

void ATSSPlayerController::TSSLook(float X, float Y, float Z, float Yaw, float Pitch)
{
	if (APawn* P = GetPawn()) P->SetActorLocation(FVector(X, Y, Z));
	SetControlRotation(FRotator(Pitch, Yaw, 0.f));
}

void ATSSPlayerController::TSSTownDemo()
{
	UTSSTownSubsystem* Town = GetWorld()->GetSubsystem<UTSSTownSubsystem>();
	ATSSRoad* Road = ATSSRoad::Find(GetWorld());
	if (!Town || !Road) { UE_LOG(LogTemp, Display, TEXT("TSS town demo: no town or road")); return; }
	Town->SetTrafficEnabled(false);
	Town->ClearTraffic();
	const float D0 = Road->GetCrosswalkDistance() + 800.f;
	const ETSSCarType Types[] = { ETSSCarType::Sedan, ETSSCarType::Hatchback, ETSSCarType::Suv, ETSSCarType::Van };
	const FLinearColor Paints[] = { FLinearColor(0.55f, 0.03f, 0.03f), FLinearColor(0.03f, 0.1f, 0.35f), FLinearColor(0.8f, 0.8f, 0.78f), FLinearColor(0.02f, 0.18f, 0.1f) };
	for (int32 i = 0; i < 4; ++i)
	{
		ATSSTrafficCar* Car = Town->SpawnCar(Types[i], true, D0 + 900.f * i, 0.f, Paints[i]);
		if (Car) UE_LOG(LogTemp, Display, TEXT("TSS town demo car %d at %s, %d triangles"), i, *Car->GetActorLocation().ToString(), Car->NumTriangles());
	}
	if (ATSSTrafficCar* Oncoming = Town->SpawnCar(ETSSCarType::Sedan, false, D0 + 400.f, 0.f, FLinearColor(0.12f, 0.2f, 0.4f))) { (void)Oncoming; }
}

void ATSSPlayerController::TSSTownCam(int32 CarIndex, int32 Which)
{
	UTSSTownSubsystem* Town = GetWorld()->GetSubsystem<UTSSTownSubsystem>();
	if (!Town) return;
	TArray<ATSSTrafficCar*> Forward;
	for (const TWeakObjectPtr<ATSSTrafficCar>& C : Town->GetCars()) if (C.IsValid() && C->GoesAlongRoad()) Forward.Add(C.Get());
	Forward.Sort([](const ATSSTrafficCar& A, const ATSSTrafficCar& B) { return A.GetDistance() < B.GetDistance(); });
	if (!Forward.IsValidIndex(CarIndex)) return;
	const ATSSTrafficCar* Car = Forward[CarIndex];
	const FVector At = Car->GetActorLocation() + FVector(0.f, 0.f, 70.f);
	const FVector Fwd = Car->GetActorForwardVector(), Rgt = Car->GetActorRightVector();
	FVector From;
	switch (Which)
	{
	case 0: From = At + Fwd * 520.f - Rgt * 360.f; break;          // front three-quarter, from the shop side
	case 1: From = At - Rgt * 620.f; break;                         // side, from the shop side
	case 2: From = At - Fwd * 520.f - Rgt * 360.f; break;          // rear three-quarter
	case 3: From = At - Fwd * 1500.f - Rgt * 700.f; break;         // down the street
	default: From = At + FVector(0.f, 0.f, 700.f) - Fwd * 500.f - Rgt * 700.f; break;
	}
	const FVector Eye = FVector(From.X, From.Y, FMath::Max(From.Z, 90.f));
	const FRotator Face = (At - (Eye + FVector(0.f, 0.f, 0.f))).Rotation();
	if (APawn* P = GetPawn()) P->SetActorLocation(FVector(Eye.X, Eye.Y, Eye.Z - 64.f));
	SetControlRotation(Face);
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
	UTSSCustomerSubsystem* CS = GetWorld()->GetSubsystem<UTSSCustomerSubsystem>();
	ATSSShelf* Shelf = nullptr;
	ATSSShopDoor* Door = nullptr;
	for (TActorIterator<ATSSShelf> It(GetWorld()); It; ++It) { Shelf = *It; break; }
	for (TActorIterator<ATSSShopDoor> It(GetWorld()); It; ++It) { Door = *It; break; }
	Check(TEXT("found player, store, customer spawner, a shelf and the shop door"), Me && Store && CS && Shelf && Door);
	if (!(Me && Store && CS && Shelf && Door)) return;

	Check(TEXT("empty hands: no box is drawn in front of the player"), !Me->IsStrayBoxDrawn());

	// Stock comes from StoreDesk orders now; here the hands are loaded directly to exercise the shelf itself.
	auto Load = [Me](ETSSProduct P) { Me->SetHeld(P); };

	// A hit on the shelf's first-tier, second-column slot, from the shelf's own frame.
	FHitResult Hit;
	Hit.ImpactPoint = Shelf->GetActorTransform().TransformPosition(FVector(24.f, -25.f, 30.f));

	Load(ETSSProduct::Phone);
	Shelf->Interact(Me, Hit);
	Check(TEXT("placing empties the hands"), Me->GetHeld() == ETSSProduct::None);
	Check(TEXT("prompt on a stocked slot offers Take"), Shelf->GetPrompt(Me, Hit).Contains(TEXT("[E / Click] Take Smartphone")));

	Shelf->Interact(Me, Hit);
	Check(TEXT("taking returns the phone to the hands"), Me->GetHeld() == ETSSProduct::Phone);
	Shelf->Interact(Me, Hit);
	for (int32 i = 0; i < 3; ++i) { Load(ETSSProduct::Phone); Shelf->Interact(Me, Hit); }
	Load(ETSSProduct::Phone);
	Shelf->Interact(Me, Hit);
	Check(TEXT("a full slot refuses a fifth phone"), Me->GetHeld() == ETSSProduct::Phone && Shelf->GetPrompt(Me, Hit).Contains(TEXT("full")));

	Load(ETSSProduct::Laptop);
	Shelf->Interact(Me, Hit);
	Check(TEXT("a slot holding phones refuses a laptop"), Me->GetHeld() == ETSSProduct::Laptop);
	Load(ETSSProduct::None);

	const int32 Price = Store->GetPrice(ETSSProduct::Phone);
	Shelf->Adjust(Me, Hit, +1);
	Shelf->Adjust(Me, Hit, +1);
	Check(TEXT("wheel up twice raises the phone price by $2"), Store->GetPrice(ETSSProduct::Phone) == Price + 2);

	// Leave the shop looking stocked for screenshots and the customer test.
	const ETSSProduct Fill[] = { ETSSProduct::Laptop, ETSSProduct::Headphones, ETSSProduct::Console, ETSSProduct::Monitor, ETSSProduct::Keyboard };
	int32 Slot = 0;
	for (ETSSProduct P : Fill)
	{
		FHitResult H;
		// Tiers 1 and 2 (the first tier already has the phones).
		const float Z = 90.f + 60.f * (Slot / ATSSShelf::Columns);
		H.ImpactPoint = Shelf->GetActorTransform().TransformPosition(FVector(24.f, -75.f + 50.f * (Slot % ATSSShelf::Columns), Z));
		for (int32 i = 0; i < FMath::Min(3, TSSProductDef(P).SlotCapacity()); ++i) { Load(P); Shelf->Interact(Me, H); }
		++Slot;
	}
	Load(ETSSProduct::None);

	// ---- the door and the Open/Closed sign ----
	Check(TEXT("the shop starts open"), Store->IsOpen());
	Check(TEXT("open: customers are allowed to come in"), CS->MaySpawn());
	Check(TEXT("door prompt offers to open it"), Door->GetPrompt(Me, FHitResult()).Contains(TEXT("Open the door")));
	Door->Interact(Me, FHitResult());
	Check(TEXT("E on the door holds it open"), Door->IsHeldOpen());
	Check(TEXT("door prompt now offers to close it"), Door->GetPrompt(Me, FHitResult()).Contains(TEXT("Close the door")));
	Door->Interact(Me, FHitResult());
	Check(TEXT("E again lets it close"), !Door->IsHeldOpen());

	Check(TEXT("the sign prompt offers to close"), Door->SignPrompt().Contains(TEXT("the shop is OPEN")));
	Door->ToggleSign();
	Check(TEXT("flipping the sign closes the shop"), !Store->IsOpen());
	Check(TEXT("closed: no customers come in even though the shelves are full"), !CS->MaySpawn());
	Check(TEXT("the sign prompt now offers to reopen"), Door->SignPrompt().Contains(TEXT("the shop is CLOSED")));
	Door->ToggleSign();
	Check(TEXT("flipping it again reopens the shop"), Store->IsOpen() && CS->MaySpawn());

	// ---- the clock, the day/night cycle and the bell ----
	UTSSBusinessSubsystem* Clock = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>();
	Check(TEXT("the bell sound asset exists"), Door->HasBellSound());
	Check(TEXT("clock formatting: 17.75 is 5:45 PM, 0 is 12:00 AM, 12 is 12:00 PM, 8.5 is 8:30 AM"),
		UTSSBusinessSubsystem::FormatClock(17.75f) == TEXT("5:45 PM") && UTSSBusinessSubsystem::FormatClock(0.f) == TEXT("12:00 AM")
		&& UTSSBusinessSubsystem::FormatClock(12.f) == TEXT("12:00 PM") && UTSSBusinessSubsystem::FormatClock(8.5f) == TEXT("8:30 AM"));
	if (Clock)
	{
		const float Hour0 = Clock->GetHour();
		Check(TEXT("a new game starts at 8:00 AM"), FMath::IsNearlyEqual(Hour0, UTSSBusinessSubsystem::StartHour, 0.3f) || Clock->GetSimSeconds() > 1.f);
		ATSSDayNight* DN = nullptr;
		for (TActorIterator<ATSSDayNight> It(GetWorld()); It; ++It) { DN = *It; break; }
		ATSSRoad* Rd = ATSSRoad::Find(GetWorld());
		Check(TEXT("the map has a day/night actor"), DN != nullptr);
		if (DN)
		{
			Clock->SetHour(17.75f);
			Check(TEXT("setting the time works: 5:45 PM"), FMath::IsNearlyEqual(Clock->GetHour(), 17.75f, 0.01f));
			auto LitWindows = [this]() { int32 N = 0; for (TActorIterator<ATSSStreetBuilding> It(GetWorld()); It; ++It) N += It->GetLitWindowCount(); return N; };
			ATSSStoreShell* ShellActor = nullptr;
			for (TActorIterator<ATSSStoreShell> It(GetWorld()); It; ++It) { ShellActor = *It; break; }
			Clock->SetHour(12.f); DN->ApplyNow();
			Check(TEXT("noon: the sun is bright and the street lamps are off"), DN->GetLightLux() > 5.f && !DN->IsNight() && (!Rd || !Rd->AreLampsOn()));
			Check(TEXT("noon: no windows are lit and the shop sign light is off"), LitWindows() == 0 && (!ShellActor || !ShellActor->IsSignLightOn()));
			Clock->SetHour(23.f); DN->ApplyNow();
			Check(TEXT("11 PM: it is night, the light is only dim moonlight, the street lamps are on"), DN->IsNight() && DN->GetLightLux() < 1.f && (!Rd || Rd->AreLampsOn()));
			Check(TEXT("11 PM: windows across the street are lit and the shop sign is floodlit"), LitWindows() > 0 && (!ShellActor || ShellActor->IsSignLightOn()));
			Clock->SetHour(6.f); DN->ApplyNow();
			Check(TEXT("sunrise at 6:00 is dimmer than noon but not dark"), DN->GetLightLux() < 8.f && DN->GetLightLux() > 1.f);
			Clock->SetHour(Hour0); DN->ApplyNow();
		}
		Check(TEXT("a new calendar day begins at midnight"), UTSSBusinessSubsystem::DayForSeconds(0.f) == 1
			&& UTSSBusinessSubsystem::DayForSeconds(UTSSBusinessSubsystem::DayLengthSeconds * 16.f / 24.f + 1.f) == 2);
	}

	// ---- the door swings away from whoever opens it ----
	{
		const FVector Home = Me->GetActorLocation();
		Me->SetActorLocation(FVector(800.f, 0.f, Home.Z));
		Door->Interact(Me, FHitResult());
		Check(TEXT("opened from outside, the door swings INTO the shop"), Door->GetSwingDir() > 0);
		Door->Interact(Me, FHitResult());
		Me->SetActorLocation(FVector(600.f, 150.f, Home.Z));
		Door->Interact(Me, FHitResult());
		Check(TEXT("opened from inside, the door swings OUT onto the street"), Door->GetSwingDir() < 0);
		Door->Interact(Me, FHitResult());
		Me->SetActorLocation(Home);
	}

	// ---- pick up and move furniture: hold E for 2 seconds ----
	UTSSBusinessSubsystem* LayoutBiz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>();
	if (UTSSMoverComponent* Mv = Me->GetMover())
	{
		FHitResult H0;
		const int32 Taps0 = Mv->GetTapCount();
		Mv->HoldStep(Me, Shelf, H0, 0.02f, true, true);
		Mv->HoldStep(Me, Shelf, H0, 0.02f, false, false);
		Check(TEXT("a quick tap on a shelf is still a normal press, not a move"), Mv->GetTapCount() == Taps0 + 1 && !Mv->IsMoving());

		Mv->HoldStep(Me, Shelf, H0, 0.02f, true, true);
		for (int32 i = 0; i < 90; ++i) Mv->HoldStep(Me, Shelf, H0, 0.02f, true, false);   // about 1.8 s
		Check(TEXT("holding E for less than 2 seconds does not pick it up"), !Mv->IsMoving() && Mv->GetHoldFraction() > 0.8f);
		for (int32 i = 0; i < 12; ++i) Mv->HoldStep(Me, Shelf, H0, 0.02f, true, false);   // past 2 s
		Check(TEXT("holding E for 2 seconds picks the shelf up"), Mv->IsMoving() && Mv->GetMoving() == Shelf);

		const FVector S0 = Shelf->GetActorLocation();
		const FRotator R0 = Shelf->GetActorRotation();
		TArray<UStaticMeshComponent*> Parts;
		Shelf->GetComponents<UStaticMeshComponent>(Parts);
		TArray<FVector> Before;
		for (UStaticMeshComponent* C : Parts) Before.Add(C->GetComponentLocation());
		int32 StockedBoxes = 0;
		for (int32 s = 0; s < ATSSShelf::Tiers * ATSSShelf::Columns; ++s) StockedBoxes += Shelf->GetSlotQty(s);

		// Somewhere free in the shop.
		bool bFound = false;
		FVector Free = S0;
		for (float X = 120.f; X <= 560.f && !bFound; X += 20.f)
		{
			for (float Y = -220.f; Y <= 220.f && !bFound; Y += 20.f)
			{
				Mv->SetPlacement(FVector(X, Y, 0.f), R0.Yaw);
				if (Mv->IsPlacementValid()) { bFound = true; Free = FVector(X, Y, S0.Z); }
			}
		}
		Check(TEXT("there is a free spot to put the shelf, and the game accepts it"), bFound);

		const FVector Delta = Shelf->GetActorLocation() - S0;
		bool bAllMoved = Parts.Num() > 5;
		for (int32 i = 0; i < Parts.Num(); ++i) bAllMoved &= (Parts[i]->GetComponentLocation() - Before[i] - Delta).Size() < 0.5f;
		Check(TEXT("the whole shelf comes along: frame, price tags and every stocked box"), bAllMoved && StockedBoxes > 0);

		Mv->SetPlacement(FVector(5000.f, 0.f, 0.f), R0.Yaw);
		Check(TEXT("it cannot be put outside the shop"), !Mv->IsPlacementValid());
		Check(TEXT("and putting it down there is refused"), !Mv->Commit() && Mv->IsMoving());
		Mv->SetPlacement(FVector(600.f, 0.f, 0.f), R0.Yaw);
		Check(TEXT("it cannot block the doorway or the path in from the door"), !Mv->IsPlacementValid());
		ATSSShelf* OtherShelf = nullptr;
		for (TActorIterator<ATSSShelf> It(GetWorld()); It; ++It) if (*It != Shelf) { OtherShelf = *It; break; }
		if (OtherShelf)
		{
			Mv->SetPlacement(OtherShelf->GetActorLocation(), R0.Yaw);
			Check(TEXT("it cannot be put on top of another shelf"), !Mv->IsPlacementValid());
		}
		Mv->SetPlacement(Free, 90.f);
		Check(TEXT("it can be rotated"), FMath::IsNearlyEqual(FMath::Abs(Shelf->GetActorRotation().Yaw), 90.f, 0.5f));
		Mv->SetPlacement(Free, R0.Yaw);
		Check(TEXT("Q puts it back where it was"), [&]() { Mv->Cancel(); return !Mv->IsMoving() && (Shelf->GetActorLocation() - S0).Size() < 0.5f; }());

		// Move it for real, and check the layout is remembered.
		Mv->BeginMove(Me, Shelf);
		Mv->SetPlacement(Free, R0.Yaw);
		const bool bCommitted = Mv->Commit();
		Check(TEXT("E puts the shelf down in its new place"), bCommitted && !Mv->IsMoving() && (Shelf->GetActorLocation() - Free).Size() < 0.5f);
		Check(TEXT("the new layout is remembered for the save"), LayoutBiz && LayoutBiz->GetMovedCount() >= 1);
		Shelf->SetActorLocation(S0);
		if (LayoutBiz) LayoutBiz->ApplyLayout();
		Check(TEXT("loading a game puts the shelf back where the owner left it"), (Shelf->GetActorLocation() - Free).Size() < 0.5f);
		Mv->BeginMove(Me, Shelf);
		Mv->SetPlacement(S0, R0.Yaw);
		Mv->Commit();   // put it back for the tests that follow

		// The StoreDesk computer, with its keyboard and mouse.
		ATSSStoreDeskComputer* DeskPc = nullptr;
		for (TActorIterator<ATSSStoreDeskComputer> It(GetWorld()); It; ++It) { DeskPc = *It; break; }
		if (DeskPc)
		{
			const FVector D0 = DeskPc->GetActorLocation();
			const FRotator DR0 = DeskPc->GetActorRotation();
			const TArray<FName> DeskTags0 = DeskPc->Tags;
			TArray<UStaticMeshComponent*> DParts;
			DeskPc->GetComponents<UStaticMeshComponent>(DParts);
			TArray<FVector> DBefore;
			for (UStaticMeshComponent* C : DParts) DBefore.Add(C->GetComponentLocation());
			Mv->BeginMove(Me, DeskPc);
			bool bDeskFound = false;
			FVector DFree = D0;
			for (float X = 120.f; X <= 560.f && !bDeskFound; X += 20.f)
			{
				for (float Y = -220.f; Y <= 220.f && !bDeskFound; Y += 20.f)
				{
					Mv->SetPlacement(FVector(X, Y, 0.f), DR0.Yaw);
					if (Mv->IsPlacementValid()) { bDeskFound = true; DFree = FVector(X, Y, D0.Z); }
				}
			}
			const FVector DDelta = DeskPc->GetActorLocation() - D0;
			bool bDeskAll = DParts.Num() > 4;
			for (int32 i = 0; i < DParts.Num(); ++i) bDeskAll &= (DParts[i]->GetComponentLocation() - DBefore[i] - DDelta).Size() < 0.5f;
			Check(TEXT("the StoreDesk computer moves with its monitor, tower, keyboard and mouse"), bDeskFound && bDeskAll);
			Check(TEXT("and can be put down"), Mv->Commit());
			Mv->BeginMove(Me, DeskPc);
			Mv->SetPlacement(D0, DR0.Yaw);
			Mv->Commit();
			DeskPc->Tags = DeskTags0;   // a test move must not change which wall the desk is anchored to
		}
	}

	// ---- a big delivery stacks in the corner instead of covering the floor ----
	{
		TArray<ATSSParcel*> Pile;
		for (int32 i = 0; i < 40; ++i)
		{
			const FVector At = ATSSParcel::NextStackSlot(GetWorld());
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ATSSParcel* Parcel = GetWorld()->SpawnActor<ATSSParcel>(ATSSParcel::StaticClass(), FTransform(At), P))
			{
				Parcel->Init(-5, ETSSProduct::Phone, 1);
				Pile.Add(Parcel);
			}
		}
		Check(TEXT("40 parcels all fit"), Pile.Num() == 40);
		int32 Stage = 0;
		for (TActorIterator<ATSSStoreShell> It(GetWorld()); It; ++It) { Stage = It->GetStage(); break; }
		bool bInside = true, bClear = true, bNoOverlap = true, bNoFloaters = true;
		float MaxZ = 0.f;
		int32 OnFloor = 0;
		for (int32 i = 0; i < Pile.Num(); ++i)
		{
			const FVector L = Pile[i]->GetActorLocation();
			MaxZ = FMath::Max(MaxZ, L.Z);
			if (FMath::IsNearlyEqual(L.Z, 20.f, 1.f)) ++OnFloor;
			bInside &= L.X > TSSStoreLayout::BackX(Stage) && L.X < TSSStoreLayout::FrontX && FMath::Abs(L.Y) < TSSStoreLayout::HalfWidth(Stage);
			bClear &= !(L.X > 330.f && FMath::Abs(L.Y) < 95.f);
			bool bBelow = FMath::IsNearlyEqual(L.Z, 20.f, 1.f);
			for (int32 j = 0; j < Pile.Num(); ++j)
			{
				if (i == j) continue;
				const FVector M = Pile[j]->GetActorLocation();
				const bool bSameColumn = FMath::Abs(L.X - M.X) < 4.f && FMath::Abs(L.Y - M.Y) < 4.f;
				if (bSameColumn && FMath::Abs(L.Z - M.Z) < 4.f) bNoOverlap = false;
				if (bSameColumn && FMath::IsNearlyEqual(M.Z, L.Z - 40.f, 1.f)) bBelow = true;
			}
			bNoFloaters &= bBelow;
		}
		Check(TEXT("the pile stays inside the shop and off the walkway from the door"), bInside && bClear);
		Check(TEXT("no two parcels are in the same place and none floats in the air"), bNoOverlap && bNoFloaters);
		Check(TEXT("it is stacked, not spread out: only a few on the floor, and under the ceiling"), OnFloor < 40 && OnFloor >= 3 && MaxZ <= 20.f + 40.f * 6.f + 1.f);
		// Unpack one from the middle of a stack: the ones above must drop down.
		ATSSParcel* Middle = nullptr;
		for (ATSSParcel* Pcl : Pile) if (FMath::IsNearlyEqual(Pcl->GetActorLocation().Z, 60.f, 1.f)) { Middle = Pcl; break; }
		if (Middle)
		{
			const FVector Col = Middle->GetActorLocation();
			Pile.Remove(Middle);
			Middle->Destroy();
			ATSSParcel::SettleStacks(GetWorld());
			bool bContiguous = true;
			TArray<float> Zs;
			for (ATSSParcel* Pcl : Pile) { const FVector L = Pcl->GetActorLocation(); if (FMath::Abs(L.X - Col.X) < 4.f && FMath::Abs(L.Y - Col.Y) < 4.f) Zs.Add(L.Z); }
			Zs.Sort();
			for (int32 i = 0; i < Zs.Num(); ++i) bContiguous &= FMath::IsNearlyEqual(Zs[i], 20.f + 40.f * i, 1.f);
			Check(TEXT("when a parcel is unpacked from a stack the ones above drop down"), bContiguous);
		}
		for (ATSSParcel* Pcl : Pile) if (Pcl) Pcl->Destroy();
	}

	// ---- sprint ----
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTSSSettingsSubsystem* Cfg = GI->GetSubsystem<UTSSSettingsSubsystem>())
		{
			const FKey Original = Cfg->GetSprintKey();
			Cfg->SetSprintKey(EKeys::RightShift, false);
			Check(TEXT("the sprint key can be rebound"), Cfg->GetSprintKey() == EKeys::RightShift);
			Cfg->BeginAwaitSprintKey();
			Check(TEXT("Settings waits for a key press when you click the sprint button"), Cfg->IsAwaitingSprintKey());
			Cfg->CancelAwaitSprintKey();
			Cfg->ResetSprintKey(false);
			Check(TEXT("Reset puts the sprint key back to Shift"), Cfg->GetSprintKey() == EKeys::LeftShift);
			Cfg->SetSprintKey(Original, false);   // session-only: the test never rewrites the player's saved settings
		}
	}
Check(TEXT("sprinting is clearly faster than walking"), ATSSCharacter::SpeedFor(true) >= ATSSCharacter::SpeedFor(false) * 1.8f);

	// ---- debit cards ----
	{
		bool bOk = true, bExpiredOk = true, bFreshOk = true;
		const FDateTime Today(2026, 11, 20);
		TSet<FString> Seen;
		for (int32 i = 0; i < 400; ++i)
		{
			const FTSSBankCard C = TSSCards::Generate(Today, false);
			const FTSSBankCard X = TSSCards::Generate(Today, true);
			if (C.Number.Len() != 16 || !C.Number.StartsWith(TEXT("9")) || !TSSCards::LuhnValid(C.Number) || C.Pin.Len() != 4 || C.Cvv.Len() != 3) bOk = false;
			if (C.ExpMonth < 1 || C.ExpMonth > 12 || C.IsExpiredOn(Today)) bFreshOk = false;
			if (!X.IsExpiredOn(Today) || !TSSCards::LuhnValid(X.Number)) bExpiredOk = false;
			Seen.Add(C.Number);
		}
		Check(TEXT("generated cards have 16 digits, start with 9, and pass the Luhn check"), bOk);
		Check(TEXT("card numbers are random (no repeats in 400)"), Seen.Num() == 400);
		Check(TEXT("fresh cards are in date and expired cards are really expired"), bFreshOk && bExpiredOk);
		FTSSBankCard Fixed;
		Fixed.ExpMonth = 11; Fixed.ExpYear = 2026;
		Check(TEXT("a card is good through the last day of its expiry month"), !Fixed.IsExpiredOn(FDateTime(2026, 11, 30)) && Fixed.IsExpiredOn(FDateTime(2026, 12, 1)));
		Check(TEXT("a number that fails the Luhn check is rejected"), TSSCards::LuhnValid(TEXT("4539578763621486")) && !TSSCards::LuhnValid(TEXT("4539578763621487")));
		Check(TEXT("card formatting: spaced, masked, expiry and PIN dots"), [&]() { FTSSBankCard K; K.Number = TEXT("9412345678901234"); K.ExpMonth = 8; K.ExpYear = 2028; return K.Spaced() == TEXT("9412 3456 7890 1234") && K.Masked() == TEXT("**** **** **** 1234") && K.Expiry() == TEXT("08/28") && TSSCards::PinDots(2) == TEXT("* * _ _"); }());
	}

	// ---- US money ----
	Check(TEXT("money is formatted with cents and thousands separators"), TSSMoney::Format(123456) == TEXT("$1,234.56") && TSSMoney::Format(5) == TEXT("$0.05") && TSSMoney::Format(-150) == TEXT("-$1.50"));
	Check(TEXT("whole-dollar amounts print short"), TSSMoney::FormatShort(2000) == TEXT("$20") && TSSMoney::FormatShort(2050) == TEXT("$20.50"));
	Check(TEXT("sales tax is 8% rounded to the cent"), TSSMoney::TaxFor(41900) == 3352 && TSSMoney::TaxFor(999) == 80 && TSSMoney::TaxFor(0) == 0);
	Check(TEXT("there are six notes and five coins with the real US values"),
		TSSMoney::NumBills == 6 && TSSMoney::Denom(0).Cents == 100 && TSSMoney::Denom(5).Cents == 10000 && TSSMoney::Denom(6).Cents == 1 && TSSMoney::Denom(9).Cents == 25 && TSSMoney::Denom(10).bCoin);
	Check(TEXT("a note is found before a coin of the same value"), TSSMoney::IndexOf(100) == 0 && TSSMoney::IndexOfCoin(100) == 10 && TSSMoney::IndexOf(25) == 9);
	{
		bool bChangeOk = true, bTenderOk = true;
		const TSSMoney::FStock Float = TSSMoney::StartingFloat();
		for (int32 Cents = 0; Cents <= 20000 && bChangeOk; Cents += 37)
		{
			TArray<int32> Pieces;
			if (!TSSMoney::MakeChange(Cents, Float, Pieces) || TSSMoney::Sum(Pieces) != Cents) bChangeOk = false;
		}
		Check(TEXT("the starting float can make change for any amount up to $200, to the cent"), bChangeOk);
		TArray<int32> Out;
		TSSMoney::FStock Empty;
		Check(TEXT("an empty drawer cannot make change, and says so"), !TSSMoney::MakeChange(5, Empty, Out) && Out.Num() == 0);
		TSSMoney::FStock OnlyTens; OnlyTens.Count[2] = 3;
		Check(TEXT("a drawer without the right pieces refuses rather than short-changing"), !TSSMoney::MakeChange(1500, OnlyTens, Out) && TSSMoney::MakeChange(2000, OnlyTens, Out) && Out.Num() == 2);
		for (int32 Trial = 0; Trial < 400; ++Trial)
		{
			const int32 Total = FMath::RandRange(1, 250000);
			const TArray<int32> Tender = TSSMoney::ChooseTender(Total);
			const int32 Paid = TSSMoney::Sum(Tender);
			if (Paid < Total || Paid > Total + 20000 + Total) bTenderOk = false;
			for (int32 P : Tender) if (P == 10) bTenderOk = false;   // nobody pays in dollar coins
		}
		Check(TEXT("customers always hand over enough, and never a silly overpayment"), bTenderOk);
	}

	UE_LOG(LogTemp, Display, TEXT("TSSTEST DONE fails=%d"), Fails);
}

void ATSSPlayerController::TSSTime(float Hour)
{
	if (UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>()) Biz->SetHour(Hour);
}

void ATSSPlayerController::TSSSettingsPage()
{
	ShowMenu(true);
	if (Menu) Menu->SetPage(ETSSMenuPage::Settings);
}

void ATSSPlayerController::TSSTruck()
{
	FTSSOrder Empty;
	Empty.Id = -1;
	const FTSSDockInfo Dock = ATSSDeliveryDock::Resolve(GetWorld());
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ATSSDeliveryTruck* Truck = GetWorld()->SpawnActor<ATSSDeliveryTruck>(ATSSDeliveryTruck::StaticClass(), FTransform(FRotator(0.f, 180.f, 0.f), Dock.TruckStart), Params))
	{
		Truck->Setup(Empty, Dock, 1.f);
	}
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
		int32 Rings0 = 0;
		float Hold = 0.f;      // wait before the next action (lets a screenshot happen)
		float Elapsed = 0.f;   // time in the current step
		int32 Fails = 0;
		bool BagSeen = false, StrayPanel = false;
		bool CashierShot = false;
		int32 Drawer0 = 0, TenderSum = 0;
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
	// The earlier test stages leave a customer wandering the shop; it would join the queue and put a
	// second basket on the counter during our runs. Start from an empty shop and a free register.
	for (TActorIterator<ATSSCustomer> CIt(GetWorld()); CIt; ++CIt) CIt->Destroy();
	Reg->ReleaseSale();
Reg->CardDeclineChance = 0.f; // deterministic; the decline path has its own check below
		Reg->ExpiredCardChance = 0.f;
		Reg->WrongPinChance = 0.f;
	if (UTSSSettingsSubsystem* Cfg = GetGameInstance()->GetSubsystem<UTSSSettingsSubsystem>()) Cfg->SetViewMode(ETSSViewMode::FirstPerson, false);

	auto Spawn = [this, St, CS, Store](ETSSPayMethod Method)
	{
		St->Step = 0; St->Elapsed = 0.f; St->Hold = 0.f;
St->Money0 = Store->GetMoneyCents(); St->Served0 = Store->GetServed(); St->Lost0 = Store->GetLost();
St->Cash0 = Store->GetCashSalesCents(); St->Card0 = Store->GetCardSalesCents(); St->Tax0 = Store->GetTaxCollectedCents();
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

	ATSSShopDoor* ShopDoor = nullptr;
	for (TActorIterator<ATSSShopDoor> DIt(GetWorld()); DIt; ++DIt) { ShopDoor = *DIt; break; }
	if (ShopDoor) { ShopDoor->ConsumePeakSwing(); St->Rings0 = ShopDoor->GetRingCount(); }

	Spawn(ETSSPayMethod::Cash);
	Check(TEXT("a customer spawns at the door"), St->Customer.IsValid());

	FTimerHandle* Handle = new FTimerHandle();
	GetWorldTimerManager().SetTimer(*Handle, [this, St, CS, Store, Reg, Me, Check, Spawn, Snap, Handle, ShopDoor]()
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
				if (Reg->GetPhase() != ETSSCheckoutPhase::Scanning || !Reg->AllItemsPlaced()) { if (bTimeout) Fail(TEXT("customer reached the counter and put their basket down")); return; }
				{
					if (ShopDoor && bCash)
					{
						Check(TEXT("the door swung open for the customer walking in"), ShopDoor->ConsumePeakSwing() > 30.f);
						Check(TEXT("it swung INTO the shop, away from the customer outside"), ShopDoor->GetSwingDir() > 0);
						Check(TEXT("the bell rang as the customer walked in"), ShopDoor->GetRingCount() > St->Rings0);
						Check(TEXT("and shut again behind them"), FMath::Abs(ShopDoor->GetAngle()) < 5.f && !ShopDoor->IsHeldOpen());
					}
					const TArray<TObjectPtr<ATSSProductItem>>& Items = Reg->GetItems();
					Check(TEXT("the basket is on the counter as real product items"), Items.Num() >= 1);
					St->ExpectedSub = 0;
for (ATSSProductItem* I : Items) St->ExpectedSub += I->UnitPrice * 100;   // the register counts cents

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
					Check(TEXT("monitor lines add up to the subtotal"), [&]() { int32 S = 0;for (const FTSSSaleLine& L : Reg->GetLines()) S += L.Total() * 100; return S == Reg->GetSubtotal(); }());
					St->ExpectedTotal = Reg->GetTotal();
					Snap(FVector(95.f, 70.f, 0.f), FVector(-24.f, 70.f, ATSSRegister::TopZ + 12.f));
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
Check(TEXT("the customer paid in real notes and coins that add up to what they handed over"), Reg->GetTender().Num() > 0 && TSSMoney::Sum(Reg->GetTender()) == Reg->GetCashReceived());
Check(TEXT("the notes and coins are on the counter"), Reg->GetTender().Num() > 0);
{
	TArray<int32> Pieces;
	const int32 DrawerBefore = Reg->GetDrawer()->GetCashOnHandCents();
	Check(TEXT("the drawer can make the change from the notes and coins it holds"), TSSMoney::MakeChange(Reg->GetChangeDue(), Reg->GetDrawer()->GetStock(), Pieces));
	for (int32 P : Pieces) Reg->GetDrawer()->ClickPiece(P);
	Check(TEXT("pieces clicked out of the drawer add up to the change due, to the cent"), Reg->GetChangeGiven() == Reg->GetChangeDue());
	Check(TEXT("and they have left the drawer"), Reg->GetDrawer()->GetCashOnHandCents() == DrawerBefore - Reg->GetChangeDue());
	St->Drawer0 = Reg->GetDrawer()->GetCashOnHandCents();
	St->TenderSum = Reg->GetCashReceived();
}
					Snap(FVector(115.f, -85.f, 0.f), FVector(54.f, -85.f, 90.f));
					St->Hold = 1.4f;
					St->Step = 3; St->Elapsed = 0.f;
				}
				else
				{
					if (Reg->GetPhase() != ETSSCheckoutPhase::CardReady) { if (bTimeout) Fail(TEXT("customer tapped a card and the terminal is ready")); return; }
Check(TEXT("customer chose card"), Reg->GetMethod() == ETSSPayMethod::Card);
						Check(TEXT("the customer's card is a real-looking invented card that passes the number check and is in date"),
							Reg->HasCard() && TSSCards::LuhnValid(Reg->GetCard().Number) && Reg->GetCard().Number.Len() == 16 && !Reg->GetCard().IsExpiredOn(Reg->Today()) && Reg->GetCard().Pin.Len() == 4 && !Reg->GetCard().Holder.IsEmpty());
if (St->Customer.IsValid())
						{
							UE_LOG(LogTemp, Display, TEXT("TSS card debug: shown=%d phase=%d state=%d busy=%d card=%s tap=%s"), St->Customer->IsCardShown(), static_cast<int32>(Reg->GetPhase()), static_cast<int32>(St->Customer->GetState()), St->Customer->IsBusyGesturing(), *St->Customer->GetCardLocation().ToString(), *Reg->TerminalTapPoint().ToString());
							Check(TEXT("the customer holds their card out at the terminal, where the owner can see it"), St->Customer->IsCardShown() && FVector::Dist(St->Customer->GetCardLocation(), Reg->TerminalTapPoint()) < 25.f);
						}
						Check(TEXT("small sales are tapped and large ones use chip and PIN"), Reg->IsContactlessSale() == (Reg->GetTotal() <= ATSSRegister::ContactlessLimit));
					Snap(FVector(46.f, 40.f, 0.f), FVector(3.f, 55.f, 126.f));
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
Check(TEXT("the store's money went up by exactly the total, to the cent"), Store->GetMoneyCents() == St->Money0 + St->ExpectedTotal);
					if (bCash) Check(TEXT("the customer's notes and coins went into the drawer"), Reg->GetDrawer()->GetCashOnHandCents() == St->Drawer0 + St->TenderSum);
				Check(TEXT("served counter and transaction log updated"), Store->GetServed() == St->Served0 + 1 && Store->GetTransactions().Num() == St->Txns0 + 1);
				Check(TEXT("sale recorded under the right payment method"),
					bCash ?(Store->GetCashSalesCents() == St->Cash0 + St->ExpectedTotal && Store->GetCardSalesCents() == St->Card0)
					      :(Store->GetCardSalesCents() == St->Card0 + St->ExpectedTotal && Store->GetCashSalesCents() == St->Cash0));
if (!bCash && Store->GetTransactions().Num() > 0)
					{
						const FTSSTransaction& Last = Store->GetTransactions().Last();
						Check(TEXT("a card sale records the network, last four digits and a six-digit authorisation code"), Last.CardLast4 == Reg->GetCard().Last4() && Last.CardNetwork == Reg->GetCard().Network && Last.AuthCode.Len() == 6);
					}
					Check(TEXT("tax collected is recorded"),Store->GetTaxCollectedCents() == St->Tax0 + TSSMoney::TaxFor(St->ExpectedSub));
				Check(TEXT("the receipt printer has a receipt"), Reg->GetPrinter()->HasReceipt());
				St->Hold = 2.0f; // let the receipt finish printing
				St->Step = 5; St->Elapsed = 0.f;
				break;
			case 5: // hand over the receipt; customer leaves
				Reg->GetPrinter()->Interact(Me, FHitResult());
				Check(TEXT("handing over the receipt lets the customer collect their shopping"), Reg->GetPhase() == ETSSCheckoutPhase::Collecting || Reg->GetPhase() == ETSSCheckoutPhase::Idle);
				St->Step = 6; St->Elapsed = 0.f;
				break;
			case 6:
				if (St->Customer.IsValid())
			{
				if (St->Customer->WasBagSeen()) St->BagSeen = true;
				if (St->Customer->WasStrayPanelSeen()) St->StrayPanel = true;
				if (St->Elapsed > 90.f) Fail(TEXT("the customer left the shop"));
				return;
			}
			if (St->Run == 0 || St->Run == 1) Check(TEXT("the shopping bag leaves as a plain bag, with no stray checkered cube"), St->BagSeen && !St->StrayPanel);
			St->BagSeen = St->StrayPanel = false;
				Check(TEXT("the customer walked out and is gone"), true);
				UE_LOG(LogTemp, Display, TEXT("TSS debug: phase=%d items=%d"), static_cast<int32>(Reg->GetPhase()), Reg->GetItems().Num());
				for (TActorIterator<ATSSCustomer> CIt(GetWorld()); CIt; ++CIt) UE_LOG(LogTemp, Display, TEXT("TSS debug customer %s state=%d at %s tracked=%d"), *CIt->GetName(), static_cast<int32>(CIt->GetState()), *CIt->GetActorLocation().ToString(), St->Customer.Get() == *CIt);
				for (ATSSProductItem* It : Reg->GetItems()) UE_LOG(LogTemp, Display, TEXT("TSS debug item %s at %s scanned=%d held=%d"), *GetNameSafe(It), *It->GetActorLocation().ToString(), It->bScanned, Me->GetHeldItem()==It);
				Check(TEXT("the counter was cleared and the register is free for the next sale"), Reg->GetPhase() == ETSSCheckoutPhase::Idle && Reg->GetItems().Num() == 0);
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
		else if (St->Run == 3)
		{
			if (!St->Customer.IsValid())
			{
				Check(TEXT("a customer arriving at a closed shop turns away: no sale, nothing on the counter"),
					Store->GetServed() == St->Served0 && Reg->GetPhase() == ETSSCheckoutPhase::Idle && Reg->GetItems().Num() == 0);
					Check(TEXT("and turning away is not counted as an unhappy walk-out"), Store->GetLost() == St->Lost0);
					Store->SetOpen(true);
					// Run 4: a hired cashier serves a customer start to finish with no help from the player.
					if (UTSSBusinessSubsystem* Biz4 = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>()) Biz4->SetCashierForTest(true);
					St->Run = 4;
					Spawn(ETSSPayMethod::Cash);
				}
			else if (Reg->GetPhase() != ETSSCheckoutPhase::Idle || bTimeout)
			{
				Check(TEXT("a customer arriving at a closed shop turns away: no sale, nothing on the counter"), false);
				Store->SetOpen(true);
				St->Run = 99;
			}
		}
else if (St->Run == 4)
{
if (!St->CashierShot && (Reg->GetPhase() == ETSSCheckoutPhase::CashDue || Reg->GetPhase() == ETSSCheckoutPhase::CardReady))
{
	St->CashierShot = true;
	Snap(FVector(-190.f, -20.f, 0.f), FVector(105.f, -95.f, 105.f));
}
if (!St->Customer.IsValid())
				{
					UTSSBusinessSubsystem* Biz4 = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>();
					Check(TEXT("a hired cashier completed the whole sale on their own"), Store->GetServed() == St->Served0 + 1 && Store->GetTransactions().Num() == St->Txns0 + 1);
Check(TEXT("and the money went into the till"), Store->GetMoneyCents() > St->Money0);
					Check(TEXT("the cashier left the register free for the next customer"), Reg->GetPhase() == ETSSCheckoutPhase::Idle && Reg->GetItems().Num() == 0);
					bool bBody = false;
					for (TActorIterator<ATSSEmployee> EIt(GetWorld()); EIt; ++EIt) bBody = true;
					Check(TEXT("the cashier is a person standing in the shop"), bBody);
					if (Biz4) Biz4->SetCashierForTest(false);
					St->Run = 99;
				}
				else if (St->Elapsed > 150.f)
				{
					Check(TEXT("a hired cashier completed the whole sale on their own"), false);
					if (UTSSBusinessSubsystem* Biz4 = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>()) Biz4->SetCashierForTest(false);
					St->Run = 99;
				}
			}
			else if (St->Run == 2)
			{
				if (Store->GetLost() > St->Lost0)
			{
				Check(TEXT("an overpriced shop loses the customer"), true);
				for (const TPair<ETSSProduct, int32>& KV : St->OldPrices) Store->SetPrice(KV.Key, KV.Value);
				// Run 3: the sign says CLOSED. A customer who arrives must turn away without ever reaching the counter.
				Store->SetOpen(false);
				St->Run = 3;
				Spawn(ETSSPayMethod::Cash);
			}
			else if (bTimeout) { Check(TEXT("an overpriced shop loses the customer"), false); St->Run = 99; }
		}

		if (St->Run == 99)
		{
			UE_LOG(LogTemp, Display, TEXT("TSSTEST CUSTOMERS DONE fails=%d"), St->Fails);
			GetWorldTimerManager().ClearTimer(*Handle);
			if (FParse::Param(FCommandLine::Get(), TEXT("TSSAutoTest"))) ConsoleCommand(TEXT("TSSDeskTest"));
		}
	}, 0.2f, true);
}

void ATSSPlayerController::TSSDeskTest()
{
	// StoreDesk end to end, with the real objects: open it from the computer, check the dashboard against the
	// real books, buy upgrades, order stock, receive the truck, unpack onto the new shelf, check the save.
	struct FState
	{
		int32 Step = 0;
		float Hold = 0.f;
		float Elapsed = 0.f;
		int32 Fails = 0;
		int32 OrderId = 0;
		TWeakObjectPtr<ATSSShopDoor> Door;
		int32 Rings0 = 0;
		int32 XP0 = 0;
	};
	TSharedRef<FState> St = MakeShared<FState>();
	for (TActorIterator<ATSSShopDoor> DIt(GetWorld()); DIt; ++DIt) { St->Door = *DIt; St->Rings0 = DIt->GetRingCount(); break; }

	UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>();
	ATSSCharacter* Me = Cast<ATSSCharacter>(GetPawn());
	ATSSStoreDeskComputer* Comp = nullptr;
	for (TActorIterator<ATSSStoreDeskComputer> It(GetWorld()); It; ++It) { Comp = *It; break; }

	auto Check = [St](const TCHAR* Name, bool bOk)
	{
		UE_LOG(LogTemp, Display, TEXT("TSSTEST %s %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), Name);
		if (!bOk) ++St->Fails;
	};
	auto Finish = [this, St]()
	{
		UE_LOG(LogTemp, Display, TEXT("TSSTEST STOREDESK DONE fails=%d"), St->Fails);
		if (FParse::Param(FCommandLine::Get(), TEXT("TSSAutoTest"))) ConsoleCommand(TEXT("QUIT_EDITOR"));
	};
	Check(TEXT("store, business and player exist"), Store && Biz && Me);
	Check(TEXT("the StoreDesk computer is in the world"), Comp != nullptr);
	if (!(Store && Biz && Me && Comp)) { Finish(); return; }

	TSSStart();
	auto CountShelves = [this]() { int32 N = 0; for (TActorIterator<ATSSShelf> It(GetWorld()); It; ++It) ++N; return N; };

	// ---- the physical computer opens the interface ----------------------------------------------------
	Check(TEXT("computer prompt invites you to use StoreDesk"), Comp->GetPrompt(Me, FHitResult()).Contains(TEXT("StoreDesk")));
	Comp->Interact(Me, FHitResult());
	Check(TEXT("E on the computer opens StoreDesk"), IsDeskOpen() && GetDesk() != nullptr);

	// ---- every dashboard number comes from the real books ------------------------------------------
	{
		const FTSSDashboard D = Biz->GetDashboard();
		int32 SumSub = 0, SumItems = 0;
		for (const FTSSTransaction& T : Store->GetTransactions()) { SumSub += T.Subtotal; SumItems += T.Items; }
		Check(TEXT("dashboard money = the store's money"), D.Money == Store->GetMoney());
		Check(TEXT("lifetime revenue = sum of the logged sales (and not zero)"), D.LifetimeRevenue == SumSub / 100 && Store->GetLifetimeRevenueCents() == SumSub && SumSub > 0);
		Check(TEXT("products sold = items in the logged sales"), D.ProductsSold == SumItems && SumItems > 0);
		Check(TEXT("customers served = logged sales"), D.CustomersServed == Store->GetTransactions().Num());
		Check(TEXT("today's revenue is real and within lifetime revenue"), D.TodayRevenue > 0 && D.TodayRevenue <= D.LifetimeRevenue);
		Check(TEXT("sales earned experience"), D.XP > 0 && D.Level >= 1);
		Check(TEXT("the first-sale achievement unlocked"), Biz->HasAchievement(TEXT("first_sale")));
		Check(TEXT("prestige is locked at this level"), !Biz->IsPrestigeUnlocked());
	}
	CloseStoreDesk();
	Check(TEXT("Esc / Log Off closes StoreDesk and returns control"), !IsDeskOpen() && IsGameActive());

	// ---- upgrades -------------------------------------------------------------------------------------
	Store->Earn(5000);
	FString Why;
	const float Eta0 = Biz->EtaFor(5);
	int32 M0 = Store->GetMoney();
	Check(TEXT("buying Express Supplier charges its price"), Biz->BuyUpgrade(TEXT("supplier_fast"), Why) && Store->GetMoney() == M0 - 400 && Biz->HasUpgrade(TEXT("supplier_fast")));
	Check(TEXT("Express Supplier halves the delivery time"), FMath::IsNearlyEqual(Biz->EtaFor(5), Eta0 * 0.5f, 0.01f));
	Check(TEXT("upgrade spending is recorded for the finance page"), Store->GetUpgradeSpend() >= 400);
	Check(TEXT("an owned upgrade cannot be bought twice"), !Biz->BuyUpgrade(TEXT("supplier_fast"), Why));
	Check(TEXT("a planned upgrade cannot be bought yet"), !Biz->BuyUpgrade(TEXT("storage"), Why));
	const int32 Shelves0 = CountShelves();
	M0 = Store->GetMoney();
	Check(TEXT("buying the shelf upgrade charges its price"), Biz->BuyUpgrade(TEXT("shelf_1"), Why) && Store->GetMoney() == M0 - 250);
	Check(TEXT("the shelf upgrade adds a real shelf to the shop"), CountShelves() == Shelves0 + 1);
	ATSSShelf* NewShelf = nullptr;
	for (TActorIterator<ATSSShelf> It(GetWorld()); It; ++It) if (It->ActorHasTag(TEXT("shelf_1"))) NewShelf = *It;
	Check(TEXT("the new shelf is the upgrade's shelf"), NewShelf != nullptr);
	// ---- store size: the shop starts small and expands ------------------------------------------------------------
	ATSSStoreShell* Shell = nullptr;
	for (TActorIterator<ATSSStoreShell> It(GetWorld()); It; ++It) Shell = *It;
	Check(TEXT("the shop has a StoreShell building it"), Shell != nullptr);
	Check(TEXT("the store starts at its smallest size"), Biz->GetStoreStage() == 0 && Shell && Shell->GetStage() == 0);
	Check(TEXT("the shop starts with only two shelves (plus the one just bought)"), CountShelves() == 3);
	Check(TEXT("a shelf that needs a bigger store is refused until you expand"), !Biz->BuyUpgrade(TEXT("shelf_2"), Why) && Why.Contains(TEXT("bigger store")));
	const float DeskX0 = Comp->GetActorLocation().X;
	const float ShelfY0 = NewShelf->GetActorLocation().Y;
	M0 = Store->GetMoney();
	const bool bCanExpand = Biz->GetLevel() >= 2;
	Check(TEXT("the first store expansion follows the level rule"), Biz->BuyUpgrade(TEXT("store_1"), Why) == bCanExpand);
	if (bCanExpand)
	{
		Check(TEXT("expanding charges its price and advances the store size"), Store->GetMoney() == M0 - 1200 && Biz->GetStoreStage() == 1 && Shell->GetStage() == 1);
		Check(TEXT("the back wall moves out and the desk moves with it"), FMath::IsNearlyEqual(Comp->GetActorLocation().X, DeskX0 + TSSStoreLayout::BackX(1) - TSSStoreLayout::BackX(0), 1.f));
		Check(TEXT("a right-wall shelf moves with the right wall"), FMath::IsNearlyEqual(NewShelf->GetActorLocation().Y, ShelfY0 + TSSStoreLayout::HalfWidth(1) - TSSStoreLayout::HalfWidth(0), 1.f));
		const int32 ShelvesBefore = CountShelves();
		Check(TEXT("the bigger store unlocks more shelf spots"), Biz->BuyUpgrade(TEXT("shelf_2"), Why) && CountShelves() == ShelvesBefore + 1);
	}
	if (!NewShelf) { Finish(); return; }

	// ---- ordering ---------------------------------------------------------------------------------------
	Biz->EtaScale = 0.05f;      // hurry the wholesaler and the courier for the test
	Biz->DeliverySpeed = 6.f;
	Check(TEXT("an empty cart cannot be ordered"), !Biz->PlaceOrder(TArray<FTSSOrderLine>(), Why));
	M0 = Store->GetMoney();
	Check(TEXT("an order you cannot afford is refused and charges nothing"), !Biz->PlaceOrder({ { ETSSProduct::Laptop, 500 } }, Why) && Store->GetMoney() == M0);
	const TArray<FTSSOrderLine> Lines = { { ETSSProduct::Phone, 3 }, { ETSSProduct::Keyboard, 2 } };
	const int32 Cost = Biz->OrderCost(Lines);
	Check(TEXT("order cost = wholesale price x quantity"), Cost == 3 * TSSProductDef(ETSSProduct::Phone).WholesaleCost + 2 * TSSProductDef(ETSSProduct::Keyboard).WholesaleCost);
	M0 = Store->GetMoney();
	const int32 Spend0 = Store->GetOrderSpend();
	int32 Id = 0;
	Check(TEXT("placing an order deducts exactly its cost"), Biz->PlaceOrder(Lines, Why, &Id) && Store->GetMoney() == M0 - Cost && Store->GetOrderSpend() == Spend0 + Cost);
	const FTSSOrder* O = Biz->FindOrder(Id);
	Check(TEXT("the order is recorded in transit with an arrival time"), O && O->Status == ETSSOrderStatus::InTransit && O->ArriveAt > O->PlacedAt && O->UnitsLeft == 5);
	Check(TEXT("the order counts as active"), Biz->ActiveOrderCount() >= 1);
	St->OrderId = Id;
	St->XP0 = Biz->GetXP();

	FTimerHandle* Handle = new FTimerHandle();
	GetWorldTimerManager().SetTimer(*Handle, [this, St, Store, Biz, Me, Comp, NewShelf, Check, Finish, Handle]()
	{
		const float Dt = 0.2f;
		St->Elapsed += Dt;
		if (St->Hold > 0.f) { St->Hold -= Dt; return; }
		auto Done = [&]()
		{
			GetWorldTimerManager().ClearTimer(*Handle);
			Finish();
		};
		auto Snap = [this]() { FTimerHandle T; GetWorldTimerManager().SetTimer(T, [this]() { ConsoleCommand(TEXT("Shot ShowUI")); }, 0.7f, false); };

		switch (St->Step)
		{
		case 0: // the truck arrives and the parcels are on the dock
		{
			const FTSSOrder* Ord = Biz->FindOrder(St->OrderId);
			if (!Ord || Ord->Status != ETSSOrderStatus::Arrived)
			{
				if (St->Elapsed > 60.f) { Check(TEXT("the truck delivered the order to the dock"), false); Done(); }
				return;
			}
			for (TActorIterator<ATSSDeliveryTruck> TIt(GetWorld()); TIt; ++TIt)
			{
				ATSSRoad* Rd = ATSSRoad::Find(GetWorld());
				Check(TEXT("the delivery truck is driving on the road"), Rd && TIt->IsOnRoad() && Rd->LaneError(TIt->GetActorLocation()) < 15.f);
				break;
			}
			ConsoleCommand(TEXT("TSSLook 850 200 150 0 -6")); // a look at the parked truck on the road
			Snap();
			if (St->Door.IsValid()) Check(TEXT("the bell rang as the delivery driver walked in"), St->Door->GetRingCount() > St->Rings0);
			TArray<ATSSParcel*> Parcels;
			for (TActorIterator<ATSSParcel> It(GetWorld()); It; ++It) Parcels.Add(*It);
			Check(TEXT("one parcel per order line is waiting on the dock"), Parcels.Num() == 2);

			// Unpack every unit into the hands and stock it on the new shelf.
			int32 SlotNext = 0;
			for (ATSSParcel* P : Parcels)
			{
				const ETSSProduct Product = P->GetProduct();
				const int32 Qty = P->GetQty();
				const int32 Slot = SlotNext++;
				TWeakObjectPtr<ATSSParcel> Weak(P);
				FHitResult Hit;
				static const float TierZ[ATSSShelf::Tiers] = { 12.f, 72.f, 132.f };
				Hit.ImpactPoint = NewShelf->GetActorTransform().TransformPosition(FVector(24.f, -75.f + 50.f * (Slot % ATSSShelf::Columns), TierZ[Slot / ATSSShelf::Columns] + 25.f));
				for (int32 n = 0; n < Qty && Weak.IsValid(); ++n)
				{
					Weak->Interact(Me, FHitResult());
					Check(TEXT("unpacking puts the product in the player's hands"), Me->GetHeld() == Product);
					NewShelf->Interact(Me, Hit);
				}
			}
			int32 Phones = 0, Keyboards = 0;
			for (int32 s = 0; s < ATSSShelf::Tiers * ATSSShelf::Columns; ++s)
			{
				if (NewShelf->GetSlotProduct(s) == ETSSProduct::Phone) Phones += NewShelf->GetSlotQty(s);
				if (NewShelf->GetSlotProduct(s) == ETSSProduct::Keyboard) Keyboards += NewShelf->GetSlotQty(s);
			}
			Check(TEXT("the unpacked products are on the shelf in the ordered quantities"), Phones == 3 && Keyboards == 2);
			Ord = Biz->FindOrder(St->OrderId);
			Check(TEXT("the order is received once everything is unpacked"), Ord && Ord->Status == ETSSOrderStatus::Received && Ord->UnitsLeft == 0);
			Check(TEXT("receiving a delivery earned experience"), Biz->GetXP() > St->XP0);
			Check(TEXT("the delivery achievement unlocked"), Biz->HasAchievement(TEXT("first_delivery")));

			// Emptied boxes do not vanish: they stay until the owner carries them to a bin.
			int32 Empties = 0;
			for (ATSSParcel* P : Parcels) if (IsValid(P) && P->IsEmpty()) ++Empties;
			Check(TEXT("emptied boxes stay in the shop until they are binned"), Empties == Parcels.Num() && Parcels.Num() == 2);
			ATSSBin* Bin = nullptr;
			for (TActorIterator<ATSSBin> It(GetWorld()); It; ++It) { Bin = *It; break; }
			Check(TEXT("the street has litter bins"), Bin != nullptr);
			if (Bin && Parcels.Num() == 2 && IsValid(Parcels[0]) && IsValid(Parcels[1]))
			{
				Parcels[0]->Interact(Me, FHitResult());
				Check(TEXT("picking up an empty box puts it in the player's hands"), Me->IsCarryingEmptyBox() && !IsValid(Parcels[0]));
				Check(TEXT("with the empty box in hand, another box says the hands are full"), Parcels[1]->GetPrompt(Me, FHitResult()).Contains(TEXT("hands are full")));
				Check(TEXT("the bin offers to take the box"), Bin->GetPrompt(Me, FHitResult()).Contains(TEXT("Put the empty box")));
				const int32 Before = Bin->GetBoxCount();
				Bin->Interact(Me, FHitResult());
				Check(TEXT("putting the box in the bin frees the hands and the bin counts it (it never fills)"), !Me->IsCarryingEmptyBox() && Bin->GetBoxCount() == Before + 1);
				Parcels[1]->Interact(Me, FHitResult());
				Bin->Interact(Me, FHitResult());
				Check(TEXT("a second box also goes in: no capacity limit yet"), Bin->GetBoxCount() == Before + 2);
			}

			// People: pavements have walkers, and a shopper arriving from the street starts far from the door.
			if (UTSSTownSubsystem* Town = GetWorld()->GetSubsystem<UTSSTownSubsystem>())
			{
				const int32 N0 = Town->PasserbyCount();
				ATSSPasserby* Walker = Town->SpawnPasserby(true, true, false);
				Check(TEXT("a passer-by walks the pavement"), Walker && Town->PasserbyCount() == N0 + 1);
				if (Walker) Walker->Destroy();
			}
			if (UTSSCustomerSubsystem* CS = GetWorld()->GetSubsystem<UTSSCustomerSubsystem>())
			{
				ATSSCustomer* Arriving = CS->GetSpawner() ? CS->GetSpawner()->SpawnCustomer(1.f, true) : nullptr;
				Check(TEXT("a shopper from the street starts far down the pavement, not at the door"), Arriving && FMath::Abs(Arriving->GetActorLocation().Y) > 1500.f);
				if (Arriving) Arriving->Destroy();
			}
			St->Step = 1; St->Elapsed = 0.f;
			break;
		}
		case 1: // the truck leaves, then the save is checked
		{
			bool bTruck = false;
			for (TActorIterator<ATSSDeliveryTruck> It(GetWorld()); It; ++It) bTruck = true;
			if (bTruck) { if (St->Elapsed > 30.f) { Check(TEXT("the truck drove away"), false); Done(); } return; }
			Check(TEXT("the truck drove away and the dock is clear"), true);

			Biz->SaveNow();
			UTSSBusinessSave* Save = Cast<UTSSBusinessSave>(UGameplayStatics::LoadGameFromSlot(Biz->GetSlotName(), 0));
			Check(TEXT("the business was written to a save file"), Save != nullptr);
			if (Save)
			{
				Check(TEXT("the save holds the money, revenue and customers"),Save->Money == Store->GetMoneyCents() && Save->LifetimeRevenue == Store->GetLifetimeRevenueCents() && Save->Served == Store->GetServed() && Save->ProductsSold == Store->GetProductsSold());
				Check(TEXT("the save holds XP, upgrades, orders and achievements"), Save->XP == Biz->GetXP() && Save->Upgrades.Contains(FName(TEXT("shelf_1"))) && Save->Orders.Num() == Biz->GetOrders().Num() && Save->Achievements.Contains(FName(TEXT("first_delivery"))));
				Check(TEXT("the save holds the sales log"), Save->Transactions.Num() == Store->GetTransactions().Num());
					// Shelf stock: every shelf is in the save with the quantities it really holds, and prices are saved too.
					bool bStockMatches = Save->ShelfStock.Num() > 0 && Save->Prices.Num() == static_cast<int32>(ETSSProduct::Count);
					int32 SavedUnits = 0, LiveUnits = 0;
					for (TActorIterator<ATSSShelf> ShIt(GetWorld()); ShIt; ++ShIt)
					{
						const FTSSShelfSave* Entry = Save->ShelfStock.FindByPredicate([&ShIt](const FTSSShelfSave& E) { return E.Key == ShIt->LayoutKey(); });
						if (!Entry) { bStockMatches = false; continue; }
						for (int32 s = 0; s < ATSSShelf::Tiers * ATSSShelf::Columns; ++s)
						{
							LiveUnits += ShIt->GetSlotQty(s);
							if (Entry->Slots.IsValidIndex(s))
							{
								SavedUnits += Entry->Slots[s].Qty;
								if (Entry->Slots[s].Qty != ShIt->GetSlotQty(s) || (Entry->Slots[s].Qty > 0 && Entry->Slots[s].Product != ShIt->GetSlotProduct(s))) bStockMatches = false;
							}
						}
					}
					Check(TEXT("the save holds what is on every shelf, and the shelf prices"), bStockMatches && LiveUnits > 0 && SavedUnits == LiveUnits);
					// Round trip: empty each shelf, then restore it from its exported form and compare.
					bool bRoundTrip = true;
					for (TActorIterator<ATSSShelf> ShIt(GetWorld()); ShIt; ++ShIt)
					{
						TArray<FTSSShelfSlotSave> Before, After;
						ShIt->ExportSlots(Before);
						ShIt->ImportSlots(TArray<FTSSShelfSlotSave>());
						ShIt->ExportSlots(After);
						for (const FTSSShelfSlotSave& Z : After) if (Z.Qty != 0) bRoundTrip = false;
						ShIt->ImportSlots(Before);
						After.Reset();
						ShIt->ExportSlots(After);
						for (int32 s = 0; s < Before.Num(); ++s) if (Before[s].Qty != After[s].Qty || Before[s].Product != After[s].Product) bRoundTrip = false;
					}
					Check(TEXT("shelf stock restores exactly from its saved form"), bRoundTrip);
			}
			Check(TEXT("Employees and Contracts are locked sections with a level requirement"),
				TSSDeskPages().Num() == 8 && TSSDeskPages()[4]->RequiredLevel() > 0 && TSSDeskPages()[5]->RequiredLevel() > 0
				&& TSSDeskPages()[4]->Badge(1) == TEXT("LOCKED") && TSSDeskPages()[0]->RequiredLevel() == 0);
Check(TEXT("Employees and Contracts are real sections once their level is reached"), TSSDeskPages()[4]->Badge(TSSDeskPages()[4]->RequiredLevel()).IsEmpty() && TSSDeskPages()[5]->Badge(TSSDeskPages()[5]->RequiredLevel()).IsEmpty() && TSSDeskPages()[7]->Badge(99).IsEmpty());
			// Screenshots of the interface and the computer.
			ConsoleCommand(TEXT("TSSLook 640 0 90 180 -3")); // overview of the shop (expanded, if the test could afford it)
			Snap();
			St->Hold = 1.6f; St->Step = 11; St->Elapsed = 0.f;
			break;
		}
		case 11:
			GetDesk()->SetDark(true);
			TSSDeskPage(0);
			Snap();
			St->Hold = 1.6f; St->Step = 2; St->Elapsed = 0.f;
			break;
		case 2:
			Check(TEXT("the dark-mode pill switches the theme"), TSSDeskDark() && TSSDeskTheme().PageBg.R < 0.2f);
			GetDesk()->GetContext().Cart.Add(ETSSProduct::Laptop, 2);
			GetDesk()->GetContext().Cart.Add(ETSSProduct::Headphones, 1);
			GetDesk()->GetContext().Selected = ETSSProduct::Laptop;
			TSSDeskPage(2);
			Snap();
			St->Hold = 1.6f; St->Step = 3;
			break;
		case 3:
			TSSDeskPage(3);
			Snap();
			St->Hold = 1.6f; St->Step = 4;
			break;
		case 4:
			TSSDeskPage(4); // a locked section, in dark mode
			Snap();
			St->Hold = 1.6f; St->Step = 5;
			break;
		case 5:
			GetDesk()->SetDark(false);
			Check(TEXT("switching back restores the light theme"), !TSSDeskDark() && TSSDeskTheme().PageBg.R > 0.8f);
			TSSDeskPage(6);
			Snap();
			St->Hold = 1.6f; St->Step = 6;
			break;
		case 6:
		{
			CloseStoreDesk();
			const FVector Wp = Comp->GetActorTransform().TransformPosition(FVector(135.f, -22.f, 0.f));
			const FVector Wt = Comp->GetActorTransform().TransformPosition(FVector(0.f, -22.f, 85.f));
			const FRotator R = (Wt - (Wp + FVector(0.f, 0.f, 154.f))).Rotation();
			ConsoleCommand(FString::Printf(TEXT("TSSLook %f %f 90 %f %f"), Wp.X, Wp.Y, R.Yaw, R.Pitch));
			Snap();
			St->Hold = 1.6f; St->Step = 7;
			break;
		}
			case 7: // the new upgrades, staff, deals and products, by their real functions
			{
				Store->Earn(500000);
				Biz->AddXP(FMath::Max(0, UTSSBusinessSubsystem::XPForLevel(8) - Biz->GetXP()));
				FString Msg;
				for (const TCHAR* Id : { TEXT("store_1"), TEXT("store_2"), TEXT("supplier_premium"), TEXT("equipment"), TEXT("storage"), TEXT("dept_gaming"), TEXT("employee") })
				{
					if (!Biz->HasUpgrade(Id)) Biz->BuyUpgrade(Id, Msg);
				}
				Check(TEXT("all five formerly planned upgrades can now be bought"),
					Biz->HasUpgrade(TEXT("supplier_premium")) && Biz->HasUpgrade(TEXT("equipment")) && Biz->HasUpgrade(TEXT("storage")) && Biz->HasUpgrade(TEXT("dept_gaming")) && Biz->HasUpgrade(TEXT("employee")));
				bool bAllReal = true;
				for (const FTSSUpgradeDef& U : UTSSBusinessSubsystem::UpgradeCatalog()) if (!U.bImplemented) bAllReal = false;
				Check(TEXT("no upgrade in the catalogue is marked as planned any more"), bAllReal);
				Check(TEXT("premium and gaming products are available once their upgrade is owned"),
					Biz->IsProductAvailable(ETSSProduct::GamingLaptop) && Biz->IsProductAvailable(ETSSProduct::Controller) && Biz->IsProductAvailable(ETSSProduct::RacingWheel));
				Check(TEXT("the improved computer doubles the order size"), Biz->MaxCartQty() == 100);

				// Back room: a shelf slot that sold out is refilled from storage.
				ATSSShelf* StockShelf = nullptr;
				int32 StockSlot = -1;
				for (TActorIterator<ATSSShelf> SIt(GetWorld()); SIt && !StockShelf; ++SIt)
				{
					for (int32 s = 0; s < ATSSShelf::Tiers * ATSSShelf::Columns; ++s)
					{
						if (SIt->GetSlotQty(s) > 0) { StockShelf = *SIt; StockSlot = s; break; }
					}
				}
				Check(TEXT("a stocked shelf slot exists to test the back room with"), StockShelf != nullptr);
				if (StockShelf)
				{
					const ETSSProduct P = StockShelf->GetSlotProduct(StockSlot);
					const int32 Before = Biz->GetStored(P);
					Check(TEXT("units can be sent to the back room"), Biz->StoreInBackRoom(P, 6) && Biz->GetStored(P) == Before + 6);
					ETSSProduct Took = ETSSProduct::None;
					while (StockShelf->GetSlotQty(StockSlot) > 0) StockShelf->TakeOne(StockSlot, Took);
					Check(TEXT("the slot is now empty but remembers what it held"), StockShelf->GetSlotQty(StockSlot) == 0 && StockShelf->GetSlotWanted(StockSlot) == P);
					const int32 Moved = Biz->RestockShelvesFromStorage(1);
					Check(TEXT("the back room refills a sold-out slot with its product"), Moved == 1 && StockShelf->GetSlotQty(StockSlot) == 1 && StockShelf->GetSlotProduct(StockSlot) == P && Biz->GetStored(P) == Before + 5);
					Check(TEXT("the back room refuses more than it can hold"), !Biz->StoreInBackRoom(P, UTSSBusinessSubsystem::StorageCapacity));
				}

				// Staff: hiring costs the fee and puts a person in the shop; dismissing takes them out.
				const int32 Money0 = Store->GetMoney();
				const int32 Wage0 = Store->GetWageSpend();
				Check(TEXT("a cashier can be hired once the upgrade is owned"), Biz->HireCashier(Msg) && Biz->HasCashier());
				Check(TEXT("hiring cost the fee, and it shows in the books"), Store->GetMoney() == Money0 - UTSSBusinessSubsystem::CashierHireFee && Store->GetWageSpend() == Wage0 + UTSSBusinessSubsystem::CashierHireFee);
				Check(TEXT("only one cashier can be hired"), !Biz->HireCashier(Msg));
				Biz->DismissCashier();
				Check(TEXT("a cashier can be dismissed"), !Biz->HasCashier());

				// Contracts: bought, effective, and they lapse.
				const int32 Cost0 = Biz->OrderCost({ { ETSSProduct::Phone, 10 } });
				const int32 CSpend0 = Store->GetContractSpend();
				Check(TEXT("a bulk deal can be signed"), Biz->SignContract(TEXT("bulk_deal"), Msg) && Biz->IsContractActive(TEXT("bulk_deal")));
				Check(TEXT("the bulk deal cuts the wholesale price by 15%"), Biz->OrderCost({ { ETSSProduct::Phone, 10 } }) == FMath::RoundToInt(Cost0 * 0.85f));
				Check(TEXT("the deal's cost shows in the books"), Store->GetContractSpend() > CSpend0);
				Check(TEXT("the same deal cannot be signed twice"), !Biz->SignContract(TEXT("bulk_deal"), Msg));
				Check(TEXT("a deal lasts three days counting today"), Biz->ContractDaysLeft(TEXT("bulk_deal")) == 3);
				Check(TEXT("the other deals change customers' behaviour"), Biz->SignContract(TEXT("promo"), Msg) && Biz->CustomerRateMultiplier() == 2.f && Biz->SignContract(TEXT("corporate"), Msg) && Biz->PriceToleranceMultiplier() > 1.2f);
				Store->StartDay(Store->GetDay() + 3);
				Biz->SaveNow();
				UTSSBusinessSave* Saved = Cast<UTSSBusinessSave>(UGameplayStatics::LoadGameFromSlot(Biz->GetSlotName(), 0));
				Check(TEXT("the save holds the back room, the wage and contract spending"), Saved && Saved->Storage.Num() > 0 && Saved->WageSpend == Store->GetWageSpend() && Saved->ContractSpend == Store->GetContractSpend() && Saved->Contracts.Num() == 3);
				Check(TEXT("a premium product can be ordered, and a gaming one"), [&]()
				{
					FString Why;
					int32 Id1 = 0;
					return Biz->PlaceOrder({ { ETSSProduct::Camera, 1 }, { ETSSProduct::Controller, 2 } }, Why, &Id1) && Id1 > 0;
				}());

// Photograph the six new products on a shelf, to see their packaging art.
for (TActorIterator<ATSSShelf> ShotIt(GetWorld()); ShotIt; ++ShotIt) if (ShotIt->LayoutKey() == FName(TEXT("shelf_2"))) { StockShelf = *ShotIt; break; }
if (StockShelf)
{
	TArray<FTSSShelfSlotSave> Show;
	const ETSSProduct New6[] = { ETSSProduct::GamingLaptop, ETSSProduct::VRHeadset, ETSSProduct::Camera, ETSSProduct::Controller, ETSSProduct::GamingHeadset, ETSSProduct::RacingWheel };
	for (int32 s = 0; s < ATSSShelf::Tiers * ATSSShelf::Columns; ++s)
	{
		FTSSShelfSlotSave& Z = Show.AddDefaulted_GetRef();
		if (s < 6) { Z.Product = New6[s]; Z.Qty = 3; }
	}
StockShelf->ImportSlots(Show);
CloseStoreDesk();
const FVector Wp = StockShelf->GetActorTransform().TransformPosition(FVector(120.f, 0.f, 0.f));
	const FVector Wt = StockShelf->GetActorTransform().TransformPosition(FVector(0.f, 0.f, 90.f));
	const FRotator R = (Wt - (Wp + FVector(0.f, 0.f, 154.f))).Rotation();
ConsoleCommand(FString::Printf(TEXT("TSSLook %f %f 90 %f %f"), Wp.X, Wp.Y, R.Yaw, R.Pitch));
UE_LOG(LogTemp, Display, TEXT("TSS shelf shot: shelf %s at %s yaw %.0f, camera %s"), *StockShelf->LayoutKey().ToString(), *StockShelf->GetActorLocation().ToString(), StockShelf->GetActorRotation().Yaw, *Wp.ToString());
Snap();
}
else
{
	ConsoleCommand(TEXT("TSSLook 640 0 90 180 -3"));
}
St->Hold = 2.0f; St->Step = 12; St->Elapsed = 0.f;
break;
}
case 12:
GetDesk()->SetDark(false);
TSSDeskPage(4);
Snap();
St->Hold = 1.6f; St->Step = 8; St->Elapsed = 0.f;
break;
case 8:
				TSSDeskPage(5);
				Snap();
				St->Hold = 1.6f; St->Step = 9;
				break;
			default:
				Done();
				break;
			}
		}, 0.2f, true);
	}
