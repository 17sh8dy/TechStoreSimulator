#include "TSSHumanAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "TwoBoneIK.h"

namespace
{
	constexpr float WalkNominalSpeed = 140.f;   // the in-place walk looks right at this ground speed
	constexpr float RunNominalSpeed = 239.f;    // the quick-walk clip travels 406 cm in 1.7 s

	float Wrap(float T, float Length) { return Length > 0.f ? FMath::Fmod(T, Length) : 0.f; }
	float Ease(float A) { return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(A, 0.f, 1.f), 2.f); }
}

/** Builds the pose on the animation thread from the data the instance copied over. */
struct FTSSHumanAnimProxy : public FAnimInstanceProxy
{
	FTSSHumanAnimProxy(UAnimInstance* InInstance) : FAnimInstanceProxy(InInstance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override
	{
		FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
		if (UTSSHumanAnimInstance* Inst = Cast<UTSSHumanAnimInstance>(InAnimInstance))
		{
			Data = Inst->Data;
			Idle = Inst->IdleAnim;
			Walk = Inst->WalkAnim;
			Run = Inst->RunAnim;
			if (USkeletalMeshComponent* Mesh = Inst->GetSkelMeshComponent()) Data.ComponentToWorld = Mesh->GetComponentTransform();
		}
	}

	virtual bool Evaluate(FPoseContext& Output) override
	{
		if (!Idle)
		{
			Output.ResetToRefPose();
			return true;
		}

		// ---- locomotion: idle -> walk -> quick walk, blended in place ----------------------------------------------------------
		FAnimationPoseData Out(Output);
		Idle->GetAnimationPose(Out, FAnimExtractContext(Data.IdleTime, false));
		if (Walk && Data.MoveWeight > 0.01f)
		{
			FPoseContext Tmp(Output);
			FAnimationPoseData TmpData(Tmp);
			Walk->GetAnimationPose(TmpData, FAnimExtractContext(Data.WalkTime, false));
			FAnimationRuntime::BlendTwoPosesTogetherInPlace(Out, TmpData, 1.f - Data.MoveWeight);
		}
		if (Run && Data.RunWeight > 0.01f)
		{
			FPoseContext Tmp(Output);
			FAnimationPoseData TmpData(Tmp);
			Run->GetAnimationPose(TmpData, FAnimExtractContext(Data.RunTime, false));
			FAnimationRuntime::BlendTwoPosesTogetherInPlace(Out, TmpData, 1.f - Data.RunWeight);
		}

		const FBoneContainer& BC = Output.Pose.GetBoneContainer();
		// The quick-walk clip carries its forward travel in the root bone: the character is moved by gameplay, so remove it.
		if (BC.GetNumBones() > 0)
		{
			FTransform& Root = Output.Pose[FCompactPoseBoneIndex(0)];
			Root.SetTranslation(FVector::ZeroVector);
		}

		// ---- hands and head: procedural, in component space --------------------------------------------------------------------
		const bool bAnyReach = Data.Reach[0].Alpha > 0.002f || Data.Reach[1].Alpha > 0.002f;
		if (!bAnyReach && Data.LookAlpha < 0.002f) return true;

		auto Index = [&BC](const TCHAR* Name) -> FCompactPoseBoneIndex
		{
			const int32 Mesh = BC.GetPoseBoneIndexForBoneName(FName(Name));
			return Mesh == INDEX_NONE ? FCompactPoseBoneIndex(INDEX_NONE) : BC.MakeCompactPoseIndex(FMeshPoseBoneIndex(Mesh));
		};

		FCSPose<FCompactPose> CS;
		CS.InitPose(Output.Pose);

		static const TCHAR* Upper[2] = { TEXT("upperarm_r"), TEXT("upperarm_l") };
		static const TCHAR* Lower[2] = { TEXT("lowerarm_r"), TEXT("lowerarm_l") };
		static const TCHAR* HandB[2] = { TEXT("hand_r"), TEXT("hand_l") };
		for (int32 h = 0; h < 2; ++h)
		{
			const FTSSReachChannel& R = Data.Reach[h];
			if (R.Alpha <= 0.002f) continue;
			const FCompactPoseBoneIndex U = Index(Upper[h]), L = Index(Lower[h]), H = Index(HandB[h]);
			if (U == INDEX_NONE || L == INDEX_NONE || H == INDEX_NONE) continue;

			FTransform UpperT = CS.GetComponentSpaceTransform(U);
			FTransform LowerT = CS.GetComponentSpaceTransform(L);
			FTransform HandT = CS.GetComponentSpaceTransform(H);
			const FVector TargetCS = Data.ComponentToWorld.InverseTransformPosition(R.WorldTarget);
			const FVector Effector = FMath::Lerp(HandT.GetLocation(), TargetCS, Ease(R.Alpha));

			// Keep the elbow bending the way the animation already bends it (axis-agnostic), pushed out a little.
			const FVector Mid = (UpperT.GetLocation() + Effector) * 0.5f;
			FVector Bend = (LowerT.GetLocation() - Mid).GetSafeNormal();
			if (Bend.IsNearlyZero()) Bend = FVector(h == 0 ? -1.f : 1.f, 0.f, -0.5f).GetSafeNormal();
			const FVector JointTarget = Mid + Bend * 45.f;

			AnimationCore::SolveTwoBoneIK(UpperT, LowerT, HandT, JointTarget, Effector, false, 0.0, 0.0);
			CS.SetComponentSpaceTransform(U, UpperT);
			CS.SetComponentSpaceTransform(L, LowerT);
			CS.SetComponentSpaceTransform(H, HandT);
		}

		if (Data.LookAlpha > 0.002f)
		{
			// Turn head and neck toward the target by a limited amount.
			const FVector TargetCS = Data.ComponentToWorld.InverseTransformPosition(Data.LookTarget);
			const FCompactPoseBoneIndex Neck = Index(TEXT("neck_02")), Head = Index(TEXT("head"));
			const FCompactPoseBoneIndex Chain[2] = { Neck, Head };
			const float Weights[2] = { 0.35f, 0.6f };
			for (int32 i = 0; i < 2; ++i)
			{
				if (Chain[i] == INDEX_NONE) continue;
				FTransform T = CS.GetComponentSpaceTransform(Chain[i]);
				const FVector Dir = (TargetCS - T.GetLocation()).GetSafeNormal();
				const FQuat Delta = FQuat::FindBetweenNormals(FVector(0.f, 1.f, 0.f), Dir);
				FVector Axis; float Angle;
				Delta.ToAxisAndAngle(Axis, Angle);
				Angle = FMath::Clamp(Angle, 0.f, FMath::DegreesToRadians(55.f));
				const FQuat Limited(Axis, Angle * Weights[i] * Ease(Data.LookAlpha));
				T.SetRotation((Limited * T.GetRotation()).GetNormalized());
				CS.SetComponentSpaceTransform(Chain[i], T);
			}
		}

		FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(CS, Output.Pose);
		Output.Pose.NormalizeRotations();
		return true;
	}

	FTSSHumanAnimData Data;
	UAnimSequence* Idle = nullptr;
	UAnimSequence* Walk = nullptr;
	UAnimSequence* Run = nullptr;
};

// ---- the instance (game thread) -------------------------------------------------------------------------------------------

FAnimInstanceProxy* UTSSHumanAnimInstance::CreateAnimInstanceProxy()
{
	return new FTSSHumanAnimProxy(this);
}

void UTSSHumanAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	const FString Base = TEXT("/Game/Crowd/Character/Anims/Loco/");
	IdleAnim = LoadObject<UAnimSequence>(nullptr, *(Base + TEXT("MTN_N_Idle.MTN_N_Idle")));
	WalkAnim = LoadObject<UAnimSequence>(nullptr, *(Base + TEXT("MTN_N_Walk_InPlace.MTN_N_Walk_InPlace")));
	RunAnim = LoadObject<UAnimSequence>(nullptr, *(Base + TEXT("MTN_N_WalkQuickly_F.MTN_N_WalkQuickly_F")));
	Data.IdleTime = FMath::FRandRange(0.f, 5.f); // so a crowd does not breathe in step
}

