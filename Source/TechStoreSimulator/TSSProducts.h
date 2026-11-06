#pragma once

#include "CoreMinimal.h"
#include "TSSProducts.generated.h"

/** New products are only ever appended before Count: prices and saves index by this value. */
UENUM(BlueprintType)
enum class ETSSProduct : uint8
{
	None,
	Phone,
	Laptop,
	Headphones,
	Console,
	Monitor,
	Keyboard,
	Tablet,
	Earbuds,
	Smartwatch,
	Speaker,
	// Premium Supplier Access (supplier_premium upgrade)
	GamingLaptop,
	VRHeadset,
	Camera,
	// Gaming Department (dept_gaming upgrade)
	Controller,
	GamingHeadset,
	RacingWheel,
	Count UMETA(Hidden)
};

/**
 * Static catalogue entry. Sizes are the retail box in cm: X = depth (front to back), Y = width, Z = height.
 * Brands are invented. Texture names a packaging image in /Game/TSS/Textures (made by Scripts/make_packaging.py).
 */
struct FTSSProductDef
{
	const TCHAR* Name;
	int32 WholesaleCost;
	FVector BoxSize;
	FLinearColor Color;
	const TCHAR* Blurb = TEXT("");
	const TCHAR* Category = TEXT("");
	const TCHAR* Brand = TEXT("");
	const TCHAR* Texture = TEXT("");
	/** Business level needed before this can be ordered from the wholesaler. */
	int32 MinLevel = 1;
	/** Upgrade id that must be owned before this can be ordered (empty = none needed). */
	const TCHAR* RequiresUpgrade = TEXT("");

	/** How many of this box fit front-to-back in one shelf slot (shelf depth is 50 cm). */
	int32 SlotCapacity() const { return FMath::Clamp(FMath::FloorToInt(46.f / BoxSize.X), 1, 4); }
	int32 DefaultPrice() const { return FMath::RoundToInt(WholesaleCost * 1.4f); }
};

const FTSSProductDef& TSSProductDef(ETSSProduct Product);

/** Pieces shared by every placeholder-art actor. */
class UStaticMeshComponent;
void TSSApplyColor(UStaticMeshComponent* Mesh, const FLinearColor& Color);
UStaticMesh* TSSCubeMesh();
