#include "TSSDeskPages.h"
#include "TSSBusinessSubsystem.h"
#include "TSSShelf.h"
#include "TSSStoreSubsystem.h"
#include "EngineUtils.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Misc/ConfigCacheIni.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FTSSDeskTheme LightTheme = {
		FLinearColor(0.03f, 0.03f, 0.06f, 1.f), FLinearColor(0.34f, 0.34f, 0.40f, 1.f), FLinearColor(0.f, 0.f, 0.5f, 1.f),
		FLinearColor(0.f, 0.42f, 0.08f, 1.f), FLinearColor(0.72f, 0.05f, 0.05f, 1.f),
		FLinearColor(0.98f, 0.98f, 0.96f, 1.f), FLinearColor(0.88f, 0.88f, 0.88f, 1.f), FLinearColor(0.93f, 0.93f, 0.90f, 1.f),
		FLinearColor(0.75f, 0.75f, 0.75f, 1.f), FLinearColor(0.35f, 0.35f, 0.38f, 1.f),
		FLinearColor(0.75f, 0.75f, 0.75f, 1.f), FLinearColor(0.62f, 0.62f, 0.62f, 1.f), FLinearColor(0.70f, 0.80f, 1.f, 1.f),
		FLinearColor(0.f, 0.f, 0.5f, 1.f), FLinearColor(0.f, 0.42f, 0.42f, 0.94f), FLinearColor(0.62f, 0.62f, 0.62f, 1.f) };

	// Linear colour values: these display noticeably brighter than they look, so the dark theme is genuinely deep.
	const FTSSDeskTheme DarkTheme = {
		FLinearColor(0.93f, 0.94f, 0.97f, 1.f), FLinearColor(0.64f, 0.68f, 0.78f, 1.f), FLinearColor(0.42f, 0.62f, 1.f, 1.f),
		FLinearColor(0.36f, 0.95f, 0.52f, 1.f), FLinearColor(1.f, 0.42f, 0.42f, 1.f),
		FLinearColor(0.040f, 0.045f, 0.065f, 1.f), FLinearColor(0.075f, 0.082f, 0.11f, 1.f), FLinearColor(0.018f, 0.022f, 0.032f, 1.f),
		FLinearColor(0.055f, 0.06f, 0.08f, 1.f), FLinearColor(0.20f, 0.22f, 0.30f, 1.f),
		FLinearColor(0.11f, 0.12f, 0.17f, 1.f), FLinearColor(0.055f, 0.058f, 0.075f, 1.f), FLinearColor(0.06f, 0.14f, 0.38f, 1.f),
		FLinearColor(0.015f, 0.03f, 0.18f, 1.f), FLinearColor(0.004f, 0.012f, 0.016f, 0.96f), FLinearColor(0.035f, 0.04f, 0.055f, 1.f) };

	int32 GDarkState = -1; // -1 = not read yet
}

bool TSSDeskDark()
{
	if (GDarkState < 0)
	{
		bool bDark = false;
		GConfig->GetBool(TEXT("TechStoreSimulator"), TEXT("StoreDeskDark"), bDark, GGameUserSettingsIni);
		GDarkState = bDark ? 1 : 0;
	}
	return GDarkState == 1;
}

void TSSDeskSetDark(bool bDark)
{
	GDarkState = bDark ? 1 : 0;
	GConfig->SetBool(TEXT("TechStoreSimulator"), TEXT("StoreDeskDark"), bDark, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

const FTSSDeskTheme& TSSDeskTheme() { return TSSDeskDark() ? DarkTheme : LightTheme; }

const FButtonStyle& TSSDeskButtonStyle()
{
	static const FSlateColorBrush Normal(FLinearColor::White);
	static const FSlateColorBrush Hover(FLinearColor(0.88f, 0.92f, 1.f, 1.f));
	static const FSlateColorBrush Press(FLinearColor(0.7f, 0.74f, 0.9f, 1.f));
	static const FButtonStyle Style = FButtonStyle().SetNormal(Normal).SetHovered(Hover).SetPressed(Press).SetDisabled(Normal)
		.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f, 1.f, 0.f, 0.f));
	return Style;
}

