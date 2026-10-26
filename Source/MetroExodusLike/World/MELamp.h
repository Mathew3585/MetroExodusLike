#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MEInteractable.h"
#include "MELamp.generated.h"

class ULightComponent;
class UMaterialInstanceDynamic;
class UNiagaraSystem;
class USoundBase;
class UAudioComponent;

UENUM(BlueprintType)
enum class EMELampState : uint8
{
	On,
	Off,
	Broken
};

/**
 * A lamp of the level: drives every light component of the actor (point, spot, rect: whatever the BP adds)
 * and the glow of its mesh (a scalar parameter of the material).
 * Turned on / off by a linked switch (AMESwitch), by hand when within reach (option), broken for good by a shot.
 * Old lamps can flicker now and then. Enemies notice a lamp they see changing.
 * One BP per model (BP_Lamp_*): mesh, lights, sounds and values.
 */
UCLASS()
class METROEXODUSLIKE_API AMELamp : public AActor, public IMEInteractable
{
	GENERATED_BODY()

public:
	AMELamp();

	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// IMEInteractable
	virtual bool CanBeFocused(const AMECharacter* Interactor) const override;
	virtual FMEInteractionInfo GetInteractionInfo(const AMECharacter* Interactor) const override;
	virtual void Interact(AMECharacter* Interactor) override;

	/** Switch: on / off (a broken lamp stays dark). */
	void SetOn(bool bOn);

	/** Breaks the lamp for good: sparks, glass, the light dies sputtering. */
	void Break(AActor* Breaker);

	EMELampState GetState() const { return State; }
	bool IsLit() const { return State == EMELampState::On; }

	/** Floor under the lamp: where someone goes to look at it. */
	FVector GetGroundLocation() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category = "Lamp")
	bool bStartsOn = true;

	/** Scalar parameter of the mesh material that scales its glow. */
	UPROPERTY(EditDefaultsOnly, Category = "Lamp|Glow")
	FName GlowParameter = TEXT("LampEmissive");

	/** Glow when lit (0 when off). */
	UPROPERTY(EditDefaultsOnly, Category = "Lamp|Glow", meta = (ClampMin = "0"))
	float GlowOn = 5.f;

	/** Within reach (desk lamp, lantern): the player can turn it on / off with the interact key, silently. */
	UPROPERTY(EditAnywhere, Category = "Lamp|Hand")
	bool bCanToggleByHand = false;

	UPROPERTY(EditDefaultsOnly, Category = "Lamp|Hand")
	TObjectPtr<USoundBase> ToggleSound;

	/** Loop played while lit (hum of a fluorescent tube). */
	UPROPERTY(EditDefaultsOnly, Category = "Lamp|Sound")
	TObjectPtr<USoundBase> HumSound;

	// --- Break ---

	UPROPERTY(EditAnywhere, Category = "Lamp|Break")
	bool bCanBreak = true;

	/** Damage from one hit that breaks it. */
	UPROPERTY(EditDefaultsOnly, Category = "Lamp|Break", meta = (ClampMin = "0", EditCondition = "bCanBreak"))
	float BreakDamage = 1.f;

	/** The light sputters this long before dying. */
	UPROPERTY(EditDefaultsOnly, Category = "Lamp|Break", meta = (ClampMin = "0", Units = "s", EditCondition = "bCanBreak"))
	float BreakFlickerTime = 0.4f;

	/** Glass breaking: how far the enemies hear it (1 = full hearing range). */
	UPROPERTY(EditDefaultsOnly, Category = "Lamp|Break", meta = (ClampMin = "0", EditCondition = "bCanBreak"))
	float BreakNoise = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Lamp|Break", meta = (EditCondition = "bCanBreak"))
	TObjectPtr<USoundBase> BreakSound;

	/** Sparks / glass, at the center of the mesh. */
	UPROPERTY(EditDefaultsOnly, Category = "Lamp|Break", meta = (EditCondition = "bCanBreak"))
	TObjectPtr<UNiagaraSystem> BreakEffect;

	// --- Flicker (old lamps) ---

	UPROPERTY(EditAnywhere, Category = "Lamp|Flicker")
	bool bFlickers = false;

	/** Time between two flickers, picked at random in this range. */
	UPROPERTY(EditAnywhere, Category = "Lamp|Flicker", meta = (ClampMin = "0.1", Units = "s", EditCondition = "bFlickers"))
	FVector2D FlickerInterval = FVector2D(2.f, 8.f);

	/** Length of a flicker. */
	UPROPERTY(EditAnywhere, Category = "Lamp|Flicker", meta = (ClampMin = "0.05", Units = "s", EditCondition = "bFlickers"))
	float FlickerTime = 0.4f;

	/** How many times per second the light jumps while flickering. */
	UPROPERTY(EditDefaultsOnly, Category = "Lamp|Flicker", meta = (ClampMin = "1"))
	float FlickerRate = 20.f;

private:
	/** 0 = dark, 1 = full light: scales every light and the glow. */
	void ApplyBrightness(float Scale);
	void ApplyState();
	void StartFlicker(float Duration);
	void ScheduleIdleFlicker();

	EMELampState State = EMELampState::On;

	UPROPERTY()
	TArray<TObjectPtr<ULightComponent>> Lights;
	TArray<float> LightIntensities;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GlowMaterials;

	UPROPERTY()
	TObjectPtr<UAudioComponent> Hum;

	float FlickerRemaining = 0.f;
	float NextFlickerJump = 0.f;
	FTimerHandle IdleFlickerTimer;
};
