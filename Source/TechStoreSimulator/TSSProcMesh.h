#pragma once

#include "CoreMinimal.h"

class UProceduralMeshComponent;
class UMaterialInterface;

/**
 * Builds detailed models out of code: boxes, cylinders, spheres, lathed (turned) shapes and lofted bodies, with smooth
 * normals where they matter. Everything added goes into a numbered section (one material per section); Commit() hands the
 * result to a procedural mesh component.
 *
 * Triangles are wound clockwise when seen from the front (Unreal's convention), so normals point out of the surface.
 * Positions are in the local frame of whatever component the result goes into; Push()/Pop() put a transform on everything
 * added in between, so a wheel can be modelled once and placed four times.
 */
class FTSSMeshBuilder
{
public:
	explicit FTSSMeshBuilder(int32 NumSections = 1);

	// ---- transforms ----
	void Push(const FTransform& T);
	void Pop();

	// ---- primitives (all take the section index first) ----
	/** One flat quad, corners clockwise seen from the front. UVs run 0..1 unless Uv0/Uv1 are given. */
	void Quad(int32 Sec, FVector A, FVector B, FVector C, FVector D, FVector2D Uv0 = FVector2D(0, 0), FVector2D Uv1 = FVector2D(1, 1));
	/** Axis-aligned box about a centre. If bTiled, UVs are in world cm / UvCm so a texture repeats along it. */
	void Box(int32 Sec, FVector Centre, FVector HalfSize, float UvCm = 0.f);
	/** A box from point A to point B with the given cross-section (a beam, pillar, strip). Up hints the roll. */
	void Beam(int32 Sec, FVector A, FVector B, float WidthA, float WidthB, float ThickA, float ThickB, FVector Up = FVector::UpVector);
	/** Upright cylinder (axis +Z) from the base centre; radii may differ for a cone. */
	void Cylinder(int32 Sec, FVector Base, float RBottom, float RTop, float Height, int32 Segs = 16, bool bCapBottom = true, bool bCapTop = true);
	/** A turned shape: Profile is (radius, height) pairs from bottom to top, spun about +Z. Smooth normals. */
	void Lathe(int32 Sec, FVector Base, const TArray<FVector2D>& Profile, int32 Segs = 20, bool bCloseBottom = false, bool bCloseTop = false);
	/** Ellipsoid. Radii per axis. */
	void Sphere(int32 Sec, FVector Centre, FVector Radii, int32 Rings = 10, int32 Segs = 16);
	/**
	 * Joins a list of rings (each a closed loop of points, same count) into a smooth skin. Normals are averaged from the
	 * neighbours. Rings run front to back; the loop direction decides which side is out (flip with bFlip if it faces in).
	 */
	void Loft(int32 Sec, const TArray<TArray<FVector>>& Rings, bool bCapStart = true, bool bCapEnd = true, bool bFlip = false);
	/** A flat disc / polygon fan around a centre, facing Normal. */
	void Disc(int32 Sec, FVector Centre, FVector Normal, float Radius, int32 Segs = 16);

	// ---- results ----
	int32 NumVertices() const;
	int32 NumTriangles() const;
	/** Creates the sections on the component, one material each (null = engine default). */
	void Commit(UProceduralMeshComponent* Mesh, const TArray<UMaterialInterface*>& Materials, bool bCollision = false) const;

private:
	struct FSection
	{
		TArray<FVector> V;
		TArray<int32> I;
		TArray<FVector> N;
		TArray<FVector2D> UV;
		TArray<FLinearColor> C;
	};
	int32 AddV(FSection& S, const FVector& P, const FVector& N, const FVector2D& Uv);
	FVector X(const FVector& P) const;
	FVector XN(const FVector& N) const;

	TArray<FSection> Sections;
	TArray<FTransform> Stack;
};

namespace TSSTown
{
	/** A shared plain-colour material (glossy paint, rubber, chrome...). Cached for the session. */
	UMaterialInterface* Paint(const FLinearColor& Color);
	/** A shared glowing material: headlights, lit windows. Falls back to a bright plain colour if the emissive material is missing. */
	UMaterialInterface* Glow(const FLinearColor& Color);
}
