#include "Player/MECharacter.h"
#include "MetroExodusLike.h"
#include "Input/MEInputConfig.h"
#include "Player/MEFirstPersonCameraComponent.h"
#include "Player/MEAnimInstance.h"
#include "Interaction/MEInteractionComponent.h"
#include "Weapons/MEWeaponComponent.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/LocalPlayer.h"
#include "Camera/PlayerCameraManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

AMECharacter::AMECharacter()
{
	// Ticks to rotate the body (see UpdateBodyRotation)
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	BaseEyeHeight = 80.f; // Matches the head of the body mesh (AI sight, camera fallback)
	CrouchedEyeHeight = 55.f;

	// The body yaw is driven by UpdateBodyRotation, not directly by the camera
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// Feet on the capsule bottom, facing forward (skeletal meshes face +Y)
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -88.f), FRotator(0.f, -90.f, 0.f));
	Body->SetCastShadow(false);
	// Always next to the camera, and the cut first-person meshes only exist in LOD 0 (ForcedLOD is 1-based)
	Body->SetForcedLOD(1);

	ShadowMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ShadowMesh"));
	ShadowMesh->SetupAttachment(Body);
	ShadowMesh->SetHiddenInGame(true);
	ShadowMesh->bCastHiddenShadow = true;
	ShadowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Hidden meshes skip bone updates by default: the shadow would freeze
	ShadowMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// Head pivot: the camera component moves it with the view every frame (placement, bob, breathing, impacts...)
	FirstPersonPivot = CreateDefaultSubobject<USceneComponent>(TEXT("FirstPersonPivot"));
	FirstPersonPivot->SetupAttachment(GetCapsuleComponent());
	FirstPersonPivot->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight)); // Until the first update

	// First-person arms, offset so their head bone sits on the pivot (see PlaceArmsOnPivot): they turn around the head
	FirstPersonArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));
	FirstPersonArms->SetupAttachment(FirstPersonPivot);
	FirstPersonArms->SetRelativeLocationAndRotation(FVector(-0.7f, 0.f, -162.6f), FRotator(0.f, -90.f, 0.f));
	FirstPersonArms->SetOnlyOwnerSee(true);
	FirstPersonArms->SetCastShadow(false);
	FirstPersonArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonArms->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	FirstPersonArms->SetForcedLOD(1);

	// On the arms' head bone, like the animation pack: the arm animations move the view exactly as authored.
	// The rotation turns the bone axes of the Mannequin head into a camera looking forward.
	FirstPersonCamera = CreateDefaultSubobject<UMEFirstPersonCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(FirstPersonArms, TEXT("head"));
	FirstPersonCamera->SetRelativeRotation(FRotator(0.f, 90.f, -90.f));

	Interaction = CreateDefaultSubobject<UMEInteractionComponent>(TEXT("Interaction"));
	Weapons = CreateDefaultSubobject<UMEWeaponComponent>(TEXT("Weapons"));

	// Heavy, grounded movement: slow acceleration, little air control
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->MaxAcceleration = 1200.f;
	Movement->BrakingDecelerationWalking = 1400.f;
	Movement->GroundFriction = 6.f;
	Movement->JumpZVelocity = 380.f;
	Movement->AirControl = 0.15f;

	// Crouch: 110 cm tall capsule. If something blocks standing up, the movement
	// component keeps us crouched and stands up as soon as there is room.
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	Movement->SetCrouchedHalfHeight(55.f);
	Movement->MaxWalkSpeedCrouched = CrouchSpeed;
	Movement->bCanWalkOffLedgesWhenCrouching = true;
}

