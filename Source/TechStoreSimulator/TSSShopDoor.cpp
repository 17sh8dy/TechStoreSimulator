#include "TSSShopDoor.h"
#include "TSSCharacter.h"
#include "TSSCustomer.h"
#include "TSSDeliveryActors.h"
#include "TSSProducts.h"
#include "TSSStoreLayout.h"
#include "TSSStoreSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

namespace
{
	// All in the door actor's frame: +X is outdoors, x = 0 is the inside face of the front wall (20 cm thick).
	constexpr float DoorHalfGapCm = TSSStoreLayout::DoorHalfGap;
	constexpr float WallThick = 20.f;
	constexpr float LeafThick = 4.f;
	constexpr float JambThick = 5.f;
	constexpr float OpeningHalf = DoorHalfGapCm - JambThick;      // half the clear opening between the jambs
	constexpr float LeafWidth = 2.f * OpeningHalf - 3.f;           // 1.5 cm of clearance either side
	constexpr float LeafBottom = 1.f;
	constexpr float HeadBottom = 235.f;                            // underside of the frame head (the lintel above is at 240)
	constexpr float LeafHeight = HeadBottom - LeafBottom - 1.f;    // 1 cm under the head
	constexpr float LeafX = WallThick * 0.5f;          // the door sits in the middle of the wall's thickness
	constexpr float HingeY = -(OpeningHalf - 1.5f);    // the hinge is on the -Y jamb
	constexpr float OpenDegrees = 100.f;               // how far the door swings, either way
	constexpr float SwingSpeed = 150.f;                // degrees per second
	constexpr float NpcOpenRadius = 150.f;
	constexpr float WatchHeight = 150.f;

	const FLinearColor LeafColor(0.10f, 0.13f, 0.22f);
	const FLinearColor GlassColor(0.42f, 0.58f, 0.68f);
	const FLinearColor MetalColor(0.78f, 0.79f, 0.82f);
	const FLinearColor FrameColor(0.06f, 0.07f, 0.1f);
	const FLinearColor OpenPlate(0.04f, 0.46f, 0.16f);
	const FLinearColor ClosedPlate(0.62f, 0.06f, 0.06f);
}

UStaticMeshComponent* ATSSShopDoor::Part(const TCHAR* Name, USceneComponent* Parent, FVector Loc, FVector Size, FLinearColor Color, bool bCollide, bool bCylinder)
{
	UStaticMeshComponent* P = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	P->SetupAttachment(Parent);
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
	P->SetRelativeScale3D(Size / 100.f);
	if (!bCollide) P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Parts.Add(P);
	PartColors.Add(Color);
	return P;
}

