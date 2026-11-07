#include "TSSTrafficCar.h"
#include "TSSProcMesh.h"
#include "TSSRoad.h"
#include "TSSTown.h"
#include "TSSStoreSubsystem.h"
#include "ProceduralMeshComponent.h"
#include "Components/SpotLightComponent.h"

namespace
{
	/** Piecewise-linear lookup of a profile given as (t, value) pairs with t rising from 0 to 1. */
	float Prof(const TArray<FVector2D>& P, float T)
	{
		if (T <= P[0].X) return P[0].Y;
		for (int32 i = 1; i < P.Num(); ++i)
		{
			if (T <= P[i].X)
			{
				const float A = (T - P[i - 1].X) / FMath::Max(P[i].X - P[i - 1].X, 1e-4f);
				return FMath::Lerp(P[i - 1].Y, P[i].Y, A);
			}
		}
		return P.Last().Y;
	}

	/** A closed loop with flat-ish top and bottom and rounded corners (a super-ellipse), at one station along the body. */
	void Ring(TArray<FVector>& Out, float X, float HalfWidth, float ZBottom, float ZTop, int32 M, float Exp)
	{
		const float Zm = (ZBottom + ZTop) * 0.5f, Hz = (ZTop - ZBottom) * 0.5f;
		for (int32 i = 0; i < M; ++i)
		{
			const float A = 2.f * PI * i / M;
			const float C = FMath::Cos(A), S = FMath::Sin(A);
			Out.Add(FVector(X, HalfWidth * FMath::Sign(C) * FMath::Pow(FMath::Abs(C), Exp), Zm + Hz * FMath::Sign(S) * FMath::Pow(FMath::Abs(S), Exp)));
		}
	}

	struct FSpec
	{
		float L, W, H;
		float Belt, Hood, Trunk, Roof;       // heights
		float CabinStart, RoofStart, RoofEnd, CabinEnd;   // as fractions of the length from the front
		float WheelR;
	};

	FSpec SpecFor(ETSSCarType T)
	{
		switch (T)
		{
		case ETSSCarType::Hatchback: return { 395.f, 176.f, 152.f, 90.f, 82.f, 98.f, 152.f, 0.27f, 0.40f, 0.78f, 0.97f, 31.f };
		case ETSSCarType::Suv:       return { 482.f, 196.f, 178.f, 108.f, 100.f, 112.f, 178.f, 0.27f, 0.37f, 0.86f, 0.97f, 38.f };
		case ETSSCarType::Van:       return { 530.f, 204.f, 208.f, 112.f, 98.f, 118.f, 208.f, 0.17f, 0.21f, 0.95f, 0.98f, 36.f };
		default:                     return { 470.f, 184.f, 146.f, 92.f, 84.f, 90.f, 146.f, 0.31f, 0.42f, 0.66f, 0.80f, 33.f };
		}
	}

	const FLinearColor GlassColor(0.012f, 0.022f, 0.03f);
	const FLinearColor TrimColor(0.012f, 0.012f, 0.014f);
	const FLinearColor ChromeColor(0.62f, 0.64f, 0.67f);
	const FLinearColor TyreColor(0.006f, 0.006f, 0.007f);
}

void ATSSTrafficCar::Dimensions(ETSSCarType T, float& L, float& W, float& H)
{
	const FSpec S = SpecFor(T);
	L = S.L; W = S.W; H = S.H;
}

ATSSTrafficCar::ATSSTrafficCar()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Body = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(true);
	for (int32 i = 0; i < 4; ++i)
	{
		UProceduralMeshComponent* W = CreateDefaultSubobject<UProceduralMeshComponent>(*FString::Printf(TEXT("Wheel%d"), i));
		W->SetupAttachment(Root);
		W->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Wheels.Add(W);
	}
	Headlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Headlight"));
	Headlight->SetupAttachment(Root);
	Headlight->SetMobility(EComponentMobility::Movable);
	Headlight->SetIntensityUnits(ELightUnits::Candelas);
	Headlight->SetIntensity(25000.f);
	Headlight->SetOuterConeAngle(38.f);
	Headlight->SetInnerConeAngle(10.f);
	Headlight->SetAttenuationRadius(2600.f);
	Headlight->SetLightColor(FLinearColor(1.f, 0.93f, 0.78f));
	Headlight->SetCastShadows(false);
	Headlight->SetVisibility(false);
}

