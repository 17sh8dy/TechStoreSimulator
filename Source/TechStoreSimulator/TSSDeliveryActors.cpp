#include "TSSDeliveryActors.h"
#include "TSSBusinessSubsystem.h"
#include "TSSCharacter.h"
#include "TSSHumanComponent.h"
#include "TSSRegister.h"
#include "TSSRoad.h"
#include "TSSCustomer.h"
#include "TSSShopDoor.h"
#include "TSSStoreLayout.h"
#include "TSSStoreShell.h"
#include "Algo/Sort.h"
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
	FString Out = FString::Printf(TEXT("[E / Click] Unpack a %s  (%d left in this box)"), TSSProductDef(Product).Name, Qty);
	if (const UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>())
	{
		if (Biz->HasUpgrade(TEXT("storage"))) Out += FString::Printf(TEXT("\n[Mouse Wheel] Send the box to the back room  (%d / %d stored)"), Biz->GetStoredTotal(), UTSSBusinessSubsystem::StorageCapacity);
	}
	return Out;
}

void ATSSParcel::Adjust(ATSSCharacter* Player, const FHitResult& Hit, int32 Direction)
{
	UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>();
	if (!Biz || Qty <= 0 || !Biz->StoreInBackRoom(Product, Qty)) return;
	const int32 Moved = Qty;
	Qty = 0;
	for (int32 i = 0; i < Moved; ++i) Biz->OnUnitUnloaded(OrderId);
	Destroy();
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

// ---- parcel stacking ------------------------------------------------------------------------------------

namespace
{
	constexpr float DlvCellX = 55.f;      // cell spacing away from the front wall
	constexpr float DlvCellY = 65.f;      // cell spacing along the side wall
	constexpr float DlvHalfX = 28.f, DlvHalfY = 33.f;   // a parcel is 50 x 60 cm (plus a little air)

	/** Is a parcel-sized spot at (X, Y) taken by furniture? */
	bool DlvBlocked(UWorld* World, float X, float Y)
	{
		for (TActorIterator<ATSSInteractable> It(World); It; ++It)
		{
			const ATSSInteractable* A = *It;
			if (Cast<ATSSParcel>(A) || Cast<ATSSCustomer>(A) || Cast<ATSSShopDoor>(A)) continue;
			FVector C, E;
			A->GetActorBounds(false, C, E);
			if (E.X < 1.f && E.Y < 1.f) continue;
			if (X - DlvHalfX < C.X + E.X && X + DlvHalfX > C.X - E.X && Y - DlvHalfY < C.Y + E.Y && Y + DlvHalfY > C.Y - E.Y) return true;
		}
		return false;
	}

	/** Usable stack positions, nearest the dock corner first. */
	TArray<FVector2D> DlvCells(UWorld* World)
	{
		const FVector Drop = ATSSDeliveryDock::Resolve(World).Drop;
		int32 Stage = 0;
		for (TActorIterator<ATSSStoreShell> It(World); It; ++It) { Stage = It->GetStage(); break; }
		const float Back = TSSStoreLayout::BackX(Stage), Half = TSSStoreLayout::HalfWidth(Stage);

		TArray<FVector2D> Cells;
		for (int32 Sum = 0; Sum < 9; ++Sum)
		{
			for (int32 R = 0; R <= Sum; ++R)
			{
				const int32 C = Sum - R;
				const float X = Drop.X - DlvCellX * R;
				const float Y = Drop.Y - DlvCellY * C;
				if (X - DlvHalfX < Back + 5.f || X + DlvHalfX > TSSStoreLayout::FrontX - 5.f) continue;
				if (Y - DlvHalfY < -Half + 5.f || Y + DlvHalfY > Half - 5.f) continue;
				if (X > 330.f && FMath::Abs(Y) < 95.f) continue;   // the walkway from the door
				if (DlvBlocked(World, X, Y)) continue;
				Cells.Add(FVector2D(X, Y));
			}
		}
		return Cells;
	}
}

FVector ATSSParcel::NextStackSlot(UWorld* World)
{
	const FVector Drop = ATSSDeliveryDock::Resolve(World).Drop;
	const TArray<FVector2D> Cells = DlvCells(World);
	if (Cells.Num() == 0) return FVector(Drop.X, Drop.Y, 20.f);

	TArray<int32> Height;
	Height.Init(0, Cells.Num());
	for (TActorIterator<ATSSParcel> It(World); It; ++It)
	{
		const FVector L = It->GetActorLocation();
		for (int32 i = 0; i < Cells.Num(); ++i)
		{
			if (FMath::Abs(L.X - Cells[i].X) < 20.f && FMath::Abs(L.Y - Cells[i].Y) < 20.f) { ++Height[i]; break; }
		}
	}
	// Fill one layer across the nearest cells before starting the next layer up.
	for (int32 Level = 0; Level < MaxStack; ++Level)
	{
		for (int32 i = 0; i < Cells.Num(); ++i)
		{
			if (Height[i] == Level) return FVector(Cells[i].X, Cells[i].Y, StackStep * 0.5f + StackStep * Level);
		}
	}
	return FVector(Cells[0].X, Cells[0].Y, StackStep * 0.5f + StackStep * Height[0]);   // absolutely full: keep going up
}

void ATSSParcel::SettleStacks(UWorld* World)
{
	TMap<FIntPoint, TArray<ATSSParcel*>> Columns;
	for (TActorIterator<ATSSParcel> It(World); It; ++It)
	{
		const FVector L = It->GetActorLocation();
		Columns.FindOrAdd(FIntPoint(FMath::RoundToInt(L.X / 4.f), FMath::RoundToInt(L.Y / 4.f))).Add(*It);
	}
	for (TPair<FIntPoint, TArray<ATSSParcel*>>& KV : Columns)
	{
		TArray<ATSSParcel*>& Stack = KV.Value;
		Algo::Sort(Stack, [](const ATSSParcel* A, const ATSSParcel* B) { return A->GetActorLocation().Z < B->GetActorLocation().Z; });
		for (int32 i = 0; i < Stack.Num(); ++i)
		{
			FVector L = Stack[i]->GetActorLocation();
			L.Z = StackStep * 0.5f + StackStep * i;
			Stack[i]->SetActorLocation(L);
		}
	}
}

void ATSSParcel::EndPlay(const EEndPlayReason::Type Reason)
{
	// An unpacked parcel leaves a gap: let the ones above it drop down once this one is really gone.
	if (Reason == EEndPlayReason::Destroyed)
	{
		if (UWorld* World = GetWorld())
		{
			TWeakObjectPtr<UWorld> Weak(World);
			World->GetTimerManager().SetTimerForNextTick([Weak]() { if (Weak.IsValid()) ATSSParcel::SettleStacks(Weak.Get()); });
		}
	}
	Super::EndPlay(Reason);
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

	// If the map has a road, the truck drives along it: start at its beginning, park beside the shop.
	Road = ATSSRoad::Find(GetWorld());
	if (Road.IsValid())
	{
		ParkDistance = FMath::Clamp(Road->DistanceNearest(Dock.TruckStop), 0.f, Road->GetLength());
		RoadDistance = 0.f;
		PlaceOnRoad();
	}
}

void ATSSDeliveryTruck::PlaceOnRoad()
{
	SetActorLocationAndRotation(Road->LaneLocation(RoadDistance), Road->LaneRotation(RoadDistance));
}

bool ATSSDeliveryTruck::GetCourierLocation(FVector& Out) const
{
	if (Phase != EPhase::ToDoor && Phase != EPhase::Dropping && Phase != EPhase::Returning) return false;
	Out = CourierRoot->GetComponentLocation();
	return true;
}

void ATSSDeliveryTruck::BuildRoute()
{
	// Courier route: out of the side of the truck that faces the shop, in through the door, to the drop point; then back.
	const FVector Here = GetActorLocation();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, GetActorForwardVector().GetSafeNormal2D());
	const float Sign = FVector::DotProduct(Dock.Door - Here, Right) >= 0.f ? 1.f : -1.f;
	const FVector Side = FVector(Here.X, Here.Y, 0.f) + Right * Sign * 150.f;
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
		const FVector At = ATSSParcel::NextStackSlot(GetWorld());   // stacked in the corner, never spread over the floor
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
		if (Road.IsValid())
		{
			// Follow the road's lane, easing to a stop beside the shop.
			const float Remaining = ParkDistance - RoadDistance;
			const float Speed = FMath::Clamp(Remaining * 2.5f * Dt, 6.f, 900.f * SpeedScale * Dt);
			if (Remaining <= Speed)
			{
				RoadDistance = ParkDistance;
				PlaceOnRoad();
				BuildRoute();
				Phase = EPhase::ToDoor;
				RouteIndex = 0;
				CourierRoot->SetWorldLocation(Route[0]);
				Courier->SetShown(true);
				CarriedBox->SetVisibility(true);
			}
			else
			{
				RoadDistance += Speed;
				PlaceOnRoad();
			}
			break;
		}
		const FVector Goal(Dock.TruckStop.X, Dock.TruckStop.Y, Loc.Z);
		const float Step = 900.f * SpeedScale * Dt;
		// Ease to a stop over the last stretch.
		const float Dist = FVector::Dist2D(Loc, Goal);
		const float Speed = FMath::Clamp(Dist * 2.5f * Dt, 6.f, Step);
		if (Dist <= Speed)
		{
			SetActorLocation(Goal);
			BuildRoute();
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
		if (Road.IsValid())
		{
			// Carry on down the road and out of sight.
			RoadDistance += Step;
			if (RoadDistance >= Road->GetLength())
			{
				if (UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>()) Biz->OnTruckFinished();
				Destroy();
			}
			else
			{
				PlaceOnRoad();
			}
			break;
		}
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