const FProgressBarStyle& TSSDeskBarStyle()
{
	static const FSlateColorBrush LightBack(FLinearColor(0.78f, 0.78f, 0.80f, 1.f));
	static const FSlateColorBrush DarkBack(FLinearColor(0.28f, 0.29f, 0.36f, 1.f));
	static const FSlateColorBrush Fill(FLinearColor::White);
	static const FProgressBarStyle Light = FProgressBarStyle().SetBackgroundImage(LightBack).SetFillImage(Fill).SetMarqueeImage(Fill);
	static const FProgressBarStyle Dark = FProgressBarStyle().SetBackgroundImage(DarkBack).SetFillImage(Fill).SetMarqueeImage(Fill);
	return TSSDeskDark() ? Dark : Light;
}

namespace
{
	// Deliberately dated look: flat grey chrome, white paper, navy and teal.
	inline const FTSSDeskTheme& Th() { return TSSDeskTheme(); }

	FSlateFontInfo DskFont(bool bBold, int32 Size) { return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size); }

	FString DskMoney(int32 V)
	{
		FString S = FString::FromInt(FMath::Abs(V));
		for (int32 i = S.Len() - 3; i > 0; i -= 3) S.InsertAt(i, TEXT(","));
		return (V < 0 ? TEXT("-$") : TEXT("$")) + S;
	}

	FString DskClock(float Seconds)
	{
		const int32 S = FMath::CeilToInt(FMath::Max(0.f, Seconds));
		return FString::Printf(TEXT("%d:%02d"), S / 60, S % 60);
	}

	TSharedRef<SWidget> DskTxt(const FString& S, int32 Size = 14, bool bBold = false, FLinearColor Color = Th().Ink)
	{
		return SNew(STextBlock).Text(FText::FromString(S)).Font(DskFont(bBold, Size)).ColorAndOpacity(Color).AutoWrapText(false);
	}

	TSharedRef<SWidget> DskWrapped(const FString& S, int32 Size = 14, FLinearColor Color = Th().Ink)
	{
		return SNew(STextBlock).Text(FText::FromString(S)).Font(DskFont(false, Size)).ColorAndOpacity(Color).AutoWrapText(true);
	}

	/** A bordered "paper" panel. */
	TSharedRef<SWidget> DskPanel(TSharedRef<SWidget> Content, FLinearColor Face = Th().Paper, float Pad = 10.f)
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(Th().Border)
			.Padding(1.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(Face)
				.Padding(Pad)
				[
					Content
				]
			];
	}

	TSharedRef<SWidget> DskButton(const FString& Label, TFunction<void()> OnClick, bool bEnabled = true, int32 Size = 14)
	{
		return SNew(SButton)
			.ButtonStyle(&TSSDeskButtonStyle())
			.ButtonColorAndOpacity(bEnabled ? Th().Button : Th().ButtonOff)
			.IsEnabled(bEnabled)
			.ContentPadding(FMargin(10.f, 4.f))
			.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(Label)).Font(DskFont(true, Size)).ColorAndOpacity(bEnabled ? Th().Ink : Th().Dim)
			];
	}

	TSharedRef<SWidget> DskHeading(const FString& S)
	{
		return SNew(SBox).Padding(FMargin(0.f, 0.f, 0.f, 6.f))
		[
			SNew(STextBlock).Text(FText::FromString(S)).Font(DskFont(true, 20)).ColorAndOpacity(Th().Heading)
		];
	}

	TSharedRef<SWidget> DskCard(const FString& Title, const FString& Value, FLinearColor ValueColor = Th().Ink)
	{
		return DskPanel(SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ DskTxt(Title, 13, false, Th().Dim) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[ DskTxt(Value, 26, true, ValueColor) ]);
	}

	TSharedRef<SWidget> DskBar(float Fraction, FLinearColor Fill)
	{
		return SNew(SBox).HeightOverride(16.f)
		[
			SNew(SProgressBar).Style(&TSSDeskBarStyle()).Percent(FMath::Clamp(Fraction, 0.f, 1.f)).FillColorAndOpacity(Fill)
		];
	}

	TSharedRef<SWidget> DskRow2(const FString& A, const FString& B, bool bBold = false, FLinearColor C = Th().Ink)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[ DskTxt(A, 15, bBold, C) ]
			+ SHorizontalBox::Slot().AutoWidth()[ DskTxt(B, 15, true, C) ];
	}

	/** Units of a product currently on the shelves. */
	int32 DskUnitsOnShelves(UWorld* World, ETSSProduct P)
	{
		int32 N = 0;
		TArray<FTSSStockRef> Stock;
		ATSSShelf::CollectStock(World, Stock);
		for (const FTSSStockRef& R : Stock)
		{
			if (R.Shelf.IsValid() && R.Shelf->GetSlotProduct(R.Slot) == P) N += R.Shelf->GetSlotQty(R.Slot);
		}
		return N;
	}

	FString DskPayName(ETSSPayMethod P) { return P == ETSSPayMethod::Cash ? TEXT("Cash") : (P == ETSSPayMethod::Card ? TEXT("Card") : TEXT("-")); }
}

