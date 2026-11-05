#include "TSSCrate.h"
#include "TSSCharacter.h"
#include "TSSProductItem.h"
#include "TSSStoreSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"

ATSSCrate::ATSSCrate()
{
	PrimaryActorTick.bCanEverTick = true;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetStaticMesh(TSSCubeMesh());
	Body->SetRelativeScale3D(FVector(0.6f, 0.9f, 0.6f));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Body);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 95.f));
	Label->SetRelativeScale3D(FVector(1.f / 0.6f, 1.f / 0.9f, 1.f / 0.6f)); // undo the body's squash
	Label->SetWorldSize(12.f);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetTextRenderColor(FColor::White);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATSSCrate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void ATSSCrate::Rebuild()
{
	const FTSSProductDef& Def = TSSProductDef(Product);
	TSSApplyColor(Body, Def.Color * 0.8f);
	Label->SetText(FText::FromString(FString::Printf(TEXT("%s\n$%d each"), Def.Name, Def.WholesaleCost)));
}

void ATSSCrate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Turn the label to face the player so it reads from any side.
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		FRotator Face = (Cam->GetCameraLocation() - Label->GetComponentLocation()).Rotation();
		Face.Pitch = 0.f;
		Label->SetWorldRotation(Face);
	}
}

FString ATSSCrate::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	if (Player->GetHeldItem()) return TEXT("Scan this item or put it back on the counter");
	const FTSSProductDef& Def = TSSProductDef(Product);
	const ETSSProduct Held = Player->GetHeld();
	if (Held == ETSSProduct::None)
	{
		return FString::Printf(TEXT("[E] Buy %s  ($%d)"), Def.Name, Def.WholesaleCost);
	}
	if (Held == Product)
	{
		return FString::Printf(TEXT("[E] Return %s  (+$%d)"), Def.Name, Def.WholesaleCost);
	}
	return TEXT("Your hands are full");
}

void ATSSCrate::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	if (Player->GetHeldItem()) return;
	UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	const int32 Cost = TSSProductDef(Product).WholesaleCost;
	const ETSSProduct Held = Player->GetHeld();

	if (Held == ETSSProduct::None)
	{
		if (Store->TrySpend(Cost)) Player->SetHeld(Product);
	}
	else if (Held == Product)
	{
		Store->Earn(Cost);
		Player->SetHeld(ETSSProduct::None);
	}
}
