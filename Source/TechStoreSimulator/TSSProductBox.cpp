#include "TSSProductBox.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** One shared material per product, kept alive for the whole session. */
	UMaterialInstanceDynamic* BodyMaterial(ETSSProduct P)
	{
		static TMap<ETSSProduct, UMaterialInstanceDynamic*> Cache;
		if (UMaterialInstanceDynamic** Found = Cache.Find(P)) { if (*Found && (*Found)->IsValidLowLevel()) return *Found; }
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (!Base) return nullptr;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, GetTransientPackage());
		M->AddToRoot();
		M->SetVectorParameterValue(TEXT("Color"), TSSProductDef(P).Color);
		Cache.Add(P, M);
		return M;
	}

	UMaterialInstanceDynamic* PackagingMaterial(ETSSProduct P)
	{
		static TMap<ETSSProduct, UMaterialInstanceDynamic*> Cache;
		if (UMaterialInstanceDynamic** Found = Cache.Find(P)) { if (*Found && (*Found)->IsValidLowLevel()) return *Found; }
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TSS/Materials/M_Packaging.M_Packaging"));
		const FString TexName = TSSProductDef(P).Texture;
		UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/TSS/Textures/%s.%s"), *TexName, *TexName));
		if (!Base || !Tex) return nullptr;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, GetTransientPackage());
		M->AddToRoot();
		M->SetTextureParameterValue(TEXT("Pack"), Tex);
		Cache.Add(P, M);
		return M;
	}

	/** Plain colours (bags, fallbacks) are shared by colour. */
	UMaterialInstanceDynamic* PlainMaterial(const FLinearColor& C)
	{
		static TMap<uint32, UMaterialInstanceDynamic*> Cache;
		const uint32 Key = C.ToFColor(true).DWColor();
		if (UMaterialInstanceDynamic** Found = Cache.Find(Key)) { if (*Found && (*Found)->IsValidLowLevel()) return *Found; }
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (!Base) return nullptr;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, GetTransientPackage());
		M->AddToRoot();
		M->SetVectorParameterValue(TEXT("Color"), C);
		Cache.Add(Key, M);
		return M;
	}
}

UTSSProductBoxComponent::UTSSProductBoxComponent()
{
	SetStaticMesh(TSSCubeMesh());
}

void UTSSProductBoxComponent::OnRegister()
{
	Super::OnRegister();
	if (!Panel)
	{
		Panel = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None, RF_Transient);
		Panel->SetStaticMesh(TSSCubeMesh());
		Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Panel->SetCastShadow(false);
		// Born hidden: it only shows once Apply() gives it packaging art. A fresh panel is visible by default and has
		// no material, so an empty hand would otherwise draw a default checkered cube.
		Panel->SetHiddenInGame(true);
		Panel->SetupAttachment(this);
		Panel->RegisterComponent();
	}
	Apply();
}

void UTSSProductBoxComponent::OnUnregister()
{
	if (Panel)
	{
		Panel->DestroyComponent();
		Panel = nullptr;
	}
	Super::OnUnregister();
}

void UTSSProductBoxComponent::SetProduct(ETSSProduct InProduct)
{
	Product = InProduct;
	PlainSize = FVector::ZeroVector;
	Apply();
}

void UTSSProductBoxComponent::SetPlain(const FVector& SizeCm, const FLinearColor& Color)
{
	Product = ETSSProduct::None;
	PlainSize = SizeCm;
	PlainColor = Color;
	Apply();
}

void UTSSProductBoxComponent::Apply()
{
	if (!IsRegistered()) return;
	const bool bPlain = !PlainSize.IsZero();
	if (!bPlain && Product == ETSSProduct::None) return;

	const FVector Size = bPlain ? PlainSize : TSSProductDef(Product).BoxSize;
	SetRelativeScale3D(Size / 100.f);

	if (bPlain)
	{
		SetMaterial(0, PlainMaterial(PlainColor));
		// Hidden-in-game, not Visibility: showing the parent with propagation (SetVisibility(true, true)) would
		// switch a Visibility-hidden panel back on, and it has no material here (a default checkered cube).
		if (Panel) Panel->SetHiddenInGame(true);
		return;
	}

	SetMaterial(0, BodyMaterial(Product));
	if (!Panel) return;
	if (UMaterialInstanceDynamic* Art = PackagingMaterial(Product))
	{
		// A paper-thin panel on the front face carrying the packaging art.
		const float ThicknessCm = 0.35f;
		Panel->SetRelativeLocation(FVector(50.f + 0.5f * ThicknessCm * 100.f / Size.X, 0.f, 0.f));
		Panel->SetRelativeScale3D(FVector(ThicknessCm / Size.X, 1.f, 1.f));
		Panel->SetMaterial(0, Art);
		Panel->SetHiddenInGame(false);
	}
	else
	{
		Panel->SetHiddenInGame(true);
	}
}