// =================================================================================================
// Dashboard
// =================================================================================================

class FDashboardPage : public FTSSDeskPage
{
public:
	virtual FText Title() const override { return FText::FromString(TEXT("Dashboard")); }
	virtual TSharedRef<SWidget> Build(FTSSDeskContext& C) const override
	{
		const FTSSDashboard D = C.Biz->GetDashboard();
		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(4.f));
		int32 Index = 0;
		auto Add = [&](const FString& Title, const FString& Value, FLinearColor Col = Th().Ink)
		{
			Grid->AddSlot(Index % 4, Index / 4)[ DskCard(Title, Value, Col) ];
			++Index;
		};
		Add(TEXT("Money"), DskMoney(D.Money), Th().Good);
		Add(TEXT("Today's revenue"), DskMoney(D.TodayRevenue));
		Add(TEXT("Lifetime revenue"), DskMoney(D.LifetimeRevenue));
		Add(TEXT("Products sold"), FString::FromInt(D.ProductsSold));
		Add(TEXT("Customers served"), FString::FromInt(D.CustomersServed));
		Add(TEXT("Walked out"), FString::FromInt(D.WalkOuts), D.WalkOuts > 0 ? Th().Bad : Th().Ink);
		Add(TEXT("Average sale"), DskMoney(D.AverageSale));
		Add(TEXT("Active orders"), FString::FromInt(D.ActiveOrders));

		return SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ DskHeading(FString::Printf(TEXT("Dashboard  -  Day %d"), D.Day)) ]
				+ SVerticalBox::Slot().AutoHeight()[ Grid ]
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 10.f, 4.f, 4.f)
				[
					DskPanel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(FString::Printf(TEXT("Business level %d"), D.Level), FString::Printf(TEXT("%d / %d XP"), D.XPIntoLevel, D.XPForNextLevel), true) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)[ DskBar(D.XPForNextLevel > 0 ? float(D.XPIntoLevel) / D.XPForNextLevel : 1.f, Th().Heading) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[ DskRow2(TEXT("Cash sales"), DskMoney(D.CashSales)) ]
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Card sales"), DskMoney(D.CardSales)) ])
				]
			];
	}
};

// =================================================================================================
// My Profile
// =================================================================================================

class FProfilePage : public FTSSDeskPage
{
public:
	virtual FText Title() const override { return FText::FromString(TEXT("My Profile")); }
	virtual TSharedRef<SWidget> Build(FTSSDeskContext& C) const override
	{
		const FTSSDashboard D = C.Biz->GetDashboard();
		TSharedRef<SVerticalBox> Ach = SNew(SVerticalBox);
		for (const FTSSAchievementDef& A : UTSSBusinessSubsystem::AchievementCatalog())
		{
			const bool bHave = C.Biz->HasAchievement(A.Id);
			Ach->AddSlot().AutoHeight().Padding(0.f, 3.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 10.f, 0.f)[ DskTxt(bHave ? TEXT("[X]") : TEXT("[ ]"), 15, true, bHave ? Th().Good : Th().Dim) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 12.f, 0.f)[ DskTxt(A.Name, 15, true, bHave ? Th().Ink : Th().Dim) ]
				+ SHorizontalBox::Slot().FillWidth(1.f)[ DskTxt(A.Description, 14, false, Th().Dim) ]
			];
		}
		const bool bPrestige = C.Biz->IsPrestigeUnlocked();
		return SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ DskHeading(TEXT("My Profile")) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DskPanel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ DskTxt(TEXT("Your Tech Store"), 24, true, Th().Heading) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 4.f)[ DskRow2(FString::Printf(TEXT("Business level %d"), D.Level), FString::Printf(TEXT("%d XP total"), D.XP), true) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[ DskBar(D.XPForNextLevel > 0 ? float(D.XPIntoLevel) / D.XPForNextLevel : 1.f, Th().Heading) ]
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Lifetime earnings"), DskMoney(D.LifetimeRevenue)) ]
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Lifetime products sold"), FString::FromInt(D.ProductsSold)) ]
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Customers served"), FString::FromInt(D.CustomersServed)) ]
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Days in business"), FString::FromInt(D.Day)) ])
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DskPanel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ DskTxt(TEXT("Prestige"), 17, true) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
						[
							DskWrapped(bPrestige ? TEXT("Prestige is available. (Resetting for a permanent bonus is not implemented yet.)")
							                  : FString::Printf(TEXT("LOCKED  -  reach business level %d to unlock."), UTSSBusinessSubsystem::PrestigeLevel), 14, bPrestige ? Th().Good : Th().Dim)
						])
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					DskPanel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[ DskTxt(FString::Printf(TEXT("Achievements  (%d / %d)"), C.Biz->AchievementCatalog().Num() - [&]() { int32 Missing = 0; for (const FTSSAchievementDef& A : UTSSBusinessSubsystem::AchievementCatalog()) if (!C.Biz->HasAchievement(A.Id)) ++Missing; return Missing; }(), C.Biz->AchievementCatalog().Num()), 17, true) ]
						+ SVerticalBox::Slot().AutoHeight()[ Ach ])
				]
			];
	}
};

