#include "TSSCharacter.h"
#include "TSSPlayerController.h"
#include "TSSSettings.h"
#include "TSSInteractable.h"
#include "TSSMover.h"
#include "TSSHumanComponent.h"
#include "TSSProductBox.h"
#include "TSSBin.h"
#include "TSSShopDoor.h"
#include "TSSProducts.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

ATSSCharacter::ATSSCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	GetCharacterMovement()->MaxWalkSpeed = 210.f;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	bUseControllerRotationYaw = false;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetCapsuleComponent());
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	// Placeholder avatar (engine primitives) so third person has something to look at.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// The shop owner: a real person (feet at the capsule bottom, facing +X). Built in BeginPlay.
	Human = CreateDefaultSubobject<UTSSHumanComponent>(TEXT("Human"));
	Human->SetupAttachment(GetCapsuleComponent());
	Human->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
	Human->SetRelativeRotation(FRotator(0.f, -90.f, 0.f)); // the kit's meshes face +Y
	HeldMesh = CreateDefaultSubobject<UTSSProductBoxComponent>(TEXT("HeldMesh"));
	HeldMesh->SetupAttachment(GetCapsuleComponent());
	HeldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeldMesh->SetVisibility(false, true);

	Mover = CreateDefaultSubobject<UTSSMoverComponent>(TEXT("Mover"));

	// The emptied cardboard box you carry to the bin: open top, two flaps.
	auto MakePart = [this](const TCHAR* Name, UStaticMeshComponent* Parent, FVector Loc, FVector Scale, FRotator Rot)
	{
		UStaticMeshComponent* P = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		P->SetupAttachment(Parent);
		P->SetStaticMesh(TSSCubeMesh());
		P->SetRelativeLocation(Loc);
		P->SetRelativeRotation(Rot);
		P->SetRelativeScale3D(Scale);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->SetCastShadow(false);
		return P;
	};
	EmptyBoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmptyBox"));
	EmptyBoxMesh->SetupAttachment(GetCapsuleComponent());
	EmptyBoxMesh->SetStaticMesh(TSSCubeMesh());
	EmptyBoxMesh->SetRelativeScale3D(FVector(0.3f, 0.38f, 0.2f));
	EmptyBoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EmptyBoxMesh->SetCastShadow(false);
	EmptyBoxMesh->SetVisibility(false, true);
	EmptyBoxParts.Add(MakePart(TEXT("EmptyBoxInside"), EmptyBoxMesh, FVector(0.f, 0.f, 0.5f), FVector(0.86f, 0.9f, 0.02f), FRotator::ZeroRotator));
	EmptyBoxParts.Add(MakePart(TEXT("EmptyBoxFlapA"), EmptyBoxMesh, FVector(0.f, 0.62f, 0.62f), FVector(0.95f, 0.5f, 0.02f), FRotator(0.f, 0.f, 55.f)));
	EmptyBoxParts.Add(MakePart(TEXT("EmptyBoxFlapB"), EmptyBoxMesh, FVector(0.f, -0.62f, 0.62f), FVector(0.95f, 0.5f, 0.02f), FRotator(0.f, 0.f, -55.f)));
}

void ATSSCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (UTSSSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UTSSSettingsSubsystem>())
	{
		SettingsHandle = Settings->OnChanged.AddUObject(this, &ATSSCharacter::ApplyView);
	}
	Human->Build(2, TEXT("crewneck"), TEXT("jeansLong"), TEXT("oxfords"));
	TSSApplyColor(EmptyBoxMesh, FLinearColor(0.58f, 0.42f, 0.24f));
	TSSApplyColor(EmptyBoxParts[0], FLinearColor(0.12f, 0.085f, 0.05f));
	TSSApplyColor(EmptyBoxParts[1], FLinearColor(0.62f, 0.46f, 0.27f));
	TSSApplyColor(EmptyBoxParts[2], FLinearColor(0.62f, 0.46f, 0.27f));
	ApplyView();
}

void ATSSCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTSSSettingsSubsystem* Settings = GI->GetSubsystem<UTSSSettingsSubsystem>())
		{
			Settings->OnChanged.Remove(SettingsHandle);
		}
	}
	Super::EndPlay(Reason);
}

void ATSSCharacter::ApplyView()
{
	const UTSSSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UTSSSettingsSubsystem>();
	if (!Settings) return;

	const bool bFirstPerson = Settings->GetViewMode() == ETSSViewMode::FirstPerson;
	bFirstPersonView = bFirstPerson;
	SpringArm->TargetArmLength = bFirstPerson ? 0.f : 320.f;
	SpringArm->SocketOffset = bFirstPerson ? FVector::ZeroVector : FVector(0.f, 0.f, 40.f);
	SpringArm->bDoCollisionTest = !bFirstPerson;
	Camera->SetFieldOfView(Settings->GetActiveFOV());

	bUseControllerRotationYaw = bFirstPerson;
	GetCharacterMovement()->bOrientRotationToMovement = !bFirstPerson;
	Human->SetHiddenFromOwner(bFirstPerson);
}

bool ATSSCharacter::IsStrayBoxDrawn() const
{
	return Held == ETSSProduct::None && (HeldMesh->IsVisible() || HeldMesh->IsPanelDrawn());
}

void ATSSCharacter::SetHeld(ETSSProduct Product)
{
	Held = Product;
	if (Product == ETSSProduct::None) HeldItem = nullptr;
	if (Product == ETSSProduct::None)
	{
		HeldMesh->SetVisibility(false, true);
		return;
	}
	const FTSSProductDef& Def = TSSProductDef(Product);
	HeldMesh->SetProduct(Product);
	// In front of the chest, a little to the right and low, so it reads in first person too.
	HeldMesh->SetRelativeLocation(FVector(46.f, 18.f, 42.f - Def.BoxSize.Z * 0.5f));
	HeldMesh->SetVisibility(true, true);
}

void ATSSCharacter::SetCarryingEmptyBox(bool bCarry)
{
	bEmptyBox = bCarry;
	EmptyBoxMesh->SetVisibility(bCarry, true);
	if (bCarry) EmptyBoxMesh->SetRelativeLocation(FVector(46.f, 16.f, 2.f));   // in front of the chest (first person keeps it here)
}

void ATSSCharacter::InteractWith(ATSSInteractable* Target, const FHitResult& Hit)
{
	if (!Target) return;
	Target->Interact(this, Hit);
	// A quick reach toward what you pressed it on (visible in third person).
	Human->Reach(ETSSHand::Right, Hit.ImpactPoint, 0.16f);
	Human->LookAt(Hit.ImpactPoint);
	GestureTimer = 0.55f;
}

void ATSSCharacter::UpdateFocus(ATSSPlayerController* PC)
{
	Focus = nullptr;
	FocusPrompt.Reset();

	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 64.f);
	const FVector End = Eye + PC->GetControlRotation().Vector() * 280.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TSSFocus), false, this);
	if (!GetWorld()->LineTraceSingleByChannel(FocusHit, Eye, End, ECC_Visibility, Params)) return;

	Focus = Cast<ATSSInteractable>(FocusHit.GetActor());
	if (Focus) FocusPrompt = Focus->GetPrompt(this, FocusHit);
}

void ATSSCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ATSSPlayerController* PC = Cast<ATSSPlayerController>(GetController());
	if (!PC || !PC->IsGameActive())
	{
		Focus = nullptr;
		FocusPrompt.Reset();
		Human->SetSpeed(0.f);
		ActiveFor = 0.f;
		return;
	}
	ActiveFor += DeltaSeconds;

	FKey SprintKey = EKeys::LeftShift;
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const UTSSSettingsSubsystem* Cfg = GI->GetSubsystem<UTSSSettingsSubsystem>()) SprintKey = Cfg->GetSprintKey();
	}
	GetCharacterMovement()->MaxWalkSpeed = SpeedFor(PC->IsInputKeyDown(SprintKey));   // hold the sprint key (Shift by default) to hurry
	Human->SetSpeed(GetVelocity().Size2D());
	UpdateFocus(PC);
	// Carrying an empty box: the hands are full, so only a bin (to throw it away) and the shop door answer E.
	const bool bBlocked = bEmptyBox && Focus && !Focus->IsA(ATSSBin::StaticClass()) && !Focus->IsA(ATSSShopDoor::StaticClass());
	if (bBlocked) FocusPrompt = TEXT("Your hands are full: carry the empty box to a bin outside");
	// Hold E on a shelf or the StoreDesk computer to pick it up and move it (a quick tap still does its normal action).
	const bool bMoving = bBlocked ? false : Mover->Update(PC, this, Focus, FocusHit, DeltaSeconds);
	const FString MoverPrompt = bBlocked ? FString() : Mover->GetPrompt();
	if (!MoverPrompt.IsEmpty()) FocusPrompt = MoverPrompt;
	else if (!bBlocked && Focus && Focus->IsMovable() && !FocusPrompt.IsEmpty()) FocusPrompt += TEXT("\n(hold E for 2 seconds to move it)");
	if (Focus && !bMoving && !bBlocked)
	{
		if (!Focus->IsMovable() && PC->WasInputKeyJustPressed(EKeys::E)) InteractWith(Focus, FocusHit);
		// Left click does what a tap of E does, on anything (a click never starts a move: that stays a hold of E).
		if (ActiveFor > 0.3f && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)) InteractWith(Focus, FocusHit);
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp) || PC->WasInputKeyJustPressed(EKeys::RightBracket)) Focus->Adjust(this, FocusHit, +1);
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown) || PC->WasInputKeyJustPressed(EKeys::LeftBracket)) Focus->Adjust(this, FocusHit, -1);
	}

	float MouseX = 0.f, MouseY = 0.f;
	PC->GetInputMouseDelta(MouseX, MouseY);
	AddControllerYawInput(MouseX * LookSensitivity);
	AddControllerPitchInput(-MouseY * LookSensitivity);

	const FRotator Yaw(0.f, PC->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
	const float Fwd = (PC->IsInputKeyDown(EKeys::W) ? 1.f : 0.f) - (PC->IsInputKeyDown(EKeys::S) ? 1.f : 0.f);
	const float Strafe = (PC->IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - (PC->IsInputKeyDown(EKeys::A) ? 1.f : 0.f);
	AddMovementInput(Forward, Fwd);
	AddMovementInput(Right, Strafe);

	if (PC->WasInputKeyJustPressed(EKeys::SpaceBar)) Jump();

	// Arm: a reach after E, otherwise a held product rests against the chest.
	if (GestureTimer > 0.f)
	{
		GestureTimer -= DeltaSeconds;
		if (GestureTimer <= 0.f) Human->StopLooking();
	}
	if (GestureTimer <= 0.f)
	{
		if (Held != ETSSProduct::None || bEmptyBox)
		{
			const FVector Carry = GetActorLocation() + GetActorForwardVector() * 38.f + GetActorRightVector() * 14.f + FVector(0.f, 0.f, 22.f);
			Human->Reach(ETSSHand::Right, Carry, 0.3f);
		}
		else
		{
			Human->Relax(ETSSHand::Right, 0.3f);
		}
	}
	// In third person the box is really in your hand; in first person it stays steady in view.
	if (bEmptyBox && !bFirstPersonView)
	{
		const FRotator Rot(0.f, GetActorRotation().Yaw, 0.f);
		EmptyBoxMesh->SetWorldLocationAndRotation(Human->GetHandWorldLocation(ETSSHand::Right) + Rot.Vector() * 20.f + FVector(0.f, 0.f, 4.f), Rot);
	}
	if (Held != ETSSProduct::None && !bFirstPersonView)
	{
		const FRotator Rot(0.f, GetActorRotation().Yaw, 0.f);
		HeldMesh->SetWorldLocationAndRotation(Human->GetHandWorldLocation(ETSSHand::Right) + Rot.Vector() * (TSSProductDef(Held).BoxSize.X * 0.5f + 3.f), Rot);
	}
}