#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TSSStreetBuilding.generated.h"

class UStaticMeshComponent;

/**
 * A simple generated building for the street scene: a body, a flat roof, rows of windows and a shopfront with an
 * awning on the ground floor. Pure code, no imported assets. Local +X is the street side (the front); the body
 * extends back along -X. Change the numbers in the Details panel and it rebuilds.
 */
UCLASS()
class ATSSStreetBuilding : public AActor
{
	GENERATED_BODY()

public:
	ATSSStreetBuilding();
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Rebuilds the parts (callable from editor scripts). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Building") void Rebuild();

	/** Along the street (Y). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building") float Width = 800.f;
	/** Back from the street (X). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building") float Depth = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building") float Height = 900.f;
	/** Picks the wall colour and awning colour (any number). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building") int32 Style = 0;
	/** Ground-floor shopfront with a glass window and an awning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building") bool bShopfront = true;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(Transient) TArray<TObjectPtr<UActorComponent>> Generated;
};
