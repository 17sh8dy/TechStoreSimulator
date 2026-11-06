#include "TSSDeliveryActors.h"
#include "TSSBusinessSubsystem.h"
#include "TSSCharacter.h"
#include "TSSHumanComponent.h"
#include "TSSRegister.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"

// ---- dock -----------------------------------------------------------------------------------------

ATSSDeliveryDock::ATSSDeliveryDock()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Mat = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mat"));
	Mat->SetupAttachment(Root);
	Mat->SetStaticMesh(TSSCubeMesh());
	Mat->SetRelativeLocation(FVector(0.f, 0.f, 0.5f));
	Mat->SetRelativeScale3D(FVector(1.4f, 2.4f, 0.01f));
	Mat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATSSDeliveryDock::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	TSSApplyColor(Mat, FLinearColor(0.9f, 0.7f, 0.05f));
}

FTSSDockInfo ATSSDeliveryDock::Resolve(UWorld* World)
{
	FTSSDockInfo Info;
	for (TActorIterator<ATSSRegister> It(World); It; ++It) { Info.Door = It->DoorPoint; break; }
	for (TActorIterator<ATSSDeliveryDock> It(World); It; ++It)
	{
		const FVector L = It->GetActorLocation();
		Info.Drop = FVector(L.X, L.Y, 0.f);
		Info.TruckStart = FVector(L.X + It->TruckStartOffset.X, L.Y + It->TruckStartOffset.Y, 0.f);
		Info.TruckStop = FVector(L.X + It->TruckStopOffset.X, L.Y + It->TruckStopOffset.Y, 0.f);
		break;
	}
	return Info;
}

// ---- parcel ---------------------------------------------------------------------------------------

ATSSParcel::ATSSParcel()
{
	PrimaryActorTick.bCanEverTick = true;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetStaticMesh(TSSCubeMesh());
	Body->SetRelativeScale3D(FVector(0.5f, 0.6f, 0.4f));

	Tape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tape"));
	Tape->SetupAttachment(Body);
	Tape->SetStaticMesh(TSSCubeMesh());
	Tape->SetRelativeLocation(FVector(0.f, 0.f, 0.505f));
	Tape->SetRelativeScale3D(FVector(0.2f, 1.01f, 0.01f));
	Tape->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Body);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 1.5f));
	Label->SetRelativeScale3D(FVector(1.f / 0.5f, 1.f / 0.6f, 1.f / 0.4f));
	Label->SetWorldSize(8.f);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetTextRenderColor(FColor::White);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATSSParcel::Init(int32 InOrderId, ETSSProduct InProduct, int32 InQty)
{
	OrderId = InOrderId;
	Product = InProduct;
	Qty = InQty;
	TSSApplyColor(Body, FLinearColor(0.62f, 0.45f, 0.26f));
	TSSApplyColor(Tape, FLinearColor(0.85f, 0.75f, 0.55f));
	UpdateLabel();
}

void ATSSParcel::UpdateLabel()
{
	Label->SetText(FText::FromString(FString::Printf(TEXT("ORDER #%d\n%s x%d"), OrderId, TSSProductDef(Product).Name, Qty)));
}

void ATSSParcel::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		FRotator Face = (Cam->GetCameraLocation() - Label->GetComponentLocation()).Rotation();
		Face.Pitch = 0.f;
		Label->SetWorldRotation(Face);
	}
}

FString ATSSParcel::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	if (Player->GetHeld() != ETSSProduct::None || Player->GetHeldItem()) return TEXT("Your hands are full");
	return FString::Printf(TEXT("[E] Unpack a %s  (%d left in this box)"), TSSProductDef(Product).Name, Qty);
}

void ATSSParcel::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	if (Qty <= 0 || Player->GetHeld() != ETSSProduct::None || Player->GetHeldItem()) return;
	Player->SetHeld(Product);
	--Qty;
	if (UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>()) Biz->OnUnitUnloaded(OrderId);
	if (Qty <= 0) Destroy();
	else UpdateLabel();
}

// ---- truck ----------------------------------------------------------------------------------------

