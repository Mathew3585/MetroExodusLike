#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "MEFirstPersonCameraComponent.generated.h"

class AMECharacter;

/** Procedural head bob for one locomotion state. */
USTRUCT(BlueprintType)
struct FMEHeadBobSettings
{
	GENERATED_BODY()

	/** Steps per second. A full sway cycle is two steps (left + right foot). */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Hz"))
	float StepFrequency = 1.9f;

	/** Up-down bob, once per step. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "cm"))
	float VerticalAmplitude = 0.6f;

	/** Side-to-side sway, once per cycle. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "cm"))
	float HorizontalAmplitude = 0.4f;

	/** Camera roll following the sway. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Degrees"))
	float RollAmplitude = 0.25f;
};

/** Breathing motion, from rested to exhausted. */
USTRUCT(BlueprintType)
struct FMEBreathingSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Hz"))
	float RestedRate = 0.22f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Hz"))
	float ExhaustedRate = 0.6f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "cm"))
	float RestedAmplitude = 0.25f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "cm"))
	float ExhaustedAmplitude = 0.9f;

	/** Camera pitch on each breath. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Degrees"))
	float RestedPitch = 0.12f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "Degrees"))
	float ExhaustedPitch = 0.45f;

	/** Exertion (0..1) gained per second of sprinting. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float ExertionGainPerSecond = 0.15f;

	/** Exertion (0..1) lost per second when not sprinting. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float ExertionRecoveryPerSecond = 0.1f;

	/** How fast breathing fades out / back in when holding breath. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float HoldBreathBlendSpeed = 3.f;
};

/** Spring that shakes the camera on jump and landing. */
USTRUCT(BlueprintType)
struct FMEImpactSettings
{
	GENERATED_BODY()

	/** Landings slower than this have no effect. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "cm/s"))
	float MinLandingSpeed = 150.f;

	/** Landing speed is clamped to this before scaling. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "cm/s"))
	float MaxLandingSpeed = 1200.f;

	/** Spring kick per cm/s of landing speed. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float LandingKickScale = 0.12f;

	/** Spring kick when jumping (negative = dips down). */
	UPROPERTY(EditAnywhere)
	float JumpKick = -20.f;

	/** Camera pitch per cm of spring offset (nod down on landing). */
	UPROPERTY(EditAnywhere, meta = (Units = "Degrees"))
	float PitchPerCm = 0.35f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float Stiffness = 120.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float Damping = 14.f;
};

/**
 * First-person camera, sitting on the head bone of the first-person arms (the arm animations move it as authored).
 * It drives the head pivot carrying the arms and itself, built in layers every frame:
 *  1. Placement: follows the body's head bone (per-axis follow amount + optional smoothing), turned with the view
 *  2. Head bob: walk / sprint sway
 *  3. Breathing: rested to exhausted, can be held
 *  4. Impacts: spring on jump and landing
 *  5. Strafe tilt: slight roll when moving sideways
 * Layers 2-5 are an offset in view space: arms and view move together, the weapon stays in frame.
 */
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEFirstPersonCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:
	UMEFirstPersonCameraComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Head pivot carrying the first-person arms and this camera: placed and turned with the view every frame. */
	void SetViewPivot(USceneComponent* Pivot) { ViewPivot = Pivot; }

	/** Hold breath (aiming, gas mask...): breathing motion fades out. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Breathing")
	void SetHoldingBreath(bool bHold) { bHoldingBreath = bHold; }

	/** 0 = rested, 1 = exhausted. Also useful for breathing sounds. */
	UFUNCTION(BlueprintPure, Category = "Camera|Breathing")
	float GetExertion() const { return Exertion; }

	/** Kicks the impact spring (explosions, damage...). Negative = down. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Impacts")
	void AddImpactKick(float Kick) { SpringVelocity += Kick; }

	/** Heavy landing: the view drops to the ground, then comes back up over Duration. */
	void PlayHeavyLanding(float Duration);

	/** Size first-person primitives are drawn at (scaled toward the eye so they don't clip into walls). */
	float GetFirstPersonVisualScale() const { return bEnableFirstPersonScale ? FirstPersonScale : 1.f; }

	/**
	 * Where a point of a first-person primitive is seen in the world: world effects leaving the weapon
	 * (casings...) start there to match what is on screen. Ignores the first-person field of view (not used).
	 */
	FVector GetFirstPersonVisualLocation(const FVector& Location) const;

