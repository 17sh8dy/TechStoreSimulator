#include "TSSDayNight.h"
#include "TSSBusinessSubsystem.h"
#include "TSSRoad.h"
#include "TSSStreetBuilding.h"
#include "TSSStoreShell.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"

ATSSDayNight::ATSSDayNight()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

float ATSSDayNight::SunElevation(float Hour)
{
	// Sunrise 6:00, noon 12:00 (62 degrees up), sunset 18:00; the same sine carries on below the horizon all night.
	return FMath::Sin((Hour - 6.f) / 12.f * PI) * 62.f;
}

float ATSSDayNight::SunLux(float E)
{
	const float T = FMath::Clamp((E + 8.f) / 20.f, 0.f, 1.f);   // fade in from -8 (end of twilight) to 12 degrees
	return FMath::Sqrt(T * T * (3.f - 2.f * T));   // square root keeps the low sun bright enough to light a twilight sky
}

void ATSSDayNight::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It) { Sun = *It; break; }
	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It) { Sky = *It; break; }
	if (Sky.IsValid())
	{
		// The captured sky was baked at noon; follow the real sky instead so nights are dark.
		Sky->GetLightComponent()->SetRealTimeCapture(true);
		bSkyRealtime = true;
	}
	ApplyNow();
}

void ATSSDayNight::Tick(float Dt)
{
	Super::Tick(Dt);
	ApplyNow();
}

void ATSSDayNight::ApplyNow()
{
	if (const UTSSBusinessSubsystem* Biz = GetWorld()->GetSubsystem<UTSSBusinessSubsystem>()) Apply(Biz->GetHour());
}

float ATSSDayNight::GetLightLux() const
{
	return Sun.IsValid() ? Sun->GetLightComponent()->Intensity : 0.f;
}

void ATSSDayNight::Apply(float Hour)
{
	const float E = SunElevation(Hour);
	const float Az = -60.f + (Hour - 6.f) * 15.f;            // east at sunrise, over the south, west at sunset
	const float CosE = FMath::Cos(FMath::DegreesToRadians(E));
	const FVector ToSun(CosE * FMath::Cos(FMath::DegreesToRadians(Az)), CosE * FMath::Sin(FMath::DegreesToRadians(Az)), FMath::Sin(FMath::DegreesToRadians(E)));

	const bool bMoon = E < -8.f;
	bNight = E < 4.f;   // the lamps come on at dusk, before it is properly dark

	if (Sun.IsValid())
	{
		UDirectionalLightComponent* L = CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent());
		FVector Dir = ToSun;
		float Lux;
		FLinearColor Color;
		if (!bMoon)
		{
			Lux = NoonLux * SunLux(E);
			const float Warm = FMath::Clamp(E / 25.f, 0.f, 1.f);   // orange near the horizon, white high up
			Color = FMath::Lerp(FLinearColor(1.f, 0.55f, 0.3f), FLinearColor(1.f, 0.97f, 0.92f), Warm);
		}
		else
		{
			// After dark the same light is the moon: opposite the sun, above the horizon, dim and blue.
			Dir = -ToSun;
			Lux = MoonLux;
			Color = FLinearColor(0.55f, 0.65f, 1.f);
		}
		Sun->SetActorRotation(FRotationMatrix::MakeFromX(-Dir).Rotator());
		L->SetIntensity(Lux);
		L->SetLightColor(Color);
	}

	if (LampState != (bNight ? 1 : 0))
	{
		LampState = bNight ? 1 : 0;
		for (TActorIterator<ATSSRoad> It(GetWorld()); It; ++It) It->SetLampsOn(bNight);
		for (TActorIterator<ATSSStreetBuilding> It(GetWorld()); It; ++It) It->SetNight(bNight);   // lit windows
		for (TActorIterator<ATSSStoreShell> It(GetWorld()); It; ++It) It->SetNightLights(bNight);  // the sign light
	}
}
