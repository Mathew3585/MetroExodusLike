#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MECharacter.generated.h"

class UMEFirstPersonCameraComponent;
class UMEInputConfig;
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
	bool IsSprinting() const;

protected:
	virtual void BeginPlay() override;
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

private:
	void Move(const FInputActionValue& Value);
	void StopMoving();
	void Look(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void JumpPressed();
	void CrouchPressed();
	void CrouchReleased();
	void UpdateMaxWalkSpeed();
	void UpdateBodyRotation(float DeltaSeconds);

	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bWantsToSprint = false;
	bool bIsTurningInPlace = false;
};
