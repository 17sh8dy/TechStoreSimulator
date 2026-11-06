#include "TSSShelf.h"
#include "TSSCharacter.h"
#include "TSSProductBox.h"
#include "TSSProductItem.h"
#include "TSSStoreSubsystem.h"
#include "TSSBusinessSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"

namespace
{
	constexpr float TierZ[ATSSShelf::Tiers] = { 12.f, 72.f, 132.f };
	constexpr float ColumnY[ATSSShelf::Columns] = { -75.f, -25.f, 25.f, 75.f };
	constexpr float BoardThickness = 3.f;
	constexpr float FrontEdge = 24.f;

	const FLinearColor ShelfGraphite(0.05f, 0.055f, 0.07f);
	const FLinearColor ShelfBoard(0.78f, 0.8f, 0.84f);
	const FLinearColor ShelfAccent(0.06f, 0.3f, 0.95f);
}

ATSSShelf::ATSSShelf()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	auto AddPart = [this, Root](const TCHAR* Name, FVector Loc, FVector Size, FLinearColor Color)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Root);
		Part->SetStaticMesh(TSSCubeMesh());
		Part->SetRelativeLocation(Loc);
		Part->SetRelativeScale3D(Size / 100.f);
		FrameParts.Add(Part);
		FrameColors.Add(Color);
		return Part;
	};
	// Graphite frame, light boards with a blue front edge, a lit header sign on top.
	AddPart(TEXT("Back"), FVector(-24.f, 0.f, 100.f), FVector(2.f, 204.f, 200.f), FLinearColor(0.11f, 0.12f, 0.15f));
	AddPart(TEXT("Left"), FVector(0.f, -102.f, 100.f), FVector(50.f, 4.f, 200.f), ShelfGraphite);
	AddPart(TEXT("Right"), FVector(0.f, 102.f, 100.f), FVector(50.f, 4.f, 200.f), ShelfGraphite);
	AddPart(TEXT("Top"), FVector(0.f, 0.f, 201.f), FVector(50.f, 204.f, 4.f), ShelfGraphite);
	for (int32 t = 0; t < Tiers; ++t)
	{
		const FString N = FString::FromInt(t);
		AddPart(*(TEXT("Tier") + N), FVector(0.f, 0.f, TierZ[t]), FVector(48.f, 200.f, BoardThickness), ShelfBoard);
		AddPart(*(TEXT("Edge") + N), FVector(FrontEdge + 0.2f, 0.f, TierZ[t] + 0.2f), FVector(0.5f, 200.f, 1.4f), ShelfAccent);
	}
	AddPart(TEXT("Header"), FVector(FrontEdge - 1.f, 0.f, 215.f), FVector(2.f, 204.f, 24.f), FLinearColor(0.03f, 0.035f, 0.05f));
	AddPart(TEXT("HeaderBar"), FVector(FrontEdge + 0.2f, 0.f, 204.5f), FVector(0.6f, 204.f, 1.6f), ShelfAccent);

	UTextRenderComponent* Sign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Sign"));
	Sign->SetupAttachment(Root);
	Sign->SetRelativeLocation(FVector(FrontEdge + 0.3f, 0.f, 215.f));
	Sign->SetWorldSize(13.f);
	Sign->SetHorizontalAlignment(EHTA_Center);
	Sign->SetVerticalAlignment(EVRTA_TextCenter);
	Sign->SetTextRenderColor(FColor(230, 240, 255));
	Sign->SetText(FText::FromString(TEXT("ELECTRONICS")));
	Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// One price tag under each slot.
	for (int32 i = 0; i < Tiers * Columns; ++i)
	{
		const FString N = FString::FromInt(i);
		const FVector Tag(FrontEdge + 0.4f, ColumnY[i % Columns], TierZ[i / Columns] - 4.6f);
		UStaticMeshComponent* Board = CreateDefaultSubobject<UStaticMeshComponent>(*(TEXT("TagBoard") + N));
		Board->SetupAttachment(Root);
		Board->SetStaticMesh(TSSCubeMesh());
		Board->SetRelativeLocation(Tag);
		Board->SetRelativeScale3D(FVector(0.006f, 0.4f, 0.075f));
		Board->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Board->SetVisibility(false);
		TagBoards.Add(Board);
		FrameParts.Add(Board);
		FrameColors.Add(FLinearColor(0.95f, 0.95f, 0.9f));

		UTextRenderComponent* Text = CreateDefaultSubobject<UTextRenderComponent>(*(TEXT("TagText") + N));
		Text->SetupAttachment(Root);
		Text->SetRelativeLocation(Tag + FVector(0.5f, 0.f, 0.f));
		Text->SetWorldSize(2.6f);
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetTextRenderColor(FColor(15, 15, 20));
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Text->SetText(FText::GetEmpty());
		TagTexts.Add(Text);
	}

	Slots.SetNum(Tiers * Columns);
}

void ATSSShelf::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	for (int32 i = 0; i < FrameParts.Num() && i < FrameColors.Num(); ++i)
	{
		if (FrameParts[i]) TSSApplyColor(FrameParts[i], FrameColors[i]);
	}
}

void ATSSShelf::BeginPlay()
{
	Super::BeginPlay();
	if (UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>())
	{
		StoreHandle = Store->OnChanged.AddUObject(this, &ATSSShelf::RefreshAllTags);
	}
}

void ATSSShelf::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* World = GetWorld())
	{
		if (UTSSStoreSubsystem* Store = World->GetSubsystem<UTSSStoreSubsystem>()) Store->OnChanged.Remove(StoreHandle);
	}
	Super::EndPlay(Reason);
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