// =================================================================================================
// Product Ordering
// =================================================================================================

class FOrderingPage : public FTSSDeskPage
{
public:
	virtual FText Title() const override { return FText::FromString(TEXT("Product Ordering")); }
	virtual TSharedRef<SWidget> Build(FTSSDeskContext& C) const override
	{
		FTSSDeskContext* Ctx = &C;
		TSharedRef<SVerticalBox> Catalog = SNew(SVerticalBox);
		for (int32 i = 1; i < static_cast<int32>(ETSSProduct::Count); ++i)
		{
			const ETSSProduct P = static_cast<ETSSProduct>(i);
			const FTSSProductDef& Def = TSSProductDef(P);
			const int32 Qty = C.Cart.FindRef(P);
			const bool bSel = C.Selected == P;
			Catalog->AddSlot().AutoHeight().Padding(0.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(&TSSDeskButtonStyle())
					.ButtonColorAndOpacity(bSel ? Th().Select : Th().Button)
					.ContentPadding(FMargin(8.f, 6.f))
					.OnClicked_Lambda([Ctx, P]() { Ctx->Selected = P; Ctx->Refresh(); return FReply::Handled(); })
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SBox).MinDesiredWidth(118.f)[ DskTxt(Def.Name, 16, true) ] ]
						+ SHorizontalBox::Slot().AutoWidth().Padding(10.f, 0.f)[ DskTxt(DskMoney(Def.WholesaleCost), 15, true) ]
						+ SHorizontalBox::Slot().AutoWidth()[ DskTxt(FString::Printf(TEXT("stock %d"), DskUnitsOnShelves(C.World, P)), 14, false, Th().Dim) ]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()[ DskButton(TEXT("-"), [Ctx, P]() { int32& Q = Ctx->Cart.FindOrAdd(P); Q = FMath::Max(0, Q - 1); Ctx->Refresh(); }, Qty > 0) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f)
				[
					SNew(SBox).MinDesiredWidth(26.f)[ DskTxt(FString::FromInt(Qty), 17, true, Qty > 0 ? Th().Heading : Th().Dim) ]
				]
				+ SHorizontalBox::Slot().AutoWidth()[ DskButton(TEXT("+"), [Ctx, P]() { int32& Q = Ctx->Cart.FindOrAdd(P); Q = FMath::Min(50, Q + 1); Ctx->Refresh(); }) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)[ DskButton(TEXT("+5"), [Ctx, P]() { int32& Q = Ctx->Cart.FindOrAdd(P); Q = FMath::Min(50, Q + 5); Ctx->Refresh(); }) ]
			];
		}

		// Details of the selected product.
		const FTSSProductDef& Sel = TSSProductDef(C.Selected);
		const int32 ShelfPrice = C.Store->GetPrice(C.Selected);
		TSharedRef<SWidget> Details = DskPanel(SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ DskTxt(Sel.Name, 22, true, Th().Heading) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 8.f)[ DskWrapped(Sel.Blurb, 14, Th().Dim) ]
			+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Wholesale price"), DskMoney(Sel.WholesaleCost)) ]
			+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Your shelf price"), DskMoney(ShelfPrice)) ]
			+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Profit per unit (before tax)"), DskMoney(ShelfPrice - Sel.WholesaleCost), true, ShelfPrice >= Sel.WholesaleCost ? Th().Good : Th().Bad) ]
			+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("On your shelves now"), FString::FromInt(DskUnitsOnShelves(C.World, C.Selected))) ]
			+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Box size (cm)"), FString::Printf(TEXT("%d x %d x %d"), FMath::RoundToInt(Sel.BoxSize.X), FMath::RoundToInt(Sel.BoxSize.Y), FMath::RoundToInt(Sel.BoxSize.Z))) ]
			+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Fits per shelf slot"), FString::FromInt(Sel.SlotCapacity())) ]);

		// Cart summary.
		TArray<FTSSOrderLine> Lines;
		for (const TPair<ETSSProduct, int32>& KV : C.Cart) if (KV.Value > 0) Lines.Add({ KV.Key, KV.Value });
		const int32 Cost = C.Biz->OrderCost(Lines);
		int32 Units = 0;
		for (const FTSSOrderLine& L : Lines) Units += L.Qty;
		const bool bCanAfford = Cost <= C.Store->GetMoney();
		const FString EtaText = Units > 0 ? FString::Printf(TEXT("  -  delivery in about %s"), *DskClock(C.Biz->EtaFor(Units))) : FString();

		TSharedRef<SWidget> CartBar = DskPanel(SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				DskTxt(Units == 0 ? FString(TEXT("Cart is empty")) : FString::Printf(TEXT("Cart: %d item%s  -  %s%s"), Units, Units == 1 ? TEXT("") : TEXT("s"), *DskMoney(Cost), *EtaText), 16, true, (Units > 0 && !bCanAfford) ? Th().Bad : Th().Ink)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f)[ DskButton(TEXT("Clear"), [Ctx]() { Ctx->Cart.Reset(); Ctx->Message.Reset(); Ctx->Refresh(); }, Units > 0) ]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				DskButton(TEXT("PLACE ORDER"), [Ctx]()
				{
					TArray<FTSSOrderLine> L;
					for (const TPair<ETSSProduct, int32>& KV : Ctx->Cart) if (KV.Value > 0) L.Add({ KV.Key, KV.Value });
					FString Why;
					Ctx->bMessageIsError = !Ctx->Biz->PlaceOrder(L, Why);
					Ctx->Message = Why;
					if (!Ctx->bMessageIsError) Ctx->Cart.Reset();
					Ctx->Refresh();
				}, Units > 0 && bCanAfford, 16)
			], Th().Panel2, 8.f);

		// Orders.
		TSharedRef<SVerticalBox> OrderList = SNew(SVerticalBox);
		const TArray<FTSSOrder>& Orders = C.Biz->GetOrders();
		int32 Shown = 0;
		for (int32 i = Orders.Num() - 1; i >= 0 && Shown < 6; --i, ++Shown)
		{
			const FTSSOrder& O = Orders[i];
			FString Summary;
			for (const FTSSOrderLine& L : O.Lines) Summary += FString::Printf(TEXT("%s%dx %s"), Summary.IsEmpty() ? TEXT("") : TEXT(", "), L.Qty, TSSProductDef(L.Product).Name);
			UTSSBusinessSubsystem* Biz = C.Biz;
			const int32 Id = O.Id;
			OrderList->AddSlot().AutoHeight().Padding(0.f, 3.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 10.f, 0.f)[ DskTxt(FString::Printf(TEXT("#%d"), O.Id), 14, true) ]
				+ SHorizontalBox::Slot().FillWidth(1.f)[ DskTxt(Summary, 14) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(10.f, 0.f)[ DskTxt(DskMoney(O.Cost), 14) ]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).MinDesiredWidth(210.f)
					[
						SNew(STextBlock)
						.Font(DskFont(true, 14))
						.Text_Lambda([Biz, Id]()
						{
							const FTSSOrder* Now = Biz->FindOrder(Id);
							if (!Now) return FText::GetEmpty();
							switch (Now->Status)
							{
							case ETSSOrderStatus::InTransit: return FText::FromString(FString::Printf(TEXT("In transit  -  %s"), *DskClock(Biz->SecondsUntilArrival(*Now))));
							case ETSSOrderStatus::Arrived: return FText::FromString(FString::Printf(TEXT("At the dock  -  %d to unpack"), Now->UnitsLeft));
							default: return FText::FromString(TEXT("Delivered"));
							}
						})
						.ColorAndOpacity(O.Status == ETSSOrderStatus::Arrived ? Th().Good : (O.Status == ETSSOrderStatus::Received ? Th().Dim : Th().Heading))
					]
				]
			];
		}
		if (Shown == 0) OrderList->AddSlot().AutoHeight()[ DskTxt(TEXT("No orders yet."), 14, false, Th().Dim) ];

		return SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ DskHeading(TEXT("Product Ordering  -  wholesale catalogue")) ]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.15f).Padding(0.f, 0.f, 8.f, 0.f)[ Catalog ]
					+ SHorizontalBox::Slot().FillWidth(0.85f)[ Details ]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[ CartBar ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[ DskTxt(C.Message, 14, true, C.bMessageIsError ? Th().Bad : Th().Good) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
				[
					DskPanel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[ DskTxt(TEXT("Orders"), 17, true) ]
						+ SVerticalBox::Slot().AutoHeight()[ OrderList ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[ DskWrapped(TEXT("Delivered orders arrive by truck at the yellow dock mat just inside the door. Unpack boxes with E and put the products on your shelves."), 13, Th().Dim) ])
				]
			];
	}
};