void AMECharacter::BeginPlay()
{
	Super::BeginPlay();

	// Each layer is a leader mesh plus parts (clothing, face...) copying its pose
	for (USkeletalMeshComponent* Part : AddMeshParts(GetMesh(), VisibleBodyParts))
	{
		Part->SetCastShadow(false);
	}
	for (USkeletalMeshComponent* Part : AddMeshParts(FirstPersonArms, FirstPersonArmsParts))
	{
		Part->SetOnlyOwnerSee(true);
		Part->SetCastShadow(false);
		Part->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	}
	for (USkeletalMeshComponent* Part : AddMeshParts(ShadowMesh, WorldBodyParts))
	{
		Part->SetHiddenInGame(true);
		Part->bCastHiddenShadow = true;
	}

	PlaceArmsOnPivot();
	FirstPersonCamera->SetViewPivot(FirstPersonPivot);

	// Picks up the value edited in the Blueprint
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

void AMECharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Speed of the fall, read in Landed (the movement component may have cleared it by then)
	if (GetCharacterMovement()->IsFalling())
	{
		FallSpeed = FMath::Max(0.f, -GetCharacterMovement()->Velocity.Z);
	}

	UpdateBodyRotation(DeltaSeconds);
	UpdateLandingRecovery(DeltaSeconds);
}

void AMECharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	const float ImpactSpeed = FMath::Max(FallSpeed, -GetCharacterMovement()->Velocity.Z);
	FallSpeed = 0.f;

	if (ImpactSpeed < LightLandingSpeed)
	{
		return;
	}

	const bool bHeavy = ImpactSpeed >= HeavyLandingSpeed;
	if (bHeavy)
	{
		// The gear's weight stops us dead
		GetCharacterMovement()->StopMovementImmediately();
		LandingStunRemaining = HeavyLandingStunTime;
		LandingRecoveryDuration = HeavyLandingRecoveryTime;
		LandingRecoveryStartScale = HeavyLandingSpeedScale;
		FirstPersonCamera->PlayHeavyLanding(HeavyLandingStunTime + HeavyLandingRecoveryTime);
	}
	else
	{
		LandingStunRemaining = 0.f;
		LandingRecoveryDuration = LightLandingRecoveryTime;
		LandingRecoveryStartScale = LightLandingSpeedScale;
	}
	LandingRecoveryRemaining = LandingRecoveryDuration;

	ForEachBodyAnimInstance([bHeavy](UMEAnimInstance& AnimInstance) { AnimInstance.PlayLanding(bHeavy); });
}

