#include "Player/MECharacter.h"
#include "MetroExodusLike.h"
#include "Input/MEInputConfig.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

AMECharacter::AMECharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	// First person: the body follows the camera yaw
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f)); // Eye height
	FirstPersonCamera->bUsePawnControlRotation = true;

	// Heavy, grounded movement: slow acceleration, little air control
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->MaxAcceleration = 1200.f;
	Movement->BrakingDecelerationWalking = 1400.f;
	Movement->GroundFriction = 6.f;
	Movement->JumpZVelocity = 380.f;
	Movement->AirControl = 0.15f;
}

bool AMECharacter::IsSprinting() const
{
	return bWantsToSprint
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
	Input->BindAction(InputConfig->JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	Input->BindAction(InputConfig->JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	Input->BindAction(InputConfig->SprintAction, ETriggerEvent::Started, this, &AMECharacter::StartSprint);
	Input->BindAction(InputConfig->SprintAction, ETriggerEvent::Completed, this, &AMECharacter::StopSprint);
}

void AMECharacter::Move(const FInputActionValue& Value)
{
	// X = right, Y = forward
	MoveInput = Value.Get<FVector2D>();

	AddMovementInput(GetActorForwardVector(), MoveInput.Y);
	AddMovementInput(GetActorRightVector(), MoveInput.X);

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
	UpdateMaxWalkSpeed();
}

void AMECharacter::StopSprint()
{
	bWantsToSprint = false;
	UpdateMaxWalkSpeed();
}

void AMECharacter::UpdateMaxWalkSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = IsSprinting() ? SprintSpeed : WalkSpeed;
}