// =================================================================================================
// Store Management (shop status + the upgrade catalogue)
// =================================================================================================

class FManagementPage : public FTSSDeskPage
{
public:
	virtual FText Title() const override { return FText::FromString(TEXT("Store Management")); }
	virtual TSharedRef<SWidget> Build(FTSSDeskContext& C) const override
	{
		FTSSDeskContext* Ctx = &C;
		int32 Shelves = 0, Slots = 0, UsedSlots = 0, Units = 0;
		for (TActorIterator<ATSSShelf> It(C.World); It; ++It)
		{
			++Shelves;
			for (int32 s = 0; s < ATSSShelf::Tiers * ATSSShelf::Columns; ++s)
			{
				++Slots;
				if (It->GetSlotQty(s) > 0) { ++UsedSlots; Units += It->GetSlotQty(s); }
			}
		}

		TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
		for (const FTSSUpgradeDef& Def : UTSSBusinessSubsystem::UpgradeCatalog())
		{
			const bool bOwned = C.Biz->HasUpgrade(Def.Id);
			FString Why;
			const bool bCan = C.Biz->CanBuyUpgrade(Def, Why);
			const FName Id(Def.Id);
			FString Status = bOwned ? TEXT("OWNED") : (!Def.bImplemented ? FString::Printf(TEXT("LOCKED  -  level %d"), Def.MinLevel) : FString::Printf(TEXT("%s  -  level %d"), *DskMoney(Def.Cost), Def.MinLevel));
			List->AddSlot().AutoHeight().Padding(0.f, 3.f)
			[
				DskPanel(SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ DskTxt(Def.Name, 16, true, Def.bImplemented ? Th().Ink : Th().Dim) ]
						+ SVerticalBox::Slot().AutoHeight()[ DskTxt(Def.Description, 13, false, Th().Dim) ]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f)[ DskTxt(Status, 14, true, bOwned ? Th().Good : (Def.bImplemented ? Th().Heading : Th().Dim)) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						DskButton(bOwned ? TEXT("Owned") : (!Def.bImplemented ? TEXT("LOCKED") : TEXT("BUY")), [Ctx, Id]()
						{
							FString Msg;
							Ctx->bMessageIsError = !Ctx->Biz->BuyUpgrade(Id, Msg);
							Ctx->Message = Msg;
							Ctx->Refresh();
						}, bCan)
					], Th().Panel2, 6.f)
			];
		}

		return SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ DskHeading(TEXT("Store Management")) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DskPanel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Shelf units"), FString::FromInt(Shelves)) ]
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Shelf slots in use"), FString::Printf(TEXT("%d / %d"), UsedSlots, Slots)) ]
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Products on shelves"), FString::FromInt(Units)) ])
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)[ DskTxt(TEXT("Upgrades"), 18, true) ]
				+ SVerticalBox::Slot().AutoHeight()[ List ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)[ DskTxt(C.Message, 14, true, C.bMessageIsError ? Th().Bad : Th().Good) ]
			];
	}
};

