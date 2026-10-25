#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MEVisorComponent.generated.h"

class AMECharacter;
class AController;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USoundBase;

/**
 * The gas mask visor as seen from inside, drawn by a post-process material:
 * the mask frame, fog from breathing (heavier out of breath), cracks from hits.
 * Wiping clears the fog. Too many cracks break the visor: the mask no longer protects.
 */
UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEVisorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMEVisorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Wipes the visor with the left hand (the weapon goes away). Only with the mask on. */
	void Wipe();

	bool IsBroken() const { return Cracks >= MaxCracks; }
	int32 GetCracks() const { return Cracks; }
	int32 GetMaxCracks() const { return MaxCracks; }
	float GetFog() const { return Fog; }

protected:
	virtual void BeginPlay() override;

	/** Post-process material of the visor (M_PP_Visor). */
	UPROPERTY(EditDefaultsOnly, Category = "Visor")
	TObjectPtr<UMaterialInterface> VisorMaterial;

	/** The frame fades in and out this fast when the mask goes on / off. */
	UPROPERTY(EditDefaultsOnly, Category = "Visor", meta = (ClampMin = "0.01", Units = "s"))
	float MaskFadeTime = 0.25f;

	/** Fog left on the visor while breathing calmly, and fully out of breath (0..1). */
	UPROPERTY(EditDefaultsOnly, Category = "Visor|Fog", meta = (ClampMin = "0", ClampMax = "1"))
	float RestedFog = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category = "Visor|Fog", meta = (ClampMin = "0", ClampMax = "1"))
	float ExhaustedFog = 0.85f;

	/** Fog builds up / clears by itself this much per second. */
	UPROPERTY(EditDefaultsOnly, Category = "Visor|Fog", meta = (ClampMin = "0"))
	float FogBuildUpSpeed = 0.12f;

	UPROPERTY(EditDefaultsOnly, Category = "Visor|Fog", meta = (ClampMin = "0"))
	float FogClearSpeed = 0.04f;

	/** How much each breath out thickens the fog for a moment (0..1). */
	UPROPERTY(EditDefaultsOnly, Category = "Visor|Fog", meta = (ClampMin = "0", ClampMax = "1"))
	float BreathPulse = 0.35f;

	/** Time of the wipe once the weapon is away: the hand sweeps across the visor. */
	UPROPERTY(EditDefaultsOnly, Category = "Visor|Wipe", meta = (ClampMin = "0.05", Units = "s"))
	float WipeTime = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Visor|Wipe")
	TObjectPtr<USoundBase> WipeSound;

	/** A hit of at least this much damage cracks the visor (mask on). */
	UPROPERTY(EditDefaultsOnly, Category = "Visor|Cracks", meta = (ClampMin = "0"))
	float CrackDamage = 10.f;

	/** Cracks the visor takes; the last one breaks it. The material draws at most 3. */
	UPROPERTY(EditDefaultsOnly, Category = "Visor|Cracks", meta = (ClampMin = "1", ClampMax = "3"))
	int32 MaxCracks = 3;

	/** Hits closer than this crack once. */
	UPROPERTY(EditDefaultsOnly, Category = "Visor|Cracks", meta = (ClampMin = "0", Units = "s"))
	float CrackCooldown = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Visor|Cracks")
	TObjectPtr<USoundBase> CrackSound;

	UPROPERTY(EditDefaultsOnly, Category = "Visor|Cracks")
	TObjectPtr<USoundBase> BreakSound;

private:
	void HandleHealthChanged(float Health, float Delta, AController* Instigator);
	void StartWipe();
	void UpdateMaterial();

	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	float MaskAlpha = 0.f;
	float Fog = 0.f;
	int32 Cracks = 0;
	double LastCrackTime = -1.0;

	/** 0 = no wipe, then the hand's position across the visor up to 1. */
	float WipeProgress = 0.f;
	bool bWiping = false;
	FTimerHandle WipeTimer;
};
