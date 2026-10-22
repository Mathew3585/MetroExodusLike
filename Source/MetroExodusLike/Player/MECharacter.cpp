#include "Player/MECharacter.h"
#include "MetroExodusLike.h"
#include "Input/MEInputConfig.h"
#include "Player/MEFirstPersonCameraComponent.h"
#include "Player/MEAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/LocalPlayer.h"
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

	ShadowMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ShadowMesh"));
	ShadowMesh->SetupAttachment(Body);
	ShadowMesh->SetHiddenInGame(true);
	ShadowMesh->bCastHiddenShadow = true;
	ShadowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Places itself on the head bone every frame and adds the procedural motion
	FirstPersonCamera = CreateDefaultSubobject<UMEFirstPersonCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight)); // Until the first update

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

	// Shadow body copies the visible body's pose instead of running its own animation
	ShadowMesh->SetLeaderPoseComponent(GetMesh());

	// Picks up the value edited in the Blueprint
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

void AMECharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateBodyRotation(DeltaSeconds);
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
	if (!bIsTurningInPlace && FMath::Abs(Offset) > TurnInPlaceThreshold)
	{
		bIsTurningInPlace = true;
		if (UMEAnimInstance* AnimInstance = Cast<UMEAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			AnimInstance->PlayTurnInPlace(Offset > 0.f);
		}
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
}

void AMECharacter::Move(const FInputActionValue& Value)
{
	// X = right, Y = forward, relative to the view (the body may lag behind it)
	MoveInput = Value.Get<FVector2D>();

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
	GetCharacterMovement()->MaxWalkSpeed = IsSprinting() ? SprintSpeed : WalkSpeed;
}
