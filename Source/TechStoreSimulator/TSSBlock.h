#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TSSBlock.generated.h"

class UStaticMeshComponent;

/** A coloured box: walls, floor, counter. Placeholder art that is editable in the level. */
UCLASS()
class ATSSBlock : public AActor
{
	GENERATED_BODY()

public:
	ATSSBlock();
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Re-applies Size and Color (also callable from editor scripts after changing them). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Tech Store") void Rebuild();

	/** Full size in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Size = FVector(100.f, 100.f, 100.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor Color = FLinearColor(0.7f, 0.7f, 0.7f);

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
};
