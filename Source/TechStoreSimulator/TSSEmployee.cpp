#include "TSSEmployee.h"
#include "TSSHumanComponent.h"
#include "TSSRegister.h"
#include "TSSRegisterDevice.h"
#include "TSSCustomer.h"
#include "EngineUtils.h"

ATSSEmployee::ATSSEmployee()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Human = CreateDefaultSubobject<UTSSHumanComponent>(TEXT("Human"));
	Human->SetupAttachment(Root);
	Human->SetRelativeRotation(FRotator(0.f, -90.f, 0.f)); // the kit's meshes face +Y; the actor faces +X
}

void ATSSEmployee::BeginPlay()
{
	Super::BeginPlay();
	// A fixed, tidy outfit so the cashier is recognisable (identity 2; the first entry of each wardrobe list).
	Human->Build(2, UTSSHumanComponent::Tops()[0], UTSSHumanComponent::Bottoms()[0], UTSSHumanComponent::Shoes()[0]);
	for (TActorIterator<ATSSRegister> It(GetWorld()); It; ++It) { Register = *It; break; }
}

void ATSSEmployee::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ATSSRegister* Reg = Register.Get();
	if (!Reg || !Human->IsBuilt()) return;

	// Face the customers' side of the counter.
	const FRotator Face(0.f, Reg->GetActorRotation().Yaw + 180.f, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 6.f));

	FVector HandTarget = FVector::ZeroVector;
	bool bReach = false;
	switch (Reg->GetPhase())
	{
	case ETSSCheckoutPhase::Scanning:
		if (Reg->AllItemsPlaced() && Reg->GetScanner()) { HandTarget = Reg->GetScanner()->GetActorLocation() + FVector(0.f, 0.f, 12.f); bReach = true; }
		break;
	case ETSSCheckoutPhase::CashDue:
		HandTarget = Reg->CashPoint(); bReach = true;
		break;
	case ETSSCheckoutPhase::CardReady:
	case ETSSCheckoutPhase::CardProcessing:
		HandTarget = Reg->TerminalTapPoint(); bReach = true;
		break;
	case ETSSCheckoutPhase::Receipt:
		if (Reg->GetPrinter()) { HandTarget = Reg->GetPrinter()->GetActorLocation() + FVector(0.f, 0.f, 12.f); bReach = true; }
		break;
	default:
		break;
	}
	if (bReach) Human->Reach(ETSSHand::Right, HandTarget); else Human->Relax(ETSSHand::Right);

	if (ATSSCustomer* C = Reg->GetCustomer()) Human->LookAt(C->GetActorLocation() + FVector(0.f, 0.f, 70.f));
	else Human->StopLooking();
}