protected:
	virtual void BeginPlay() override;

	// --- Placement ---

	/** Bone of the body the view follows (crouch, lean...). */
	UPROPERTY(EditAnywhere, Category = "Camera|Placement")
	FName HeadBoneName = TEXT("head");

	/** Pivot (= camera bone of the arms) relative to the body's head bone, in view-yaw space (X forward, Z up). */
	UPROPERTY(EditAnywhere, Category = "Camera|Placement")
	FVector EyeOffset = FVector(10.f, 0.f, 6.f);

	/** How much the camera follows the head bone sideways/forward (0 = fixed on the capsule, 1 = full head motion). */
	UPROPERTY(EditAnywhere, Category = "Camera|Placement", meta = (ClampMin = "0", ClampMax = "1"))
	float HeadFollowHorizontal = 1.f;

	/** How much the camera follows the head bone up/down (0 = fixed eye height, 1 = full head motion). */
	UPROPERTY(EditAnywhere, Category = "Camera|Placement", meta = (ClampMin = "0", ClampMax = "1"))
	float HeadFollowVertical = 1.f;

	/** Smooths the head motion. 0 = no smoothing, higher = follows faster. */
	UPROPERTY(EditAnywhere, Category = "Camera|Placement", meta = (ClampMin = "0"))
	float PlacementSmoothingSpeed = 0.f;

	/** Head follow multiplier while in the air (the fall animation moves the head a lot). */
	UPROPERTY(EditAnywhere, Category = "Camera|Placement", meta = (ClampMin = "0", ClampMax = "1"))
	float InAirHeadFollow = 0.2f;

	/** How fast the head follow blends between ground and air. */
	UPROPERTY(EditAnywhere, Category = "Camera|Placement", meta = (ClampMin = "0"))
	float InAirFollowBlendSpeed = 6.f;

	// --- Head bob ---

	UPROPERTY(EditAnywhere, Category = "Camera|Head Bob")
	FMEHeadBobSettings WalkBob;

	UPROPERTY(EditAnywhere, Category = "Camera|Head Bob")
	FMEHeadBobSettings SprintBob;

	/** Ground speed at which the bob reaches full strength. */
	UPROPERTY(EditAnywhere, Category = "Camera|Head Bob", meta = (ClampMin = "1", Units = "cm/s"))
	float FullBobSpeed = 350.f;

	/** How fast the bob blends between idle / walk / sprint. */
	UPROPERTY(EditAnywhere, Category = "Camera|Head Bob", meta = (ClampMin = "0"))
	float BobBlendSpeed = 6.f;

	// --- Breathing / impacts ---

	UPROPERTY(EditAnywhere, Category = "Camera|Breathing")
	FMEBreathingSettings Breathing;

	UPROPERTY(EditAnywhere, Category = "Camera|Impacts")
	FMEImpactSettings Impacts;

	/** How far the view drops toward the ground on a heavy landing. */
	UPROPERTY(EditAnywhere, Category = "Camera|Impacts", meta = (ClampMin = "0", ClampMax = "80", Units = "Degrees"))
	float HeavyLandingLookDown = 22.f;

	/** Time for the view to drop on impact. */
	UPROPERTY(EditAnywhere, Category = "Camera|Impacts", meta = (ClampMin = "0.01", Units = "s"))
	float HeavyLandingDropTime = 0.15f;

	// --- Aiming ---

	/** Head bob strength while fully aiming (the weapon must stay on target). */
	UPROPERTY(EditAnywhere, Category = "Camera|Aiming", meta = (ClampMin = "0", ClampMax = "1"))
	float AimBobScale = 0.3f;

	// --- Strafe tilt ---

	/** Roll when strafing at full bob speed. Positive leans into the movement, 0 disables. */
	UPROPERTY(EditAnywhere, Category = "Camera|Strafe Tilt", meta = (Units = "Degrees"))
	float StrafeRollAngle = 1.f;

	UPROPERTY(EditAnywhere, Category = "Camera|Strafe Tilt", meta = (ClampMin = "0"))
	float StrafeRollSpeed = 5.f;

private:
	/** Returns the world location of the pivot. */
	FVector UpdatePlacement(float DeltaTime);
	void UpdateHeadBob(float DeltaTime, FVector& OutLocation, FRotator& OutRotation);
	void UpdateBreathing(float DeltaTime, FVector& OutLocation, FRotator& OutRotation);
	void UpdateImpacts(float DeltaTime, FVector& OutLocation, FRotator& OutRotation);
	void UpdateStrafeTilt(float DeltaTime, FRotator& OutRotation);
	void UpdateHeavyLanding(float DeltaTime, FRotator& OutRotation);
	void UpdateAiming();

	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> ViewPivot;

	// Placement
	FVector SmoothedLocalOffset = FVector::ZeroVector;
	bool bHasPlacement = false;
	float AirFollowScale = 1.f;

	/** Field of view when not aiming (the one set on the component). */
	float HipFieldOfView = 90.f;

	// Head bob
	float StepPhase = 0.f;
	float MoveWeight = 0.f;
	float SprintWeight = 0.f;

	// Breathing
	float BreathPhase = 0.f;
	float Exertion = 0.f;
	float BreathWeight = 1.f;
	bool bHoldingBreath = false;

	// Impacts
	float SpringOffset = 0.f;
	float SpringVelocity = 0.f;
	float LastFallSpeed = 0.f;
	bool bWasFalling = false;

	// Strafe tilt
	float StrafeRoll = 0.f;

	// Heavy landing look-down
	float HeavyLandingElapsed = 0.f;
	float HeavyLandingDuration = 0.f;
};
