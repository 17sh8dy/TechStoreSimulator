#include "TSSCheckoutScreens.h"
#include "TSSRegister.h"
#include "TSSProducts.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor ScrBg(0.025f, 0.04f, 0.07f, 1.f);
	const FLinearColor ScrAccent(0.12f, 0.5f, 0.95f, 1.f);
	const FLinearColor ScrMuted(0.62f, 0.7f, 0.8f, 1.f);
	const FLinearColor ScrGood(0.35f, 0.95f, 0.6f, 1.f);
	constexpr int32 ScrRows = 7;

	FSlateFontInfo ScrFont(const TCHAR* Style, int32 Size) { return FCoreStyle::GetDefaultFontStyle(Style, Size); }
}

// ---- monitor --------------------------------------------------------------------------------------

FText UTSSMonitorWidget::RowName(int32 Row) const
{
	if (!Reg.IsValid() || !Reg->GetLines().IsValidIndex(Row)) return FText::GetEmpty();
	const FTSSSaleLine& L = Reg->GetLines()[Row];
	return FText::FromString(FString::Printf(TEXT("%dx  %s"), L.Qty, TSSProductDef(L.Product).Name));
}

FText UTSSMonitorWidget::RowAmount(int32 Row) const
{
	if (!Reg.IsValid() || !Reg->GetLines().IsValidIndex(Row)) return FText::GetEmpty();
return FText::FromString(TSSMoney::Format(Reg->GetLines()[Row].Total() * 100));
}

FText UTSSMonitorWidget::StatusText() const
{
	if (!Reg.IsValid()) return FText::GetEmpty();
	switch (Reg->GetPhase())
	{
	case ETSSCheckoutPhase::Idle: return FText::FromString(TEXT("READY"));
	case ETSSCheckoutPhase::Scanning: return FText::FromString(TEXT("SCANNING"));
	case ETSSCheckoutPhase::WaitingForPayment: return FText::FromString(TEXT("AWAITING PAYMENT"));
	case ETSSCheckoutPhase::CashDue: return FText::FromString(TEXT("CASH PAYMENT"));
	case ETSSCheckoutPhase::Receipt:
	case ETSSCheckoutPhase::Collecting: return FText::FromString(TEXT("COMPLETE"));
	default: return FText::FromString(TEXT("CARD PAYMENT"));
	}
}

FText UTSSMonitorWidget::SubtotalText() const { return FText::FromString(TSSMoney::Format(Reg.IsValid() ? Reg->GetSubtotal() : 0)); }
FText UTSSMonitorWidget::TaxText() const { return FText::FromString(TSSMoney::Format(Reg.IsValid() ? Reg->GetTax() : 0)); }
FText UTSSMonitorWidget::TotalText() const { return FText::FromString(TSSMoney::Format(Reg.IsValid() ? Reg->GetTotal() : 0)); }

FText UTSSMonitorWidget::FooterText() const
{
	if (!Reg.IsValid()) return FText::GetEmpty();
	if (!Reg->GetNotice().IsEmpty()) return FText::FromString(Reg->GetNotice());
	switch (Reg->GetPhase())
	{
	case ETSSCheckoutPhase::Idle:
		return FText::FromString(Reg->GetLastTotal() > 0 ?FString::Printf(TEXT("Last sale  %s"), *TSSMoney::Format(Reg->GetLastTotal())) : FString(TEXT("Waiting for the next customer")));
	case ETSSCheckoutPhase::CashDue:
		return FText::FromString(FString::Printf(TEXT("Received %s   Change due %s   Giving %s"), *TSSMoney::Format(Reg->GetCashReceived()), *TSSMoney::Format(Reg->GetChangeDue()), *TSSMoney::Format(Reg->GetChangeGiven())));
	case ETSSCheckoutPhase::CardTap:
	case ETSSCheckoutPhase::CardReady:
	case ETSSCheckoutPhase::CardProcessing:
	case ETSSCheckoutPhase::CardResult:
		return FText::FromString(FString::Printf(TEXT("Card terminal:  %s"), *Reg->TerminalStatus()));
	default: return FText::GetEmpty();
	}
}

