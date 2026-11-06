#include "TSSProductItem.h"
#include "TSSCharacter.h"
#include "TSSRegister.h"
#include "TSSProductBox.h"

ATSSProductItem::ATSSProductItem()
{
	Body = CreateDefaultSubobject<UTSSProductBoxComponent>(TEXT("Body"));
	SetRootComponent(Body);
}

void ATSSProductItem::Init(ETSSProduct InProduct, int32 InPrice, ATSSRegister* InRegister)
{
	Product = InProduct;
	UnitPrice = InPrice;
	Register = InRegister;
	Body->SetProduct(Product);
}

void ATSSProductItem::PlaceAt(const FVector& SurfacePoint, const FRotator& Rotation)
{
	// The root is the box centre, so lift it by half its height to sit on the surface.
	SetActorLocationAndRotation(SurfacePoint + FVector(0.f, 0.f, TSSProductDef(Product).BoxSize.Z * 0.5f), Rotation);
	SetCarried(false);
}

void ATSSProductItem::SetCarried(bool bCarried)
{
	SetActorHiddenInGame(bCarried);
	SetActorEnableCollision(!bCarried);
}

FString ATSSProductItem::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	const FTSSProductDef& Def = TSSProductDef(Product);
	if (bScanned) return FString::Printf(TEXT("%s  -  scanned"), Def.Name);
	if (Player->GetHeld() != ETSSProduct::None) return TEXT("Your hands are full");
	return FString::Printf(TEXT("[E] Pick up %s  ($%d)"), Def.Name, UnitPrice);
}

void ATSSProductItem::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	if (bScanned || Player->GetHeld() != ETSSProduct::None) return;
	Player->SetHeld(Product);
	Player->SetHeldItem(this);
	SetCarried(true);
}