ATSSShopDoor::ATSSShopDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Frame: two jambs and a head, filling the 120 cm opening.
	const float JambY = DoorHalfGapCm - JambThick * 0.5f;
	Part(TEXT("JambLeft"), Root, FVector(LeafX, -JambY, 120.f), FVector(WallThick + 2.f, JambThick, 240.f), FrameColor, true);
	Part(TEXT("JambRight"), Root, FVector(LeafX, JambY, 120.f), FVector(WallThick + 2.f, JambThick, 240.f), FrameColor, true);
	Part(TEXT("Head"), Root, FVector(LeafX, 0.f, HeadBottom + JambThick * 0.5f), FVector(WallThick + 2.f, 2.f * DoorHalfGapCm, JambThick), FrameColor, true);
	// A sill closes the small gap under the door.
	Part(TEXT("Sill"), Root, FVector(LeafX, 0.f, 1.f), FVector(WallThick + 2.f, 2.f * OpeningHalf, 2.f), FrameColor, false);

	// The door turns about this point; everything on the door is a child of the leaf.
	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(Root);
	Hinge->SetRelativeLocation(FVector(LeafX, HingeY, 0.f));

	Leaf = Part(TEXT("Leaf"), Hinge, FVector(0.f, LeafWidth * 0.5f, LeafBottom + LeafHeight * 0.5f), FVector(LeafThick, LeafWidth, LeafHeight), LeafColor, true);
	// Everything on the door hangs off the hinge (not the leaf), so it is not squashed by the leaf's scale.
	auto OnLeaf = [this](const TCHAR* Name, FVector Loc, FVector Size, FLinearColor Color, bool bCylinder = false)
	{
		return Part(Name, Hinge, Loc, Size, Color, false, bCylinder);
	};
	const float Zc = LeafBottom; // heights below are from the floor
	OnLeaf(TEXT("WindowFrame"), FVector(0.f, LeafWidth * 0.5f, Zc + 160.f), FVector(LeafThick + 0.6f, 72.f, 74.f), FrameColor);
	OnLeaf(TEXT("Window"), FVector(0.f, LeafWidth * 0.5f, Zc + 160.f), FVector(LeafThick + 1.2f, 66.f, 68.f), GlassColor);

	// Handles on both faces, near the latch edge: a vertical bar on two posts.
	const float HandleY = LeafWidth - 14.f;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float S = Side == 0 ? 1.f : -1.f;
		const FString N = Side == 0 ? TEXT("Out") : TEXT("In");
		OnLeaf(*(TEXT("HandlePostTop") + N), FVector(S * (LeafThick * 0.5f + 2.5f), HandleY, Zc + 112.f), FVector(5.f, 2.5f, 2.5f), MetalColor);
		OnLeaf(*(TEXT("HandlePostBottom") + N), FVector(S * (LeafThick * 0.5f + 2.5f), HandleY, Zc + 88.f), FVector(5.f, 2.5f, 2.5f), MetalColor);
		OnLeaf(*(TEXT("HandleBar") + N), FVector(S * (LeafThick * 0.5f + 5.f), HandleY, Zc + 100.f), FVector(2.8f, 2.8f, 30.f), MetalColor, true);
	}
	OnLeaf(TEXT("Latch"), FVector(0.f, LeafWidth + 0.5f, Zc + 100.f), FVector(LeafThick - 1.f, 2.f, 10.f), MetalColor);

	// The sign: a plate through the door, so it reads from outside and from inside. Flip it with E.
	SignPlate = OnLeaf(TEXT("SignPlate"), FVector(0.f, LeafWidth * 0.42f, Zc + 112.f), FVector(LeafThick + 3.f, 34.f, 20.f), OpenPlate);
	SignPlate->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SignPlate->SetCollisionResponseToAllChannels(ECR_Ignore);
	SignPlate->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // only the player's look-trace sees it

	auto MakeText = [this](const TCHAR* Name, float XSign) -> UTextRenderComponent*
	{
		UTextRenderComponent* T = CreateDefaultSubobject<UTextRenderComponent>(Name);
		T->SetupAttachment(Hinge);
		T->SetRelativeLocation(FVector(XSign * ((LeafThick + 3.f) * 0.5f + 0.25f), LeafWidth * 0.42f, 2.f + 112.f));
		T->SetRelativeRotation(FRotator(0.f, XSign > 0.f ? 0.f : 180.f, 0.f));
		T->SetWorldSize(9.f);
		T->SetHorizontalAlignment(EHTA_Center);
		T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetTextRenderColor(FColor::White);
		T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		T->SetText(FText::FromString(TEXT("OPEN")));
		return T;
	};
	SignOutside = MakeText(TEXT("SignOutside"), 1.f);
	SignInside = MakeText(TEXT("SignInside"), -1.f);

	// The shop bell: a small brass bell hung on a bracket inside, above the door. It swings when it rings.
	Part(TEXT("BellBracket"), Root, FVector(-2.f, 0.f, 262.f), FVector(4.f, 26.f, 3.f), MetalColor);
	BellPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BellPivot"));
	BellPivot->SetupAttachment(Root);
	BellPivot->SetRelativeLocation(FVector(-5.f, 0.f, 260.f));
	Part(TEXT("BellSpring"), BellPivot, FVector(0.f, 0.f, -4.f), FVector(1.2f, 1.2f, 8.f), MetalColor, false, true);
	Part(TEXT("BellBody"), BellPivot, FVector(0.f, 0.f, -13.f), FVector(11.f, 11.f, 10.f), FLinearColor(0.78f, 0.58f, 0.14f), false, true);
	Part(TEXT("BellRim"), BellPivot, FVector(0.f, 0.f, -18.5f), FVector(13.5f, 13.5f, 1.6f), FLinearColor(0.7f, 0.5f, 0.1f), false, true);
}

void ATSSShopDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	for (int32 i = 0; i < Parts.Num() && i < PartColors.Num(); ++i)
	{
		if (Parts[i]) TSSApplyColor(Parts[i], PartColors[i]);
	}
	ShownSign = -1;
	Hinge->SetRelativeRotation(FRotator(0.f, CurrentAngle, 0.f));
}

// ---- interaction -------------------------------------------------------------------------------------

void ATSSShopDoor::ToggleDoor()
{
	bHeldOpen = !bHeldOpen;
	// The door swings away from whoever opens it: from outside it opens into the shop, from inside it opens out.
	if (bHeldOpen && FMath::Abs(CurrentAngle) < 2.f)
	{
		if (const APawn* P = UGameplayStatics::GetPlayerPawn(this, 0)) SwingDir = DirectionAwayFrom(P->GetActorLocation());
	}
}

int32 ATSSShopDoor::DirectionAwayFrom(const FVector& World) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(World);
	return L.X > WallThick * 0.5f ? +1 : -1;   // +1 inward, -1 outward
}

bool ATSSShopDoor::PlayerInSwingZone() const
{
	const APawn* P = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!P) return false;
	const FVector L = GetActorTransform().InverseTransformPosition(P->GetActorLocation());
	return L.X < WallThick * 0.5f && L.X > -(LeafWidth + 25.f) && FMath::Abs(L.Y) < DoorHalfGapCm + 20.f;
}

void ATSSShopDoor::ToggleSign()
{
	if (UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>()) Store->SetOpen(!Store->IsOpen());
}

FString ATSSShopDoor::SignPrompt() const
{
	const UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	const bool bOpen = !Store || Store->IsOpen();
	return bOpen ? FString(TEXT("[E / Click] Flip the sign to CLOSED  (the shop is OPEN: customers come in)"))
	             : FString(TEXT("[E / Click] Flip the sign to OPEN  (the shop is CLOSED: no customers)"));
}

FString ATSSShopDoor::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	if (Hit.GetComponent() == SignPlate) return SignPrompt();
	return bHeldOpen ? FString(TEXT("[E / Click] Close the door")) : FString(TEXT("[E / Click] Open the door"));
}

void ATSSShopDoor::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	if (Hit.GetComponent() == SignPlate) ToggleSign();
	else ToggleDoor();
}

// ---- motion ---------------------------------------------------------------------------------------------

bool ATSSShopDoor::PlayerInDoorway() const
{
	const APawn* P = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!P) return false;
	const FVector L = GetActorTransform().InverseTransformPosition(P->GetActorLocation());
	return FMath::Abs(L.Y) < DoorHalfGapCm + 40.f && L.X > -70.f && L.X < 130.f;
}

bool ATSSShopDoor::NpcNearDoor(int32& OutDir) const
{
	// The nearest walker decides which way the door swings: away from them.
	const FVector Centre = GetActorLocation() + GetActorForwardVector() * (WallThick * 0.5f);
	float Best = NpcOpenRadius;
	bool bFound = false;
	auto Consider = [&](const FVector& P)
	{
		const float D = FVector::Dist2D(P, Centre);
		if (D < Best) { Best = D; OutDir = DirectionAwayFrom(P); bFound = true; }
	};
	for (TActorIterator<ATSSCustomer> It(GetWorld()); It; ++It) Consider(It->GetActorLocation());
	for (TActorIterator<ATSSDeliveryTruck> It(GetWorld()); It; ++It)
	{
		FVector C;
		if (It->GetCourierLocation(C)) Consider(C);
	}
	return bFound;
}

void ATSSShopDoor::RefreshSign(bool bOpen)
{
	const int32 State = bOpen ? 1 : 0;
	if (State == ShownSign) return;
	ShownSign = State;
	TSSApplyColor(SignPlate, bOpen ? OpenPlate : ClosedPlate);
	const FText T = FText::FromString(bOpen ? TEXT("OPEN") : TEXT("CLOSED"));
	SignOutside->SetText(T);
	SignInside->SetText(T);
}

