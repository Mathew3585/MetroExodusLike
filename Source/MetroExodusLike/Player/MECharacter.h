#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MECharacter.generated.h"

class UMEFirstPersonCameraComponent;
class UMEInputConfig;
class UMEInteractionComponent;
struct FInputActionValue;

/**
 * True first-person player character: a full body (without head) is visible,
 * and the camera follows the head bone.
 */
UCLASS()
class METROEXODUSLIKE_API AMECharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMECharacter();

	virtual void Tick(float DeltaSeconds) override;

	UMEFirstPersonCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
	UMEInteractionComponent* GetInteraction() const { return Interaction; }
	const UMEInputConfig* GetInputConfig() const { return InputConfig; }
	bool IsSprinting() const;

	/** True while absorbing a heavy landing: no movement, no jump. */
	bool IsLandingStunned() const { return LandingStunRemaining > 0.f; }

	/** Interactions need both feet on the ground and no landing stun. */
	bool CanInteract() const;

protected:
	virtual void BeginPlay() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEFirstPersonCameraComponent> FirstPersonCamera;

	/**
	 * Full body (with head), hidden but casting the shadow, so our shadow has a head.
	 * The visible body (GetMesh) is the headless version and casts no shadow.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> ShadowMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEInteractionComponent> Interaction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UMEInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0", Units = "cm/s"))
	float WalkSpeed = 350.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0", Units = "cm/s"))
	float SprintSpeed = 600.f;

	/** Forward input needed to keep sprinting (no sprinting sideways or backwards). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0", ClampMax = "1"))
	float SprintMinForwardInput = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Crouch", meta = (ClampMin = "0", Units = "cm/s"))
	float CrouchSpeed = 160.f;

	/** true = press to crouch / press again to stand. false = hold to crouch. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Crouch")
	bool bCrouchIsToggle = true;

	/** Standing still, the body turns once the view is this far from it. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Body Rotation", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float TurnInPlaceThreshold = 70.f;

	/** How fast the body turns in place to catch up with the view. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Body Rotation", meta = (ClampMin = "0", Units = "DegreesPerSecond"))
	float TurnInPlaceRate = 220.f;

	/** How fast the body follows the view while moving. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Body Rotation", meta = (ClampMin = "0"))
	float MovingTurnSpeed = 15.f;

	// --- Landing (heavy gear: weapon, backpack, vest) ---

	/** Landings slower than this have no effect. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Landing", meta = (ClampMin = "0", Units = "cm/s"))
	float LightLandingSpeed = 300.f;

	/** Landings faster than this are heavy: full stop, absorb, look down. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Landing", meta = (ClampMin = "0", Units = "cm/s"))
	float HeavyLandingSpeed = 650.f;

	/** Light landing: speed multiplier right after the impact, back to 1 over the recovery time. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Landing", meta = (ClampMin = "0", ClampMax = "1"))
	float LightLandingSpeedScale = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Landing", meta = (ClampMin = "0", Units = "s"))
	float LightLandingRecoveryTime = 0.35f;

	/** Heavy landing: time the character cannot move or jump. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Landing", meta = (ClampMin = "0", Units = "s"))
	float HeavyLandingStunTime = 1.1f;

	/** Heavy landing: speed multiplier when the stun ends, back to 1 over the recovery time. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Landing", meta = (ClampMin = "0", ClampMax = "1"))
	float HeavyLandingSpeedScale = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Landing", meta = (ClampMin = "0", Units = "s"))
	float HeavyLandingRecoveryTime = 0.6f;

private:
	void Move(const FInputActionValue& Value);
	void StopMoving();
	void Look(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void JumpPressed();
	void CrouchPressed();
	void CrouchReleased();
	void InteractPressed();
	void InteractReleased();
	void UpdateMaxWalkSpeed();
	void UpdateBodyRotation(float DeltaSeconds);
	void UpdateLandingRecovery(float DeltaSeconds);

	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bWantsToSprint = false;
	bool bIsTurningInPlace = false;

	// Landing
	float FallSpeed = 0.f;
	float LandingStunRemaining = 0.f;
	float LandingRecoveryRemaining = 0.f;
	float LandingRecoveryDuration = 0.f;
	float LandingRecoveryStartScale = 1.f;
	float LandingSpeedScale = 1.f;
};