void ATSSTrafficCar::BuildWheel(UProceduralMeshComponent* Wheel, float R, float Wd, bool bRight)
{
	FTSSMeshBuilder B(4);   // 0 tyre, 1 rim, 2 hub, 3 spokes
	// Built spinning about +Z and then laid on its side (axis along Y). Seen from the outside the spokes face out.
	B.Push(FTransform(FRotator(0.f, 0.f, bRight ? -90.f : 90.f)));
	const float H = Wd * 0.5f;
	TArray<FVector2D> Tyre = {
		FVector2D(R * 0.60f, -H), FVector2D(R * 0.84f, -H), FVector2D(R * 0.95f, -H * 0.82f), FVector2D(R * 0.995f, -H * 0.45f),
		FVector2D(R, 0.f), FVector2D(R * 0.995f, H * 0.45f), FVector2D(R * 0.95f, H * 0.82f), FVector2D(R * 0.84f, H), FVector2D(R * 0.60f, H) };
	B.Lathe(0, FVector::ZeroVector, Tyre, 28);
	TArray<FVector2D> Rim = { FVector2D(R * 0.62f, -H * 0.8f), FVector2D(R * 0.60f, H * 0.8f), FVector2D(R * 0.64f, H * 0.95f), FVector2D(R * 0.22f, H * 0.78f) };
	B.Lathe(1, FVector::ZeroVector, Rim, 28, false, false);
	B.Cylinder(2, FVector(0.f, 0.f, H * 0.5f), R * 0.16f, R * 0.12f, H * 0.5f, 14, false, true);
	for (int32 s = 0; s < 5; ++s)
	{
		const float A = 2.f * PI * s / 5.f + 0.3f;
		const FVector D(FMath::Cos(A), FMath::Sin(A), 0.f);
		B.Beam(3, D * (R * 0.14f) + FVector(0, 0, H * 0.78f), D * (R * 0.60f) + FVector(0, 0, H * 0.78f), R * 0.11f, R * 0.14f, H * 0.18f, H * 0.18f, FVector(0, 0, 1));
	}
	B.Pop();
	B.Commit(Wheel, { TSSTown::Paint(TyreColor), TSSTown::Paint(FLinearColor(0.2f, 0.21f, 0.23f)), TSSTown::Paint(ChromeColor), TSSTown::Paint(FLinearColor(0.34f, 0.35f, 0.37f)) });
	Triangles += B.NumTriangles();
}

