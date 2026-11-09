#pragma once

#include "CoreMinimal.h"

class UProceduralMeshComponent;

/** Detailed street furniture built from code (trees), so the street does not read as primitives. */
namespace TSSStreet
{
	/** A street tree with its iron-grated pit at the origin: tapered trunk with a root flare, forked branches, and a lumpy crown made of many leaf clumps in three greens. */
	void BuildTree(UProceduralMeshComponent* Mesh, int32 Seed, float TrunkHeight);
}
