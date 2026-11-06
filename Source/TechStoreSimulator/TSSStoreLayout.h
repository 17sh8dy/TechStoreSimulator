#pragma once

#include "CoreMinimal.h"

/**
 * The shop's footprint at each size stage. The front wall (and the door in it) never moves; expanding pushes the
 * back wall and the side walls outward. Things that live against a wall carry an anchor tag, so they slide with it.
 * Everything is in stage-0 coordinates unless a function says otherwise.
 */
namespace TSSStoreLayout
{
	constexpr int32 NumStages = 3;
	constexpr float FrontX = 700.f;         // inside face of the front wall
	constexpr float DoorHalfGap = 60.f;     // half the width of the doorway (a 120 cm opening for a ~100 cm door)

	inline int32 ClampStage(int32 Stage) { return FMath::Clamp(Stage, 0, NumStages - 1); }
	/** X of the inside face of the back wall. */
	inline float BackX(int32 Stage) { static const float V[NumStages] = { 0.f, -350.f, -700.f }; return V[ClampStage(Stage)]; }
	/** Half the width (Y) of the shop. */
	inline float HalfWidth(int32 Stage) { static const float V[NumStages] = { 300.f, 400.f, 500.f }; return V[ClampStage(Stage)]; }
	inline float Length(int32 Stage) { return FrontX - BackX(Stage); }
	inline int32 LightColumns(int32 Stage) { static const int32 V[NumStages] = { 2, 3, 4 }; return V[ClampStage(Stage)]; }

	inline const TCHAR* StageName(int32 Stage)
	{
		static const TCHAR* V[NumStages] = { TEXT("Small shop"), TEXT("Medium shop"), TEXT("Full-size store") };
		return V[ClampStage(Stage)];
	}

	enum class EAnchor : uint8 { None, Back, Left, Right };

	inline const TCHAR* AnchorTag(EAnchor A)
	{
		switch (A)
		{
		case EAnchor::Back: return TEXT("TSSAnchorBack");
		case EAnchor::Left: return TEXT("TSSAnchorLeft");
		case EAnchor::Right: return TEXT("TSSAnchorRight");
		default: return TEXT("");
		}
	}

	/** How far something anchored to a wall has moved by `Stage`, relative to where it sits at stage 0. */
	inline FVector AnchorDelta(EAnchor A, int32 Stage)
	{
		switch (A)
		{
		case EAnchor::Back: return FVector(BackX(Stage) - BackX(0), 0.f, 0.f);
		case EAnchor::Left: return FVector(0.f, -(HalfWidth(Stage) - HalfWidth(0)), 0.f);
		case EAnchor::Right: return FVector(0.f, HalfWidth(Stage) - HalfWidth(0), 0.f);
		default: return FVector::ZeroVector;
		}
	}
}