void ATSSTrafficCar::Build(ETSSCarType InType, const FLinearColor& Paint)
{
	Type = InType;
	PaintColor = Paint;
	const FSpec S = SpecFor(Type);
	Length = S.L; Width = S.W; Height = S.H; WheelRadius = S.WheelR;
	const float HL = Length * 0.5f, HW = Width * 0.5f;
	WheelX = Length * 0.31f;
	const float Ground = 24.f;   // clearance under the body
	const float X0 = HL, X1 = -HL;   // front at +X

	auto XAt = [&](float T) { return X0 + (X1 - X0) * T; };   // t = 0 at the nose, 1 at the tail

	// ---- the tub: lower body, lofted from nose to tail ----
	const bool bBoot = Type == ETSSCarType::Sedan;
	const TArray<FVector2D> Top = {
		{ 0.f, 54.f }, { 0.025f, 68.f }, { 0.10f, S.Hood - 2.f }, { S.CabinStart, S.Hood }, { S.CabinStart + 0.05f, S.Belt }, { S.CabinEnd - (bBoot ? 0.0f : 0.02f), S.Belt },
		{ S.CabinEnd + (bBoot ? 0.07f : 0.02f), S.Trunk }, { 0.975f, S.Trunk - 4.f }, { 1.f, S.Trunk - 24.f } };
	const TArray<FVector2D> Bottom = { { 0.f, 40.f }, { 0.05f, Ground + 2.f }, { 0.94f, Ground + 2.f }, { 1.f, 42.f } };
	const TArray<FVector2D> Half = { { 0.f, 0.80f }, { 0.03f, 0.92f }, { 0.10f, 1.f }, { 0.90f, 1.f }, { 0.97f, 0.93f }, { 1.f, 0.82f } };

	const int32 Stations = 64, M = 28;
	TArray<TArray<FVector>> Tub;
	for (int32 i = 0; i <= Stations; ++i)
	{
		const float T = float(i) / Stations;
		const float X = XAt(T);
		const float W = HW * Prof(Half, T);
		TArray<FVector> R;
		Ring(R, X, W, Prof(Bottom, T), Prof(Top, T), M, 0.30f);
		// Wheel arches: near each axle the lower corners of the body are pushed in, leaving a recess the wheel sits in.
		for (FVector& P : R)
		{
			for (const float Axle : { WheelX, -WheelX })
			{
				const float Dx = FMath::Abs(P.X - Axle);
				const float Rad = S.WheelR + 5.f;
				if (Dx < Rad)
				{
					const float ArchTop = S.WheelR + Ground * 0.0f + 4.f + FMath::Sqrt(FMath::Max(Rad * Rad - Dx * Dx, 0.f));
					if (P.Z < ArchTop && FMath::Abs(P.Y) > W * 0.55f) P.Y = FMath::Sign(P.Y) * FMath::Min(FMath::Abs(P.Y), W - 16.f);
				}
			}
		}
		Tub.Add(R);
	}

	FTSSMeshBuilder B(6);   // 0 paint, 1 glass, 2 dark trim, 3 chrome, 4 headlight, 5 tail light
	B.Loft(0, Tub, true, true);

	// ---- greenhouse: glass loft with the roof, pillars and window frames in body colour ----
	const TArray<FVector2D> CabTop = {
		{ S.CabinStart + 0.02f, S.Belt }, { S.RoofStart, S.Roof - 3.f }, { S.RoofEnd, S.Roof - 3.f }, { S.CabinEnd, S.Belt + (bBoot ? 4.f : 16.f) } };
	TArray<TArray<FVector>> Cab;
	const int32 CabSt = 28;
	const float CabT0 = S.CabinStart + 0.02f, CabT1 = S.CabinEnd;
	for (int32 i = 0; i <= CabSt; ++i)
	{
		const float T = CabT0 + (CabT1 - CabT0) * float(i) / CabSt;
		const float Zt = Prof(CabTop, T);
		const float Wb = HW * 0.90f * Prof(Half, T), Wr = HW * 0.76f;
		const float Fh = FMath::Clamp((Zt - S.Belt) / FMath::Max(S.Roof - S.Belt - 3.f, 1.f), 0.f, 1.f);
		TArray<FVector> R;
		Ring(R, XAt(T), FMath::Lerp(Wb, Wr, FMath::Min(1.f, Fh * 1.4f)) * 0.99f, S.Belt - 4.f, Zt, 20, 0.34f);
		Cab.Add(R);
	}
	B.Loft(1, Cab, true, true);

	// roof panel and pillars in paint
	TArray<TArray<FVector>> Roof;
	for (int32 i = 0; i <= 8; ++i)
	{
		const float T = (S.RoofStart - 0.015f) + ((S.RoofEnd + 0.015f) - (S.RoofStart - 0.015f)) * float(i) / 8.f;
		TArray<FVector> R;
		Ring(R, XAt(T), HW * 0.775f, S.Roof - 7.f, S.Roof + (i == 0 || i == 8 ? -1.f : 1.f), 14, 0.32f);
		Roof.Add(R);
	}
	B.Loft(0, Roof, true, true);
	for (const float Sd : { -1.f, 1.f })
	{
		auto Pillar = [&](float T0, float W0, float Z0, float T1, float W1, float Z1, float Wd)
		{
			B.Beam(0, FVector(XAt(T0), Sd * HW * W0, Z0), FVector(XAt(T1), Sd * HW * W1, Z1), Wd, Wd * 0.8f, 5.f, 4.f, FVector(0.f, Sd, 0.f));
		};
		Pillar(CabT0 + 0.005f, 0.88f, S.Belt, S.RoofStart, 0.755f, S.Roof - 5.f, 6.f);                               // A
		Pillar((S.RoofStart + S.RoofEnd) * 0.5f, 0.88f, S.Belt, (S.RoofStart + S.RoofEnd) * 0.5f, 0.765f, S.Roof - 5.f, 7.f);   // B
		Pillar(CabT1 - 0.005f, 0.86f, S.Belt + (bBoot ? 4.f : 14.f), S.RoofEnd, 0.755f, S.Roof - 5.f, 8.f);      // C
		// door seams and handles
		for (const float Tseam : { S.CabinStart + 0.015f, (S.RoofStart + S.RoofEnd) * 0.5f, S.CabinEnd - 0.04f })
		{
			B.Box(2, FVector(XAt(Tseam), Sd * (HW * 0.995f), (Ground + 8.f + S.Belt) * 0.5f), FVector(0.35f, 0.25f, (S.Belt - Ground - 12.f) * 0.5f));
		}
		for (const float Th : { S.CabinStart + 0.075f, (S.RoofStart + S.RoofEnd) * 0.5f + 0.065f })
		{
			B.Box(3, FVector(XAt(Th), Sd * (HW * 1.0f + 0.6f), S.Belt - 9.f), FVector(6.f, 0.7f, 1.1f));
		}
		// wing mirrors
		B.Box(0, FVector(XAt(S.CabinStart + 0.045f), Sd * (HW * 0.9f + 9.f), S.Belt + 4.f), FVector(4.f, 6.5f, 4.5f));
		B.Beam(2, FVector(XAt(S.CabinStart + 0.05f), Sd * HW * 0.9f, S.Belt + 1.f), FVector(XAt(S.CabinStart + 0.045f), Sd * (HW * 0.9f + 6.f), S.Belt + 3.f), 2.5f, 2.5f, 2.5f, 2.5f);
	}

	// ---- front and rear: grille, bumpers, lights, plates ----
	const float FrontX = X0, RearX = X1;
	B.Box(2, FVector(FrontX - 2.f, 0.f, S.Hood - 30.f), FVector(3.f, HW * 0.55f, 11.f));                  // grille
	for (int32 s = 0; s < 4; ++s) B.Box(3, FVector(FrontX + 0.4f, 0.f, S.Hood - 38.f + s * 6.5f), FVector(1.f, HW * 0.52f, 1.f));
	B.Beam(2, FVector(FrontX - 3.f, -HW * 0.84f, 34.f), FVector(FrontX - 3.f, HW * 0.84f, 34.f), 9.f, 9.f, 14.f, 14.f);   // lower bumper
	B.Beam(2, FVector(RearX + 3.f, -HW * 0.84f, 34.f), FVector(RearX + 3.f, HW * 0.84f, 34.f), 9.f, 9.f, 14.f, 14.f);
	B.Box(3, FVector(FrontX + 0.2f, 0.f, 33.f), FVector(1.5f, 26.f, 5.f));                                     // front lip chrome
	B.Box(3, FVector(FrontX - 1.f, 0.f, 18.f + 40.f), FVector(1.f, 26.f, 6.f));                                  // number plate
	B.Box(3, FVector(RearX + 0.3f, 0.f, S.Trunk - 34.f), FVector(1.f, 26.f, 6.f));
	for (const float Sd : { -1.f, 1.f })
	{
		B.Sphere(4, FVector(FrontX - 6.f, Sd * HW * 0.70f, S.Hood - 16.f), FVector(9.f, 17.f, 7.5f), 8, 14);   // headlights
		B.Box(5, FVector(RearX + 1.5f, Sd * HW * 0.76f, S.Trunk - 14.f), FVector(2.f, 17.f, 4.5f));            // tail lights
		B.Box(4, FVector(FrontX - 3.f, Sd * HW * 0.93f, 56.f), FVector(3.f, 5.f, 3.f));                          // fog lights
	}
	if (Type == ETSSCarType::Van || Type == ETSSCarType::Suv)
	{
		B.Beam(2, FVector(XAt(S.RoofStart + 0.02f), -HW * 0.66f, S.Roof + 1.5f), FVector(XAt(S.RoofEnd - 0.02f), -HW * 0.66f, S.Roof + 1.5f), 4.f, 4.f, 3.f, 3.f);   // roof rails
		B.Beam(2, FVector(XAt(S.RoofStart + 0.02f), HW * 0.66f, S.Roof + 1.5f), FVector(XAt(S.RoofEnd - 0.02f), HW * 0.66f, S.Roof + 1.5f), 4.f, 4.f, 3.f, 3.f);
	}
	if (Type == ETSSCarType::Sedan)
	{
		B.Box(0, FVector(XAt(0.93f), 0.f, S.Trunk + 2.f), FVector(8.f, HW * 0.6f, 1.2f));   // a boot lid spoiler lip
	}

TArray<UMaterialInterface*> Mats = {
		TSSTown::Paint(Paint), TSSTown::Paint(GlassColor), TSSTown::Paint(TrimColor), TSSTown::Paint(ChromeColor),
		TSSTown::Paint(FLinearColor(0.7f, 0.72f, 0.74f)), TSSTown::Paint(FLinearColor(0.35f, 0.02f, 0.02f)) };
	B.Commit(Body, Mats);
	Triangles += B.NumTriangles();

	for (int32 i = 0; i < 4; ++i)
	{
		const bool bRight = (i % 2) == 1;
		const float Ax = i < 2 ? WheelX : -WheelX;
		BuildWheel(Wheels[i], S.WheelR, 22.f, bRight);
		Wheels[i]->SetRelativeLocation(FVector(Ax, (bRight ? 1.f : -1.f) * (HW * 0.985f - 12.f), S.WheelR + 1.f));
	}
	Headlight->SetRelativeLocation(FVector(FrontX + 10.f, 0.f, S.Hood - 14.f));
	Headlight->SetRelativeRotation(FRotator(-3.f, 0.f, 0.f));
	bBuilt = true;
	SetNight(bNightNow);
}

