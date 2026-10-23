#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MEShellCasing.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class USoundBase;

/**
 * Spent casing thrown out of the weapon on each shot: simulated physics, lies on the ground for a while, then shrinks away.
 * Pooled by the weapon component: Launch() is called again to reuse it.
 * Blueprint child (BP_ShellCasing_556...) sets the mesh and the sound.
 */
UCLASS(Abstract)
class METROEXODUSLIKE_API AMEShellCasing : public AActor
{
	GENERATED_BODY()

public:
	AMEShellCasing();

	virtual void Tick(float DeltaTime) override;

	/**
	 * Throws the casing from Location (where it is seen leaving the weapon).
	 * StartScale: visual scale it starts at, grows to 1 in flight (first-person weapons are drawn scaled toward the eye).
	 */
	void Launch(const FVector& Location, const FRotator& Rotation, const FVector& Velocity, const FVector& AngularVelocity, float StartScale);

	/** Lying on the ground or flying: not yet expired. */
	bool IsActive() const { return bActive; }

protected:
	virtual void BeginPlay() override;

	/** Collision and physics body. Its axis is the length of the casing. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCapsuleComponent> Collision;

	/** Visual only: no collision. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Time on the ground before going away. */
	UPROPERTY(EditDefaultsOnly, Category = "Shell", meta = (ClampMin = "0.1", Units = "s"))
	float Lifetime = 5.f;

	/** Shrink at the end of the lifetime, so it does not pop out. */
	UPROPERTY(EditDefaultsOnly, Category = "Shell", meta = (ClampMin = "0", Units = "s"))
	float ShrinkTime = 0.3f;

	/** Time to grow from the first-person scale to its real size, while leaving the weapon. */
	UPROPERTY(EditDefaultsOnly, Category = "Shell", meta = (ClampMin = "0.01", Units = "s"))
	float GrowTime = 0.25f;

	/** Tinkle on the ground. Optional. */
	UPROPERTY(EditDefaultsOnly, Category = "Shell|Sound")
	TObjectPtr<USoundBase> ImpactSound;

	/** Softer hits make no sound (rolling, settling). */
	UPROPERTY(EditDefaultsOnly, Category = "Shell|Sound", meta = (ClampMin = "0", Units = "cm/s"))
	float MinImpactSpeed = 60.f;

	/** Bounces that make a sound: the first ones only, a casing does not ring forever. */
	UPROPERTY(EditDefaultsOnly, Category = "Shell|Sound", meta = (ClampMin = "0"))
	int32 MaxImpactSounds = 2;

	/** Keeps fast bounces from playing the sound several times in a row. */
	UPROPERTY(EditDefaultsOnly, Category = "Shell|Sound", meta = (ClampMin = "0", Units = "s"))
	float MinTimeBetweenImpactSounds = 0.1f;

private:
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Lifetime over: hidden and asleep until launched again. */
	void Deactivate();

	void SetMeshScale(float Scale);

	bool bActive = false;
	float Age = 0.f;
	float StartScale = 1.f;
	int32 ImpactSoundsPlayed = 0;
	double LastImpactSoundTime = -1.0;
	FTimerHandle LifetimeTimer;
};
