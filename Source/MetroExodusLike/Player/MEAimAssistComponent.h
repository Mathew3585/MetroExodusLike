#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MEAimAssistComponent.generated.h"

class AMECharacter;
class AMEAICharacter;

/**
 * Gamepad aim assist, never with the mouse:
 *  - slowdown: the stick turns slower while the view crosses an enemy, easier to stay on him;
 *  - snap: aiming down sights with an enemy almost in the sights gives a small nudge onto his chest;
 *  - tracking: aiming at a moving enemy, the view follows part of his motion, only while the player moves (stick or feet).
 * Only living enemies in clear sight count.
 */
UCLASS(ClassGroup = (Player), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEAimAssistComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMEAimAssistComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Multiplier of the stick turn speed: below 1 over an enemy. */
	float GetLookScale() const { return LookScale; }

	/** Aiming down sights started: small nudge onto an enemy already almost in the sights. */
	void StartAimSnap();

	const AMEAICharacter* GetCurrentTarget() const { return CurrentTarget.Get(); }
	bool IsSnapping() const { return bSnapping; }
	bool IsTracking() const { return TrackedTarget.IsValid(); }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist")
	bool bEnabled = true;

	/** Enemies further away get no help. */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist", meta = (ClampMin = "0", Units = "cm"))
	float MaxDistance = 4000.f;

	/** Height of the aimed point above the enemy's capsule center, as a fraction of the half height (chest). */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist", meta = (ClampMin = "0", ClampMax = "1"))
	float ChestHeight = 0.4f;

	// --- Slowdown ---

	/** Size of the slowdown zone around the aimed point, as a distance at the target (so it shrinks with range). */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Slowdown", meta = (ClampMin = "0", Units = "cm"))
	float SlowdownRadius = 80.f;

	/** The slowdown zone never gets smaller than this on screen (far targets). */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Slowdown", meta = (ClampMin = "0", Units = "Degrees"))
	float MinSlowdownAngle = 1.5f;

	/** Stick speed right on the aimed point; back to full speed at the edge of the zone. */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Slowdown", meta = (ClampMin = "0", ClampMax = "1"))
	float SlowdownScale = 0.45f;

	// --- Snap ---

	/** The enemy must be this close to the line of sight, as a distance at the target... */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Snap", meta = (ClampMin = "0", Units = "cm"))
	float SnapRadius = 50.f;

	/** ...and never further than this from the center of the view (close targets). */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Snap", meta = (ClampMin = "0", ClampMax = "45", Units = "Degrees"))
	float SnapAngle = 4.f;

	/** Time of the nudge (it eases out). */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Snap", meta = (ClampMin = "0.01", Units = "s"))
	float SnapTime = 0.08f;

	/** Part of the way covered: the player finishes the aim. */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Snap", meta = (ClampMin = "0", ClampMax = "1"))
	float SnapStrength = 0.5f;

	/** The nudge never turns the view more than this. */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Snap", meta = (ClampMin = "0", Units = "Degrees"))
	float MaxSnapRotation = 3.f;

	// --- Tracking ---

	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Tracking")
	bool bTracking = true;

	/** Part of the enemy's motion (as seen from the eye) the view follows, right on him; none at the edge of the zone. */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Tracking", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bTracking"))
	float TrackingStrength = 0.4f;

	/** Size of the tracking zone around the aimed point, as a distance at the target. */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Tracking", meta = (ClampMin = "0", Units = "cm", EditCondition = "bTracking"))
	float TrackingRadius = 60.f;

	/** The tracking zone never gets smaller than this on screen (far targets). */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Tracking", meta = (ClampMin = "0", Units = "Degrees", EditCondition = "bTracking"))
	float MinTrackingAngle = 1.5f;

private:
	/** Living enemy in clear sight closest to the center of the view, within MaxAngle (degrees). */
	AMEAICharacter* FindTarget(float MaxAngle, FVector& OutAimPoint, float& OutAngle) const;
	FVector GetAimPoint(const AMEAICharacter& Target) const;
	bool IsInClearSight(const AMEAICharacter& Target, const FVector& Point) const;

	/** Angle on screen of a zone of this radius at this distance, never below MinAngle. */
	static float GetZoneAngle(float Radius, float Distance, float MinAngle);

	/** Angle between the view and this point, in degrees. */
	float GetAngleTo(const FVector& Point) const;

	bool CanAssist() const;
	void UpdateCurrentTarget();
	float ComputeLookScale() const;
	void UpdateSnap(float DeltaTime);
	void UpdateTracking();
	void StopTracking();

	UPROPERTY()
	TObjectPtr<AMECharacter> Character;

	float LookScale = 1.f;

	// Target of this frame (one search per frame, shared by slowdown and snap)
	TWeakObjectPtr<AMEAICharacter> CurrentTarget;
	FVector CurrentAimPoint = FVector::ZeroVector;
	float CurrentAngle = 0.f;
	float CurrentDistance = 0.f;

	// Snap: the offset to the target is taken once, then applied with an ease out
	FRotator SnapOffset = FRotator::ZeroRotator;
	float SnapElapsed = 0.f;
	bool bSnapping = false;

	// Tracking: locked on one enemy while he stays in the zone, follows the change of direction to him
	TWeakObjectPtr<AMEAICharacter> TrackedTarget;
	FVector LastTrackDirection = FVector::ForwardVector;
};