void ATSSTrafficCar::SetNight(bool bNight)
{
	bNightNow = bNight;
	if (!bBuilt) return;
	Headlight->SetVisibility(bNight);
	Body->SetMaterial(4, bNight ? TSSTown::Glow(FLinearColor(1.f, 0.9f, 0.62f)) : TSSTown::Paint(FLinearColor(0.7f, 0.72f, 0.74f)));
	Body->SetMaterial(5, bNight || bBraking ? TSSTown::Glow(FLinearColor(1.f, 0.05f, 0.03f)) : TSSTown::Paint(FLinearColor(0.35f, 0.02f, 0.02f)));
}

void ATSSTrafficCar::Drive(ATSSRoad* InRoad, bool bAlongRoad, float StartDistance, float InCruise)
{
	Road = InRoad;
	bForward = bAlongRoad;
	Distance = StartDistance;
	Cruise = InCruise;
	Speed = InCruise * 0.8f;
	UpdatePose(0.f);
}

void ATSSTrafficCar::UpdatePose(float Dt)
{
	if (!Road) return;
	const float Offset = Road->GetLaneOffset() * (bForward ? 1.f : -1.f);
	FVector P = Road->PointAt(Distance, Offset);
	P.Z += 2.f;
	FRotator R = Road->YawAt(Distance);
	if (!bForward) R.Yaw += 180.f;
	SetActorLocationAndRotation(P, R);
}

