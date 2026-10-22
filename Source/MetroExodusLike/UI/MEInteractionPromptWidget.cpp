#include "UI/MEInteractionPromptWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UMEInteractionPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	HidePrompt();
}

void UMEInteractionPromptWidget::ShowPrompt(const FText& KeyText, const FText& ActionText, bool bCanInteract, bool bIsHold)
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("Key"), KeyText);
	Args.Add(TEXT("Action"), ActionText);

	PromptText->SetText(bCanInteract ? FText::Format(PromptFormat, Args) : ActionText);
	PromptText->SetColorAndOpacity(bCanInteract ? AvailableColor : UnavailableColor);

	HoldBar->SetPercent(0.f);
	HoldBar->SetVisibility(bCanInteract && bIsHold ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMEInteractionPromptWidget::HidePrompt()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMEInteractionPromptWidget::SetProgress(float Progress)
{
	HoldBar->SetPercent(Progress);
}