void ATSSShopDoor::Tick(float Dt)
{
	Super::Tick(Dt);

	const UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	RefreshSign(!Store || Store->IsOpen());

	int32 NpcDir = SwingDir;
	bSomeoneNear = NpcNearDoor(NpcDir);
	WatchCrossings();

	// The bell swings and settles after each ring.
	RingTime += Dt;
	const float Swing = RingTime < 3.f ? 24.f * FMath::Exp(-RingTime * 2.2f) * FMath::Sin(RingTime * 26.f) : 0.f;
	BellPivot->SetRelativeRotation(FRotator(0.f, 0.f, Swing));
	// Open if the owner opened it or somebody is walking through; never shut it on the owner.
	const bool bWantOpen = bHeldOpen || bSomeoneNear || PlayerInDoorway();
	// The direction is chosen as the door starts to open and then kept until it has shut again.
	if (bWantOpen && !bHeldOpen && bSomeoneNear && FMath::Abs(CurrentAngle) < 2.f)
	{
		SwingDir = NpcDir;
		if (SwingDir > 0 && PlayerInSwingZone()) SwingDir = -1;   // do not swing into the owner standing just inside
	}
	const float Target = bWantOpen ? SwingDir * OpenDegrees : 0.f;
	if (!FMath::IsNearlyEqual(CurrentAngle, Target))
	{
		CurrentAngle = FMath::FInterpConstantTo(CurrentAngle, Target, Dt, SwingSpeed);
		PeakSwing = FMath::Max(PeakSwing, FMath::Abs(CurrentAngle));
		Hinge->SetRelativeRotation(FRotator(0.f, CurrentAngle, 0.f));
	}
}

// ---- the bell -------------------------------------------------------------------------------------------

static USoundBase* ShopBellSound()
{
	return LoadObject<USoundBase>(nullptr, TEXT("/Game/TSS/Audio/S_ShopBell.S_ShopBell"));
}

bool ATSSShopDoor::HasBellSound() const { return ShopBellSound() != nullptr; }

void ATSSShopDoor::RingBell()
{
	++Rings;
	RingTime = 0.f;
	// A door bell is heard in the shop and out on the pavement, fading with distance.
	static USoundAttenuation* Falloff = nullptr;
	if (!Falloff)
	{
		Falloff = NewObject<USoundAttenuation>(GetTransientPackage());
		Falloff->AddToRoot();
		Falloff->Attenuation.bAttenuate = true;
		Falloff->Attenuation.bSpatialize = true;
		Falloff->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
		Falloff->Attenuation.AttenuationShapeExtents = FVector(900.f);
		Falloff->Attenuation.FalloffDistance = 2200.f;
	}
	USoundBase* Sound = ShopBellSound();
	if (Sound) UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation() + FVector(-5.f, 0.f, 255.f), 1.f, 1.f, 0.f, Falloff);
	UE_LOG(LogTemp, Display, TEXT("TSS bell rang #%d (sound %s)"), Rings, Sound ? TEXT("played") : TEXT("MISSING"));
}

void ATSSShopDoor::WatchCrossings()
{
	for (auto It = WasOutside.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();

	// A walker "comes in" when it goes from outdoors (local x > 10) to indoors through the doorway.
	auto Watch = [this](AActor* Who, const FVector& World)
	{
		const FVector L = GetActorTransform().InverseTransformPosition(World);
		const bool bOutside = L.X > WallThick * 0.5f;
		if (bool* Prev = WasOutside.Find(Who))
		{
			if (*Prev && !bOutside && FMath::Abs(L.Y) < DoorHalfGapCm + 30.f) RingBell();
			*Prev = bOutside;
		}
		else
		{
			WasOutside.Add(Who, bOutside);
		}
	};
	for (TActorIterator<ATSSCustomer> It(GetWorld()); It; ++It) Watch(*It, It->GetActorLocation());
	for (TActorIterator<ATSSDeliveryTruck> It(GetWorld()); It; ++It)
	{
		FVector C;
		if (It->GetCourierLocation(C)) Watch(*It, C);
	}
}
