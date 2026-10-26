#include "Interaction/MEInteractionComponent.h"
#include "Player/MECharacter.h"
#include "Player/MEFirstPersonCameraComponent.h"
#include "Engine/World.h"

UMEInteractionComponent::UMEInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMEInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AMECharacter>(GetOwner());
}

void UMEInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Character || !Character->IsLocallyControlled())
	{
		return;
	}

	UpdateFocus();

	if (!bIsHolding)
	{
		return;
	}

	// Looking away or losing the ability to interact cancels the hold
	if (!FocusedActor.IsValid() || !Character->CanInteract())
	{
		CancelHold();
		return;
	}

	HoldElapsed += DeltaTime;
	OnProgress.Broadcast(FMath::Clamp(HoldElapsed / HoldDuration, 0.f, 1.f));

	if (HoldElapsed >= HoldDuration)
	{
		CompleteInteraction();
	}
}

void UMEInteractionComponent::BeginInteract()
{
	const IMEInteractable* Interactable = GetFocusedInteractable();
	if (!Interactable || bIsHolding || !Character->CanInteract())
	{
		return;
	}

	const FMEInteractionInfo Info = Interactable->GetInteractionInfo(Character);
	if (!Info.bCanInteract)
	{
		return;
	}

	OnInteractionStarted.Broadcast(Info.Gesture);

	if (Info.HoldDuration <= 0.f)
	{
		CompleteInteraction();
		return;
	}

	bIsHolding = true;
	HoldElapsed = 0.f;
	HoldDuration = Info.HoldDuration;
	OnProgress.Broadcast(0.f);
}

void UMEInteractionComponent::EndInteract()
{
	if (bIsHolding)
	{
		CancelHold();
	}
}

void UMEInteractionComponent::UpdateFocus()
{
	const UMEFirstPersonCameraComponent* Camera = Character->GetFirstPersonCamera();
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Character->GetControlRotation().Vector() * Reach;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEInteractionTrace), false, Character);
	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(TraceRadius), Params);

	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
	const IMEInteractable* Interactable = Cast<IMEInteractable>(HitActor);
	SetFocus(Interactable && Interactable->CanBeFocused(Character) ? HitActor : nullptr);

	// The offer can change while focused (door opened, light switched...)
	BroadcastFocus();
}

void UMEInteractionComponent::SetFocus(AActor* NewFocus)
{
	if (FocusedActor.Get() == NewFocus)
	{
		return;
	}

	if (bIsHolding)
	{
		CancelHold();
	}
	FocusedActor = NewFocus;
}

void UMEInteractionComponent::BroadcastFocus()
{
	const IMEInteractable* Interactable = GetFocusedInteractable();
	const bool bHasFocus = Interactable != nullptr;
	const FMEInteractionInfo Info = bHasFocus ? Interactable->GetInteractionInfo(Character) : FMEInteractionInfo();

	const bool bChanged = bHasFocus != bHadFocus
		|| !Info.ActionText.EqualTo(LastInfo.ActionText)
		|| Info.HoldDuration != LastInfo.HoldDuration
		|| Info.bCanInteract != LastInfo.bCanInteract;

	if (bChanged)
	{
		bHadFocus = bHasFocus;
		LastInfo = Info;
		OnFocusChanged.Broadcast(bHasFocus, Info);
	}
}

void UMEInteractionComponent::CompleteInteraction()
{
	bIsHolding = false;
	HoldElapsed = 0.f;

	if (IMEInteractable* Interactable = GetFocusedInteractable())
	{
		Interactable->Interact(Character);
	}

	OnProgress.Broadcast(0.f);
}

void UMEInteractionComponent::CancelHold()
{
	bIsHolding = false;
	HoldElapsed = 0.f;
	OnProgress.Broadcast(0.f);
}

IMEInteractable* UMEInteractionComponent::GetFocusedInteractable() const
{
	return Cast<IMEInteractable>(FocusedActor.Get());
}