// =================================================================================================
// Finances
// =================================================================================================

class FFinancesPage : public FTSSDeskPage
{
public:
	virtual FText Title() const override { return FText::FromString(TEXT("Finances")); }
	virtual TSharedRef<SWidget> Build(FTSSDeskContext& C) const override
	{
		UTSSStoreSubsystem* S = C.Store;
		const int32 Net = S->GetLifetimeRevenue() - S->GetOrderSpend() - S->GetUpgradeSpend();

		TSharedRef<SVerticalBox> Recent = SNew(SVerticalBox);
		const TArray<FTSSTransaction>& T = S->GetTransactions();
		int32 Shown = 0;
		for (int32 i = T.Num() - 1; i >= 0 && Shown < 8; --i, ++Shown)
		{
			const FTSSTransaction& Tx = T[i];
			Recent->AddSlot().AutoHeight().Padding(0.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[ SNew(SBox).MinDesiredWidth(60.f)[ DskTxt(FString::Printf(TEXT("#%d"), Tx.Id), 14, true) ] ]
				+ SHorizontalBox::Slot().AutoWidth()[ SNew(SBox).MinDesiredWidth(80.f)[ DskTxt(FString::Printf(TEXT("Day %d"), Tx.Day), 14, false, Th().Dim) ] ]
				+ SHorizontalBox::Slot().AutoWidth()[ SNew(SBox).MinDesiredWidth(110.f)[ DskTxt(FString::Printf(TEXT("%d item%s"), Tx.Items, Tx.Items == 1 ? TEXT("") : TEXT("s")), 14) ] ]
				+ SHorizontalBox::Slot().AutoWidth()[ SNew(SBox).MinDesiredWidth(80.f)[ DskTxt(DskPayName(Tx.Method), 14) ] ]
				+ SHorizontalBox::Slot().FillWidth(1.f)[ DskTxt(FString::Printf(TEXT("sub %s  tax %s"), *DskMoney(Tx.Subtotal), *DskMoney(Tx.Tax)), 14, false, Th().Dim) ]
				+ SHorizontalBox::Slot().AutoWidth()[ DskTxt(DskMoney(Tx.Total), 14, true, Th().Good) ]
			];
		}
		if (Shown == 0) Recent->AddSlot().AutoHeight()[ DskTxt(TEXT("No sales yet."), 14, false, Th().Dim) ];

		return SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ DskHeading(TEXT("Finances")) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DskPanel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Cash on hand"), DskMoney(S->GetMoney()), true, Th().Good) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ DskRow2(TEXT("Revenue (pre-tax)"), DskMoney(S->GetLifetimeRevenue())) ]
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("  of which cash"), DskMoney(S->GetCashSales())) ]
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("  of which card"), DskMoney(S->GetCardSales())) ]
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Sales tax collected"), DskMoney(S->GetTaxCollected())) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ DskRow2(TEXT("Spent on stock orders"), DskMoney(S->GetOrderSpend())) ]
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(TEXT("Spent on upgrades"), DskMoney(S->GetUpgradeSpend())) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ DskRow2(TEXT("Net (revenue - orders - upgrades)"), DskMoney(Net), true, Net >= 0 ? Th().Good : Th().Bad) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[ DskWrapped(TEXT("Stock bought directly from the wholesale crates in the shop is not counted under orders yet."), 12, Th().Dim) ])
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					DskPanel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[ DskTxt(TEXT("Recent sales"), 17, true) ]
						+ SVerticalBox::Slot().AutoHeight()[ Recent ])
				]
			];
	}
};

