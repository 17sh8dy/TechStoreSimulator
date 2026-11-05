#pragma once

#include "CoreMinimal.h"
#include "TSSProducts.generated.h"

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
	Count UMETA(Hidden)
};

/** Static catalogue entry. Sizes are the retail box in cm: X = depth (front to back), Y = width, Z = height. */
struct FTSSProductDef
{
	const TCHAR* Name;
	int32 WholesaleCost;
	FVector BoxSize;
	FLinearColor Color;
	const TCHAR* Blurb = TEXT("");

	/** How many of this box fit front-to-back in one shelf slot (shelf depth is 50 cm). */
	int32 SlotCapacity() const { return FMath::Clamp(FMath::FloorToInt(46.f / BoxSize.X), 1, 4); }
	int32 DefaultPrice() const { return FMath::RoundToInt(WholesaleCost * 1.4f); }
};

const FTSSProductDef& TSSProductDef(ETSSProduct Product);

/** Pieces shared by every placeholder-art actor. */
class UStaticMeshComponent;
void TSSApplyColor(UStaticMeshComponent* Mesh, const FLinearColor& Color);
UStaticMesh* TSSCubeMesh();
