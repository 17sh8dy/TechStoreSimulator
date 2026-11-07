#include "TSSProcMesh.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

FTSSMeshBuilder::FTSSMeshBuilder(int32 NumSections)
{
	Sections.SetNum(FMath::Max(1, NumSections));
}

void FTSSMeshBuilder::Push(const FTransform& T)
{
	Stack.Add(Stack.Num() > 0 ? T * Stack.Last() : T);
}

void FTSSMeshBuilder::Pop()
{
	if (Stack.Num() > 0) Stack.Pop();
}

FVector FTSSMeshBuilder::X(const FVector& P) const { return Stack.Num() > 0 ? Stack.Last().TransformPosition(P) : P; }
FVector FTSSMeshBuilder::XN(const FVector& N) const { return Stack.Num() > 0 ? Stack.Last().TransformVectorNoScale(N).GetSafeNormal() : N; }

int32 FTSSMeshBuilder::AddV(FSection& S, const FVector& P, const FVector& N, const FVector2D& Uv)
{
	S.V.Add(X(P));
	S.N.Add(XN(N));
	S.UV.Add(Uv);
	S.C.Add(FLinearColor::White);
	return S.V.Num() - 1;
}

int32 FTSSMeshBuilder::NumVertices() const { int32 N = 0; for (const FSection& S : Sections) N += S.V.Num(); return N; }
int32 FTSSMeshBuilder::NumTriangles() const { int32 N = 0; for (const FSection& S : Sections) N += S.I.Num() / 3; return N; }

namespace
{
	/** Adds a triangle wound so that it faces the same way as `Want` (the intended outward normal). */
	void Tri(TArray<int32>& I, const TArray<FVector>& V, int32 A, int32 B, int32 C, const FVector& Want)
	{
		const FVector Geo = FVector::CrossProduct(V[B] - V[A], V[C] - V[A]);
		if (FVector::DotProduct(Geo, Want) >= 0.f) { I.Add(A); I.Add(B); I.Add(C); }
		else { I.Add(A); I.Add(C); I.Add(B); }
	}
}

void FTSSMeshBuilder::Quad(int32 Sec, FVector A, FVector B, FVector C, FVector D, FVector2D Uv0, FVector2D Uv1)
{
	FSection& S = Sections[Sec];
	const FVector N = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
	const int32 a = AddV(S, A, N, FVector2D(Uv0.X, Uv1.Y));
	const int32 b = AddV(S, B, N, FVector2D(Uv0.X, Uv0.Y));
	const int32 c = AddV(S, C, N, FVector2D(Uv1.X, Uv0.Y));
	const int32 d = AddV(S, D, N, FVector2D(Uv1.X, Uv1.Y));
	// The quad is given clockwise from the front, so (a,b,c) and (a,c,d) already face along N.
	Tri(S.I, S.V, a, b, c, XN(N));
	Tri(S.I, S.V, a, c, d, XN(N));
}

void FTSSMeshBuilder::Box(int32 Sec, FVector Centre, FVector H, float UvCm)
{
	const FVector Normals[6] = { FVector(1, 0, 0), FVector(-1, 0, 0), FVector(0, 1, 0), FVector(0, -1, 0), FVector(0, 0, 1), FVector(0, 0, -1) };
	for (const FVector& N : Normals)
	{
		const FVector T1 = FMath::Abs(N.Z) < 0.9f ? FVector(0, 0, 1) : FVector(1, 0, 0);
		const FVector T2 = FVector::CrossProduct(T1, N);
		const float H1 = FMath::Abs(FVector::DotProduct(H, T1));
		const float H2 = FMath::Abs(FVector::DotProduct(H, T2));
		const FVector C = Centre + N * FMath::Abs(FVector::DotProduct(H, N));
		const FVector A = C - T1 * H1 - T2 * H2;
		const FVector B = A + T2 * (2.f * H2);
		const FVector Cc = C + T1 * H1 + T2 * H2;
		const FVector D = A + T1 * (2.f * H1);
		if (UvCm > 0.f) Quad(Sec, A, B, Cc, D, FVector2D(0, 0), FVector2D(2.f * H2 / UvCm, 2.f * H1 / UvCm));
		else Quad(Sec, A, B, Cc, D);
	}
}

