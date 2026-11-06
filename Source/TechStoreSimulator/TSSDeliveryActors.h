#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TSSInteractable.h"
#include "TSSBusinessTypes.h"
#include "TSSDeliveryActors.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UTSSHumanComponent;
class ATSSRoad;

/** Where deliveries happen (world space, cm). */
struct FTSSDockInfo
{
	FVector Drop = FVector(620.f, 260.f, 0.f);        // parcels are put down here, inside the shop
	FVector TruckStart = FVector(2900.f, 260.f, 0.f); // the truck appears here (outside)
	FVector TruckStop = FVector(1250.f, 260.f, 0.f);  // and parks here
	FVector Door = FVector(760.f, 0.f, 0.f);          // the courier walks through this doorway
};

/** A floor mat marking the delivery drop zone just inside the door. Move it in the editor to move deliveries. */
UCLASS()
class ATSSDeliveryDock : public AActor
{
	GENERATED_BODY()

public:
	ATSSDeliveryDock();
	virtual void OnConstruction(const FTransform& Transform) override;

	/** The dock in the world, or sensible defaults (door taken from the register) if the map has none. */
	static FTSSDockInfo Resolve(UWorld* World);

	/** Truck positions, relative to this actor. */
	UPROPERTY(EditAnywhere, Category = "Delivery") FVector TruckStartOffset = FVector(2280.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Delivery") FVector TruckStopOffset = FVector(630.f, 0.f, 0.f);

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mat;
};

/** A box of ordered products on the dock. Unpack one unit at a time into your hands, then shelve it. */
UCLASS()
class ATSSParcel : public ATSSInteractable
{
	GENERATED_BODY()

public:
	ATSSParcel();
	virtual void Tick(float DeltaSeconds) override;
	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;

	void Init(int32 InOrderId, ETSSProduct InProduct, int32 InQty);

	/**
	 * Where the next parcel goes. Parcels are stacked in the corner by the delivery dock, a layer at a time (the
	 * floor first, then a second layer on top, and so on), so a big delivery never spreads across the shop.
	 * Cells that are blocked by furniture or sit on the path from the door are skipped.
	 */
	static FVector NextStackSlot(UWorld* World);
	/** Drops any parcel that is left floating after the one under it was unpacked. */
	static void SettleStacks(UWorld* World);

	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	static constexpr float StackStep = 40.f;   // height of one parcel
	static constexpr int32 MaxStack = 7;       // 7 x 40 cm = 2.8 m, under the 3.2 m ceiling
	int32 GetQty() const { return Qty; }
	int32 GetOrderId() const { return OrderId; }
	ETSSProduct GetProduct() const { return Product; }

private:
	void UpdateLabel();

	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Tape;
	UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
	int32 OrderId = 0;
	ETSSProduct Product = ETSSProduct::None;
	int32 Qty = 0;
};

/** A delivery truck: drives in, parks, a courier carries the parcels into the shop, then it leaves. */
UCLASS()
class ATSSDeliveryTruck : public AActor
{
	GENERATED_BODY()

public:
	ATSSDeliveryTruck();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	void Setup(const FTSSOrder& InOrder, const FTSSDockInfo& InDock, float InSpeedScale);

	/** While the courier is out of the truck, where they are (the shop door uses this to open for them). */
	bool GetCourierLocation(FVector& Out) const;
	/** True if this truck is driving on a road (rather than the old straight line). */
	bool IsOnRoad() const { return Road.IsValid(); }
	/** The road this truck follows, if any. */
	ATSSRoad* GetRoad() const { return Road.Get(); }

private:
	enum class EPhase : uint8 { Driving, ToDoor, Dropping, Returning, Leaving };

	UStaticMeshComponent* Box(const TCHAR* Name, FVector Loc, FVector Size, FLinearColor Color, FRotator Rot = FRotator::ZeroRotator, bool bCylinder = false);
	bool WalkTo(const FVector& Target, float Dt);
	void BuildRoute();
	void PlaceOnRoad();
	void DropParcels();

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	TArray<FLinearColor> PartColors;
	UPROPERTY() TObjectPtr<USceneComponent> CourierRoot;
	UPROPERTY() TObjectPtr<UTSSHumanComponent> Courier;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> CarriedBox;

	FTSSOrder Order;
	FTSSDockInfo Dock;
	float SpeedScale = 1.f;
	EPhase Phase = EPhase::Driving;
	TArray<FVector> Route;   // courier waypoints, there and back
	int32 RouteIndex = 0;
	bool bDropped = false;

	// Road driving: the truck follows the road's lane by distance along it.
	TWeakObjectPtr<ATSSRoad> Road;
	float RoadDistance = 0.f;
	float ParkDistance = 0.f;
};
