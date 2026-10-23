#include "Player/MEPlayerController.h"
#include "MetroExodusLike.h"
#include "Player/MECharacter.h"
#include "Input/MEInputConfig.h"
#include "Interaction/MEInteractionComponent.h"
#include "Weapons/MEWeaponComponent.h"
#include "UI/MEInteractionPromptWidget.h"
#include "UI/MECrosshairWidget.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

void AMEPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!IsLocalController())
	{
		return;
	}

	if (!Crosshair)
	{
		if (CrosshairClass)
		{
			Crosshair = CreateWidget<UMECrosshairWidget>(this, CrosshairClass);
			Crosshair->SetDefaultCrosshair(DefaultCrosshair);
			Crosshair->AddToViewport();
		}
		else
		{
			UE_LOG(LogME, Warning, TEXT("%s: CrosshairClass is not set, no crosshair."), *GetName());
		}
	}

	if (!InteractionPrompt)
	{
		if (!InteractionPromptClass)
		{
			UE_LOG(LogME, Error, TEXT("%s: InteractionPromptClass is not set, no interaction prompt."), *GetName());
			return;
		}
		InteractionPrompt = CreateWidget<UMEInteractionPromptWidget>(this, InteractionPromptClass);
		InteractionPrompt->AddToViewport();
	}

	if (const AMECharacter* MECharacter = Cast<AMECharacter>(InPawn))
	{
		UMEInteractionComponent* Interaction = MECharacter->GetInteraction();
		Interaction->OnFocusChanged.AddUObject(this, &AMEPlayerController::HandleInteractionFocusChanged);
		Interaction->OnProgress.AddUObject(this, &AMEPlayerController::HandleInteractionProgress);
		BoundInteraction = Interaction;

		UMEWeaponComponent* Weapons = MECharacter->GetWeapons();
		Weapons->OnTargetHit.AddUObject(this, &AMEPlayerController::HandleTargetHit);
		BoundWeapons = Weapons;
	}
}

void AMEPlayerController::OnUnPossess()
{
	if (UMEInteractionComponent* Interaction = BoundInteraction.Get())
	{
		Interaction->OnFocusChanged.RemoveAll(this);
		Interaction->OnProgress.RemoveAll(this);
	}
	BoundInteraction.Reset();

	if (UMEWeaponComponent* Weapons = BoundWeapons.Get())
	{
		Weapons->OnTargetHit.RemoveAll(this);
	}
	BoundWeapons.Reset();

	if (InteractionPrompt)
	{
		InteractionPrompt->HidePrompt();
	}

	Super::OnUnPossess();
}

void AMEPlayerController::HandleInteractionFocusChanged(bool bHasFocus, const FMEInteractionInfo& Info)
{
	if (bHasFocus)
	{
		InteractionPrompt->ShowPrompt(GetInteractKeyText(), Info.ActionText, Info.bCanInteract, Info.HoldDuration > 0.f);
	}
	else
	{
		InteractionPrompt->HidePrompt();
	}
}

void AMEPlayerController::HandleInteractionProgress(float Progress)
{
	InteractionPrompt->SetProgress(Progress);
}

void AMEPlayerController::HandleTargetHit(bool bKilled)
{
	if (Crosshair)
	{
		Crosshair->ShowHitMarker(bKilled);
	}
}

FText AMEPlayerController::GetInteractKeyText() const
{
	const AMECharacter* MECharacter = Cast<AMECharacter>(GetPawn());
	const UMEInputConfig* InputConfig = MECharacter ? MECharacter->GetInputConfig() : nullptr;
	const UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());

	if (InputConfig && InputConfig->InteractAction && Subsystem)
	{
		const TArray<FKey> Keys = Subsystem->QueryKeysMappedToAction(InputConfig->InteractAction);
		if (Keys.Num() > 0)
		{
			return Keys[0].GetDisplayName(false);
		}
	}
	return FText::FromString(TEXT("E"));
}
