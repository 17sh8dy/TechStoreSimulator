#include "TSSBlock.h"
#include "TSSProducts.h"
#include "Components/StaticMeshComponent.h"

ATSSBlock::ATSSBlock()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetStaticMesh(TSSCubeMesh());
	Mesh->SetMobility(EComponentMobility::Static);
}

void ATSSBlock::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void ATSSBlock::Rebuild()
{
	Mesh->SetWorldScale3D(Size / 100.f);
	TSSApplyColor(Mesh, Color);
}