TSharedRef<SWidget> UTSSMonitorWidget::RebuildWidget()
{
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (int32 i = 0; i < ScrRows; ++i)
	{
		List->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(STextBlock).Text_UObject(this, &UTSSMonitorWidget::RowName, i).Font(ScrFont(TEXT("Regular"), 22))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock).Text_UObject(this, &UTSSMonitorWidget::RowAmount, i).Font(ScrFont(TEXT("Bold"), 22))
			]
		];
	}

	auto Line = [this](const TCHAR* Caption, FText (UTSSMonitorWidget::*Value)() const, int32 Size, FLinearColor Color) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(STextBlock).Text(FText::FromString(Caption)).Font(ScrFont(TEXT("Regular"), Size)).ColorAndOpacity(ScrMuted)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock).Text_UObject(this, Value).Font(ScrFont(TEXT("Bold"), Size)).ColorAndOpacity(Color)
			];
	};

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(ScrBg)
		.Padding(0.f)
		[
			SNew(SVerticalBox)
			// Header
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(ScrAccent)
				.Padding(FMargin(18.f, 8.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("TECH STORE  POS"))).Font(ScrFont(TEXT("Bold"), 24))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock).Text_UObject(this, &UTSSMonitorWidget::StatusText).Font(ScrFont(TEXT("Bold"), 20))
					]
				]
			]
			// Items
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(FMargin(22.f, 10.f))
			[
				List
			]
			// Totals
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(22.f, 0.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ Line(TEXT("Subtotal"), &UTSSMonitorWidget::SubtotalText, 20, ScrMuted) ]
				+ SVerticalBox::Slot().AutoHeight()[ Line(TEXT("Tax  8%"), &UTSSMonitorWidget::TaxText, 20, ScrMuted) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[ Line(TEXT("TOTAL"), &UTSSMonitorWidget::TotalText, 34, ScrGood) ]
			]
			// Status / notice
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(FLinearColor(0.06f, 0.09f, 0.14f, 1.f))
				.Padding(FMargin(22.f, 8.f))
				[
					SNew(STextBlock).Text_UObject(this, &UTSSMonitorWidget::FooterText).Font(ScrFont(TEXT("Regular"), 19)).ColorAndOpacity(FLinearColor(0.85f, 0.92f, 1.f, 1.f))
				]
			]
		];
}

// ---- card terminal --------------------------------------------------------------------------------

FText UTSSTerminalWidget::AmountText() const
{
	const bool bShow = Reg.IsValid() && Reg->GetTotal() > 0 && Reg->GetPhase() != ETSSCheckoutPhase::Idle;
return FText::FromString(bShow ? TSSMoney::Format(Reg->GetTotal()) : FString(TEXT("$0.00")));
}

FText UTSSTerminalWidget::StatusText() const
{
	return FText::FromString(Reg.IsValid() ? Reg->TerminalStatus() : FString(TEXT("STANDBY")));
}

FSlateColor UTSSTerminalWidget::StatusColor() const
{
	return FSlateColor(Reg.IsValid() ? Reg->TerminalColor() : FLinearColor(0.1f, 0.12f, 0.16f));
}

TSharedRef<SWidget> UTSSTerminalWidget::RebuildWidget()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(ScrBg)
		.Padding(FMargin(14.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("TECH PAY"))).Font(ScrFont(TEXT("Bold"), 20)).ColorAndOpacity(ScrAccent)
			]
			+ SVerticalBox::Slot().FillHeight(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text_UObject(this, &UTSSTerminalWidget::AmountText).Font(ScrFont(TEXT("Bold"), 52))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor_UObject(this, &UTSSTerminalWidget::StatusColor)
				.Padding(FMargin(8.f, 14.f))
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock).Text_UObject(this, &UTSSTerminalWidget::StatusText).Font(ScrFont(TEXT("Bold"), 22))
				]
			]
		];
}
