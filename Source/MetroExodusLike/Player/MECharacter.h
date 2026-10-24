#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MECharacter.generated.h"

class UMEFirstPersonCameraComponent;
class UMEInputConfig;
class UMEInteractionComponent;
class UMEWeaponComponent;
class UMEInventoryComponent;
class UMEHealthComponent;
class UMEDamageFeedbackComponent;
class UMEMedkitComponent;
class UMEToxicityComponent;
class UMEFootstepComponent;
class AController;
class UMEAnimInstance;
class UAnimSequenceBase;
struct FInputActionValue;

/**
 * Hybrid first-person player character, built like the AR-15 pack demo:
 *  - head pivot (eye height, turned with the view) -> first-person arms -> camera on the arms' head bone,
 *    so the arm animations frame the weapon and move the view exactly as authored;
 *  - visible body = legs only; world body = hidden full body holding the world weapon (shadow, AI).
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
	UMEWeaponComponent* GetWeapons() const { return Weapons; }
	UMEInventoryComponent* GetInventory() const { return Inventory; }
	UMEHealthComponent* GetHealth() const { return Health; }
	UMEMedkitComponent* GetMedkit() const { return Medkit; }
	UMEToxicityComponent* GetToxicity() const { return Toxicity; }
	USkeletalMeshComponent* GetFirstPersonArms() const { return FirstPersonArms; }
	USkeletalMeshComponent* GetWorldBody() const { return ShadowMesh; }
	const UMEInputConfig* GetInputConfig() const { return InputConfig; }

	/** Upper-body pose of the bodies holding the current weapon (null = no weapon). */
	void SetBodyWeaponPose(UAnimSequenceBase* Pose);
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
	 * World body: the full character holding the world weapon, hidden for the player.
	 * It casts our shadow and is what the AI sees. Later it can show during full-body actions.
	 * The visible body (GetMesh + VisibleBodyParts) is legs and torso only, no arms, no head, no shadow.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> ShadowMesh;

	/** Head pivot at eye height, turned with the view by the camera component. Carries the arms and the camera. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> FirstPersonPivot;

	/** First-person arms (view model) holding the weapon. Only the player sees them. The camera is on their head bone. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> FirstPersonArms;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEInteractionComponent> Interaction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEWeaponComponent> Weapons;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEInventoryComponent> Inventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEHealthComponent> Health;

	/** Hits, low health and death on screen and in the sound (no health bar). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEDamageFeedbackComponent> DamageFeedback;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEMedkitComponent> Medkit;

	/** Toxic air breathed (toxic zones), wears off in clean air. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEToxicityComponent> Toxicity;

	/** Footsteps read from the body's feet, landing sounds. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMEFootstepComponent> Footsteps;

	/**
	 * Extra meshes of the visible body (clothing...), following its pose.
	 * Visible body meshes are cut without arms and head: the first-person arms replace them.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	TArray<TObjectPtr<USkeletalMesh>> VisibleBodyParts;

	/** Extra meshes of the first-person arms (sleeves, gloves...), following their pose. Cut at the shoulder. */
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	TArray<TObjectPtr<USkeletalMesh>> FirstPersonArmsParts;

	/** Extra meshes of the world body (clothing, face...), following its pose. Complete, uncut. */
	UPROPERTY(EditDefaultsOnly, Category = "First Person")
	TArray<TObjectPtr<USkeletalMesh>> WorldBodyParts;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UMEInputConfig> InputConfig;

	/** Lowest view pitch: below it the view would sink into the visible body. */
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (ClampMin = "-89", ClampMax = "0", Units = "Degrees"))
	float MinViewPitch = -65.f;

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

	/**
	 * Turn-in-place threshold when looking straight down: the body follows the view closer,
	 * so the shoulders do not show under the first-person arms.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Body Rotation", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float LookingDownTurnInPlaceThreshold = 20.f;

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

	/** Landings faster than this are heard (both feet on the ground). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Landing", meta = (ClampMin = "0", Units = "cm/s"))
	float LandingSoundMinSpeed = 200.f;

	/** Landings faster than this hurt. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Landing", meta = (ClampMin = "0", Units = "cm/s"))
	float FallDamageSpeed = 900.f;

	/** Landing at this speed (and faster) deals FallDamageMax. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Landing", meta = (ClampMin = "0", Units = "cm/s"))
	float FallDamageMaxSpeed = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Landing", meta = (ClampMin = "0"))
	float FallDamageMax = 100.f;

	// --- Death ---

	/** Time from death to coming back at the start (later: the last checkpoint). */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (ClampMin = "0", Units = "s"))
	float RespawnDelay = 4.f;

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
	void FirePressed();
	void FireReleased();
	void ReloadPressed();
	void AimPressed();
	void AimReleased();
	void InspectPressed();
	void SwitchFireModePressed();
	void MedkitPressed();
	void UpdateMaxWalkSpeed();
	void UpdateBodyRotation(float DeltaSeconds);
	void UpdateLandingRecovery(float DeltaSeconds);
	void PlaceArmsOnPivot();
	void ForEachBodyAnimInstance(TFunctionRef<void(UMEAnimInstance&)> Function) const;
	void HandleDeath(AController* Killer);
	void Respawn();

	/** Spawns a component per mesh, attached to the leader and copying its pose. */
	TArray<USkeletalMeshComponent*> AddMeshParts(USkeletalMeshComponent* Leader, const TArray<TObjectPtr<USkeletalMesh>>& Meshes);

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

	FTimerHandle RespawnTimer;
};
