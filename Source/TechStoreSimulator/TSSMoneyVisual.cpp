#include "TSSMoneyVisual.h"
#include "TSSMoney.h"
#include "TSSProducts.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** One material per kind of money, kept alive for the session. */
	UMaterialInstanceDynamic* MoneyMid(int32 Index)
	{
		static UMaterialInstanceDynamic* Cache[TSSMoney::NumDenoms] = {};
		Index = FMath::Clamp(Index, 0, TSSMoney::NumDenoms - 1);
		if (Cache[Index] && Cache[Index]->IsValidLowLevel()) return Cache[Index];

		const TSSMoney::FDenom& Def = TSSMoney::Denom(Index);
		UMaterialInterface* ArtBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TSS/Materials/M_Packaging.M_Packaging"));
		const FString Tex = Def.Texture;
		UTexture2D* Art = Tex.IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/TSS/Textures/%s.%s"), *Tex, *Tex));
		UMaterialInstanceDynamic* M = nullptr;
		if (ArtBase && Art)
		{
			M = UMaterialInstanceDynamic::Create(ArtBase, GetTransientPackage());
			M->SetTextureParameterValue(TEXT("Pack"), Art);
		}
		else if (UMaterialInterface* Plain = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			M = UMaterialInstanceDynamic::Create(Plain, GetTransientPackage());
			M->SetVectorParameterValue(TEXT("Color"), Def.Tint);
		}
		if (M) M->AddToRoot();
		Cache[Index] = M;
		return M;
	}
}

UMaterialInterface* TSSMoneyArt::Material(int32 DenomIndex) { return MoneyMid(DenomIndex); }

UStaticMesh* TSSMoneyArt::CylinderMesh()
{
	static TWeakObjectPtr<UStaticMesh> Cached;
	if (!Cached.IsValid()) Cached = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	return Cached.Get();
}

float TSSMoneyArt::PieceHeight(int32 DenomIndex)
{
	const TSSMoney::FDenom& D = TSSMoney::Denom(DenomIndex);
	return D.bCoin ? FMath::Max(0.35f, D.SizeB * CoinScale) : 0.14f;
}

void TSSMoneyArt::MakePiece(UStaticMeshComponent* Mesh, int32 DenomIndex)
{
	const TSSMoney::FDenom& D = TSSMoney::Denom(DenomIndex);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	if (D.bCoin)
	{
		Mesh->SetStaticMesh(CylinderMesh());
		const float Dia = D.SizeA * CoinScale;
		Mesh->SetRelativeScale3D(FVector(Dia / 100.f, Dia / 100.f, PieceHeight(DenomIndex) / 100.f));
	}
	else
	{
		Mesh->SetStaticMesh(TSSCubeMesh());
		Mesh->SetRelativeScale3D(FVector(D.SizeA * NoteScale / 100.f, D.SizeB * NoteScale / 100.f, PieceHeight(DenomIndex) / 100.f));
	}
	Mesh->SetMaterial(0, MoneyMid(DenomIndex));
}

// ---- layer ----------------------------------------------------------------------------------------------

UStaticMeshComponent* UTSSMoneyLayer::Take(int32 Index)
{
	while (Pool.Num() <= Index)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None, RF_Transient);
		C->SetupAttachment(this);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->RegisterComponent();
		Pool.Add(C);
	}
	return Pool[Index];
}

void UTSSMoneyLayer::SetPieces(const TArray<int32>& Pieces)
{
	// How many of each kind.
	int32 Count[TSSMoney::NumDenoms] = {};
	for (int32 P : Pieces) ++Count[FMath::Clamp(P, 0, TSSMoney::NumDenoms - 1)];

	int32 Used = 0;
	float Cursor = 0.f;   // piles run along +Y, one after another
	for (int32 d = 0; d < TSSMoney::NumDenoms; ++d)
	{
		if (Count[d] == 0) continue;
		const TSSMoney::FDenom& Def = TSSMoney::Denom(d);
		const int32 Visible = FMath::Min(Count[d], Def.bCoin ? 12 : 10);
		// A loose pile: each note a little off the one below, so it reads as handled money.
		FRandomStream Jitter(d * 977 + Count[d]);
		const float Pitch = Def.bCoin ? Def.SizeA * TSSMoneyArt::CoinScale + 0.6f : Def.SizeB * TSSMoneyArt::NoteScale + 1.8f;
		const float Y = Cursor + Pitch * 0.5f;
		for (int32 i = 0; i < Visible; ++i)
		{
			UStaticMeshComponent* M = Take(Used++);
			TSSMoneyArt::MakePiece(M, d);
			M->SetVisibility(true);
			const float H = TSSMoneyArt::PieceHeight(d);
			if (Def.bCoin)
			{
				M->SetRelativeLocation(FVector(0.f, Y, H * (i + 0.5f)));
				M->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
			}
			else
			{
				M->SetRelativeLocation(FVector(Jitter.FRandRange(-0.8f, 0.8f), Y + Jitter.FRandRange(-0.5f, 0.5f), H * (i + 0.5f) + 0.05f));
				M->SetRelativeRotation(FRotator(0.f, Jitter.FRandRange(-5.f, 5.f), 0.f));
			}
		}
		Cursor += Pitch;
	}
	for (int32 i = Used; i < Pool.Num(); ++i) Pool[i]->SetVisibility(false);
	Shown = Used;
}
