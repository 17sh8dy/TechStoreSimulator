#include "TSSHumanComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GroomAsset.h"
#include "GroomBindingAsset.h"
#include "GroomComponent.h"

namespace
{
	const TCHAR* Kit = TEXT("/Game/Crowd/Character/");

	FString MeshPath(const FString& Name)
	{
		return FString::Printf(TEXT("%sMale/NormalWeight/Meshes/m_tal_nrw_%s.m_tal_nrw_%s"), Kit, *Name, *Name);
	}
}

const TArray<FString>& UTSSHumanComponent::Tops()
{
	static const TArray<FString> V = { TEXT("buttonDown"), TEXT("buttonOpen"), TEXT("crewneck"), TEXT("turtleneck") };
	return V;
}
const TArray<FString>& UTSSHumanComponent::Bottoms()
{
	static const TArray<FString> V = { TEXT("jeans"), TEXT("jeansLong"), TEXT("slacks") };
	return V;
}
const TArray<FString>& UTSSHumanComponent::Shoes()
{
	static const TArray<FString> V = { TEXT("loafers"), TEXT("oxfords") };
	return V;
}

USkeletalMeshComponent* UTSSHumanComponent::AddPiece(const TCHAR* Path, bool bFollowLeader)
{
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, Path);
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("TSS human: missing mesh %s"), Path);
		return nullptr;
	}
	USkeletalMeshComponent* C = NewObject<USkeletalMeshComponent>(GetOwner());
	C->SetSkeletalMesh(Mesh);
	C->SetupAttachment(this);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCanEverAffectNavigation(false);
	C->RegisterComponent();
	if (bFollowLeader && Leader) C->SetLeaderPoseComponent(Leader);
	Pieces.Add(C);
	return C;
}

void UTSSHumanComponent::Build(int32 Identity, const FString& Top, const FString& Bottom, const FString& ShoesName)
{
	if (bBuilt) return;
	bBuilt = true;
	Identity = FMath::Clamp(Identity, 1, 2);

	// The leader is the bare skeleton: it plays the animation, everything else copies its pose.
	Leader = AddPiece(*MeshPath(TEXT("base")), false);
	if (!Leader) return;
	Leader->SetVisibility(false);
	Leader->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	// No Animation Blueprint: UTSSHumanAnimInstance builds the pose (locomotion blend plus arm / head IK) in code.
	Leader->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Leader->SetAnimInstanceClass(UTSSHumanAnimInstance::StaticClass());

	AddPiece(*MeshPath(TEXT("body")), true);
	AddPiece(*MeshPath(Top), true);
	AddPiece(*MeshPath(Bottom), true);
	AddPiece(*MeshPath(ShoesName), true);

	const FString Id = FString::Printf(TEXT("m_%03d"), Identity);
	USkeletalMeshComponent* Face = AddPiece(*FString::Printf(TEXT("%sMale/%s/Face/%s_nrw_FaceMesh.%s_nrw_FaceMesh"), Kit, *Id, *Id, *Id), true);

	// Hair and beard are strand grooms glued to the face mesh.
	auto AddGroom = [&](const TCHAR* Groom, const TCHAR* Binding)
	{
		if (!Face) return;
		UGroomAsset* G = LoadObject<UGroomAsset>(nullptr, *FString::Printf(TEXT("%sMale/%s/Hair/Hair/%s.%s"), Kit, *Id, Groom, Groom));
		UGroomBindingAsset* B = LoadObject<UGroomBindingAsset>(nullptr, *FString::Printf(TEXT("%sMale/%s/Hair/GroomBinding/%s.%s"), Kit, *Id, Binding, Binding));
		if (!G) return;
		UGroomComponent* GC = NewObject<UGroomComponent>(GetOwner());
		GC->SetupAttachment(Face);
		GC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GC->SetGroomAsset(G, B);
		GC->RegisterComponent();
		Pieces.Add(GC);
	};
	if (Identity == 1)
	{
		AddGroom(TEXT("Hair_S_AfroFade"), TEXT("Hair_S_AfroFade_m_head_Archetype_Binding"));
		AddGroom(TEXT("Beard_S_Stubble"), TEXT("Beard_S_Stubble_m_head_Archetype_Binding"));
	}
	else
	{
		AddGroom(TEXT("Hair_S_CurlyFade"), TEXT("Hair_S_CurlyFade_m_med_nrw_head_skmesh_Face_Archetype_Binding"));
		AddGroom(TEXT("Beard_S_Full"), TEXT("Beard_S_Full_m_head_Archetype_Binding"));
	}
}

UTSSHumanAnimInstance* UTSSHumanComponent::AnimInst() const
{
	return Leader ? Cast<UTSSHumanAnimInstance>(Leader->GetAnimInstance()) : nullptr;
}

void UTSSHumanComponent::SetSpeed(float CmPerSecond)
{
	if (UTSSHumanAnimInstance* A = AnimInst()) A->SetSpeed(CmPerSecond);
}

void UTSSHumanComponent::Reach(ETSSHand Hand, const FVector& WorldTarget, float BlendSeconds)
{
	if (UTSSHumanAnimInstance* A = AnimInst()) A->SetReach(Hand, true, WorldTarget, BlendSeconds);
}

void UTSSHumanComponent::Relax(ETSSHand Hand, float BlendSeconds)
{
	if (UTSSHumanAnimInstance* A = AnimInst()) A->SetReach(Hand, false, FVector::ZeroVector, BlendSeconds);
}

float UTSSHumanComponent::ReachAlpha(ETSSHand Hand) const
{
	const UTSSHumanAnimInstance* A = AnimInst();
	return A ? A->GetReachAlpha(Hand) : 0.f;
}

void UTSSHumanComponent::LookAt(const FVector& WorldTarget)
{
	if (UTSSHumanAnimInstance* A = AnimInst()) A->SetLook(true, WorldTarget);
}

void UTSSHumanComponent::SetLean(float Degrees)
{
	if (UTSSHumanAnimInstance* A = AnimInst()) A->SetLean(Degrees);
}

void UTSSHumanComponent::SetPointing(ETSSHand Hand, bool bOn)
{
	if (UTSSHumanAnimInstance* A = AnimInst()) A->SetPointing(Hand, bOn);
}

void UTSSHumanComponent::StopLooking()
{
	if (UTSSHumanAnimInstance* A = AnimInst()) A->SetLook(false, FVector::ZeroVector);
}

FVector UTSSHumanComponent::GetHandWorldLocation(ETSSHand Hand) const
{
	if (Leader) return Leader->GetSocketLocation(Hand == ETSSHand::Right ? TEXT("hand_r") : TEXT("hand_l"));
	return GetComponentLocation();
}

void UTSSHumanComponent::SetHiddenFromOwner(bool bHidden)
{
	for (UPrimitiveComponent* P : Pieces)
	{
		if (P) P->SetOwnerNoSee(bHidden);
	}
}

void UTSSHumanComponent::SetShown(bool bShown)
{
	for (UPrimitiveComponent* P : Pieces)
	{
		if (P && P != Leader) P->SetVisibility(bShown);
	}
}
