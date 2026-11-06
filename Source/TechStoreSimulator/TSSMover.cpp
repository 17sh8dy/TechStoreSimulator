#include "TSSMover.h"
#include "TSSBusinessSubsystem.h"
#include "TSSCharacter.h"
#include "TSSCustomer.h"
#include "TSSInteractable.h"
#include "TSSPlayerController.h"
#include "TSSProducts.h"
#include "TSSRegister.h"
#include "TSSShopDoor.h"
#include "TSSStoreLayout.h"
#include "TSSStoreShell.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"

UTSSMoverComponent::UTSSMoverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;   // driven by the character
}

ATSSInteractable* UTSSMoverComponent::GetMoving() const { return Moving.Get(); }

// ---- hold E ---------------------------------------------------------------------------------------------

void UTSSMoverComponent::HoldStep(ATSSCharacter* Who, ATSSInteractable* Target, const FHitResult& Hit, float Dt, bool bDown, bool bPressed)
{
	if (!Target) { bArmed = false; Hold = 0.f; return; }
	if (bPressed) { HoldTarget = Target; Hold = 0.f; bArmed = true; }
	if (!bArmed) return;
	if (HoldTarget.Get() != Target) { bArmed = false; Hold = 0.f; return; }   // looked away

	if (bDown)
	{
		Hold += Dt;
		if (Hold >= HoldSeconds) { bArmed = false; Hold = 0.f; BeginMove(Who, Target); }
	}
	else
	{
		// Let go early: it was an ordinary press (stock the shelf, open StoreDesk...).
		if (Hold < TapSeconds)
		{
			++Taps;
			if (Who) Who->InteractWith(Target, Hit);
		}
		bArmed = false;
		Hold = 0.f;
	}
}

bool UTSSMoverComponent::Update(ATSSPlayerController* PC, ATSSCharacter* Who, ATSSInteractable* Focus, const FHitResult& Hit, float Dt)
{
	if (Moving.IsValid())
	{
		if (!Mover.IsValid()) Mover = Who;
		if (PC->WasInputKeyJustPressed(EKeys::Q)) { Cancel(); return false; }

		float NewYaw = Yaw;
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp)) NewYaw += 15.f;
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown)) NewYaw -= 15.f;
		if (PC->WasInputKeyJustPressed(EKeys::R)) NewYaw += 90.f;

		// The object goes where the crosshair meets the floor, within arm's reach.
		const FVector Eye = Who->GetActorLocation() + FVector(0.f, 0.f, 64.f);
		const FVector Dir = PC->GetControlRotation().Vector();
		FVector Spot = Who->GetActorLocation() + Who->GetActorForwardVector() * 200.f;
		if (Dir.Z < -0.05f) Spot = Eye + Dir * (-Eye.Z / Dir.Z);
		FVector Flat = Spot - Who->GetActorLocation();
		Flat.Z = 0.f;
		const float Len = FMath::Clamp(Flat.Size(), 90.f, 420.f);
		Flat = Flat.GetSafeNormal() * Len;
		FVector Pos = Who->GetActorLocation() + Flat;
		Pos.X = FMath::GridSnap(Pos.X, 5.f);
		Pos.Y = FMath::GridSnap(Pos.Y, 5.f);
		SetPlacement(Pos, NewYaw);

		if (PC->WasInputKeyJustPressed(EKeys::E)) Commit();
		return true;
	}

	if (Focus && Focus->IsMovable())
	{
		HoldStep(Who, Focus, Hit, Dt, PC->IsInputKeyDown(EKeys::E), PC->WasInputKeyJustPressed(EKeys::E));
	}
	else
	{
		bArmed = false;
		Hold = 0.f;
	}
	return Moving.IsValid();
}

// ---- moving ---------------------------------------------------------------------------------------------

void UTSSMoverComponent::BeginMove(ATSSCharacter* Who, ATSSInteractable* Target)
{
	if (!Target || !Target->IsMovable()) return;
	Moving = Target;
	Mover = Who;
	Origin = Target->GetActorTransform();
	Yaw = Origin.Rotator().Yaw;
	++MovesStarted;
	bValid = true;
	InvalidReason.Reset();
	UpdateMarker();
}

void UTSSMoverComponent::SetPlacement(const FVector& Pos, float YawDeg)
{
	ATSSInteractable* Obj = Moving.Get();
	if (!Obj) return;
	Yaw = FRotator::NormalizeAxis(YawDeg);
	Obj->SetActorLocationAndRotation(FVector(Pos.X, Pos.Y, Origin.GetLocation().Z), FRotator(0.f, Yaw, 0.f));
	bValid = CheckPlacement(Obj, Mover.Get());
	UpdateMarker();
}

