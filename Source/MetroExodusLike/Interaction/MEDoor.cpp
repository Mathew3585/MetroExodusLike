#include "Interaction/MEDoor.h"
#include "Player/MECharacter.h"
#include "Components/StaticMeshComponent.h"

#define LOCTEXT_NAMESPACE "MEDoor"

AMEDoor::AMEDoor()
{
	// Only ticks while swinging
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	// The hinge rotates, the leaf is offset from it (set its transform in the Blueprint)
	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(GetRootComponent());

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(Hinge);
}

FMEInteractionInfo AMEDoor::GetInteractionInfo(const AMECharacter* Interactor) const
{
	FMEInteractionInfo Info;
	Info.Gesture = EMEInteractionGesture::Door;

	if (bLocked)
	{
		Info.ActionText = LOCTEXT("Locked", "Locked");
		Info.bCanInteract = false;
	}
	else
	{
		Info.ActionText = bIsOpen ? LOCTEXT("Close", "Close") : LOCTEXT("Open", "Open");
	}
	return Info;
}

void AMEDoor::Interact(AMECharacter* Interactor)
{
	if (bLocked)
	{
		return;
	}

	bIsOpen = !bIsOpen;

	if (bIsOpen)
	{
		// Positive yaw swings the leaf toward +Y: open toward the side the player is not on
		const float PlayerSide = FVector::DotProduct(Interactor->GetActorLocation() - GetActorLocation(), GetActorRightVector());
		TargetAngle = PlayerSide > 0.f ? -OpenAngle : OpenAngle;
	}
	else
	{
		TargetAngle = 0.f;
	}

	SetActorTickEnabled(true);
}

void AMEDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CurrentAngle = FMath::FInterpTo(CurrentAngle, TargetAngle, DeltaSeconds, SwingSpeed);
	if (FMath::IsNearlyEqual(CurrentAngle, TargetAngle, 0.1f))
	{
		CurrentAngle = TargetAngle;
		SetActorTickEnabled(false);
	}

	Hinge->SetRelativeRotation(FRotator(0.f, CurrentAngle, 0.f));
}

#undef LOCTEXT_NAMESPACE
