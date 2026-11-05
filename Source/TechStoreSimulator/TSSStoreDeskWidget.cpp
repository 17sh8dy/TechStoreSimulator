#include "TSSStoreDeskWidget.h"
#include "TSSBusinessSubsystem.h"
#include "TSSStoreSubsystem.h"
#include "Brushes/SlateNoResource.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor WinCrt(0.35f, 1.f, 0.45f, 1.f);

	FSlateFontInfo WinFont(bool bBold, int32 Size) { return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size); }

	FString WinMoney(int32 V)
	{
		FString S = FString::FromInt(FMath::Abs(V));
		for (int32 i = S.Len() - 3; i > 0; i -= 3) S.InsertAt(i, TEXT(","));
		return (V < 0 ? TEXT("-$") : TEXT("$")) + S;
	}
}

// ---- the application window -----------------------------------------------------------------------

void UTSSStoreDeskWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UWorld* World = GetWorld();
	Ctx.World = World;
	Ctx.Store = World ? World->GetSubsystem<UTSSStoreSubsystem>() : nullptr;
	Ctx.Biz = World ? World->GetSubsystem<UTSSBusinessSubsystem>() : nullptr;
	Ctx.Refresh = [this]() { MarkDirty(); };
	if (Ctx.Store) StoreHandle = Ctx.Store->OnChanged.AddUObject(this, &UTSSStoreDeskWidget::MarkDirty);
	if (Ctx.Biz) BizHandle = Ctx.Biz->OnChanged.AddUObject(this, &UTSSStoreDeskWidget::MarkDirty);
	bDirty = true;
}

void UTSSStoreDeskWidget::NativeDestruct()
{
	if (Ctx.Store) Ctx.Store->OnChanged.Remove(StoreHandle);
	if (Ctx.Biz) Ctx.Biz->OnChanged.Remove(BizHandle);
	Super::NativeDestruct();
}

void UTSSStoreDeskWidget::SetPage(int32 Index)
{
	PageIndex = FMath::Clamp(Index, 0, TSSDeskPages().Num() - 1);
	Ctx.Message.Reset();
	bDirty = true;
}

void UTSSStoreDeskWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshPage();
}

void UTSSStoreDeskWidget::RefreshPage()
{
	if (!bDirty || !PageHost.IsValid()) return;
	if (UWorld* World = GetWorld())
	{
		Ctx.World = World;
		if (!Ctx.Store) Ctx.Store = World->GetSubsystem<UTSSStoreSubsystem>();
		if (!Ctx.Biz) Ctx.Biz = World->GetSubsystem<UTSSBusinessSubsystem>();
	}
	if (!Ctx.Store || !Ctx.Biz) return;
	bDirty = false;
	PageHost->SetContent(TSSDeskPages()[PageIndex]->Build(Ctx));
}

TSharedRef<SWidget> UTSSStoreDeskWidget::RebuildWidget()
{
	TSharedRef<SBox> Root = SAssignNew(RootHost, SBox)[ BuildWindow() ];
	Root->RegisterActiveTimer(0.05f, FWidgetActiveTimerDelegate::CreateLambda([Weak = TWeakObjectPtr<UTSSStoreDeskWidget>(this)](double, float)
	{
		if (Weak.IsValid()) Weak->RefreshPage();
		return EActiveTimerReturnType::Continue;
	}));
	return Root;
}

void UTSSStoreDeskWidget::SetDark(bool bDark)
{
	TSSDeskSetDark(bDark);
	if (RootHost.IsValid()) RootHost->SetContent(BuildWindow());
	bDirty = true;
}