void FTSSMeshBuilder::Beam(int32 Sec, FVector A, FVector B, float WA, float WB, float TA, float TB, FVector Up)
{
	const FVector F = (B - A).GetSafeNormal();
	FVector S = FVector::CrossProduct(Up, F);
	if (S.SizeSquared() < 1e-4f) S = FVector::CrossProduct(FVector(1, 0, 0), F);
	S.Normalize();
	const FVector U = FVector::CrossProduct(F, S).GetSafeNormal();
	auto Corner = [&](const FVector& P, float W, float T, float sx, float sy) { return P + S * (sx * W * 0.5f) + U * (sy * T * 0.5f); };
	const FVector a[4] = { Corner(A, WA, TA, -1, -1), Corner(A, WA, TA, 1, -1), Corner(A, WA, TA, 1, 1), Corner(A, WA, TA, -1, 1) };
	const FVector b[4] = { Corner(B, WB, TB, -1, -1), Corner(B, WB, TB, 1, -1), Corner(B, WB, TB, 1, 1), Corner(B, WB, TB, -1, 1) };
	const FVector Mid = (A + B) * 0.5f;
	auto Face = [&](FVector P0, FVector P1, FVector P2, FVector P3)
	{
		// Make the quad face away from the beam's middle.
		const FVector N = FVector::CrossProduct(P1 - P0, P2 - P0);
		const FVector C = (P0 + P1 + P2 + P3) * 0.25f;
		if (FVector::DotProduct(N, C - Mid) >= 0.f) Quad(Sec, P0, P1, P2, P3);
		else Quad(Sec, P3, P2, P1, P0);
	};
	for (int32 i = 0; i < 4; ++i) Face(a[i], a[(i + 1) % 4], b[(i + 1) % 4], b[i]);
	Face(a[0], a[1], a[2], a[3]);
	Face(b[0], b[1], b[2], b[3]);
}

void FTSSMeshBuilder::Disc(int32 Sec, FVector Centre, FVector Normal, float R, int32 Segs)
{
	FSection& S = Sections[Sec];
	const FVector N = Normal.GetSafeNormal();
	const FVector T1 = FMath::Abs(N.Z) < 0.9f ? FVector::CrossProduct(N, FVector(0, 0, 1)).GetSafeNormal() : FVector(1, 0, 0);
	const FVector T2 = FVector::CrossProduct(N, T1);
	const int32 C = AddV(S, Centre, N, FVector2D(0.5f, 0.5f));
	TArray<int32> Ring;
	for (int32 i = 0; i < Segs; ++i)
	{
		const float A = 2.f * PI * i / Segs;
		Ring.Add(AddV(S, Centre + (T1 * FMath::Cos(A) + T2 * FMath::Sin(A)) * R, N, FVector2D(0.5f + 0.5f * FMath::Cos(A), 0.5f + 0.5f * FMath::Sin(A))));
	}
	for (int32 i = 0; i < Segs; ++i) Tri(S.I, S.V, C, Ring[i], Ring[(i + 1) % Segs], XN(N));
}

void FTSSMeshBuilder::Lathe(int32 Sec, FVector Base, const TArray<FVector2D>& Profile, int32 Segs, bool bCloseBottom, bool bCloseTop)
{
	if (Profile.Num() < 2) return;
	FSection& S = Sections[Sec];
	const int32 Rows = Profile.Num();
	TArray<int32> Idx;
	Idx.SetNum(Rows * (Segs + 1));
	for (int32 i = 0; i < Rows; ++i)
	{
		const FVector2D Prev = Profile[FMath::Max(0, i - 1)];
		const FVector2D Next = Profile[FMath::Min(Rows - 1, i + 1)];
		FVector2D T = Next - Prev;
		T.Normalize();
		const FVector2D N2(T.Y, -T.X);   // outward in the (radius, height) plane
		for (int32 j = 0; j <= Segs; ++j)
		{
			const float A = 2.f * PI * j / Segs;
			const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
			const FVector P = Base + Dir * Profile[i].X + FVector(0, 0, Profile[i].Y);
			const FVector N = (Dir * N2.X + FVector(0, 0, N2.Y)).GetSafeNormal();
			Idx[i * (Segs + 1) + j] = AddV(S, P, N, FVector2D(float(j) / Segs, float(i) / (Rows - 1)));
		}
	}
	for (int32 i = 0; i + 1 < Rows; ++i)
	{
		for (int32 j = 0; j < Segs; ++j)
		{
			const int32 a = Idx[i * (Segs + 1) + j], b = Idx[i * (Segs + 1) + j + 1];
			const int32 c = Idx[(i + 1) * (Segs + 1) + j + 1], d = Idx[(i + 1) * (Segs + 1) + j];
			const FVector Want = S.N[a] + S.N[b] + S.N[c] + S.N[d];   // already in the output frame
			Tri(S.I, S.V, a, b, c, Want);
			Tri(S.I, S.V, a, c, d, Want);
		}
	}
	if (bCloseBottom && Profile[0].X > 0.01f) Disc(Sec, Base + FVector(0, 0, Profile[0].Y), FVector(0, 0, -1), Profile[0].X, Segs);
	if (bCloseTop && Profile.Last().X > 0.01f) Disc(Sec, Base + FVector(0, 0, Profile.Last().Y), FVector(0, 0, 1), Profile.Last().X, Segs);
}

