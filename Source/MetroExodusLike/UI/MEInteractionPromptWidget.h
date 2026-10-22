#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MEInteractionPromptWidget.generated.h"

class UTextBlock;
class UProgressBar;

/**
 * Small contextual prompt under the screen center ("[E] Open"), with a thin bar for held interactions.
 * Built entirely in C++, no widget Blueprint needed.
 */
UCLASS()
class METROEXODUSLIKE_API UMEInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowPrompt(const FText& KeyText, const FText& ActionText, bool bCanInteract, bool bIsHold);
	void HidePrompt();
	void SetProgress(float Progress);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PromptText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HoldBar;
};
