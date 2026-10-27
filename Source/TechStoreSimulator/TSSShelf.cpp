#include "TSSShelf.h"
#include "TSSCharacter.h"
#include "TSSStoreSubsystem.h"
#include "Components/StaticMeshComponent.h"

namespace
{
	constexpr float TierZ[ATSSShelf::Tiers] = { 12.f, 72.f, 132.f };
	constexpr float ColumnY[ATSSShelf::Columns] = { -75.f, -25.f, 25.f, 75.f };
	constexpr float BoardThickness = 3.f;
	constexpr float FrontEdge = 24.f;
}

ATSSShelf::ATSSShelf()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	auto AddPart = [this, Root](const TCHAR* Name, FVector Loc, FVector Size)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Root);
		Part->SetStaticMesh(TSSCubeMesh());
		Part->SetRelativeLocation(Loc);
		Part->SetRelativeScale3D(Size / 100.f);
		FrameParts.Add(Part);
	};
	AddPart(TEXT("Back"), FVector(-24.f, 0.f, 100.f), FVector(2.f, 204.f, 200.f));
	AddPart(TEXT("Left"), FVector(0.f, -102.f, 100.f), FVector(50.f, 4.f, 200.f));
	AddPart(TEXT("Right"), FVector(0.f, 102.f, 100.f), FVector(50.f, 4.f, 200.f));
	AddPart(TEXT("Top"), FVector(0.f, 0.f, 201.f), FVector(50.f, 204.f, 4.f));
	AddPart(TEXT("Tier0"), FVector(0.f, 0.f, TierZ[0]), FVector(48.f, 200.f, BoardThickness));
	AddPart(TEXT("Tier1"), FVector(0.f, 0.f, TierZ[1]), FVector(48.f, 200.f, BoardThickness));
	AddPart(TEXT("Tier2"), FVector(0.f, 0.f, TierZ[2]), FVector(48.f, 200.f, BoardThickness));

	Slots.SetNum(Tiers * Columns);
}

void ATSSShelf::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	for (UStaticMeshComponent* Part : FrameParts)
	{
		TSSApplyColor(Part, FLinearColor(0.82f, 0.84f, 0.88f));
	}
}

FVector ATSSShelf::SlotOrigin(int32 Index) const
{
	const int32 Tier = Index / Columns;
	const int32 Col = Index % Columns;
	return FVector(FrontEdge, ColumnY[Col], TierZ[Tier] + BoardThickness * 0.5f);
}

int32 ATSSShelf::SlotFromHit(const FHitResult& Hit) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(Hit.ImpactPoint);

	// Looking at a product on a board counts as looking at that board's slot.
	int32 Tier = 0;
	float BestZ = TNumericLimits<float>::Max();
	for (int32 t = 0; t < Tiers; ++t)
	{
		const float D = FMath::Abs(Local.Z - (TierZ[t] + 25.f));
		if (D < BestZ) { BestZ = D; Tier = t; }
	}
	int32 Col = 0;
	float BestY = TNumericLimits<float>::Max();
	for (int32 c = 0; c < Columns; ++c)
	{
		const float D = FMath::Abs(Local.Y - ColumnY[c]);
		if (D < BestY) { BestY = D; Col = c; }
	}
	return Tier * Columns + Col;
}

void ATSSShelf::RefreshSlot(int32 Index)
{
	FSlot& Slot = Slots[Index];
	const FTSSProductDef& Def = TSSProductDef(Slot.Product);

	while (Slot.Boxes.Num() < Slot.Qty)
	{
		UStaticMeshComponent* Box = NewObject<UStaticMeshComponent>(this);
		Box->SetStaticMesh(TSSCubeMesh());
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Box->SetupAttachment(GetRootComponent());
		Box->RegisterComponent();
		Slot.Boxes.Add(Box);
	}

	const FVector Origin = SlotOrigin(Index);
	for (int32 i = 0; i < Slot.Boxes.Num(); ++i)
	{
		UStaticMeshComponent* Box = Slot.Boxes[i];
		const bool bUsed = i < Slot.Qty;
		Box->SetVisibility(bUsed);
		if (!bUsed) continue;
		Box->SetRelativeScale3D(Def.BoxSize / 100.f);
		Box->SetRelativeLocation(FVector(
			Origin.X - Def.BoxSize.X * 0.5f - 1.f - i * (Def.BoxSize.X + 1.5f),
			Origin.Y,
			Origin.Z + Def.BoxSize.Z * 0.5f));
		TSSApplyColor(Box, Def.Color);
	}
}

FString ATSSShelf::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	const FSlot& Slot = Slots[SlotFromHit(Hit)];
	const ETSSProduct Held = Player->GetHeld();
	FString Out;

	if (Held != ETSSProduct::None)
	{
		const FTSSProductDef& HeldDef = TSSProductDef(Held);
		if (Slot.Qty == 0 || Slot.Product == Held)
		{
			Out = (Slot.Qty < HeldDef.SlotCapacity())
				? FString::Printf(TEXT("[E] Stock %s"), HeldDef.Name)
				: FString(TEXT("This slot is full"));
		}
		else
		{
			Out = FString::Printf(TEXT("This slot holds %s"), TSSProductDef(Slot.Product).Name);
		}
	}
	else if (Slot.Qty > 0)
	{
		Out = FString::Printf(TEXT("[E] Take %s"), TSSProductDef(Slot.Product).Name);
	}
	else
	{
		return TEXT("Empty slot");
	}

	if (Slot.Qty > 0)
	{
		const int32 Price = GetWorld()->GetSubsystem<UTSSStoreSubsystem>()->GetPrice(Slot.Product);
		Out += FString::Printf(TEXT("\n%s x%d  |  Price $%d  |  [Mouse Wheel] change price"),
			TSSProductDef(Slot.Product).Name, Slot.Qty, Price);
	}
	return Out;
}

void ATSSShelf::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	const int32 Index = SlotFromHit(Hit);
	FSlot& Slot = Slots[Index];
	const ETSSProduct Held = Player->GetHeld();

	if (Held != ETSSProduct::None)
	{
		if (Slot.Qty > 0 && Slot.Product != Held) return;
		if (Slot.Qty >= TSSProductDef(Held).SlotCapacity()) return;
		Slot.Product = Held;
		++Slot.Qty;
		Player->SetHeld(ETSSProduct::None);
	}
	else if (Slot.Qty > 0)
	{
		Player->SetHeld(Slot.Product);
		--Slot.Qty;
		if (Slot.Qty == 0) Slot.Product = ETSSProduct::None;
	}
	else
	{
		return;
	}
	RefreshSlot(Index);
}

void ATSSShelf::Adjust(ATSSCharacter* Player, const FHitResult& Hit, int32 Direction)
{
	const FSlot& Slot = Slots[SlotFromHit(Hit)];
	if (Slot.Qty == 0) return;
	UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	Store->SetPrice(Slot.Product, Store->GetPrice(Slot.Product) + Direction);
}
