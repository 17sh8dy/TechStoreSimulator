#include "TSSProducts.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

const FTSSProductDef& TSSProductDef(ETSSProduct Product)
{
	// Name, wholesale cost, box (depth, width, height), box colour, blurb, category, brand, packaging texture, level.
	static const FTSSProductDef Defs[] = {
		{ TEXT("None"), 0, FVector(1, 1, 1), FLinearColor::Black },
		{ TEXT("Smartphone"), 450, FVector(6, 9, 17), FLinearColor(0.16f, 0.2f, 0.62f),
			TEXT("Pocket computer with a camera. Small, easy to stock, always in demand."), TEXT("Phones"), TEXT("LUMIO"), TEXT("T_Pack_Phone"), 1 },
		{ TEXT("Laptop"), 700, FVector(6, 36, 25), FLinearColor(0.1f, 0.42f, 0.42f),
			TEXT("Portable workstation. High price, high margin, but the boxes take shelf space."), TEXT("Computers"), TEXT("ZENTO"), TEXT("T_Pack_Laptop"), 1 },
		{ TEXT("Headphones"), 120, FVector(10, 19, 22), FLinearColor(0.12f, 0.08f, 0.05f),
			TEXT("Over-ear audio. An impulse buy, tempting next to the checkout."), TEXT("Audio"), TEXT("VEXA"), TEXT("T_Pack_Headphones"), 1 },
		{ TEXT("Console"), 350, FVector(12, 36, 28), FLinearColor(0.36f, 0.12f, 0.5f),
			TEXT("Home gaming console. Popular, mid margin, bulky box."), TEXT("Gaming"), TEXT("KYRA"), TEXT("T_Pack_Console"), 1 },
		{ TEXT("Monitor"), 200, FVector(10, 46, 34), FLinearColor(0.1f, 0.42f, 0.42f),
			TEXT("Desktop display. A steady seller for home offices."), TEXT("Displays"), TEXT("ZENTO"), TEXT("T_Pack_Monitor"), 1 },
		{ TEXT("Keyboard"), 60, FVector(6, 44, 15), FLinearColor(0.14f, 0.46f, 0.24f),
			TEXT("Mechanical keyboard. Cheap, quick to sell, low margin."), TEXT("Accessories"), TEXT("TERNO"), TEXT("T_Pack_Keyboard"), 1 },
		{ TEXT("Tablet"), 350, FVector(5, 22, 28), FLinearColor(0.16f, 0.2f, 0.62f),
			TEXT("Ten-inch tablet with a stylus. A family favourite with a healthy margin."), TEXT("Computers"), TEXT("LUMIO"), TEXT("T_Pack_Tablet"), 2 },
		{ TEXT("Earbuds"), 80, FVector(4, 7, 11), FLinearColor(0.12f, 0.08f, 0.05f),
			TEXT("True wireless earbuds. Tiny box, great for small shelves and impulse buys."), TEXT("Audio"), TEXT("VEXA"), TEXT("T_Pack_Earbuds"), 1 },
		{ TEXT("Smartwatch"), 220, FVector(7, 10, 12), FLinearColor(0.36f, 0.12f, 0.5f),
			TEXT("Fitness and notifications on your wrist. A strong seller once word gets around."), TEXT("Wearables"), TEXT("KYRA"), TEXT("T_Pack_Smartwatch"), 3 },
		{ TEXT("Speaker"), 95, FVector(14, 14, 22), FLinearColor(0.14f, 0.46f, 0.24f),
			TEXT("Waterproof portable speaker. Sells well in the warm months."), TEXT("Audio"), TEXT("TERNO"), TEXT("T_Pack_Speaker"), 2 },
			// Premium Supplier Access .
			{ TEXT("Gaming Laptop"), 900, FVector(8, 44, 32), FLinearColor(0.5f, 0.06f, 0.08f),
				TEXT("A high-refresh laptop with a serious graphics card. Big ticket, big margin."), TEXT("Computers"), TEXT("ZENTO"), TEXT("T_Pack_GamingLaptop"), 6, TEXT("supplier_premium") },
			{ TEXT("VR Headset"), 350, FVector(16, 28, 28), FLinearColor(0.08f, 0.1f, 0.14f),
				TEXT("Standalone virtual-reality headset with two controllers."), TEXT("Wearables"), TEXT("KYRA"), TEXT("T_Pack_VRHeadset"), 6, TEXT("supplier_premium") },
			{ TEXT("Mirrorless Camera"), 780, FVector(14, 22, 18), FLinearColor(0.2f, 0.2f, 0.22f),
				TEXT("Interchangeable-lens camera for people who outgrew their phone."), TEXT("Photography"), TEXT("LUMIO"), TEXT("T_Pack_Camera"), 7, TEXT("supplier_premium") },
			// Gaming Department
			{ TEXT("Game Controller"), 50, FVector(8, 18, 16), FLinearColor(0.36f, 0.12f, 0.5f),
				TEXT("Wireless gamepad. Cheap, and everyone who buys a console wants a second one."), TEXT("Gaming"), TEXT("KYRA"), TEXT("T_Pack_Controller"), 4, TEXT("dept_gaming") },
			{ TEXT("Gaming Headset"), 80, FVector(14, 22, 24), FLinearColor(0.5f, 0.08f, 0.3f),
				TEXT("Headset with a boom mic and loud opinions about team chat."), TEXT("Gaming"), TEXT("VEXA"), TEXT("T_Pack_GamingHeadset"), 4, TEXT("dept_gaming") },
			{ TEXT("Racing Wheel"), 260, FVector(24, 44, 40), FLinearColor(0.08f, 0.1f, 0.14f),
				TEXT("Force-feedback wheel and pedals. A bulky box that gamers happily pay for."), TEXT("Gaming"), TEXT("TERNO"), TEXT("T_Pack_RacingWheel"), 5, TEXT("dept_gaming") },
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
