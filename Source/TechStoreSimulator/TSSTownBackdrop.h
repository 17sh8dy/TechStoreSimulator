#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TSSTownBackdrop.generated.h"

class ATSSRoad;
class UInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * Layer 4 of the town: the cheap far background. A ground plane that reaches the horizon, the street carrying on past
 * both ends of the real road, plain low-detail blocks lining it, a skyline of taller blocks and tree lines further out.
 * Nothing here has collision or a shadow, and everything is instanced (one component per colour), so it costs a handful
 * of draw calls. Spawned by the town subsystem at the start of play (the saved map is not touched); `-TSSNoBackdrop` skips it.
 */
UCLASS()
class ATSSTownBackdrop : public AActor
{
	GENERATED_BODY()

public:
	ATSSTownBackdrop();

	/** Builds everything around the given road. */
	void Build(const ATSSRoad* Road);
	int32 NumInstances() const { return Instances; }

private:
	void Inst(UStaticMesh* Mesh, const FLinearColor& Color, const FVector& Pos, const FRotator& Rot, const FVector& SizeCm);

	UPROPERTY(Transient) TArray<TObjectPtr<UInstancedStaticMeshComponent>> Groups;
	TMap<uint32, int32> GroupIndex;
	int32 Instances = 0;
};
