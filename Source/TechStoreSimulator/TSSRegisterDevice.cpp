#include "TSSRegisterDevice.h"
#include "TSSRegister.h"
#include "TSSCharacter.h"
#include "TSSCheckoutScreens.h"
#include "TSSProducts.h"
#include "TSSMoneyVisual.h"
#include "TSSBusinessSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"

namespace
{
	constexpr float DevTop = ATSSRegister::TopZ;
	const FLinearColor DevDark(0.06f, 0.06f, 0.08f);
	const FLinearColor DevSteel(0.55f, 0.57f, 0.6f);

	void ConfigureScreen(UWidgetComponent* W, TSubclassOf<UUserWidget> Class, FIntPoint Pixels, float WidthCm)
	{
		W->SetWidgetClass(Class);
		W->SetDrawSize(FVector2D(Pixels.X, Pixels.Y));
		W->SetRelativeScale3D(FVector(1.f, WidthCm / Pixels.X, WidthCm / Pixels.X));
		W->SetPivot(FVector2D(0.5f, 0.5f));
		W->SetWidgetSpace(EWidgetSpace::World);
		W->SetBlendMode(EWidgetBlendMode::Opaque);
		W->SetTwoSided(false);
		W->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

// ---- base -----------------------------------------------------------------------------------------

ATSSRegisterDevice::ATSSRegisterDevice()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

UStaticMeshComponent* ATSSRegisterDevice::Box(const TCHAR* Name, FVector Loc, FVector Size, FLinearColor Color, FRotator Rot, USceneComponent* Parent)
{
	UStaticMeshComponent* P = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	P->SetupAttachment(Parent ? Parent : GetRootComponent());
	P->SetStaticMesh(TSSCubeMesh());
	P->SetRelativeLocation(Loc);
	P->SetRelativeRotation(Rot);
	P->SetRelativeScale3D(Size / 100.f);
	Parts.Add(P);
	PartColors.Add(Color);
	return P;
}

UTextRenderComponent* ATSSRegisterDevice::Label(const TCHAR* Name, FVector Loc, FRotator Rot, float WorldSize, FColor Color, USceneComponent* Parent)
{
	UTextRenderComponent* T = CreateDefaultSubobject<UTextRenderComponent>(Name);
	T->SetupAttachment(Parent ? Parent : GetRootComponent());
	T->SetRelativeLocation(Loc);
	T->SetRelativeRotation(Rot);
	T->SetWorldSize(WorldSize);
	T->SetHorizontalAlignment(EHTA_Center);
	T->SetVerticalAlignment(EVRTA_TextCenter);
	T->SetTextRenderColor(Color);
	T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return T;
}

void ATSSRegisterDevice::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	for (int32 i = 0; i < Parts.Num() && i < PartColors.Num(); ++i) if (Parts[i]) TSSApplyColor(Parts[i], PartColors[i]);
}

ATSSRegister* ATSSRegisterDevice::GetRegister() const
{
	return Cast<ATSSRegister>(GetParentActor());
}

// ---- monitor --------------------------------------------------------------------------------------

ATSSRegisterMonitor::ATSSRegisterMonitor()
{
	Box(TEXT("Foot"), FVector(18.f, -85.f, DevTop + 1.f), FVector(16.f, 24.f, 2.f), DevDark);
	Box(TEXT("Neck"), FVector(18.f, -85.f, DevTop + 8.f), FVector(4.f, 6.f, 12.f), DevDark);
	Box(TEXT("Bezel"), FVector(17.f, -85.f, DevTop + 27.f), FVector(3.f, 44.f, 31.f), DevDark, FRotator(10.f, 0.f, 0.f));

	Screen = CreateDefaultSubobject<UWidgetComponent>(TEXT("Screen"));
	Screen->SetupAttachment(GetRootComponent());
	// The screen faces the owner (+X) and tilts up slightly, matching the bezel.
	Screen->SetRelativeLocation(FVector(18.6f, -85.f, DevTop + 27.f + 0.3f));
	Screen->SetRelativeRotation(FRotator(10.f, 0.f, 0.f));
	ConfigureScreen(Screen, UTSSMonitorWidget::StaticClass(), FIntPoint(560, 380), 40.f);
}

void ATSSRegisterMonitor::BeginPlay()
{
	Super::BeginPlay();
	Screen->InitWidget();
	if (UTSSMonitorWidget* W = Cast<UTSSMonitorWidget>(Screen->GetUserWidgetObject())) W->SetRegister(GetRegister());
}

// ---- keyboard -------------------------------------------------------------------------------------

ATSSRegisterKeyboard::ATSSRegisterKeyboard()
{
	Box(TEXT("Base"), FVector(31.f, -85.f, DevTop + 0.8f), FVector(13.f, 34.f, 1.6f), DevDark);
	Box(TEXT("Keys"), FVector(31.f, -85.f, DevTop + 1.9f), FVector(10.5f, 30.f, 0.8f), FLinearColor(0.4f, 0.42f, 0.46f));
	Box(TEXT("Enter"), FVector(31.f, -73.f, DevTop + 2.3f), FVector(5.f, 7.f, 0.8f), FLinearColor(0.1f, 0.5f, 0.9f));
}

FString ATSSRegisterKeyboard::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	const ATSSRegister* R = GetRegister();
	return R ? R->KeyboardPrompt() : FString();
}