ATSSDeliveryTruck::ATSSDeliveryTruck()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Local +X is the front. Cargo box, cab, wind-screen, stripe and four wheels.
	Box(TEXT("Cargo"), FVector(-60.f, 0.f, 170.f), FVector(420.f, 215.f, 200.f), FLinearColor(0.9f, 0.9f, 0.92f));
	Box(TEXT("Cab"), FVector(195.f, 0.f, 135.f), FVector(130.f, 205.f, 130.f), FLinearColor(0.08f, 0.45f, 0.9f));
	Box(TEXT("Glass"), FVector(261.f, 0.f, 160.f), FVector(4.f, 170.f, 55.f), FLinearColor(0.05f, 0.1f, 0.15f));
	Box(TEXT("Stripe"), FVector(-60.f, 0.f, 120.f), FVector(422.f, 217.f, 28.f), FLinearColor(0.08f, 0.45f, 0.9f));
	Box(TEXT("Bumper"), FVector(263.f, 0.f, 62.f), FVector(10.f, 210.f, 24.f), FLinearColor(0.2f, 0.2f, 0.22f));
	const float WheelX[] = { 195.f, -150.f };
	int32 W = 0;
	for (float X : WheelX)
	{
		for (float Y : { -100.f, 100.f })
		{
			Box(*FString::Printf(TEXT("Wheel%d"), W++), FVector(X, Y, 42.f), FVector(84.f, 84.f, 26.f), FLinearColor(0.04f, 0.04f, 0.05f), FRotator(0.f, 0.f, 90.f), true);
		}
	}

	CourierRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CourierRoot"));
	CourierRoot->SetupAttachment(Root);
	CourierRoot->SetUsingAbsoluteLocation(true);
	CourierRoot->SetUsingAbsoluteRotation(true);
	Courier = CreateDefaultSubobject<UTSSHumanComponent>(TEXT("Courier"));
	Courier->SetupAttachment(CourierRoot);
	Courier->SetRelativeRotation(FRotator(0.f, -90.f, 0.f)); // the kit's meshes face +Y

	CarriedBox = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarriedBox"));
	CarriedBox->SetupAttachment(CourierRoot);
	CarriedBox->SetStaticMesh(TSSCubeMesh());
	CarriedBox->SetRelativeLocation(FVector(38.f, 0.f, 105.f));
	CarriedBox->SetRelativeScale3D(FVector(0.4f, 0.55f, 0.4f));
	CarriedBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CarriedBox->SetVisibility(false);
}

UStaticMeshComponent* ATSSDeliveryTruck::Box(const TCHAR* Name, FVector Loc, FVector Size, FLinearColor Color, FRotator Rot, bool bCylinder)
{
	UStaticMeshComponent* P = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	P->SetupAttachment(GetRootComponent());
	if (bCylinder)
	{
		static UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		P->SetStaticMesh(Cyl);
	}
	else
	{
		P->SetStaticMesh(TSSCubeMesh());
	}
	P->SetRelativeLocation(Loc);
	P->SetRelativeRotation(Rot);
	P->SetRelativeScale3D(Size / 100.f);
	P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Parts.Add(P);
	PartColors.Add(Color);
	return P;
}

void ATSSDeliveryTruck::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	for (int32 i = 0; i < Parts.Num() && i < PartColors.Num(); ++i) if (Parts[i]) TSSApplyColor(Parts[i], PartColors[i]);
}

void ATSSDeliveryTruck::Setup(const FTSSOrder& InOrder, const FTSSDockInfo& InDock, float InSpeedScale)
{
	Order = InOrder;
	Dock = InDock;
	SpeedScale = FMath::Max(0.1f, InSpeedScale);

	// A courier from the kit, in a work shirt.
	Courier->Build(1, TEXT("buttonOpen"), TEXT("jeansLong"), TEXT("oxfords"));
	Courier->SetShown(false);

	// Courier route: out of the passenger side, in through the shop door, to the drop point; then back the same way.
	const FVector Side = Dock.TruckStop + FVector(-20.f, Dock.Door.Y < Dock.TruckStop.Y ? -150.f : 150.f, 0.f);
	Route = { Side, FVector(Dock.Door.X + 180.f, Dock.Door.Y, 0.f), Dock.Door, FVector(Dock.Door.X - 120.f, Dock.Door.Y, 0.f), Dock.Drop + FVector(70.f, 0.f, 0.f) };
}