void FTSSMeshBuilder::Cylinder(int32 Sec, FVector Base, float RB, float RT, float Height, int32 Segs, bool bCapBottom, bool bCapTop)
{
	TArray<FVector2D> P;
	P.Add(FVector2D(RB, 0.f));
	P.Add(FVector2D(RT, Height));
	Lathe(Sec, Base, P, Segs, bCapBottom, bCapTop);
}

void FTSSMeshBuilder::Sphere(int32 Sec, FVector Centre, FVector R, int32 Rings, int32 Segs)
{
	FSection& S = Sections[Sec];
	TArray<int32> Idx;
	Idx.SetNum((Rings + 1) * (Segs + 1));
	for (int32 i = 0; i <= Rings; ++i)
	{
		const float Phi = PI * i / Rings;   // 0 at the top pole
		for (int32 j = 0; j <= Segs; ++j)
		{
			const float Th = 2.f * PI * j / Segs;
			const FVector U(FMath::Sin(Phi) * FMath::Cos(Th), FMath::Sin(Phi) * FMath::Sin(Th), FMath::Cos(Phi));
			const FVector N = FVector(U.X / FMath::Max(R.X, 0.01f), U.Y / FMath::Max(R.Y, 0.01f), U.Z / FMath::Max(R.Z, 0.01f)).GetSafeNormal();
			Idx[i * (Segs + 1) + j] = AddV(S, Centre + U * R, N, FVector2D(float(j) / Segs, float(i) / Rings));
		}
	}
	for (int32 i = 0; i < Rings; ++i)
	{
		for (int32 j = 0; j < Segs; ++j)
		{
			const int32 a = Idx[i * (Segs + 1) + j], b = Idx[i * (Segs + 1) + j + 1];
			const int32 c = Idx[(i + 1) * (Segs + 1) + j + 1], d = Idx[(i + 1) * (Segs + 1) + j];
			const FVector Want = S.N[a] + S.N[b] + S.N[c] + S.N[d];   // already in the output frame
			Tri(S.I, S.V, a, b, c, Want);
			Tri(S.I, S.V, a, c, d, Want);
		}
	}
}