void ATSSRegisterKeyboard::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	if (ATSSRegister* R = GetRegister()) R->KeyboardPress();
}

// ---- scanner --------------------------------------------------------------------------------------

ATSSScanner::ATSSScanner()
{
	PrimaryActorTick.bCanEverTick = true;
	Box(TEXT("Pad"), FVector(-8.f, -5.f, DevTop + 1.25f), FVector(28.f, 30.f, 2.5f), DevDark);
	Glass = Box(TEXT("Glass"), FVector(-8.f, -5.f, DevTop + 2.7f), FVector(22.f, 24.f, 0.5f), FLinearColor(0.7f, 0.05f, 0.05f));
}

void ATSSScanner::Beep(bool bOk)
{
	bFlashOk = bOk;
	FlashTimer = 0.4f;
	bGlassIsIdle = false;
	TSSApplyColor(Glass, bOk ? FLinearColor(0.1f, 1.f, 0.35f) : FLinearColor(1.f, 0.2f, 0.1f));
}

void ATSSScanner::Tick(float Dt)
{
	Super::Tick(Dt);
	if (FlashTimer > 0.f)
	{
		FlashTimer -= Dt;
		if (FlashTimer <= 0.f && !bGlassIsIdle)
		{
			bGlassIsIdle = true;
			TSSApplyColor(Glass, FLinearColor(0.7f, 0.05f, 0.05f));
		}
	}
}

FString ATSSScanner::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	const ATSSRegister* R = GetRegister();
	return R ? R->ScannerPrompt(Player) : FString();
}

void ATSSScanner::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	if (ATSSRegister* R = GetRegister()) R->TryScanHeld(Player);
}

// ---- cash drawer ----------------------------------------------------------------------------------

namespace
{
	constexpr float DrawerY = -85.f;
	constexpr float DrawerClosedX = 8.f;
	constexpr float DrawerTravel = 46.f;
	// Tray-local layout: notes in the back row, coins in the front row (the owner stands on the +X side).
	constexpr float NoteRowX = -9.f;
	constexpr float CoinRowX = 13.f;
	constexpr float NotePitch = 9.2f;
	constexpr float CoinPitch = 10.5f;
	float SlotY(int32 Piece)
	{
		return Piece < TSSMoney::NumBills ? -(Piece - 2.5f) * NotePitch : -(Piece - TSSMoney::NumBills - 2.f) * CoinPitch;
	}
	float SlotX(int32 Piece) { return Piece < TSSMoney::NumBills ? NoteRowX : CoinRowX; }
}

