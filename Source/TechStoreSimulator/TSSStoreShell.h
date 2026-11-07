#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TSSStoreShell.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * The building itself: floor, walls, ceiling, lights, door sign and decor, generated for the current size stage
 * (see TSSStoreLayout.h). Buying an expansion in StoreDesk moves the walls outward and slides every wall-anchored
 * actor (tagged TSSAnchorBack / TSSAnchorLeft / TSSAnchorRight) with them.
 */
UCLASS()
class ATSSStoreShell : public AActor
{
	GENERATED_BODY()

public:
	ATSSStoreShell();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Rebuilds the walls for `Stage` and slides anchored actors by the difference from the stage they were at.
	 *  bInitial: first application at load - upgrade shelves are already spawned in place, so they are skipped. */
	void ApplyStage(int32 Stage, bool bInitial);
	int32 GetStage() const { return CurrentStage; }

	/** Night lights: a floodlight on the "TECH STORE" sign over the door. */
	void SetNightLights(bool bOn);
	bool IsSignLightOn() const { return bSignLightOn; }

private:
	UStaticMeshComponent* Box(const TCHAR* Name, FLinearColor Color, bool bCollide = true);
	void Layout(int32 Stage);
	void OnBusinessChanged();
	/** The sign over the door reads the store's name, sized to fit the board. */
	void ShowStoreName(const FString& Name);

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	UPROPERTY() TObjectPtr<class UPointLightComponent> SignLight;
	bool bSignLightOn = false;
	TArray<FLinearColor> PartColors;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Floor;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Ceiling;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> WallBack;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> WallLeft;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> WallRight;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> WallFrontA;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> WallFrontB;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Lintel;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> FeatureWall;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> DoorMat;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SignBoard;
	UPROPERTY() TObjectPtr<UTextRenderComponent> SignText;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> LightPanels;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Lights;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Skirting;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Posters;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> PosterTexts;

	int32 CurrentStage = 0;
	FDelegateHandle BizHandle;
};
