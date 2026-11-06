#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TSSHumanAnimInstance.generated.h"

class UAnimSequence;

enum class ETSSHand : uint8 { Right = 0, Left = 1 };

/** Where a hand should go. Alpha eases toward 1 while Active, back to 0 when released. */
struct FTSSReachChannel
{
	bool bActive = false;
	FVector WorldTarget = FVector::ZeroVector;
	float Alpha = 0.f;
	float BlendSeconds = 0.35f;
};

/** Everything the animation worker thread needs, copied from the game thread once per frame. */
struct FTSSHumanAnimData
{
	float IdleTime = 0.f, WalkTime = 0.f, RunTime = 0.f;
	float MoveWeight = 0.f;   // 0 = standing, 1 = walking
	float RunWeight = 0.f;    // 0 = walking, 1 = quick walk / jog
	FTSSReachChannel Reach[2];
	float LookAlpha = 0.f;
	FVector LookTarget = FVector::ZeroVector;
	FTransform ComponentToWorld;
};

/**
 * The animation brain for a person (the leader skeleton of UTSSHumanComponent):
 *  - blends idle / walk / run by speed with smoothed transitions, and strips the run clip's root motion;
 *  - then bends either arm toward a world-space target with two-bone IK (reach, pick up, place, tap a card);
 *  - and turns the head toward a point of interest.
 * There is no Animation Blueprint: the pose is built in code by FTSSHumanAnimProxy::Evaluate.
 */
UCLASS(Transient)
class UTSSHumanAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	/** Current ground speed in cm/s (walk ~140, quick walk ~240). */
	void SetSpeed(float InSpeed) { TargetSpeed = InSpeed; }
	void SetReach(ETSSHand Hand, bool bActive, const FVector& WorldTarget, float BlendSeconds);
	void SetLook(bool bActive, const FVector& WorldTarget);
	float GetReachAlpha(ETSSHand Hand) const { return Data.Reach[static_cast<int32>(Hand)].Alpha; }
	bool IsReaching(ETSSHand Hand) const { return Data.Reach[static_cast<int32>(Hand)].bActive; }

	UPROPERTY(Transient) TObjectPtr<UAnimSequence> IdleAnim;
	UPROPERTY(Transient) TObjectPtr<UAnimSequence> WalkAnim;
	UPROPERTY(Transient) TObjectPtr<UAnimSequence> RunAnim;

	FTSSHumanAnimData Data;

private:
	float TargetSpeed = 0.f;
	float Speed = 0.f;
	bool bLookActive = false;
};