ATSSCashDrawer::ATSSCashDrawer()
{
	PrimaryActorTick.bCanEverTick = true;
	Tray = CreateDefaultSubobject<USceneComponent>(TEXT("Tray"));
	Tray->SetupAttachment(GetRootComponent());
	Tray->SetRelativeLocation(FVector(DrawerClosedX, DrawerY, 0.f));

	Box(TEXT("TrayFloor"), FVector(0.f, 0.f, 86.f), FVector(50.f, 60.f, 3.f), DevSteel, FRotator::ZeroRotator, Tray);
	Box(TEXT("TrayFront"), FVector(25.f, 0.f, 89.f), FVector(2.f, 60.f, 9.f), FLinearColor(0.15f, 0.15f, 0.18f), FRotator::ZeroRotator, Tray);
	Box(TEXT("TrayBack"), FVector(-25.f, 0.f, 89.f), FVector(2.f, 60.f, 9.f), DevSteel, FRotator::ZeroRotator, Tray);
	Box(TEXT("TraySideL"), FVector(0.f, 30.f, 89.f), FVector(50.f, 2.f, 9.f), DevSteel, FRotator::ZeroRotator, Tray);
	Box(TEXT("TraySideR"), FVector(0.f, -30.f, 89.f), FVector(50.f, 2.f, 9.f), DevSteel, FRotator::ZeroRotator, Tray);
	// Dividers between the slots.
	Box(TEXT("RowDivider"), FVector(2.f, 0.f, 88.f), FVector(1.f, 58.f, 5.f), DevSteel, FRotator::ZeroRotator, Tray);

	for (int32 i = 0; i < TSSMoney::NumDenoms; ++i)
	{
		const FString Idx = FString::FromInt(i);
		const bool bCoin = TSSMoney::Denom(i).bCoin;
		// What shows in the slot: the top note of the pile, or the top coin of the stack.
		UStaticMeshComponent* Top = CreateDefaultSubobject<UStaticMeshComponent>(*(TEXT("Top") + Idx));
		Top->SetupAttachment(Tray);
		Top->SetStaticMesh(bCoin ? TSSMoneyArt::CylinderMesh() : TSSCubeMesh());
		Top->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Top->SetCastShadow(false);
		Top->SetRelativeLocation(FVector(SlotX(i), SlotY(i), 90.f));
		Tops.Add(Top);

		UTextRenderComponent* L = Label(*(TEXT("Denom") + Idx), FVector(bCoin ? CoinRowX + 7.5f : NoteRowX + 9.f, SlotY(i), 91.5f), FRotator(90.f, 0.f, 0.f), 2.6f, FColor(235, 245, 235), Tray);
		SlotLabels.Add(L);
	}
	CashLabel = Label(TEXT("CashLabel"), FVector(26.5f, 0.f, 89.f), FRotator::ZeroRotator, 3.f, FColor(190, 255, 200), Tray);
	Stock = TSSMoney::StartingFloat();
}

void ATSSCashDrawer::BeginPlay()
{
	Super::BeginPlay();
	// Notes and coins use the packaging material for their art; give every slot its art now that the world exists.
	Refresh();
}

void ATSSCashDrawer::Refresh()
{
	for (int32 i = 0; i < TSSMoney::NumDenoms; ++i)
	{
		const TSSMoney::FDenom& Def = TSSMoney::Denom(i);
		UStaticMeshComponent* Top = Tops[i];
		const int32 N = Stock.Count[i];
		Top->SetMaterial(0, TSSMoneyArt::Material(i));
		if (Def.bCoin)
		{
			const float Dia = 4.2f;
			const float H = FMath::Clamp(N * 0.12f, 0.4f, 3.5f);
			Top->SetRelativeScale3D(FVector(Dia / 100.f, Dia / 100.f, H / 100.f));
			Top->SetRelativeLocation(FVector(SlotX(i), SlotY(i), 87.5f + H * 0.5f));
		}
		else
		{
			// Notes are shown well under life size in the drawer (about 0.6x): a little pile of that denomination.
			const float H = FMath::Clamp(N * 0.1f, 0.3f, 3.f);
			Top->SetRelativeScale3D(FVector(9.2f / 100.f, 4.0f / 100.f * 1.0f, H / 100.f));
			Top->SetRelativeLocation(FVector(SlotX(i), SlotY(i), 87.5f + H * 0.5f));
		}
		Top->SetVisibility(N > 0);
		SlotLabels[i]->SetText(FText::FromString(FString::Printf(TEXT("%s\nx%d"), Def.Short, N)));
		SlotLabels[i]->SetTextRenderColor(N > 0 ? FColor(235, 245, 235) : FColor(255, 120, 100));
	}
	CashLabel->SetText(FText::FromString(FString::Printf(TEXT("TILL  %s"), *TSSMoney::Format(Stock.TotalCents()))));
}

