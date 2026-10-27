#include "TSSCharacter.h"
#include "TSSPlayerController.h"
#include "TSSSettings.h"
#include "TSSInteractable.h"
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
	GetCharacterMovement()->MaxWalkSpeed = 450.f;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	bUseControllerRotationYaw = false;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetCapsuleComponent());
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	// Placeholder avatar (engine primitives) so third person has something to look at.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	auto AddPart = [this](const TCHAR* Name, UStaticMesh* PartMesh, FVector Loc, FVector Scale)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(GetCapsuleComponent());
		Part->SetStaticMesh(PartMesh);
		Part->SetRelativeLocation(Loc);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(true);
		BodyParts.Add(Part);
	};
	AddPart(TEXT("Torso"), Cylinder.Object, FVector(0.f, 0.f, -23.f), FVector(0.6f, 0.6f, 1.3f));
	AddPart(TEXT("Head"), Sphere.Object, FVector(0.f, 0.f, 62.f), FVector(0.36f, 0.36f, 0.36f));
	HeldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldMesh"));
	HeldMesh->SetupAttachment(GetCapsuleComponent());
	HeldMesh->SetStaticMesh(Cube.Object);
	HeldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeldMesh->SetVisibility(false);
	AddPart(TEXT("Nose"), Cube.Object, FVector(18.f, 0.f, 62.f), FVector(0.2f, 0.1f, 0.08f));
}

void ATSSCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (UTSSSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UTSSSettingsSubsystem>())
	{
		SettingsHandle = Settings->OnChanged.AddUObject(this, &ATSSCharacter::ApplyView);
	}
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
	SpringArm->TargetArmLength = bFirstPerson ? 0.f : 320.f;
	SpringArm->SocketOffset = bFirstPerson ? FVector::ZeroVector : FVector(0.f, 0.f, 40.f);
	SpringArm->bDoCollisionTest = !bFirstPerson;
	Camera->SetFieldOfView(Settings->GetActiveFOV());

	bUseControllerRotationYaw = bFirstPerson;
	GetCharacterMovement()->bOrientRotationToMovement = !bFirstPerson;
	for (UStaticMeshComponent* Part : BodyParts)
	{
		Part->SetOwnerNoSee(bFirstPerson);
	}
}

void ATSSCharacter::SetHeld(ETSSProduct Product)
{
	Held = Product;
	if (Product == ETSSProduct::None)
	{
		HeldMesh->SetVisibility(false);
		return;
	}
	const FTSSProductDef& Def = TSSProductDef(Product);
	HeldMesh->SetRelativeScale3D(Def.BoxSize / 100.f);
	// In front of the chest, a little to the right and low, so it reads in first person too.
	HeldMesh->SetRelativeLocation(FVector(46.f, 18.f, 42.f - Def.BoxSize.Z * 0.5f));
	TSSApplyColor(HeldMesh, Def.Color);
	HeldMesh->SetVisibility(true);
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
		return;
	}

	UpdateFocus(PC);
	if (Focus)
	{
		if (PC->WasInputKeyJustPressed(EKeys::E)) Focus->Interact(this, FocusHit);
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
}