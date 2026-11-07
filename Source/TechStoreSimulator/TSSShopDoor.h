#pragma once

#include "CoreMinimal.h"
#include "TSSInteractable.h"
#include "TSSShopDoor.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * The shop's front door: a frame in the doorway, a hinged door with handles on both sides, and an Open/Closed sign
 * hanging on it. Place it at the front wall (x = FrontX, y = 0), facing +X (outdoors).
 *
 * - E on the door opens or closes it. It swings outward.
 * - E on the sign flips it. While the sign says CLOSED no new customers come in, however full the shelves are.
 * - Customers and the delivery courier open the door themselves as they walk through it; it shuts behind them
 *   unless you are holding it open. It never closes on the player standing in the doorway.
 */
UCLASS()
class ATSSShopDoor : public ATSSInteractable
{
	GENERATED_BODY()

public:
	ATSSShopDoor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual FString GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const override;
	virtual void Interact(ATSSCharacter* Player, const FHitResult& Hit) override;

	/** What the HUD says over the sign. */
	FString SignPrompt() const;

	/** What E does, callable directly (the self-test uses these). */
	void ToggleDoor();
	void ToggleSign();

	/** The player's wish: true once they have opened it (staying open until they close it). */
	bool IsHeldOpen() const { return bHeldOpen; }
	/** True while the door is actually swung open, for whatever reason. */
	bool IsOpenNow() const { return FMath::Abs(CurrentAngle) > 45.f; }
	/** Where the leaf is: 0 closed; positive = swung into the shop, negative = swung out onto the street. */
	float GetAngle() const { return CurrentAngle; }
	/** +1 if the door last opened (or will open) into the shop, -1 if outward. */
	int32 GetSwingDir() const { return SwingDir; }
	/** The widest the door has swung (degrees, always positive) since the last call; resets the record. For tests. */
	float ConsumePeakSwing() { const float P = PeakSwing; PeakSwing = FMath::Abs(CurrentAngle); return P; }
	/** True if anything (customer, courier) is making the door open right now. */
	bool IsBeingUsed() const { return bSomeoneNear; }

	/** The bell over the door rings when a customer or the courier walks in. */
	int32 GetRingCount() const { return Rings; }
	/** True if the bell sound asset (/Game/TSS/Audio/S_ShopBell) loads. */
	bool HasBellSound() const;
	/** Rings the bell now: sound plus the little bell swinging (public so tests and dev tools can trigger it). */
	void RingBell();


private:
	UStaticMeshComponent* Part(const TCHAR* Name, USceneComponent* Parent, FVector Loc, FVector Size, FLinearColor Color, bool bCollide = false, bool bCylinder = false);
	void RefreshSign(bool bOpen);
	bool PlayerInDoorway() const;
	bool NpcNearDoor(int32& OutDir) const;
	int32 DirectionAwayFrom(const FVector& World) const;
	bool PlayerInSwingZone() const;
	void WatchCrossings();

	UPROPERTY() TObjectPtr<USceneComponent> Hinge;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Leaf;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SignPlate;
	UPROPERTY() TObjectPtr<UTextRenderComponent> SignOutside;
	UPROPERTY() TObjectPtr<UTextRenderComponent> SignInside;
	UPROPERTY() TObjectPtr<USceneComponent> BellPivot;
	TMap<TWeakObjectPtr<AActor>, bool> WasOutside;   // per walker: was it outdoors last frame?
	int32 Rings = 0;
	float RingTime = 99.f;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	TArray<FLinearColor> PartColors;

	float CurrentAngle = 0.f;
	float PeakSwing = 0.f;
	int32 SwingDir = -1;   // +1 swings into the shop, -1 out onto the street
	bool bHeldOpen = false;
	bool bSomeoneNear = false;
	int32 ShownSign = -1;   // -1 unknown, 0 closed, 1 open
};