TSharedRef<SWidget> UTSSStoreDeskWidget::BuildWindow()
{
	const FTSSDeskTheme& Th = TSSDeskTheme();
	const bool bDark = TSSDeskDark();

	// Side menu: one button per registered page.
	TSharedRef<SVerticalBox> Menu = SNew(SVerticalBox);
	const TArray<TSharedRef<FTSSDeskPage>>& Pages = TSSDeskPages();
	for (int32 i = 0; i < Pages.Num(); ++i)
	{
		Menu->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(SButton)
			.ButtonStyle(&TSSDeskButtonStyle())
			.ButtonColorAndOpacity_Lambda([this, i, &Th]() { return FSlateColor(PageIndex == i ? Th.Select : Th.Button); })
			.ContentPadding(FMargin(10.f, 9.f))
			.OnClicked_Lambda([this, i]() { SetPage(i); return FReply::Handled(); })
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(STextBlock).Text(Pages[i]->Title()).Font(WinFont(true, 15)).ColorAndOpacity(Th.Ink) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(WinFont(true, 11)).ColorAndOpacity(Th.Bad)
					.Text_Lambda([this, i]() { return FText::FromString(Ctx.Biz ? TSSDeskPages()[i]->Badge(Ctx.Biz->GetLevel()) : FString()); })
				]
			]
		];
	}

	PageHost = SNew(SBox);

	// Dark-mode pill: a rounded switch with a sliding knob.
	static const FSlateRoundedBoxBrush Round(FLinearColor::White, 11.f);
	static const FSlateNoResource NoBrush;
	static const FButtonStyle PillStyle = FButtonStyle().SetNormal(NoBrush).SetHovered(NoBrush).SetPressed(NoBrush).SetDisabled(NoBrush);
	TSharedRef<SWidget> Pill =
		SNew(SButton)
		.ButtonStyle(&PillStyle)
		.ContentPadding(FMargin(0.f))
		.ToolTipText(FText::FromString(TEXT("Switch between light and dark mode")))
		.OnClicked_Lambda([this, bDark]() { SetDark(!bDark); return FReply::Handled(); })
		[
			SNew(SBorder)
			.BorderImage(&Round)
			.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.22f))
			.Padding(FMargin(12.f, 3.f, 4.f, 3.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(STextBlock).Text(FText::FromString(bDark ? TEXT("Dark mode") : TEXT("Light mode"))).Font(WinFont(true, 13)).ColorAndOpacity(FLinearColor::White)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(46.f).HeightOverride(22.f)
					[
						SNew(SBorder)
						.BorderImage(&Round)
						.BorderBackgroundColor(bDark ? FLinearColor(0.3f, 0.55f, 1.f, 1.f) : FLinearColor(0.55f, 0.55f, 0.6f, 1.f))
						.Padding(3.f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(bDark ? 1.f : 0.001f)[ SNew(SSpacer) ]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SBox).WidthOverride(16.f).HeightOverride(16.f)
								[
									SNew(SBorder).BorderImage(&Round).BorderBackgroundColor(FLinearColor::White)
								]
							]
							+ SHorizontalBox::Slot().FillWidth(bDark ? 0.001f : 1.f)[ SNew(SSpacer) ]
						]
					]
				]
			]
		];

	TSharedRef<SWidget> Window =
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(Th.Border)
		.Padding(2.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(Th.Window)
			.Padding(3.f)
			[
				SNew(SVerticalBox)
				// Title bar
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(Th.TitleBar)
					.Padding(FMargin(8.f, 4.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("StoreDesk 1.0  -  Tech Store Systems"))).Font(WinFont(true, 16)).ColorAndOpacity(FLinearColor::White)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
						[
							Pill
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton)
							.ButtonStyle(&TSSDeskButtonStyle())
							.ButtonColorAndOpacity(Th.Button)
							.ContentPadding(FMargin(10.f, 1.f))
							.OnClicked_Lambda([this]() { if (OnClose) OnClose(); return FReply::Handled(); })
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Log Off  [Esc]"))).Font(WinFont(true, 13)).ColorAndOpacity(Th.Ink)
							]
						]
					]
				]
				// Menu + page
				+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 3.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
					[
						SNew(SBox).WidthOverride(240.f)[ Menu ]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(Th.PageBg)
						.Padding(12.f)
						[
							PageHost.ToSharedRef()
						]
					]
				]
				// Status bar
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(Th.StatusBar)
					.Padding(FMargin(8.f, 3.f))
					[
						SNew(STextBlock)
						.Font(WinFont(false, 13))
						.ColorAndOpacity(Th.Ink)
						.Text_Lambda([this]()
						{
							if (!Ctx.Store || !Ctx.Biz) return FText::GetEmpty();
							return FText::FromString(FString::Printf(TEXT("Cash %s     Day %d     Business level %d     Active orders %d"),
								*WinMoney(Ctx.Store->GetMoney()), Ctx.Store->GetDay(), Ctx.Biz->GetLevel(), Ctx.Biz->ActiveOrderCount()));
						})
					]
				]
			]
		];

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(Th.Backdrop)
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(1100.f).HeightOverride(660.f)[ Window ]
		];
}

// ---- the CRT's idle screen ------------------------------------------------------------------------

FText UTSSDeskIdleWidget::StatusLine() const
{
	if (!Store.IsValid() || !Biz.IsValid()) return FText::GetEmpty();
	return FText::FromString(FString::Printf(TEXT("CASH %s    DAY %d    LVL %d    ORDERS %d"),
		*WinMoney(Store->GetMoney()), Store->GetDay(), Biz->GetLevel(), Biz->ActiveOrderCount()));
}

FText UTSSDeskIdleWidget::Prompt() const
{
	// Blinking prompt, like a terminal.
	const bool bOn = FMath::Fmod(FPlatformTime::Seconds(), 1.0) < 0.6;
	return FText::FromString(bOn ? TEXT("PRESS [E] TO LOG IN_") : TEXT("PRESS [E] TO LOG IN"));
}

TSharedRef<SWidget> UTSSDeskIdleWidget::RebuildWidget()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(FLinearColor(0.01f, 0.04f, 0.02f, 1.f))
		.Padding(FMargin(22.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text(FText::FromString(TEXT("STOREDESK 1.0"))).Font(WinFont(true, 40)).ColorAndOpacity(WinCrt) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 18.f)[ SNew(STextBlock).Text(FText::FromString(TEXT("TECH STORE SYSTEMS  (C) 1998"))).Font(WinFont(false, 16)).ColorAndOpacity(WinCrt) ]
			+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text_UObject(this, &UTSSDeskIdleWidget::StatusLine).Font(WinFont(false, 20)).ColorAndOpacity(WinCrt) ]
			+ SVerticalBox::Slot().FillHeight(1.f).VAlign(VAlign_Bottom)
			[
				SNew(STextBlock).Text_UObject(this, &UTSSDeskIdleWidget::Prompt).Font(WinFont(true, 24)).ColorAndOpacity(WinCrt)
			]
		];
}