bool ATSSCashDrawer::TakePiece(int32 Piece)
{
	if (Piece < 0 || Piece >= TSSMoney::NumDenoms || Stock.Count[Piece] <= 0) return false;
	--Stock.Count[Piece];
	Refresh();
	return true;
}

void ATSSCashDrawer::ReturnPiece(int32 Piece)
{
	if (Piece < 0 || Piece >= TSSMoney::NumDenoms) return;
	++Stock.Count[Piece];
	Refresh();
}

void ATSSCashDrawer::Receive(const TArray<int32>& Pieces)
{
	for (int32 P : Pieces) if (P >= 0 && P < TSSMoney::NumDenoms) ++Stock.Count[P];
	Refresh();
}

void ATSSCashDrawer::RefillFloat()
{
	const TSSMoney::FStock Float = TSSMoney::StartingFloat();
	for (int32 i = 0; i < TSSMoney::NumDenoms; ++i) Stock.Count[i] = FMath::Max(Stock.Count[i], Float.Count[i]);
	Refresh();
}

void ATSSCashDrawer::Tick(float Dt)
{
	Super::Tick(Dt);
	const float Target = bWantOpen ? 1.f : 0.f;
	if (FMath::IsNearlyEqual(OpenAlpha, Target)) return;
	OpenAlpha = FMath::FInterpConstantTo(OpenAlpha, Target, Dt, 3.f);
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, OpenAlpha, 2.f);
	Tray->SetRelativeLocation(FVector(DrawerClosedX + DrawerTravel * Eased, DrawerY, 0.f));
}

int32 ATSSCashDrawer::SlotAt(const FHitResult& Hit) const
{
	const FVector Local = Tray->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint);
	const bool bCoins = Local.X > 2.f;
	const int32 From = bCoins ? TSSMoney::NumBills : 0;
	const int32 To = bCoins ? TSSMoney::NumDenoms : TSSMoney::NumBills;
	int32 Best = From;
	float BestD = TNumericLimits<float>::Max();
	for (int32 i = From; i < To; ++i)
	{
		const float D = FMath::Abs(Local.Y - SlotY(i));
		if (D < BestD) { BestD = D; Best = i; }
	}
	return Best;
}

FString ATSSCashDrawer::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	const ATSSRegister* R = GetRegister();
	if (!R) return FString();
	if (R->GetPhase() != ETSSCheckoutPhase::CashDue || !IsOpen())
		return FString::Printf(TEXT("Cash drawer\nTill: %s"), *TSSMoney::Format(Stock.TotalCents()));
	const int32 Slot = SlotAt(Hit);
	const TSSMoney::FDenom& Def = TSSMoney::Denom(Slot);
	if (Stock.Count[Slot] <= 0) return FString::Printf(TEXT("Out of %s\nGiving %s of %s due"), Def.Name, *TSSMoney::Format(R->GetChangeGiven()), *TSSMoney::Format(R->GetChangeDue()));
	return FString::Printf(TEXT("[E / Click] Add a %s to the change   |   [Wheel] take the last one back\nGiving %s of %s due"),
		Def.Name, *TSSMoney::Format(R->GetChangeGiven()), *TSSMoney::Format(R->GetChangeDue()));
}

