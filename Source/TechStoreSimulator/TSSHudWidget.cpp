#include "TSSHudWidget.h"
#include "TSSCharacter.h"
#include "TSSStoreSubsystem.h"
#include "TSSBusinessSubsystem.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor HudPanel(0.015f, 0.025f, 0.04f, 0.72f);
	const FLinearColor HudAccent(0.12f, 0.5f, 0.95f, 1.f);
	const FLinearColor HudMuted(0.7f, 0.76f, 0.84f, 1.f);
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
return FText::FromString(TSSMoney::Format(Store ? Store->GetMoneyCents() : 0));
}

FText UTSSHudWidget::HeldText() const
{
	const ATSSCharacter* Player = GetPlayer();
	if (Player && Player->IsCarryingEmptyBox()) return FText::FromString(TEXT("Carrying an empty box (take it to a bin)"));
	if (!Player || Player->GetHeld() == ETSSProduct::None)
	{
		return FText::FromString(TEXT("Hands empty"));
	}
	return FText::FromString(FString::Printf(TEXT("Holding: %s"), TSSProductDef(Player->GetHeld()).Name));
}

FText UTSSHudWidget::SalesText() const
{
	const UWorld* World = GetWorld();
	const UTSSStoreSubsystem* Store = World ? World->GetSubsystem<UTSSStoreSubsystem>() : nullptr;
	return FText::FromString(FString::Printf(TEXT("%s   Sales: %d   Cash $%d   Card $%d   Walked out: %d"),
		(Store && !Store->IsOpen()) ? TEXT("CLOSED") : TEXT("OPEN"), Store ? Store->GetServed() : 0, Store ? Store->GetCashSales() : 0, Store ? Store->GetCardSales() : 0, Store ? Store->GetLost() : 0));
}

FText UTSSHudWidget::ClockText() const
{
	const UWorld* World = GetWorld();
	const UTSSBusinessSubsystem* Biz = World ? World->GetSubsystem<UTSSBusinessSubsystem>() : nullptr;
	return FText::FromString(Biz ? UTSSBusinessSubsystem::FormatClock(Biz->GetHour()) : FString(TEXT("--:--")));
}

FText UTSSHudWidget::LevelText() const
{
	const UWorld* World = GetWorld();
	const UTSSBusinessSubsystem* Biz = World ? World->GetSubsystem<UTSSBusinessSubsystem>() : nullptr;
	return FText::FromString(FString::Printf(TEXT("LEVEL %d"), Biz ? Biz->GetLevel() : 1));
}

TOptional<float> UTSSHudWidget::LevelFraction() const
{
	const UWorld* World = GetWorld();
	const UTSSBusinessSubsystem* Biz = World ? World->GetSubsystem<UTSSBusinessSubsystem>() : nullptr;
	if (!Biz) return 0.f;
	const int32 L = Biz->GetLevel();
	const int32 From = UTSSBusinessSubsystem::XPForLevel(L), To = UTSSBusinessSubsystem::XPForLevel(L + 1);
	return To > From ? FMath::Clamp(static_cast<float>(Biz->GetXP() - From) / static_cast<float>(To - From), 0.f, 1.f) : 1.f;
}

FText UTSSHudWidget::XpText() const
{
	const UWorld* World = GetWorld();
	const UTSSBusinessSubsystem* Biz = World ? World->GetSubsystem<UTSSBusinessSubsystem>() : nullptr;
	if (!Biz) return FText::GetEmpty();
	return FText::FromString(FString::Printf(TEXT("%d / %d XP"), Biz->GetXP(), UTSSBusinessSubsystem::XPForLevel(Biz->GetLevel() + 1)));
}

FText UTSSHudWidget::DayText() const
{
	const UWorld* World = GetWorld();
	const UTSSStoreSubsystem* Store = World ? World->GetSubsystem<UTSSStoreSubsystem>() : nullptr;
	return FText::FromString(FString::Printf(TEXT("Day %d"), Store ? Store->GetDay() : 1));
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
			.BorderBackgroundColor(HudPanel)
			.Padding(FMargin(18.f, 12.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text_UObject(this, &UTSSHudWidget::MoneyText)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 30))
					.ColorAndOpacity(HudAccent)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text_UObject(this, &UTSSHudWidget::HeldText)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
					.ColorAndOpacity(HudMuted)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text_UObject(this, &UTSSHudWidget::SalesText)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
					.ColorAndOpacity(HudMuted)
				]
				// Level and a progress bar to the next level.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_UObject(this, &UTSSHudWidget::LevelText)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
						.ColorAndOpacity(HudAccent)
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)
					[
						SNew(SBox).HeightOverride(10.f).MinDesiredWidth(150.f)
						[
							SNew(SProgressBar)
							.Percent_UObject(this, &UTSSHudWidget::LevelFraction)
							.FillColorAndOpacity(HudAccent)
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f).HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Text_UObject(this, &UTSSHudWidget::XpText)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
					.ColorAndOpacity(HudMuted)
				]
			]
		]
		// The clock, top centre.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 24.f, 0.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(HudPanel)
			.Padding(FMargin(22.f, 8.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text_UObject(this, &UTSSHudWidget::ClockText)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 26))
					.ColorAndOpacity(HudAccent)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text_UObject(this, &UTSSHudWidget::DayText)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
					.ColorAndOpacity(HudMuted)
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
			.BorderBackgroundColor(HudPanel)
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
			.Text(FText::FromString(TEXT("WASD move   |   Mouse look   |E or Left-click interact   |   Wheel / [ ] price   |   Esc menu")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
			.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.55f))
		];
}