TArray<USkeletalMeshComponent*> AMECharacter::AddMeshParts(USkeletalMeshComponent* Leader, const TArray<TObjectPtr<USkeletalMesh>>& Meshes)
{
	TArray<USkeletalMeshComponent*> Parts;
	for (USkeletalMesh* PartMesh : Meshes)
	{
		USkeletalMeshComponent* Part = NewObject<USkeletalMeshComponent>(this);
		Part->SetSkeletalMeshAsset(PartMesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetupAttachment(Leader);
		Part->SetLeaderPoseComponent(Leader);
		// Same detail level as the leader, cut meshes only exist in LOD 0
		Part->SetForcedLOD(Leader->GetForcedLOD());
		Part->RegisterComponent();
		Parts.Add(Part);
	}
	return Parts;
}

void AMECharacter::PlaceArmsOnPivot()
{
	// The camera bone (head) must sit exactly on the pivot, whatever the arms mesh
	const USkeletalMesh* ArmsMesh = FirstPersonArms->GetSkeletalMeshAsset();
	const FName CameraBone = FirstPersonCamera->GetAttachSocketName();
	const int32 BoneIndex = ArmsMesh ? ArmsMesh->GetRefSkeleton().FindBoneIndex(CameraBone) : INDEX_NONE;
	if (BoneIndex == INDEX_NONE)
	{
		UE_LOG(LogME, Warning, TEXT("%s: first-person arms have no '%s' bone, the camera cannot follow them."), *GetName(), *CameraBone.ToString());
		return;
	}

	const FVector BoneInMesh = FAnimationRuntime::GetComponentSpaceTransformRefPose(ArmsMesh->GetRefSkeleton(), BoneIndex).GetLocation();
	FirstPersonArms->SetRelativeLocation(-FirstPersonArms->GetRelativeRotation().RotateVector(BoneInMesh));
}

void AMECharacter::SetBodyWeaponPose(UAnimSequenceBase* Pose)
{
	ForEachBodyAnimInstance([Pose](UMEAnimInstance& AnimInstance) { AnimInstance.SetWeaponPose(Pose); });
}

void AMECharacter::ForEachBodyAnimInstance(TFunctionRef<void(UMEAnimInstance&)> Function) const
{
	// Visible body and shadow body animate separately and must play the same one-shots
	for (const USkeletalMeshComponent* Body : { GetMesh(), ShadowMesh.Get() })
	{
		if (UMEAnimInstance* AnimInstance = Cast<UMEAnimInstance>(Body->GetAnimInstance()))
		{
			Function(*AnimInstance);
		}
	}
}

void AMECharacter::UpdateLandingRecovery(float DeltaSeconds)
{
	if (LandingStunRemaining > 0.f)
	{
		LandingStunRemaining -= DeltaSeconds;
		LandingSpeedScale = 0.f;
	}
	else if (LandingRecoveryRemaining > 0.f)
	{
		LandingRecoveryRemaining = FMath::Max(0.f, LandingRecoveryRemaining - DeltaSeconds);
		const float Alpha = 1.f - LandingRecoveryRemaining / LandingRecoveryDuration;
		LandingSpeedScale = FMath::Lerp(LandingRecoveryStartScale, 1.f, Alpha);
	}
	else
	{
		LandingSpeedScale = 1.f;
	}

	UpdateMaxWalkSpeed();
}

void AMECharacter::UpdateBodyRotation(float DeltaSeconds)
{
	const FRotator BodyRotation = GetActorRotation();
	const FRotator ViewYaw(0.f, GetControlRotation().Yaw, 0.f);
	const float Offset = FRotator::NormalizeAxis(ViewYaw.Yaw - BodyRotation.Yaw);

	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const bool bIsMoving = Movement->IsFalling()
		|| Movement->Velocity.SizeSquared2D() > FMath::Square(10.f)
		|| !Movement->GetCurrentAcceleration().IsNearlyZero();

	// Moving: the body follows the view smoothly
	if (bIsMoving)
	{
		bIsTurningInPlace = false;
		SetActorRotation(FMath::RInterpTo(BodyRotation, ViewYaw, DeltaSeconds, MovingTurnSpeed));
		return;
	}

	// Standing still: the body only turns once the view is too far from it, stepping with the turn animation
	// Looking down, the body must stay under the arms: the threshold shrinks with the pitch
	const float LookDownAlpha = FMath::Clamp(-FRotator::NormalizeAxis(GetControlRotation().Pitch) / 45.f, 0.f, 1.f);
	const float Threshold = FMath::Lerp(TurnInPlaceThreshold, LookingDownTurnInPlaceThreshold, LookDownAlpha);

	if (!bIsTurningInPlace && FMath::Abs(Offset) > Threshold)
	{
		bIsTurningInPlace = true;
		const bool bTurnRight = Offset > 0.f;
		ForEachBodyAnimInstance([bTurnRight](UMEAnimInstance& AnimInstance) { AnimInstance.PlayTurnInPlace(bTurnRight); });
	}

	if (bIsTurningInPlace)
	{
		SetActorRotation(FMath::RInterpConstantTo(BodyRotation, ViewYaw, DeltaSeconds, TurnInPlaceRate));
		bIsTurningInPlace = FMath::Abs(Offset) > 1.f;
	}
}

bool AMECharacter::IsSprinting() const
{
	return bWantsToSprint
		&& !bIsCrouched
		&& MoveInput.Y >= SprintMinForwardInput
		&& GetCharacterMovement()->IsMovingOnGround();
}

void AMECharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	const APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->ViewPitchMin = MinViewPitch;
	}

	if (!PC || !InputConfig || !InputConfig->DefaultMappingContext)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InputConfig->DefaultMappingContext, 0);
	}
}

void AMECharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!InputConfig)
	{
		UE_LOG(LogME, Error, TEXT("%s: InputConfig is not set, the player cannot move."), *GetName());
		return;
	}

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	Input->BindAction(InputConfig->MoveAction, ETriggerEvent::Triggered, this, &AMECharacter::Move);
	Input->BindAction(InputConfig->MoveAction, ETriggerEvent::Completed, this, &AMECharacter::StopMoving);
	Input->BindAction(InputConfig->LookAction, ETriggerEvent::Triggered, this, &AMECharacter::Look);
	Input->BindAction(InputConfig->JumpAction, ETriggerEvent::Started, this, &AMECharacter::JumpPressed);
	Input->BindAction(InputConfig->JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	Input->BindAction(InputConfig->SprintAction, ETriggerEvent::Started, this, &AMECharacter::StartSprint);
	Input->BindAction(InputConfig->SprintAction, ETriggerEvent::Completed, this, &AMECharacter::StopSprint);
	Input->BindAction(InputConfig->CrouchAction, ETriggerEvent::Started, this, &AMECharacter::CrouchPressed);
	Input->BindAction(InputConfig->CrouchAction, ETriggerEvent::Completed, this, &AMECharacter::CrouchReleased);
	Input->BindAction(InputConfig->InteractAction, ETriggerEvent::Started, this, &AMECharacter::InteractPressed);
	Input->BindAction(InputConfig->InteractAction, ETriggerEvent::Completed, this, &AMECharacter::InteractReleased);
	Input->BindAction(InputConfig->FireAction, ETriggerEvent::Started, this, &AMECharacter::FirePressed);
	Input->BindAction(InputConfig->FireAction, ETriggerEvent::Completed, this, &AMECharacter::FireReleased);
	Input->BindAction(InputConfig->ReloadAction, ETriggerEvent::Started, this, &AMECharacter::ReloadPressed);
}

bool AMECharacter::CanInteract() const
{
	return !IsLandingStunned() && !GetCharacterMovement()->IsFalling();
}

void AMECharacter::InteractPressed()
{
	Interaction->BeginInteract();
}

void AMECharacter::FirePressed()
{
	// Firing ends the sprint: the weapon comes up
	bWantsToSprint = false;
	UpdateMaxWalkSpeed();
	Weapons->StartFire();
}

void AMECharacter::FireReleased()
{
	Weapons->StopFire();
}

void AMECharacter::ReloadPressed()
{
	Weapons->Reload();
}

void AMECharacter::InteractReleased()
{
	Interaction->EndInteract();
}

void AMECharacter::Move(const FInputActionValue& Value)
{
	// X = right, Y = forward, relative to the view (the body may lag behind it)
	MoveInput = Value.Get<FVector2D>();

	if (IsLandingStunned())
	{
		return;
	}

	const FRotationMatrix ViewYaw(FRotator(0.f, GetControlRotation().Yaw, 0.f));
	AddMovementInput(ViewYaw.GetUnitAxis(EAxis::X), MoveInput.Y);
	AddMovementInput(ViewYaw.GetUnitAxis(EAxis::Y), MoveInput.X);

	UpdateMaxWalkSpeed();
}

void AMECharacter::StopMoving()
{
	MoveInput = FVector2D::ZeroVector;
	UpdateMaxWalkSpeed();
}

void AMECharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void AMECharacter::StartSprint()
{
	bWantsToSprint = true;

	// Sprinting stands the player up (stays crouched under a ceiling)
	if (bIsCrouched)
	{
		UnCrouch();
	}

	UpdateMaxWalkSpeed();
}

void AMECharacter::StopSprint()
{
	bWantsToSprint = false;
	UpdateMaxWalkSpeed();
}

void AMECharacter::JumpPressed()
{
	if (IsLandingStunned())
	{
		return;
	}

	// Jump while crouched stands up instead
	if (bIsCrouched)
	{
		UnCrouch();
		return;
	}

	Jump();
}

void AMECharacter::CrouchPressed()
{
	if (bCrouchIsToggle && GetCharacterMovement()->bWantsToCrouch)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AMECharacter::CrouchReleased()
{
	if (!bCrouchIsToggle)
	{
		UnCrouch();
	}
}

void AMECharacter::UpdateMaxWalkSpeed()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = (IsSprinting() ? SprintSpeed : WalkSpeed) * LandingSpeedScale;
	Movement->MaxWalkSpeedCrouched = CrouchSpeed * LandingSpeedScale;
}
