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

	IdleAnim = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("%sAnims/Loco/MTN_N_Idle.MTN_N_Idle"), Kit));
	WalkAnim = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("%sAnims/Loco/MTN_N_Walk_InPlace.MTN_N_Walk_InPlace"), Kit));

	// The leader is the bare skeleton: it plays the animation, everything else copies its pose.
	Leader = AddPiece(*MeshPath(TEXT("base")), false);
	if (!Leader) return;
	Leader->SetVisibility(false);
	Leader->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Leader->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	if (IdleAnim) Leader->PlayAnimation(IdleAnim, true);
	// Desynchronise a crowd so everybody does not breathe in time.
	Leader->SetPosition(FMath::FRandRange(0.f, 5.f));

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

void UTSSHumanComponent::SetWalking(bool bWalking)
{
	if (!Leader || bWalking == bWalkingNow) return;
	bWalkingNow = bWalking;
	if (UAnimSequence* Anim = bWalking ? WalkAnim.Get() : IdleAnim.Get())
	{
		Leader->PlayAnimation(Anim, true);
	}
}

void UTSSHumanComponent::SetHiddenFromOwner(bool bHidden)
{
	for (UPrimitiveComponent* P : Pieces)
	{
		if (P) P->SetOwnerNoSee(bHidden);
	}
}
