#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MEInteractionPromptWidget.generated.h"

class UTextBlock;
class UProgressBar;

/**
 * Contextual interaction prompt ("[E] Open") with a bar for held interactions.
 * Logic only: the layout lives in WBP_InteractionPrompt, which must contain
 * a TextBlock named PromptText and a ProgressBar named HoldBar.
 */
UCLASS(Abstract)
class METROEXODUSLIKE_API UMEInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowPrompt(const FText& KeyText, const FText& ActionText, bool bCanInteract, bool bIsHold);
	void HidePrompt();
	void SetProgress(float Progress);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PromptText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HoldBar;

	/** {Key} and {Action} are replaced by the bound key and the interaction verb. */
	UPROPERTY(EditDefaultsOnly, Category = "Prompt")
	FText PromptFormat = NSLOCTEXT("MEInteractionPrompt", "Format", "[{Key}] {Action}");

	UPROPERTY(EditDefaultsOnly, Category = "Prompt")
	FSlateColor AvailableColor = FLinearColor(1.f, 1.f, 1.f, 0.85f);

	/** Used when the interaction is shown but not possible (locked door...). */
	UPROPERTY(EditDefaultsOnly, Category = "Prompt")
	FSlateColor UnavailableColor = FLinearColor(0.6f, 0.6f, 0.6f, 0.7f);
};