// =================================================================================================
// Employees / Contracts / Research & Progression
// =================================================================================================

/** A section that is not in the game yet. It says which business level it is reserved for. */
class FLockedPage : public FTSSDeskPage
{
public:
	FLockedPage(const TCHAR* InTitle, int32 InLevel, const TCHAR* InBlurb) : TitleText(InTitle), Level(InLevel), Blurb(InBlurb) {}
	virtual FText Title() const override { return FText::FromString(TitleText); }
	virtual int32 RequiredLevel() const override { return Level; }
	virtual FString Badge(int32 PlayerLevel) const override { return PlayerLevel < Level ? TEXT("LOCKED") : TEXT("NOT YET"); }
	virtual TSharedRef<SWidget> Build(FTSSDeskContext& C) const override
	{
		const FTSSDashboard D = C.Biz->GetDashboard();
		const bool bReached = D.Level >= Level;
		const int32 NeedXP = UTSSBusinessSubsystem::XPForLevel(Level);
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ DskHeading(TitleText) ]
			+ SVerticalBox::Slot().AutoHeight()[ DskPanel(SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ DskTxt(TEXT("LOCKED"), 30, true, Th().Bad) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
				[
					DskWrapped(bReached ? FString::Printf(TEXT("You have reached business level %d, but this section is not in the game yet."), Level)
					                    : FString::Printf(TEXT("Reach business level %d to unlock this section."), Level), 17, Th().Ink)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 4.f)[ DskRow2(FString::Printf(TEXT("Your level: %d"), D.Level), FString::Printf(TEXT("Needs level %d  (%d XP)"), Level, NeedXP), true) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[ DskBar(NeedXP > 0 ? float(D.XP) / NeedXP : 1.f, Th().Heading) ]
				+ SVerticalBox::Slot().AutoHeight()[ DskWrapped(Blurb, 14, Th().Dim) ]) ];
	}

private:
	FString TitleText;
	int32 Level;
	FString Blurb;
};