void FTSSMeshBuilder::Loft(int32 Sec, const TArray<TArray<FVector>>& Rings, bool bCapStart, bool bCapEnd, bool bFlip)
{
	const int32 R = Rings.Num();
	if (R < 2) return;
	const int32 M = Rings[0].Num();
	FSection& S = Sections[Sec];
	TArray<int32> Idx;
	Idx.SetNum(R * (M + 1));
	for (int32 i = 0; i < R; ++i)
	{
		FVector Centre = FVector::ZeroVector;
		for (const FVector& P : Rings[i]) Centre += P;
		Centre /= M;
		const TArray<FVector>& Prev = Rings[FMath::Max(0, i - 1)];
		const TArray<FVector>& Next = Rings[FMath::Min(R - 1, i + 1)];
		for (int32 j = 0; j <= M; ++j)
		{
			const int32 k = j % M;
			const FVector Along = (Next[k] - Prev[k]).GetSafeNormal();                         // front to back
			const FVector Around = (Rings[i][(k + 1) % M] - Rings[i][(k + M - 1) % M]).GetSafeNormal();   // round the loop
			FVector N = FVector::CrossProduct(Along, Around).GetSafeNormal();
			if (FVector::DotProduct(N, Rings[i][k] - Centre) < 0.f) N = -N;                       // always away from the middle
			if (bFlip) N = -N;
			Idx[i * (M + 1) + j] = AddV(S, Rings[i][k], N, FVector2D(float(j) / M, float(i) / (R - 1)));
		}
	}
	for (int32 i = 0; i + 1 < R; ++i)
	{
		for (int32 j = 0; j < M; ++j)
		{
			const int32 a = Idx[i * (M + 1) + j], b = Idx[i * (M + 1) + j + 1];
			const int32 c = Idx[(i + 1) * (M + 1) + j + 1], d = Idx[(i + 1) * (M + 1) + j];
			const FVector Want = S.N[a] + S.N[b] + S.N[c] + S.N[d];   // already in the output frame
			Tri(S.I, S.V, a, b, c, Want);
			Tri(S.I, S.V, a, c, d, Want);
		}
	}
	auto Cap = [&](int32 Ring, const FVector& Out)
	{
		FVector C = FVector::ZeroVector;
		for (const FVector& P : Rings[Ring]) C += P;
		C /= M;
		const FVector N = bFlip ? -Out : Out;
		const int32 Ci = AddV(S, C, N, FVector2D(0.5f, 0.5f));
		TArray<int32> Ids;
		for (int32 j = 0; j < M; ++j) Ids.Add(AddV(S, Rings[Ring][j], N, FVector2D(0.5f, 0.5f)));
		for (int32 j = 0; j < M; ++j) Tri(S.I, S.V, Ci, Ids[j], Ids[(j + 1) % M], XN(N));
	};
	auto Centroid = [&](int32 Ring) { FVector C = FVector::ZeroVector; for (const FVector& P : Rings[Ring]) C += P; return C / M; };
	if (bCapStart) Cap(0, (Centroid(0) - Centroid(1)).GetSafeNormal());
	if (bCapEnd) Cap(R - 1, (Centroid(R - 1) - Centroid(R - 2)).GetSafeNormal());
}

void FTSSMeshBuilder::Commit(UProceduralMeshComponent* Mesh, const TArray<UMaterialInterface*>& Materials, bool bCollision) const
{
	if (!Mesh) return;
	Mesh->ClearAllMeshSections();
	for (int32 i = 0; i < Sections.Num(); ++i)
	{
		const FSection& S = Sections[i];
		if (S.V.Num() == 0) continue;
		Mesh->CreateMeshSection_LinearColor(i, S.V, S.I, S.N, S.UV, S.C, TArray<FProcMeshTangent>(), bCollision);
		if (Materials.IsValidIndex(i) && Materials[i]) Mesh->SetMaterial(i, Materials[i]);
		Mesh->SetMeshSectionVisible(i, true);
	}
}

// ---- shared materials -----------------------------------------------------------------------------------

UMaterialInterface* TSSTown::Paint(const FLinearColor& Color)
{
	static TMap<uint32, UMaterialInstanceDynamic*> Cache;
	const uint32 Key = Color.ToFColor(true).DWColor();
	if (UMaterialInstanceDynamic** F = Cache.Find(Key)) { if (*F && (*F)->IsValidLowLevel()) return *F; }
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Base) return nullptr;
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, GetTransientPackage());
	M->AddToRoot();
	M->SetVectorParameterValue(TEXT("Color"), Color);
	Cache.Add(Key, M);
	return M;
}

UMaterialInterface* TSSTown::Glow(const FLinearColor& Color)
{
	static TMap<uint32, UMaterialInstanceDynamic*> Cache;
	const uint32 Key = Color.ToFColor(true).DWColor() ^ 0x9E3779B9u;
	if (UMaterialInstanceDynamic** F = Cache.Find(Key)) { if (*F && (*F)->IsValidLowLevel()) return *F; }
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TSS/Materials/M_TSSEmissive.M_TSSEmissive"));
	const bool bEmissive = Base != nullptr;
	if (!Base) Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Base) return nullptr;
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, GetTransientPackage());
	M->AddToRoot();
	M->SetVectorParameterValue(TEXT("Color"), bEmissive ? Color : Color * 2.5f);
	Cache.Add(Key, M);
	return M;
}
