#pragma once

#include "CoreMinimal.h"
#include "Components/SpotLightComponent.h"
#include "MEFlashlightComponent.generated.h"

class USoundBase;

/**
 * Head-mounted flashlight: a spotlight on the forehead, a little above the eyes so its shadows show.
 * Basic for now: on / off, with a short flicker when switched on. Battery and charger come with Phase 6.
 * The light settings (intensity, cone, radius) are tuned on BP_MECharacter.
 */
UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEFlashlightComponent : public USpotLightComponent
{
	GENERATED_BODY()

public:
	UMEFlashlightComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Toggle();
	void SetOn(bool bOn);
	bool IsOn() const { return bIsOn; }

protected:
	virtual void BeginPlay() override;

	/** Lit when the game starts. */
	UPROPERTY(EditDefaultsOnly, Category = "Flashlight")
	bool bStartsOn = false;

	/** Switched on, the bulb flickers this long before it holds (0 = no flicker). */
	UPROPERTY(EditDefaultsOnly, Category = "Flashlight|Flicker", meta = (ClampMin = "0", Units = "s"))
	float FlickerTime = 0.35f;

	/** How many times per second the light jumps while flickering. */
	UPROPERTY(EditDefaultsOnly, Category = "Flashlight|Flicker", meta = (ClampMin = "1"))
	float FlickerRate = 25.f;

	/** Chance of a blackout at each jump at the start of the flicker; it fades to none by the end. */
	UPROPERTY(EditDefaultsOnly, Category = "Flashlight|Flicker", meta = (ClampMin = "0", ClampMax = "1"))
	float FlickerOffChance = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight")
	TObjectPtr<USoundBase> SwitchOnSound;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight")
	TObjectPtr<USoundBase> SwitchOffSound;

private:
	void StopFlicker();

	bool bIsOn = false;

	/** Intensity set on the component (BP), the flicker scales it. */
	float BaseIntensity = 0.f;
	float FlickerRemaining = 0.f;
	float NextFlickerJump = 0.f;
};
