#include "Player/MECharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"

AMECharacter::AMECharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f)); // Eye height
	FirstPersonCamera->bUsePawnControlRotation = true;
}
