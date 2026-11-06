#pragma once

#include "CoreMinimal.h"
#include "TSSProducts.h"
#include "TSSBusinessTypes.generated.h"

UENUM()
enum class ETSSOrderStatus : uint8
{
	InTransit,  // paid for, supplier is preparing / the truck is on its way
	Arrived,    // parcels are at the delivery dock waiting to be unloaded
	Received,   // everything unpacked
};

USTRUCT()
struct FTSSOrderLine
{
	GENERATED_BODY()

	UPROPERTY() ETSSProduct Product = ETSSProduct::None;
	UPROPERTY() int32 Qty = 0;
};

/** A wholesale order placed from StoreDesk. */
USTRUCT()
struct FTSSOrder
{
	GENERATED_BODY()

	UPROPERTY() int32 Id = 0;
	UPROPERTY() TArray<FTSSOrderLine> Lines;
	UPROPERTY() int32 Cost = 0;
	/** Business-clock seconds (it only runs while the shop is running). */
	UPROPERTY() float PlacedAt = 0.f;
	UPROPERTY() float ArriveAt = 0.f;
	UPROPERTY() ETSSOrderStatus Status = ETSSOrderStatus::InTransit;
	UPROPERTY() int32 UnitsLeft = 0;
	/** Runtime only: a truck has been dispatched for this order. */
	bool bTruckSent = false;

	int32 TotalUnits() const { int32 N = 0; for (const FTSSOrderLine& L : Lines) N += L.Qty; return N; }
};

/** One purchasable upgrade. Data-driven: add a row to the catalog, and a case in ApplyUpgrade if it does something. */
struct FTSSUpgradeDef
{
	const TCHAR* Id;
	const TCHAR* Name;
	const TCHAR* Description;
	int32 Cost;
	int32 MinLevel;
	/** Store size stage needed first (0 = any). */
	int32 MinStage;
	/** False = shown in the catalog as planned, cannot be bought yet. */
	bool bImplemented;
};
