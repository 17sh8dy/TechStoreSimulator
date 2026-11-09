#include "TSSStreetProps.h"
#include "TSSProcMesh.h"
#include "ProceduralMeshComponent.h"
#include "Algo/Reverse.h"

void TSSStreet::BuildTree(UProceduralMeshComponent* Mesh, int32 Seed, float H)
{
	FRandomStream R(Seed * 7919 + 13);
	FTSSMeshBuilder B(6);   // 0 bark, 1 leaves (inner/dark), 2 leaves (mid), 3 leaves (sunlit), 4 iron, 5 soil

	// Grate frame, bars and soil in the pit.
	for (float S : { -1.f, 1.f })
	{
		B.Box(4, FVector(S * 56.f, 0.f, 2.5f), FVector(4.f, 60.f, 2.5f));
		B.Box(4, FVector(0.f, S * 56.f, 2.5f), FVector(60.f, 4.f, 2.5f));
	}
	for (float O : { -30.f, -10.f, 10.f, 30.f })
	{
		B.Box(4, FVector(O, 0.f, 1.8f), FVector(1.2f, 52.f, 1.2f));
		B.Box(4, FVector(0.f, O, 1.8f), FVector(52.f, 1.2f, 1.2f));
	}
	B.Box(5, FVector(0.f, 0.f, 1.f), FVector(52.f, 52.f, 1.f));

	// Trunk: flared at the ground, tapering, slightly knobbly.
	B.Lathe(0, FVector(0.f, 0.f, 3.f), {
		FVector2D(24.f, 0.f), FVector2D(19.f, 7.f), FVector2D(15.5f, 26.f), FVector2D(14.5f, H * 0.2f), FVector2D(15.2f, H * 0.3f),
		FVector2D(13.f, H * 0.45f), FVector2D(11.5f, H * 0.62f), FVector2D(9.5f, H * 0.82f), FVector2D(8.f, H) }, 14, false, true);

	// Branches fork out of the upper trunk and reach into the crown.
	const int32 NumBranches = R.RandRange(5, 7);
	const float Phase = R.FRandRange(0.f, 6.28f);
	const float RxyCrown = R.FRandRange(150.f, 205.f), RzCrown = R.FRandRange(120.f, 165.f);
	const FVector Crown(0.f, 0.f, H + RzCrown * 0.42f);
	for (int32 i = 0; i < NumBranches; ++i)
	{
		const float A = Phase + i * (6.28318f / NumBranches) + R.FRandRange(-0.3f, 0.3f);
		const float Z0 = H * R.FRandRange(0.6f, 0.86f);
		const float Len = R.FRandRange(60.f, 120.f), Rise = R.FRandRange(70.f, 130.f);
		const FVector A0(FMath::Cos(A) * 5.f, FMath::Sin(A) * 5.f, Z0 + 3.f);
		const FVector A1(FMath::Cos(A) * Len, FMath::Sin(A) * Len, Z0 + Rise);
		B.Beam(0, A0, A1, 8.f, 3.5f, 8.f, 3.5f);
		const float A2 = A + R.FRandRange(-0.6f, 0.6f);
		B.Beam(0, A0 + (A1 - A0) * 0.55f, A1 + FVector(FMath::Cos(A2) * 35.f, FMath::Sin(A2) * 35.f, 45.f), 4.5f, 2.f, 4.5f, 2.f);
	}

	// Crown: a dark core (no see-through gaps) and many overlapping clumps, lighter towards the sun.
	B.Sphere(1, Crown, FVector(RxyCrown * 0.55f, RxyCrown * 0.55f, RzCrown * 0.5f), 7, 12);
	const int32 NumBlobs = 26;
	for (int32 i = 0; i < NumBlobs; ++i)
	{
		FVector D;
		do { D = FVector(R.FRandRange(-1.f, 1.f), R.FRandRange(-1.f, 1.f), R.FRandRange(-1.f, 1.f)); } while (D.SizeSquared() > 1.f || D.SizeSquared() < 0.06f);
		const FVector P = Crown + FVector(D.X * RxyCrown * 0.78f, D.Y * RxyCrown * 0.78f, D.Z * RzCrown * 0.8f);
		const float Rad = R.FRandRange(48.f, 88.f);
		const float T = (D.Z + 1.f) * 0.5f + R.FRandRange(-0.15f, 0.15f);
		const int32 Sec = T > 0.66f ? 3 : (T > 0.34f ? 2 : 1);
		B.Sphere(Sec, P, FVector(Rad, Rad, Rad * R.FRandRange(0.7f, 0.9f)), 6, 10);
	}

	// Leaf colour varies per tree.
	const float G = R.FRandRange(0.2f, 0.36f), Y = R.FRandRange(0.f, 0.14f);
	const FLinearColor Dark(0.04f + Y * 0.3f, G * 0.55f, 0.03f), Mid(0.07f + Y * 0.6f, G * 0.8f, 0.04f), Light(0.14f + Y, G * 1.05f, 0.05f);
	B.Commit(Mesh, { TSSTown::Paint(FLinearColor(0.16f, 0.11f, 0.075f)), TSSTown::Paint(Dark), TSSTown::Paint(Mid), TSSTown::Paint(Light),
		TSSTown::Paint(FLinearColor(0.07f, 0.075f, 0.08f)), TSSTown::Paint(FLinearColor(0.1f, 0.07f, 0.045f)) });
}