void ATSSCashDrawer::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	ATSSRegister* R = GetRegister();
	if (!R || !IsOpen() || R->GetPhase() != ETSSCheckoutPhase::CashDue) return;
	R->AddChange(SlotAt(Hit));
}

void ATSSCashDrawer::Adjust(ATSSCharacter* Player, const FHitResult& Hit, int32 Direction)
{
	if (ATSSRegister* R = GetRegister()) R->RemoveLastChange();
}

void ATSSCashDrawer::ClickPiece(int32 Piece)
{
	if (ATSSRegister* R = GetRegister()) if (IsOpen()) R->AddChange(Piece);
}

// ---- card terminal --------------------------------------------------------------------------------

namespace
{
	const FVector TermPos(-2.f, 55.f, DevTop);
	const FVector CardFrom(-52.f, 55.f, DevTop + 26.f);
	const FVector CardTo(3.f, 55.f, DevTop + 27.f);
}

ATSSCardTerminal::ATSSCardTerminal()
{
	PrimaryActorTick.bCanEverTick = true;
	Box(TEXT("Foot"), TermPos + FVector(0.f, 0.f, 1.f), FVector(14.f, 18.f, 2.f), DevDark);
	Box(TEXT("Post"), TermPos + FVector(-1.f, 0.f, 6.f), FVector(4.f, 5.f, 8.f), DevDark);
	Box(TEXT("Body"), TermPos + FVector(0.f, 0.f, 17.f), FVector(12.f, 11.f, 18.f), DevDark, FRotator(25.f, 0.f, 0.f));
	Card = Box(TEXT("Card"), CardFrom, FVector(8.5f, 5.4f, 0.3f), FLinearColor(0.1f, 0.3f, 0.8f));
	Card->SetVisibility(false);

	Screen = CreateDefaultSubobject<UWidgetComponent>(TEXT("Screen"));
	Screen->SetupAttachment(GetRootComponent());
	Screen->SetRelativeLocation(TermPos + FVector(5.9f, 0.f, 17.f + 2.7f));
	Screen->SetRelativeRotation(FRotator(25.f, 0.f, 0.f));
	ConfigureScreen(Screen, UTSSTerminalWidget::StaticClass(), FIntPoint(220, 300), 10.f);
}

void ATSSCardTerminal::BeginPlay()
{
	Super::BeginPlay();
	Screen->InitWidget();
	if (UTSSTerminalWidget* W = Cast<UTSSTerminalWidget>(Screen->GetUserWidgetObject())) W->SetRegister(GetRegister());
}

FVector ATSSCardTerminal::GetTapPoint() const
{
	return GetActorTransform().TransformPosition(CardTo + FVector(0.f, 0.f, 3.f));
}

void ATSSCardTerminal::Tick(float Dt)
{
	Super::Tick(Dt);
	// The customer's own hand brings the card to the terminal now; the old flying-card animation is retired.
}

FString ATSSCardTerminal::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	const ATSSRegister* R = GetRegister();
	return R ? R->TerminalPrompt() : FString();
}

void ATSSCardTerminal::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	if (ATSSRegister* R = GetRegister()) R->ProcessCard();
}

// ---- receipt printer ------------------------------------------------------------------------------

namespace
{
	const FVector PrinterPos(28.f, -122.f, DevTop);
constexpr float PaperLength = 30.f;
	constexpr float PaperWidth = 15.f;
}

ATSSReceiptPrinter::ATSSReceiptPrinter()
{
	PrimaryActorTick.bCanEverTick = true;
	Box(TEXT("Body"), PrinterPos + FVector(0.f, 0.f, 6.f), FVector(14.f, 16.f, 12.f), DevDark);
	Box(TEXT("Slot"), PrinterPos + FVector(0.f, 0.f, 12.2f), FVector(2.f, 9.f, 0.6f), FLinearColor(0.f, 0.f, 0.f));
	Box(TEXT("Led"), PrinterPos + FVector(7.2f, 5.f, 8.f), FVector(0.6f, 1.5f, 1.5f), FLinearColor(0.1f, 0.9f, 0.3f));
Paper = Box(TEXT("Paper"), PrinterPos + FVector(0.f, 0.f, 12.f), FVector(0.3f, PaperWidth, 0.1f), FLinearColor(0.97f, 0.97f, 0.94f));
	Paper->SetVisibility(false);
	PaperText = Label(TEXT("PaperText"), PrinterPos + FVector(0.5f, 0.f, 12.f + PaperLength * 0.5f),FRotator::ZeroRotator, 0.9f, FColor(20, 20, 20));
	PaperText->SetVisibility(false);
}

