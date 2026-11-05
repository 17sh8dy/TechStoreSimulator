#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TSSHumanComponent.generated.h"

class USkeletalMeshComponent;
class UAnimSequence;

/**
 * A real-looking person assembled from City Sample's crowd kit: an invisible leader skeleton plays the
 * animation while skin, clothes, face and hair follow it. Origin is the feet; the person faces +X.
 * Shared by the player and every customer so there is one place to change how people look.
 */
UCLASS()
class UTSSHumanComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/** Identity 1 or 2 (face + hair + beard); Top/Bottom/Shoes are suffixes of the m_tal_nrw_* clothing meshes. */
	void Build(int32 Identity, const FString& Top, const FString& Bottom, const FString& Shoes);
	/** Idle or walk cycle. Cheap to call every frame. */
	void SetWalking(bool bWalking);
	/** Show or hide the whole person (the invisible leader skeleton stays hidden either way). */
	void SetShown(bool bShown);
	/** True hides the whole person from their own camera (first person). */
	void SetHiddenFromOwner(bool bHidden);
	/** How many of the Identity/outfit choices exist, for random casting. */
	static int32 NumIdentities() { return 2; }
	static const TArray<FString>& Tops();
	static const TArray<FString>& Bottoms();
	static const TArray<FString>& Shoes();

private:
	USkeletalMeshComponent* AddPiece(const TCHAR* MeshPath, bool bFollowLeader);

	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Leader;
	UPROPERTY() TArray<TObjectPtr<UPrimitiveComponent>> Pieces;
	UPROPERTY() TObjectPtr<UAnimSequence> IdleAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> WalkAnim;
	bool bWalkingNow = false;
	bool bBuilt = false;
};
