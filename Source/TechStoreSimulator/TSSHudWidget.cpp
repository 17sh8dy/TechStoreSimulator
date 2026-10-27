#include "TSSHudWidget.h"
#include "TSSCharacter.h"
#include "TSSStoreSubsystem.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor Panel(0.015f, 0.025f, 0.04f, 0.72f);
	const FLinearColor Accent(0.12f, 0.5f, 0.95f, 1.f);
	const FLinearColor Muted(0.7f, 0.76f, 0.84f, 1.f);
}

ATSSCharacter* UTSSHudWidget::GetPlayer() const
{
	APlayerController* PC = GetOwningPlayer();
	return PC ? Cast<ATSSCharacter>(PC->GetPawn()) : nullptr;
}

FText UTSSHudWidget::MoneyText() const
{
	const UWorld* World = GetWorld();
	const UTSSStoreSubsystem* Store = World ? World->GetSubsystem<UTSSStoreSubsystem>() : nullptr;
	return FText::FromString(FString::Printf(TEXT("$%d"), Store ? Store->GetMoney() : 0));
}

FText UTSSHudWidget::HeldText() const
{
	const ATSSCharacter* Player = GetPlayer();
	if (!Player || Player->GetHeld() == ETSSProduct::None)
	{
		return FText::FromString(TEXT("Hands empty"));
	}
	return FText::FromString(FString::Printf(TEXT("Holding: %s"), TSSProductDef(Player->GetHeld()).Name));
}

FText UTSSHudWidget::PromptText() const
{
	const ATSSCharacter* Player = GetPlayer();
	return FText::FromString(Player ? Player->GetFocusPrompt() : FString());
}

EVisibility UTSSHudWidget::PromptVisibility() const
{
	const ATSSCharacter* Player = GetPlayer();
	return (Player && !Player->GetFocusPrompt().IsEmpty()) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

TSharedRef<SWidget> UTSSHudWidget::RebuildWidget()
{
	return SNew(SOverlay)
		// Cash and hands, top left.
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(Panel)
			.Padding(FMargin(18.f, 12.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text_UObject(this, &UTSSHudWidget::MoneyText)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 30))
					.ColorAndOpacity(Accent)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text_UObject(this, &UTSSHudWidget::HeldText)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
					.ColorAndOpacity(Muted)
				]
			]
		]
		// Crosshair.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(6.f).HeightOverride(6.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.85f))
			]
		]
		// Interaction prompt, below the crosshair.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0.f, 120.f, 0.f, 0.f)
		[
			SNew(SBorder)
			.Visibility_UObject(this, &UTSSHudWidget::PromptVisibility)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(Panel)
			.Padding(FMargin(20.f, 10.f))
			[
				SNew(STextBlock)
				.Text_UObject(this, &UTSSHudWidget::PromptText)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
				.Justification(ETextJustify::Center)
			]
		]
		// Controls reminder, bottom.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 18.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("WASD move   |   Mouse look   |   E interact   |   Wheel / [ ] price   |   Esc menu")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
			.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.55f))
		];
}