bool ATSSDeliveryTruck::WalkTo(const FVector& Target, float Dt)
{
	const FVector Pos = CourierRoot->GetComponentLocation();
	const FVector Goal(Target.X, Target.Y, 0.f);
	const FVector Delta = Goal - Pos;
	const float Dist = Delta.Size2D();
	const float Step = 150.f * SpeedScale * Dt;
	if (Dist <= Step)
	{
		CourierRoot->SetWorldLocation(Goal);
		return true;
	}
	CourierRoot->SetWorldLocation(Pos + Delta.GetSafeNormal2D() * Step);
	const FRotator Want(0.f, Delta.Rotation().Yaw, 0.f);
	CourierRoot->SetWorldRotation(FMath::RInterpConstantTo(CourierRoot->GetComponentRotation(), Want, Dt, 540.f));
	return false;
}

void ATSSDeliveryTruck::DropParcels()
{
	bDropped = true;
	CarriedBox->SetVisibility(false);
	int32 Slot = 0;
	for (const FTSSOrderLine& L : Order.Lines)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector At = Dock.Drop + FVector(0.f, 70.f * (Slot - (Order.Lines.Num() - 1) * 0.5f), 20.f);
		if (ATSSParcel* Parcel = GetWorld()->SpawnActor<ATSSParcel>(ATSSParcel::StaticClass(), FTransform(At), P))
		{
			Parcel->Init(Order.Id, L.Product, L.Qty);
		}
		++Slot;
	}
	if (UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>()) Biz->OnParcelsDelivered(Order.Id);
}

void ATSSDeliveryTruck::Tick(float Dt)
{
	Super::Tick(Dt);
	const FVector Loc = GetActorLocation();

	switch (Phase)
	{
	case EPhase::Driving:
	{
		const FVector Goal(Dock.TruckStop.X, Dock.TruckStop.Y, Loc.Z);
		const float Step = 900.f * SpeedScale * Dt;
		// Ease to a stop over the last stretch.
		const float Dist = FVector::Dist2D(Loc, Goal);
		const float Speed = FMath::Clamp(Dist * 2.5f * Dt, 6.f, Step);
		if (Dist <= Speed)
		{
			SetActorLocation(Goal);
			Phase = EPhase::ToDoor;
			RouteIndex = 0;
			CourierRoot->SetWorldLocation(Route[0]);
			Courier->SetShown(true);
			CarriedBox->SetVisibility(true);
		}
		else
		{
			SetActorLocation(Loc + (Goal - Loc).GetSafeNormal2D() * Speed);
		}
		break;
	}
	case EPhase::ToDoor:
		Courier->SetWalking(true);
		if (WalkTo(Route[RouteIndex], Dt) && ++RouteIndex >= Route.Num())
		{
			Phase = EPhase::Dropping;
		}
		break;
	case EPhase::Dropping:
		Courier->SetWalking(false);
		if (!bDropped) DropParcels();
		Phase = EPhase::Returning;
		RouteIndex = Route.Num() - 2;
		break;
	case EPhase::Returning:
		Courier->SetWalking(true);
		if (WalkTo(Route[RouteIndex], Dt) && --RouteIndex < 0)
		{
			Courier->SetShown(false);
			Courier->SetWalking(false);
			Phase = EPhase::Leaving;
		}
		break;
	case EPhase::Leaving:
	{
		const float Step = 900.f * SpeedScale * Dt;
		SetActorLocation(Loc + FVector(Step, 0.f, 0.f));
		if (GetActorLocation().X > Dock.TruckStart.X)
		{
			if (UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>()) Biz->OnTruckFinished();
			Destroy();
		}
		break;
	}
	}
}
