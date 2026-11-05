#include "TSSRegister.h"
#include "TSSCustomer.h"
#include "TSSCharacter.h"
#include "TSSProducts.h"
#include "Components/StaticMeshComponent.h"

ATSSRegister::ATSSRegister()
{
	Counter = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Counter"));
	SetRootComponent(Counter);
	Counter->SetStaticMesh(TSSCubeMesh());
	Counter->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	Counter->SetRelativeScale3D(FVector(0.7f, 2.6f, 1.0f));

	Terminal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Terminal"));
	Terminal->SetupAttachment(Counter);
	Terminal->SetStaticMesh(TSSCubeMesh());
	Terminal->SetRelativeLocation(FVector(0.f, -0.18f, 0.62f));
	Terminal->SetRelativeScale3D(FVector(0.45f, 0.14f, 0.24f));

	Screen = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Counter);
	Screen->SetStaticMesh(TSSCubeMesh());
	Screen->SetRelativeLocation(FVector(0.f, -0.18f, 0.9f));
	Screen->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
	Screen->SetRelativeScale3D(FVector(0.08f, 0.11f, 0.2f));
}

void ATSSRegister::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	TSSApplyColor(Counter, FLinearColor(0.35f, 0.25f, 0.18f));
	TSSApplyColor(Terminal, FLinearColor(0.08f, 0.08f, 0.1f));
	TSSApplyColor(Screen, FLinearColor(0.1f, 0.8f, 0.5f));
}

ATSSCustomer* ATSSRegister::Front() const
{
	return Queue.Num() > 0 ? Queue[0].Get() : nullptr;
}

FVector ATSSRegister::QueueSpot(int32 Index) const
{
	// Customers stand on the shop side (-X in the register's own frame), a body-width apart.
	return GetActorTransform().TransformPosition(FVector(-80.f - 70.f * Index, 0.f, 0.f)) * FVector(1.f, 1.f, 0.f);
}

int32 ATSSRegister::Join(ATSSCustomer* Customer)
{
	Queue.RemoveAll([](const TWeakObjectPtr<ATSSCustomer>& C) { return !C.IsValid(); });
	Queue.AddUnique(Customer);
	return Queue.Num() - 1;
}

void ATSSRegister::Leave(ATSSCustomer* Customer)
{
	Queue.RemoveAll([Customer](const TWeakObjectPtr<ATSSCustomer>& C) { return !C.IsValid() || C.Get() == Customer; });
}

int32 ATSSRegister::IndexOf(const ATSSCustomer* Customer) const
{
	for (int32 i = 0; i < Queue.Num(); ++i)
	{
		if (Queue[i].Get() == Customer) return i;
	}
	return INDEX_NONE;
}

bool ATSSRegister::Checkout()
{
	ATSSCustomer* C = Front();
	if (!C || !C->IsReadyToPay()) return false;
	C->CompleteSale();
	return true;
}

FString ATSSRegister::GetPrompt(const ATSSCharacter* Player, const FHitResult& Hit) const
{
	const ATSSCustomer* C = Front();
	if (!C) return TEXT("Register\nNo customers");
	if (!C->IsReadyToPay()) return TEXT("Register\nA customer is walking up...");
	return FString::Printf(TEXT("[E] Ring up %s  ($%d)"), TSSProductDef(C->GetCarried()).Name, C->GetAgreedPrice());
}

void ATSSRegister::Interact(ATSSCharacter* Player, const FHitResult& Hit)
{
	Checkout();
}