void ATSSTrafficCar::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!Road || !bBuilt) return;
	const UTSSStoreSubsystem* Store = GetWorld()->GetSubsystem<UTSSStoreSubsystem>();
	if (Store && Store->IsSimPaused()) return;   // frozen while a menu is up, like everything else

	UTSSTownSubsystem* Town = GetWorld()->GetSubsystem<UTSSTownSubsystem>();
	const float Allowed = Town ? Town->AllowedSpeed(*this) : Cruise;
	const float Target = FMath::Min(Cruise, Allowed);
	const bool bSlowing = Target < Speed - 8.f;
	Speed = FMath::FInterpConstantTo(Speed, Target, Dt, bSlowing ? 1100.f : 320.f);
	if (bSlowing != bBraking)
	{
		bBraking = bSlowing;
		SetNight(bNightNow);   // refreshes the tail lights
	}
	Distance += (bForward ? 1.f : -1.f) * Speed * Dt;
	UpdatePose(Dt);

	WheelSpin += Speed * Dt / WheelRadius;
	for (UProceduralMeshComponent* W : Wheels) if (W) W->SetRelativeRotation(FRotator(-FMath::RadiansToDegrees(WheelSpin), 0.f, 0.f));

	const float Len = Road->GetLength();
	if (Distance < -600.f || Distance > Len + 600.f) Destroy();
}
