#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TSSHumanAnimInstance.h"
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
	/** Ground speed in cm/s: 0 stands, ~140 walks, ~240+ moves with the quick-walk clip. Cheap to call every frame. */
	void SetSpeed(float CmPerSecond);
	void SetWalking(bool bWalking) { SetSpeed(bWalking ? 140.f : 0.f); }

	// ---- hands and head ---------------------------------------------------------------------------------------------------
	/** Bring a hand to a world-space point (it eases in over BlendSeconds). */
	void Reach(ETSSHand Hand, const FVector& WorldTarget, float BlendSeconds = 0.35f);
	/** Let the arm return to the animation. */
	void Relax(ETSSHand Hand, float BlendSeconds = 0.35f);
	/** 0..1: how far the hand has got to its target. */
	float ReachAlpha(ETSSHand Hand) const;
	bool IsReachSettled(ETSSHand Hand, float Threshold = 0.9f) const { return ReachAlpha(Hand) >= Threshold; }
	bool IsRelaxed(ETSSHand Hand) const { return ReachAlpha(Hand) <= 0.05f; }
	void LookAt(const FVector& WorldTarget);
	/** Bend forward from the waist (degrees; 0 = upright). Eases in and out. */
	void SetLean(float Degrees);
	/** Fingers curled with the index out, to press keys. */
	void SetPointing(ETSSHand Hand, bool bOn);
	void StopLooking();
	/** Where the hand actually is right now (a held product follows this). */
	FVector GetHandWorldLocation(ETSSHand Hand) const;
	bool IsBuilt() const { return bBuilt && Leader != nullptr; }
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
	UTSSHumanAnimInstance* AnimInst() const;
	bool bBuilt = false;
};
