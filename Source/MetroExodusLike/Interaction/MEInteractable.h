#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MEInteractable.generated.h"

class AMECharacter;

/**
 * Left-hand gesture the player makes for an interaction (the weapon stays in the right hand).
 * Drives the gesture animation once weapons and hands exist (Phase 3).
 */
UENUM(BlueprintType)
enum class EMEInteractionGesture : uint8
{
	Pickup,		// Grab an item
	Door,		// Push / pull a door
	Switch,		// Flip a switch, pull a lever
	Use			// Generic
};

/** What an interactable offers right now, shown in the prompt. */
USTRUCT(BlueprintType)
struct FMEInteractionInfo
{
	GENERATED_BODY()

	/** Verb shown in the prompt ("Open", "Pick up ammo"...). */
	UPROPERTY(BlueprintReadOnly)
	FText ActionText;

	UPROPERTY(BlueprintReadOnly)
	EMEInteractionGesture Gesture = EMEInteractionGesture::Use;

	/** Seconds the key must be held. 0 = instant. */
	UPROPERTY(BlueprintReadOnly)
	float HoldDuration = 0.f;

	/** false = shown but not usable (locked door...). */
	UPROPERTY(BlueprintReadOnly)
	bool bCanInteract = true;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UMEInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything the player can interact with. Implemented by actors, found by UMEInteractionComponent.
 */
class METROEXODUSLIKE_API IMEInteractable
{
	GENERATED_BODY()

public:
	virtual FMEInteractionInfo GetInteractionInfo(const AMECharacter* Interactor) const = 0;
	virtual void Interact(AMECharacter* Interactor) = 0;
};