class FResearchPage : public FTSSDeskPage
{
public:
	virtual FText Title() const override { return FText::FromString(TEXT("Research & Progression")); }
	virtual TSharedRef<SWidget> Build(FTSSDeskContext& C) const override
	{
		const FTSSDashboard D = C.Biz->GetDashboard();
		TSharedRef<SVerticalBox> Unlocks = SNew(SVerticalBox);
		for (const FTSSUpgradeDef& Def : UTSSBusinessSubsystem::UpgradeCatalog())
		{
			const bool bOpen = D.Level >= Def.MinLevel;
			Unlocks->AddSlot().AutoHeight().Padding(0.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[ SNew(SBox).MinDesiredWidth(90.f)[ DskTxt(FString::Printf(TEXT("Level %d"), Def.MinLevel), 14, true, bOpen ? Th().Good : Th().Dim) ] ]
				+ SHorizontalBox::Slot().FillWidth(1.f)[ DskTxt(Def.Name, 14, false, bOpen ? Th().Ink : Th().Dim) ]
			];
		}
		return SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ DskHeading(TEXT("Research & Progression")) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DskPanel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ DskRow2(FString::Printf(TEXT("Business level %d"), D.Level), FString::Printf(TEXT("%d / %d XP"), D.XPIntoLevel, D.XPForNextLevel), true) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)[ DskBar(D.XPForNextLevel > 0 ? float(D.XPIntoLevel) / D.XPForNextLevel : 1.f, Th().Heading) ]
						+ SVerticalBox::Slot().AutoHeight()[ DskWrapped(TEXT("You earn XP from every sale, every delivery you unpack, and every upgrade you buy."), 13, Th().Dim) ])
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DskPanel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[ DskTxt(TEXT("What each level unlocks"), 17, true) ]
						+ SVerticalBox::Slot().AutoHeight()[ Unlocks ])
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					DskPanel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ DskTxt(TEXT("Research"), 17, true) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[ DskTxt(TEXT("LOCKED  -  research projects unlock at business level 6 (not in the game yet)."), 14, true, Th().Bad) ])
				]
			];
	}
};

// =================================================================================================

const TArray<TSharedRef<FTSSDeskPage>>& TSSDeskPages()
{
	static const TArray<TSharedRef<FTSSDeskPage>> Pages = {
		MakeShared<FDashboardPage>(),
		MakeShared<FProfilePage>(),
		MakeShared<FOrderingPage>(),
		MakeShared<FManagementPage>(),
		MakeShared<FLockedPage>(TEXT("Employees"), 4, TEXT("Hire and manage staff: cashiers, stockers and managers.")),
		MakeShared<FLockedPage>(TEXT("Contracts"), 5, TEXT("Sign bulk supplier deals and take on corporate customers.")),
		MakeShared<FFinancesPage>(),
		MakeShared<FResearchPage>(),
	};
	return Pages;
}
