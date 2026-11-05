#include "TSSRegisterDevice.h"
#include "TSSRegister.h"
#include "TSSCharacter.h"
#include "TSSCheckoutScreens.h"
#include "TSSProducts.h"
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
	constexpr float StackPitch = 9.2f;
	float StackY(int32 Index) { return -(Index - 2.5f) * StackPitch; } // tray-local; $1 on the owner's left

	const FLinearColor BillColors[TSSMoney::NumDenoms] = {
		FLinearColor(0.62f, 0.78f, 0.62f), FLinearColor(0.5f, 0.66f, 0.85f), FLinearColor(0.88f, 0.76f, 0.45f),
		FLinearColor(0.45f, 0.8f, 0.5f), FLinearColor(0.88f, 0.55f, 0.65f), FLinearColor(0.72f, 0.85f, 0.72f) };
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
	for (int32 i = 0; i < TSSMoney::NumDenoms; ++i)
	{
		const FString Idx = FString::FromInt(i);
		Box(*(TEXT("Stack") + Idx), FVector(4.f, StackY(i), 90.5f), FVector(15.f, 8.f, 4.f), BillColors[i], FRotator::ZeroRotator, Tray);
		// Flat label on the front lip of each stack, readable from the owner's side.
		Label(*(TEXT("Denom") + Idx), FVector(-8.f, StackY(i), 93.f), FRotator(90.f, 0.f, 0.f), 4.5f, FColor(20, 40, 20), Tray)
			->SetText(FText::FromString(FString::Printf(TEXT("$%d"), TSSMoney::Denoms[i])));
	}
	CashLabel = Label(TEXT("CashLabel"), FVector(26.5f, 0.f, 89.f), FRotator::ZeroRotator, 3.f, FColor(190, 255, 200), Tray);
}

void ATSSCashDrawer::UpdateLabel()
{
	CashLabel->SetText(FText::FromString(FString::Printf(TEXT("TILL  $%d"), CashOnHand)));
}

void ATSSCashDrawer::AddCash(int32 Amount)
{
	CashOnHand += Amount;
	UpdateLabel();
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

int32 ATSSCashDrawer::StackAt(const FHitResult& Hit) const
{
	const FVector Local = Tray->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint);
	int32 Best = 0;
	float BestD = TNumericLimits<float>::Max();
	for (int32 i = 0; i < TSSMoney::NumDenoms; ++i)
	{
		const float D = FMath::Abs(Local.Y - StackY(i));
		if (D < BestD) { BestD = D; Best = i; }
	}
	return Best;
}

FString ATSSCashDrawer::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	const ATSSRegister* R = GetRegister();
	if (!R) return FString();
	if (R->GetPhase() != ETSSCheckoutPhase::CashDue || !IsOpen()) return FString::Printf(TEXT("Cash drawer\nTill: $%d"), CashOnHand);
	return FString::Printf(TEXT("[E] Add a $%d bill to the change   |   [Wheel] take the last one back\nGiving $%d of $%d due"),
		TSSMoney::Denoms[StackAt(Hit)], R->GetChangeGiven(), R->GetChangeDue());
}

void ATSSCashDrawer::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	ATSSRegister* R = GetRegister();
	if (!R || !IsOpen() || R->GetPhase() != ETSSCheckoutPhase::CashDue) return;
	R->AddChange(TSSMoney::Denoms[StackAt(Hit)]);
}

void ATSSCashDrawer::Adjust(ATSSCharacter* Player, const FHitResult& Hit, int32 Direction)
{
	if (ATSSRegister* R = GetRegister()) R->RemoveLastChange();
}

void ATSSCashDrawer::ClickDenom(int32 Denom)
{
	if (ATSSRegister* R = GetRegister()) if (IsOpen()) R->AddChange(Denom);
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

void ATSSCardTerminal::Tick(float Dt)
{
	Super::Tick(Dt);
	const float Target = bCardWanted ? 1.f : 0.f;
	if (FMath::IsNearlyEqual(CardAlpha, Target)) return;
	CardAlpha = FMath::FInterpConstantTo(CardAlpha, Target, Dt, 1.4f);
	Card->SetVisibility(CardAlpha > 0.01f);
	Card->SetRelativeLocation(FMath::Lerp(CardFrom, CardTo, FMath::InterpEaseOut(0.f, 1.f, CardAlpha, 2.f)));
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
	constexpr float PaperLength = 26.f;
}

ATSSReceiptPrinter::ATSSReceiptPrinter()
{
	PrimaryActorTick.bCanEverTick = true;
	Box(TEXT("Body"), PrinterPos + FVector(0.f, 0.f, 6.f), FVector(14.f, 16.f, 12.f), DevDark);
	Box(TEXT("Slot"), PrinterPos + FVector(0.f, 0.f, 12.2f), FVector(2.f, 9.f, 0.6f), FLinearColor(0.f, 0.f, 0.f));
	Box(TEXT("Led"), PrinterPos + FVector(7.2f, 5.f, 8.f), FVector(0.6f, 1.5f, 1.5f), FLinearColor(0.1f, 0.9f, 0.3f));
	Paper = Box(TEXT("Paper"), PrinterPos + FVector(0.f, 0.f, 12.f), FVector(0.3f, 8.f, 0.1f), FLinearColor(0.97f, 0.97f, 0.94f));
	Paper->SetVisibility(false);
	PaperText = Label(TEXT("PaperText"), PrinterPos + FVector(0.5f, 0.f, 12.f + PaperLength * 0.5f), FRotator::ZeroRotator, 1.5f, FColor(20, 20, 20));
	PaperText->SetVisibility(false);
}

void ATSSReceiptPrinter::Print(const FTSSTransaction& Sale)
{
	FString Text = TEXT("TECH STORE\n----------\n");
	Text += FString::Printf(TEXT("%d item%s\n"), Sale.Items, Sale.Items == 1 ? TEXT("") : TEXT("s"));
	Text += FString::Printf(TEXT("Subtotal $%d\nTax $%d\nTOTAL $%d\n"), Sale.Subtotal, Sale.Tax, Sale.Total);
	Text += Sale.Method == ETSSPayMethod::Cash ? FString::Printf(TEXT("CASH $%d\nChange $%d"), Sale.CashReceived, Sale.Change) : FString(TEXT("CARD"));
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
	Paper->SetRelativeScale3D(FVector(0.003f, 0.08f, L / 100.f));
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
