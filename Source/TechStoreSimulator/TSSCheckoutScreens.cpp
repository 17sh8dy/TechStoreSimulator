#include "TSSCheckoutScreens.h"
#include "TSSRegister.h"
#include "TSSProducts.h"
#include "TSSBankCard.h"
#include "TSSBusinessSubsystem.h"
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
						SNew(STextBlock).Text_Lambda([this]()
						{
							const UWorld* W = Reg.IsValid() ? Reg->GetWorld() : nullptr;
							const UTSSBusinessSubsystem* B = W ? W->GetSubsystem<UTSSBusinessSubsystem>() : nullptr;
							return FText::FromString((B ? B->GetStoreName() : FString(TEXT("Tech Store"))).ToUpper() + TEXT("  POS"));
						}).Font(ScrFont(TEXT("Bold"), 24))
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

FText UTSSTerminalWidget::CardText() const
{
	if (!Reg.IsValid() || !Reg->HasCard()) return FText::GetEmpty();
	const FTSSBankCard& C = Reg->GetCard();
	FString Out = FString::Printf(TEXT("%s DEBIT\n%s\nEXP %s"), *C.Network, *C.Masked(), *C.Expiry());
	if (Reg->GetPhase() == ETSSCheckoutPhase::CardPin) Out += FString::Printf(TEXT("\n\nPIN  %s   %d/4"), *TSSCards::PinDots(Reg->GetPinTyped()), FMath::Clamp(Reg->GetPinTyped(), 0, 4));
	return FText::FromString(Out);
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
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ SNew(STextBlock).Text_UObject(this, &UTSSTerminalWidget::AmountText).Font(ScrFont(TEXT("Bold"), 34)) ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f, 0.f, 0.f)
					[
						SNew(STextBlock).Text_UObject(this, &UTSSTerminalWidget::CardText).Font(ScrFont(TEXT("Regular"), 14)).ColorAndOpacity(ScrMuted).Justification(ETextJustify::Center)
					]
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

// ---- receipt ----------------------------------------------------------------------------------------

namespace
{
	const FLinearColor PaperCol(0.97f, 0.96f, 0.91f, 1.f);
	const FLinearColor InkCol(0.09f, 0.09f, 0.11f, 1.f);
	const FLinearColor InkSoft(0.38f, 0.38f, 0.42f, 1.f);

	TSharedRef<SWidget> RcText(const FString& S, int32 Size, bool bBold, ETextJustify::Type J = ETextJustify::Left, FLinearColor Col = InkCol)
	{
		return SNew(STextBlock).Text(FText::FromString(S)).Font(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size)).ColorAndOpacity(Col).Justification(J);
	}

	TSharedRef<SWidget> RcRow(const FString& Left, const FString& Right, int32 Size, bool bBold)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[ RcText(Left, Size, bBold) ]
			+ SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f, 0.f, 0.f)[ RcText(Right, Size, bBold) ];
	}

	TSharedRef<SWidget> RcRule(bool bBold = false)
	{
		return SNew(SBox).Padding(FMargin(0.f, 5.f))
			[
				RcText(bBold ? TEXT("==========================") : TEXT("- - - - - - - - - - - - - - - - - -"), 12, false, ETextJustify::Center, InkSoft)
			];
	}

	TSharedRef<SWidget> RcBarcode(int32 Seed)
	{
		TSharedRef<SHorizontalBox> Bars = SNew(SHorizontalBox);
		FRandomStream R(Seed * 7919 + 13);
		for (int32 i = 0; i < 46; ++i)
		{
			const float W = 1.f + R.RandRange(0, 3);
			const bool bBar = (i % 2) == 0;
			Bars->AddSlot().AutoWidth()
			[
				SNew(SBox).WidthOverride(W).HeightOverride(46.f)
				[
					SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(bBar ? InkCol : PaperCol)
				]
			];
		}
		return Bars;
	}
}

TSharedRef<SWidget> UTSSReceiptWidget::RebuildWidget()
{
	Host = SNew(SBox);
	SetReceipt(TEXT("Tech Store"), FTSSTransaction(), {});
	return Host.ToSharedRef();
}

void UTSSReceiptWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Host.Reset();
}

void UTSSReceiptWidget::SetReceipt(const FString& StoreName, const FTSSTransaction& Sale, const TArray<FTSSSaleLine>& Lines)
{
	if (!Host.IsValid()) return;
	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	auto Add = [&Body](TSharedRef<SWidget> W, float PadTop = 0.f) { Body->AddSlot().AutoHeight().Padding(0.f, PadTop, 0.f, 0.f)[ W ]; };

	Add(RcText(StoreName.ToUpper(), StoreName.Len() > 14 ? 24 : 32, true, ETextJustify::Center));
	Add(RcText(TEXT("100 Commerce Way, Harbor City"), 13, false, ETextJustify::Center, InkSoft), 2.f);
	Add(RcText(TEXT("Open every day  -  Thank you for shopping"), 11, false, ETextJustify::Center, InkSoft));
	Add(RcRule());
	Add(RcRow(UTSSBusinessSubsystem::FormatDate(Sale.Day), UTSSBusinessSubsystem::FormatClock(Sale.TimeMinutes / 60.f), 14, false));
	Add(RcRow(FString::Printf(TEXT("Receipt #%05d"), Sale.Id), Sale.Method == ETSSPayMethod::Card ? TEXT("CARD") : TEXT("CASH"), 14, false));
	Add(RcRule());
	for (const FTSSSaleLine& L : Lines)
	{
		Add(RcRow(FString::Printf(TEXT("%s"), TSSProductDef(L.Product).Name), TSSMoney::Format(L.Total() * 100), 16, true), 2.f);
		Add(RcText(FString::Printf(TEXT("    %d x %s"), L.Qty, *TSSMoney::Format(L.UnitPrice * 100)), 12, false, ETextJustify::Left, InkSoft));
	}
	Add(RcRule());
	Add(RcRow(TEXT("Subtotal"), TSSMoney::Format(Sale.Subtotal), 15, false));
	Add(RcRow(FString::Printf(TEXT("Sales tax (%.2f%%)"), TSSMoney::TaxRate * 100.f), TSSMoney::Format(Sale.Tax), 15, false), 2.f);
	Add(RcRule(true));
	Add(RcRow(TEXT("TOTAL"), TSSMoney::Format(Sale.Total), 24, true));
	Add(RcRule(true));
	if (Sale.Method == ETSSPayMethod::Cash)
	{
		Add(RcRow(TEXT("Cash tendered"), TSSMoney::Format(Sale.CashReceived), 15, false));
		Add(RcRow(TEXT("Change due"), TSSMoney::Format(Sale.Change), 15, true), 2.f);
	}
	else if (Sale.Method == ETSSPayMethod::Card)
	{
		Add(RcRow(FString::Printf(TEXT("%s DEBIT"), *Sale.CardNetwork), FString::Printf(TEXT("**** %s"), *Sale.CardLast4), 15, false));
		Add(RcRow(TEXT("Authorisation"), Sale.AuthCode, 15, false), 2.f);
		Add(RcText(TEXT("APPROVED"), 20, true, ETextJustify::Center), 6.f);
	}
	Add(RcRule());
	Add(RcText(TEXT("THANK YOU!"), 26, true, ETextJustify::Center), 4.f);
	Add(RcText(TEXT("Keep this receipt for returns"), 12, false, ETextJustify::Center, InkSoft), 2.f);
	Add(RcText(TEXT("within 14 days"), 12, false, ETextJustify::Center, InkSoft));
	Body->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 12.f, 0.f, 0.f)[ RcBarcode(Sale.Id) ];
	Add(RcText(FString::Printf(TEXT("%05d  %06d"), Sale.Id, Sale.AuthCode.IsEmpty() ? Sale.Total : FCString::Atoi(*Sale.AuthCode)), 11, false, ETextJustify::Center, InkSoft), 3.f);

	Host->SetContent(
		SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(PaperCol).Padding(FMargin(18.f, 16.f))
		[ Body ]);
}
