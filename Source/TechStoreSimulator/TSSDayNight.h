#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TSSDayNight.generated.h"

class ADirectionalLight;
class ASkyLight;

/**
 * Day/night cycle. Reads the in-game clock (UTSSBusinessSubsystem::GetHour) every frame and drives the map's sun:
 * it rises in the east at 6:00, peaks at noon and sets at 18:00. After dark the same light becomes a dim blue moon,
 * the sky light follows the sky, and the street lamps switch on. Place one in the map.
 */
UCLASS()
class ATSSDayNight : public AActor
{
	GENERATED_BODY()

public:
	ATSSDayNight();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Re-applies the lighting for the current clock right now (the self-test uses this). */
	void ApplyNow();

	/** Sun elevation in degrees for an hour of the day (negative = below the horizon). */
	static float SunElevation(float Hour);
	/** Brightness of the sun for an elevation (lux), 0 below the horizon. */
	static float SunLux(float ElevationDeg);

	float GetLightLux() const;
	bool IsNight() const { return bNight; }

	UPROPERTY(EditAnywhere, Category = "DayNight") float NoonLux = 10.f;
	UPROPERTY(EditAnywhere, Category = "DayNight") float MoonLux = 0.35f;
	/** Sky-light fill outdoors: the shadow side of the street is lit by this in daytime, and it falls back to the night value after dark. The shop interior is lit by its own lamps, so this is kept moderate. */
	UPROPERTY(EditAnywhere, Category = "DayNight") float SkyDayIntensity = 1.3f;
	UPROPERTY(EditAnywhere, Category = "DayNight") float SkyNightIntensity = 0.15f;

private:
	void Apply(float Hour);

	TWeakObjectPtr<ADirectionalLight> Sun;
	TWeakObjectPtr<ASkyLight> Sky;
	bool bNight = false;
	int32 LampState = -1;   // -1 not yet applied, 0 off, 1 on
	bool bSkyRealtime = false;
};