bool UTSSMoverComponent::CheckPlacement(const AActor* Obj, const ATSSCharacter* Who)
{
	InvalidReason.Reset();
	UWorld* World = GetWorld();
	if (!Obj || !World) return false;

	FVector Centre, Extent;
	Obj->GetActorBounds(false, Centre, Extent);
	const FVector Min = Centre - Extent, Max = Centre + Extent;

	int32 Stage = 0;
	for (TActorIterator<ATSSStoreShell> It(World); It; ++It) { Stage = It->GetStage(); break; }
	const float Back = TSSStoreLayout::BackX(Stage), Half = TSSStoreLayout::HalfWidth(Stage);
	if (Min.X < Back + 3.f || Max.X > TSSStoreLayout::FrontX - 3.f || Min.Y < -Half + 3.f || Max.Y > Half - 3.f)
	{
		InvalidReason = TEXT("It has to stay inside the shop");
		return false;
	}
	// Customers and deliveries walk from the door straight in to the middle of the shop.
	if (Max.X > 330.f && Min.Y < 80.f && Max.Y > -80.f)
	{
		InvalidReason = TEXT("Keep the doorway and the path from the door clear");
		return false;
	}
	if (Who)
	{
		const FVector P = Who->GetActorLocation();
		const double Dx = FMath::Max3(Min.X - P.X, 0.0, P.X - Max.X), Dy = FMath::Max3(Min.Y - P.Y, 0.0, P.Y - Max.Y);
		if (Dx * Dx + Dy * Dy < 44.f * 44.f) { InvalidReason = TEXT("You are standing in the way"); return false; }
	}
	for (TActorIterator<ATSSRegister> It(World); It; ++It)
	{
		for (int32 i = 0; i < 3; ++i)
		{
			const FVector Q = It->QueueSpot(i);
			if (Q.X > Min.X - 45.f && Q.X < Max.X + 45.f && Q.Y > Min.Y - 45.f && Q.Y < Max.Y + 45.f)
			{
				InvalidReason = TEXT("Customers queue here");
				return false;
			}
		}
	}
	for (TActorIterator<ATSSInteractable> It(World); It; ++It)
	{
		const ATSSInteractable* Other = *It;
		if (Other == Obj || Cast<ATSSCustomer>(Other) || Cast<ATSSShopDoor>(Other)) continue;
		FVector C, E;
		Other->GetActorBounds(false, C, E);
		if (E.X < 1.f && E.Y < 1.f) continue;
		if (Min.X < C.X + E.X + 3.f && Max.X > C.X - E.X - 3.f && Min.Y < C.Y + E.Y + 3.f && Max.Y > C.Y - E.Y - 3.f)
		{
			InvalidReason = TEXT("Something is already there");
			return false;
		}
	}
	return true;
}

bool UTSSMoverComponent::Commit()
{
	ATSSInteractable* Obj = Moving.Get();
	if (!Obj) return false;
	if (!bValid) return false;
	if (UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>()) Biz->FinishMove(Obj);
	else Obj->OnMoved();
	Moving.Reset();
	HideMarker();
	return true;
}

void UTSSMoverComponent::Cancel()
{
	if (ATSSInteractable* Obj = Moving.Get()) Obj->SetActorTransform(Origin);
	Moving.Reset();
	bValid = false;
	HideMarker();
}

// ---- feedback -------------------------------------------------------------------------------------------

FString UTSSMoverComponent::GetPrompt() const
{
	if (const ATSSInteractable* Obj = Moving.Get())
	{
		FString S = FString::Printf(TEXT("MOVING THE %s\n[Look] aim    [Wheel / R] rotate    [E] put down    [Q] cancel"), *Obj->GetMoveName().ToUpper());
		if (!bValid) S += FString::Printf(TEXT("\nCan't put it down here: %s"), *InvalidReason);
		return S;
	}
	if (bArmed && Hold > TapSeconds)
	{
		return FString::Printf(TEXT("Hold E to pick it up...  %d%%"), FMath::RoundToInt(GetHoldFraction() * 100.f));
	}
	return FString();
}

void UTSSMoverComponent::UpdateMarker()
{
	ATSSInteractable* Obj = Moving.Get();
	if (!Obj) return;
	if (!Marker)
	{
		Marker = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None, RF_Transient);
		Marker->SetStaticMesh(TSSCubeMesh());
		Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Marker->SetCastShadow(false);
		Marker->SetUsingAbsoluteLocation(true);
		Marker->SetUsingAbsoluteRotation(true);
		Marker->SetUsingAbsoluteScale(true);
		Marker->SetupAttachment(GetOwner()->GetRootComponent());
		Marker->RegisterComponent();
	}
	FVector C, E;
	Obj->GetActorBounds(false, C, E);
	Marker->SetWorldLocation(FVector(C.X, C.Y, 1.5f));
	Marker->SetWorldRotation(FRotator::ZeroRotator);
	Marker->SetWorldScale3D(FVector((E.X * 2.f + 14.f) / 100.f, (E.Y * 2.f + 14.f) / 100.f, 0.01f));
	TSSApplyColor(Marker, bValid ? FLinearColor(0.1f, 0.85f, 0.25f) : FLinearColor(0.9f, 0.1f, 0.1f));
	Marker->SetVisibility(true);
}

void UTSSMoverComponent::HideMarker()
{
	if (Marker) Marker->SetVisibility(false);
}
