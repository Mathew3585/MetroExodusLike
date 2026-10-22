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
};
