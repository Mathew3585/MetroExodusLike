#include "UI/MEInteractionPromptWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "MEInteractionPrompt"

namespace
{
	// Discreet, Metro-like: small, slightly transparent, below the crosshair position
	const FLinearColor PromptColor(1.f, 1.f, 1.f, 0.85f);
	const FLinearColor DisabledColor(0.6f, 0.6f, 0.6f, 0.7f);
	const FVector2D PromptOffset(0.f, 70.f);
	const FVector2D HoldBarSize(120.f, 3.f);
	constexpr int32 FontSize = 14;
}

void UMEInteractionPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	UCanvasPanelSlot* BoxSlot = Root->AddChildToCanvas(Box);
	BoxSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	BoxSlot->SetAlignment(FVector2D(0.5f, 0.f));
	BoxSlot->SetPosition(PromptOffset);
	BoxSlot->SetAutoSize(true);

	PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PromptText"));
	FSlateFontInfo Font = PromptText->GetFont();
	Font.Size = FontSize;
	PromptText->SetFont(Font);
	PromptText->SetShadowOffset(FVector2D(1.f, 1.f));
	PromptText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	PromptText->SetJustification(ETextJustify::Center);
	Box->AddChildToVerticalBox(PromptText)->SetHorizontalAlignment(HAlign_Center);

	// Thin bar, sized by a size box
	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HoldBarSize"));
	BarSize->SetWidthOverride(HoldBarSize.X);
	BarSize->SetHeightOverride(HoldBarSize.Y);
	UVerticalBoxSlot* BarSlot = Box->AddChildToVerticalBox(BarSize);
	BarSlot->SetHorizontalAlignment(HAlign_Center);
	BarSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

	HoldBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HoldBar"));
	HoldBar->SetFillColorAndOpacity(PromptColor);
	HoldBar->SetRenderOpacity(0.8f);
	BarSize->AddChild(HoldBar);

	HidePrompt();
}

void UMEInteractionPromptWidget::ShowPrompt(const FText& KeyText, const FText& ActionText, bool bCanInteract, bool bIsHold)
{
	PromptText->SetText(bCanInteract
		? FText::Format(LOCTEXT("Prompt", "[{0}] {1}"), KeyText, ActionText)
		: ActionText);
	PromptText->SetColorAndOpacity(bCanInteract ? PromptColor : DisabledColor);

	HoldBar->SetPercent(0.f);
	HoldBar->GetParent()->SetVisibility(bCanInteract && bIsHold ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

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

#undef LOCTEXT_NAMESPACE
