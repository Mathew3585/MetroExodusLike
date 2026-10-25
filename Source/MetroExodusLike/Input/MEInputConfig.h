#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MEInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Groups every input asset the player uses, so the character only needs one reference.
 * Create one in the editor (DA_InputConfig) and assign it on BP_MECharacter.
 */
UCLASS()
class METROEXODUSLIKE_API UMEInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> InspectAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> SwitchFireModeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> MedkitAction;

	/** Tap: put on / take off the gas mask. Hold: change the filter. */
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> GasMaskAction;

	/** Wipe the gas mask visor. */
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> WipeVisorAction;
};
