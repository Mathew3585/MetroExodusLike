#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MECharacter.generated.h"

class UCameraComponent;
class UMEInputConfig;
struct FInputActionValue;

/**
 * First-person player character.
 */
UCLASS()
class METROEXODUSLIKE_API AMECharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMECharacter();

	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
	bool IsSprinting() const;

protected:
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UMEInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0", Units = "cm/s"))
	float WalkSpeed = 350.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0", Units = "cm/s"))
	float SprintSpeed = 600.f;

	/** Forward input needed to keep sprinting (no sprinting sideways or backwards). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0", ClampMax = "1"))
	float SprintMinForwardInput = 0.5f;

private:
	void Move(const FInputActionValue& Value);
	void StopMoving();
	void Look(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void UpdateMaxWalkSpeed();

	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bWantsToSprint = false;
};