void ATSSShelf::RefreshTag(int32 Index)
{
	if (!TagTexts.IsValidIndex(Index)) return;
	const FSlot& Slot = Slots[Index];
	const UTSSStoreSubsystem* Store = GetWorld() ? GetWorld()->GetSubsystem<UTSSStoreSubsystem>() : nullptr;
	const bool bShow = Slot.Qty > 0 && Store;
	TagBoards[Index]->SetVisibility(bShow);
	TagTexts[Index]->SetVisibility(bShow);
	if (!bShow) return;
	const FString Text = FString::Printf(TEXT("%s\n$%d"), TSSProductDef(Slot.Product).Name, Store->GetPrice(Slot.Product));
	if (!TagTexts[Index]->Text.ToString().Equals(Text)) TagTexts[Index]->SetText(FText::FromString(Text));
}

void ATSSShelf::RefreshAllTags()
{
	for (int32 i = 0; i < Slots.Num(); ++i) RefreshTag(i);
}

void ATSSShelf::RefreshSlot(int32 Index)
{
	FSlot& Slot = Slots[Index];
	const FTSSProductDef& Def = TSSProductDef(Slot.Product);

	while (Slot.Boxes.Num() < Slot.Qty)
	{
		UTSSProductBoxComponent* Box = NewObject<UTSSProductBoxComponent>(this);
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Box->SetupAttachment(GetRootComponent());
		Box->RegisterComponent();
		Slot.Boxes.Add(Box);
	}

	const FVector Origin = SlotOrigin(Index);
	for (int32 i = 0; i < Slot.Boxes.Num(); ++i)
	{
		UTSSProductBoxComponent* Box = Slot.Boxes[i];
		const bool bUsed = i < Slot.Qty;
		Box->SetVisibility(bUsed, true);
		if (!bUsed) continue;
		Box->SetProduct(Slot.Product);
		Box->SetRelativeLocation(FVector(
			Origin.X - Def.BoxSize.X * 0.5f - 1.f - i * (Def.BoxSize.X + 1.5f),
			Origin.Y,
			Origin.Z + Def.BoxSize.Z * 0.5f));
	}
	RefreshTag(Index);
}

FString ATSSShelf::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	if (Player->GetHeldItem()) return TEXT("Scan this item or put it back on the counter");
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
	if (Player->GetHeldItem()) return;
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
	NotifyStockChanged();
}

void ATSSShelf::Adjust(ATSSCharacter* Player, const FHitResult& Hit, int32 Direction)
{
	const FSlot& Slot = Slots[SlotFromHit(Hit)];
	if (Slot.Qty == 0) return;
	UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	Store->SetPrice(Slot.Product, Store->GetPrice(Slot.Product) + Direction);
}

void ATSSShelf::CollectStock(UWorld* World, TArray<FTSSStockRef>& Out)
{
	for (TActorIterator<ATSSShelf> It(World); It; ++It)
	{
		for (int32 i = 0; i < It->Slots.Num(); ++i)
		{
			if (It->Slots[i].Qty > 0) Out.Add({ *It, i });
		}
	}
}

bool ATSSShelf::TakeOne(int32 Slot, ETSSProduct& OutProduct)
{
	FSlot& S = Slots[Slot];
	if (S.Qty <= 0) return false;
	OutProduct = S.Product;
	--S.Qty;
	if (S.Qty == 0) S.Product = ETSSProduct::None;
	RefreshSlot(Slot);
	NotifyStockChanged();
	return true;
}

bool ATSSShelf::PutBack(int32 Slot, ETSSProduct Product)
{
	FSlot& S = Slots[Slot];
	if (S.Qty > 0 && S.Product != Product) return false;
	if (S.Qty >= TSSProductDef(Product).SlotCapacity()) return false;
	S.Product = Product;
	++S.Qty;
	RefreshSlot(Slot);
	NotifyStockChanged();
	return true;
}

FVector ATSSShelf::GetStandPoint(int32 Slot) const
{
	const FVector O = SlotOrigin(Slot);
	return GetActorTransform().TransformPosition(FVector(FrontEdge + 75.f, O.Y, 0.f));
}

FVector ATSSShelf::GetSlotWorldPos(int32 Slot) const
{
	return GetActorTransform().TransformPosition(SlotOrigin(Slot));
}

void ATSSShelf::ExportSlots(TArray<FTSSShelfSlotSave>& Out) const
{
	Out.Reset();
	for (const FSlot& S : Slots)
	{
		FTSSShelfSlotSave& Item = Out.AddDefaulted_GetRef();
		Item.Product = S.Qty > 0 ? S.Product : ETSSProduct::None;
		Item.Qty = S.Qty > 0 ? S.Qty : 0;
	}
}

void ATSSShelf::ImportSlots(const TArray<FTSSShelfSlotSave>& In)
{
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		FSlot& S = Slots[i];
		S.Product = ETSSProduct::None;
		S.Qty = 0;
		if (In.IsValidIndex(i) && In[i].Product != ETSSProduct::None && In[i].Product != ETSSProduct::Count && In[i].Qty > 0)
		{
			S.Product = In[i].Product;
			S.Qty = FMath::Min(In[i].Qty, TSSProductDef(S.Product).SlotCapacity());
		}
		RefreshSlot(i);
	}
}

void ATSSShelf::NotifyStockChanged()
{
	if (UTSSBusinessSubsystem* Biz = GetWorld() ? GetWorld()->GetSubsystem<UTSSBusinessSubsystem>() : nullptr) Biz->NoteStockChanged();
}

FName ATSSShelf::LayoutKey() const
{
	// Shelves bought as upgrades are spawned fresh each session, so they are identified by their upgrade name.
	for (const FName& T : Tags)
	{
		if (T.ToString().StartsWith(TEXT("shelf_"))) return T;
	}
	return GetFName();
}
