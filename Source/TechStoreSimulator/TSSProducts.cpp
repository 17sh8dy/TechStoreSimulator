#include "TSSProducts.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

const FTSSProductDef& TSSProductDef(ETSSProduct Product)
{
	static const FTSSProductDef Defs[] = {
		{ TEXT("None"),        0,   FVector(1, 1, 1),     FLinearColor::Black },
		{ TEXT("Smartphone"),  300, FVector(6, 9, 17),    FLinearColor(0.10f, 0.65f, 0.95f), TEXT("Pocket computer with a camera. Small, easy to stock, always in demand.") },
		{ TEXT("Laptop"),      700, FVector(6, 34, 24),   FLinearColor(0.70f, 0.72f, 0.78f), TEXT("Portable workstation. High price, high margin, but the boxes take shelf space.") },
		{ TEXT("Headphones"),  120, FVector(16, 18, 20),  FLinearColor(0.85f, 0.15f, 0.25f), TEXT("Over-ear audio. An impulse buy, tempting next to the checkout.") },
		{ TEXT("Console"),     400, FVector(22, 32, 10),  FLinearColor(0.30f, 0.20f, 0.85f), TEXT("Home gaming console. Popular, mid margin, bulky box.") },
		{ TEXT("Monitor"),     250, FVector(10, 40, 30),  FLinearColor(0.12f, 0.12f, 0.14f), TEXT("Desktop display. A steady seller for home offices.") },
		{ TEXT("Keyboard"),    60,  FVector(14, 38, 3),   FLinearColor(0.95f, 0.65f, 0.10f), TEXT("Mechanical keyboard. Cheap, quick to sell, low margin.") },
	};
	const int32 Index = FMath::Clamp(static_cast<int32>(Product), 0, static_cast<int32>(ETSSProduct::Count) - 1);
	return Defs[Index];
}

UStaticMesh* TSSCubeMesh()
{
	static TWeakObjectPtr<UStaticMesh> Cached;
	if (!Cached.IsValid())
	{
		Cached = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	}
	return Cached.Get();
}

void TSSApplyColor(UStaticMeshComponent* Mesh, const FLinearColor& Color)
{
	if (!Mesh) return;
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Base) return;
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, Mesh);
	Mid->SetVectorParameterValue(TEXT("Color"), Color);
	Mesh->SetMaterial(0, Mid);
}
