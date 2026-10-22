#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MECharacter.generated.h"

class UCameraComponent;
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

	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
	bool IsSprinting() const;

protected:
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

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

	/** Bone the camera follows. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FName HeadBoneName = TEXT("head");

	/** Eye position relative to the head bone, in view-yaw space (X forward, Z up). */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector EyeOffset = FVector(10.f, 0.f, 6.f);

private:
	void Move(const FInputActionValue& Value);
	void StopMoving();
	void Look(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void UpdateMaxWalkSpeed();
	void UpdateFirstPersonCamera();

	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bWantsToSprint = false;
};