void UTSSHumanAnimInstance::SetReach(ETSSHand Hand, bool bActive, const FVector& WorldTarget, float BlendSeconds)
{
	FTSSReachChannel& R = Data.Reach[static_cast<int32>(Hand)];
	R.bActive = bActive;
	if (bActive) R.WorldTarget = WorldTarget;
	R.BlendSeconds = FMath::Max(0.05f, BlendSeconds);
}

void UTSSHumanAnimInstance::SetLook(bool bActive, const FVector& WorldTarget)
{
	bLookActive = bActive;
	if (bActive) Data.LookTarget = WorldTarget;
}

void UTSSHumanAnimInstance::NativeUpdateAnimation(float Dt)
{
	Super::NativeUpdateAnimation(Dt);
	if (!IdleAnim) return;

	// Smooth the speed, then derive how much of each clip is in the mix.
	Speed = FMath::FInterpTo(Speed, TargetSpeed, Dt, 7.f);
	const float MoveTarget = FMath::Clamp(Speed / 12.f, 0.f, 1.f) * (Speed > 5.f ? 1.f : 0.f);
	const float WalkBlend = FMath::Clamp(Speed / WalkNominalSpeed, 0.f, 1.f);
	const float RunBlend = FMath::Clamp((Speed - 150.f) / 90.f, 0.f, 1.f);
	Data.MoveWeight = FMath::FInterpTo(Data.MoveWeight, FMath::Min(MoveTarget, 1.f) * WalkBlend, Dt, 9.f);
	Data.RunWeight = FMath::FInterpTo(Data.RunWeight, RunBlend, Dt, 6.f);

	// Clips advance at a rate that keeps the feet roughly matched to the ground speed.
	const float WalkRate = FMath::Clamp(Speed / WalkNominalSpeed, 0.6f, 1.5f);
	const float RunRate = FMath::Clamp(Speed / RunNominalSpeed, 0.8f, 1.4f);
	Data.IdleTime = Wrap(Data.IdleTime + Dt, IdleAnim->GetPlayLength());
	if (WalkAnim) Data.WalkTime = Wrap(Data.WalkTime + Dt * WalkRate, WalkAnim->GetPlayLength());
	if (RunAnim) Data.RunTime = Wrap(Data.RunTime + Dt * RunRate, RunAnim->GetPlayLength());

	for (FTSSReachChannel& R : Data.Reach)
	{
		const float Step = Dt / R.BlendSeconds;
		R.Alpha = R.bActive ? FMath::Min(1.f, R.Alpha + Step) : FMath::Max(0.f, R.Alpha - Step);
	}
	Data.LookAlpha = bLookActive ? FMath::Min(1.f, Data.LookAlpha + Dt / 0.4f) : FMath::Max(0.f, Data.LookAlpha - Dt / 0.4f);
}
