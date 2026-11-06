#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TSSMoneyVisual.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * How money looks in the shop. Notes and coins are drawn larger than life (notes about 1.6 times real size) and
 * only ever shown on their front: they are props for a game, not reproductions of real currency.
 */
namespace TSSMoneyArt
{
	/** Notes are drawn this many times larger than real (so no note is ever a life-size copy), coins this many. */
	constexpr float NoteScale = 1.6f;
	constexpr float CoinScale = 1.5f;
	/** Material for one kind of note or coin (index into the TSSMoney table); falls back to a plain tint if the art is missing. */
	UMaterialInterface* Material(int32 DenomIndex);
	UStaticMesh* CylinderMesh();
	/** Sets a mesh component up as one note (flat, lying along local X) or one coin (a flat disc). */
	void MakePiece(UStaticMeshComponent* Mesh, int32 DenomIndex);
	/** Height of one note / coin in cm, for stacking. */
	float PieceHeight(int32 DenomIndex);
}

/**
 * A handful of money lying on a surface: notes in a pile for each value, coins in little stacks.
 * Origin is the corner of the layout; piles run along +Y. Built from pooled components, so changing it every
 * few frames costs nothing.
 */
UCLASS()
class UTSSMoneyLayer : public USceneComponent
{
	GENERATED_BODY()

public:
	/** Lays out the pieces (TSSMoney table indices). */
	void SetPieces(const TArray<int32>& Pieces);
	void Clear() { SetPieces(TArray<int32>()); }
	int32 NumShown() const { return Shown; }

private:
	UStaticMeshComponent* Take(int32 Index);

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Pool;
	int32 Shown = 0;
};