void ATSSReceiptPrinter::Print(const FTSSTransaction& Sale, const TArray<FTSSSaleLine>& Lines)
{
	auto Money = [](int32 Cents) { return TSSMoney::Format(Cents); };
	auto Pad = [](const FString& Left, const FString& Right) { return Left + TEXT("   ") + Right; };
	FString Text = TEXT("TECH STORE\n100 Commerce Way\nHarbor City\n--------------------\n");
	Text += FString::Printf(TEXT("%s  %s\nTrans #%d\n--------------------\n"), *UTSSBusinessSubsystem::FormatDate(Sale.Day), *UTSSBusinessSubsystem::FormatClock(Sale.TimeMinutes / 60.f), Sale.Id);
	for (const FTSSSaleLine& L : Lines) Text += Pad(FString::Printf(TEXT("%dx %s"), L.Qty, TSSProductDef(L.Product).Name), Money(L.Total() * 100)) + TEXT("\n");
	Text += TEXT("--------------------\n");
	Text += Pad(TEXT("Subtotal"), Money(Sale.Subtotal)) + TEXT("\n");
	Text += Pad(FString::Printf(TEXT("Tax %.2f%%"), TSSMoney::TaxRate * 100.f), Money(Sale.Tax)) + TEXT("\n");
	Text += Pad(TEXT("TOTAL"), Money(Sale.Total)) + TEXT("\n--------------------\n");
	if (Sale.Method == ETSSPayMethod::Cash)
	{
		Text += Pad(TEXT("CASH"), Money(Sale.CashReceived)) + TEXT("\n");
		Text += Pad(TEXT("CHANGE"), Money(Sale.Change)) + TEXT("\n");
	}
	else
	{
		Text += FString::Printf(TEXT("%s DEBIT  ****%s\nAUTH %s\nAPPROVED  %s\n"), *Sale.CardNetwork, *Sale.CardLast4, *Sale.AuthCode, *Money(Sale.Total));
	}
	Text += TEXT("\nTHANK YOU!\nKeep this receipt for returns");
	PaperText->SetText(FText::FromString(Text));
	bPrinting = true;
	PrintAlpha = 0.f;
	Paper->SetVisibility(true);
	PaperText->SetVisibility(true);
}

void ATSSReceiptPrinter::Tear()
{
	bPrinting = false;
	PrintAlpha = 0.f;
	Paper->SetVisibility(false);
	PaperText->SetVisibility(false);
}

void ATSSReceiptPrinter::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!bPrinting || PrintAlpha >= 1.f) return;
	PrintAlpha = FMath::Min(1.f, PrintAlpha + Dt / 1.6f);
	// The paper grows upward out of the slot.
	const float L = FMath::Max(0.1f, PaperLength * PrintAlpha);
Paper->SetRelativeScale3D(FVector(0.003f, PaperWidth / 100.f, L / 100.f));
	Paper->SetRelativeLocation(PrinterPos + FVector(0.f, 0.f, 12.f + L * 0.5f));
	PaperText->SetRelativeLocation(PrinterPos + FVector(0.5f, 0.f, 12.f + L * 0.5f));
}

FString ATSSReceiptPrinter::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	const ATSSRegister* R = GetRegister();
	return R ? R->PrinterPrompt() : FString();
}

void ATSSReceiptPrinter::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	if (ATSSRegister* R = GetRegister()) R->TakeReceipt();
}
