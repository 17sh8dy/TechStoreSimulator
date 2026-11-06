#include "TSSMenuWidget.h"
#include "TSSPlayerController.h"
#include "Engine/GameInstance.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TSSMenu"

namespace
{
	const FLinearColor MenuDim(0.015f, 0.025f, 0.04f, 0.9f);
	const FLinearColor MenuAccent(0.12f, 0.5f, 0.95f, 1.f);
	const FLinearColor MenuIdle(0.12f, 0.15f, 0.2f, 1.f);
	const FLinearColor MenuMuted(0.62f, 0.68f, 0.75f, 1.f);

	TSharedRef<STextBlock> MenuLabel(const FString& Text, int32 Size, bool bBold = false, FLinearColor Color = FLinearColor::White)
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.Font(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size))
			.ColorAndOpacity(Color)
			.Justification(ETextJustify::Center)
			.AutoWrapText(true);
	}
}

UTSSSettingsSubsystem* UTSSMenuWidget::GetSettings() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UTSSSettingsSubsystem>() : nullptr;
}

TSharedRef<SWidget> UTSSMenuWidget::RebuildWidget()
{
	SAssignNew(Content, SBox);
	Content->SetContent(BuildMain());
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(MenuDim)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			Content.ToSharedRef()
		];
}

void UTSSMenuWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Content.Reset();
}

void UTSSMenuWidget::SetPage(ETSSMenuPage NewPage)
{
	Page = NewPage;
	if (!Content.IsValid()) return;
	switch (Page)
	{
	case ETSSMenuPage::Main: Content->SetContent(BuildMain()); break;
	case ETSSMenuPage::ViewSelect:
		if (UTSSSettingsSubsystem* S = GetSettings()) PendingMode = S->GetViewMode();
		Content->SetContent(BuildViewSelect());
		break;
	case ETSSMenuPage::Settings: Content->SetContent(BuildSettings()); break;
	case ETSSMenuPage::Pause: Content->SetContent(BuildPause()); break;
	}
}

bool UTSSMenuWidget::HandleBack()
{
	if (Page == ETSSMenuPage::Settings) { SetPage(SettingsReturnPage); return true; }
	if (Page == ETSSMenuPage::ViewSelect) { SetPage(ETSSMenuPage::Main); return true; }
	return false;
}

// ---- actions ----

FReply UTSSMenuWidget::OnNewGame() { SetPage(ETSSMenuPage::ViewSelect); return FReply::Handled(); }

FReply UTSSMenuWidget::OnSettings()
{
	SettingsReturnPage = Page;
	SetPage(ETSSMenuPage::Settings);
	return FReply::Handled();
}

FReply UTSSMenuWidget::OnQuit()
{
	if (ATSSPlayerController* PC = Cast<ATSSPlayerController>(GetOwningPlayer())) PC->QuitToDesktop();
	return FReply::Handled();
}

FReply UTSSMenuWidget::OnContinue()
{
	if (UTSSSettingsSubsystem* S = GetSettings())
	{
		S->SetViewMode(PendingMode);
		S->MarkViewChosen();
	}
	if (ATSSPlayerController* PC = Cast<ATSSPlayerController>(GetOwningPlayer())) PC->StartNewGame();
	return FReply::Handled();
}

FReply UTSSMenuWidget::OnResume()
{
	if (ATSSPlayerController* PC = Cast<ATSSPlayerController>(GetOwningPlayer())) PC->ResumeGame();
	return FReply::Handled();
}

FReply UTSSMenuWidget::OnBack() { HandleBack(); return FReply::Handled(); }

// ---- building blocks ----

TSharedRef<SWidget> UTSSMenuWidget::Header(const FString& Subtitle)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ MenuLabel(TEXT("TECH STORE SIMULATOR"), 40, true) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4, 0, 28)[ MenuLabel(TEXT("NOVAGAMES"), 14, false, MenuAccent) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 24)
		[
			MenuLabel(Subtitle, 22, true)
		];
}

TSharedRef<SWidget> UTSSMenuWidget::MenuButton(const FString& Text, FOnClicked OnClicked, float Width)
{
	return SNew(SBox).WidthOverride(Width)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.ContentPadding(FMargin(0.f, 12.f))
			.OnClicked(OnClicked)
			.ButtonColorAndOpacity(MenuAccent)
			[
				MenuLabel(Text, 16, true)
			]
		];
}

TSharedRef<SWidget> UTSSMenuWidget::ViewCard(ETSSViewMode Mode, const FString& Title, const FString& Body, bool bPendingChoice)
{
	return SNew(SBox).WidthOverride(440.f)
		[
			SNew(SButton)
			.HAlign(HAlign_Fill)
			.ContentPadding(FMargin(24.f, 28.f))
			.OnClicked(FOnClicked::CreateLambda([this, Mode, bPendingChoice]()
			{
				if (bPendingChoice)
				{
					PendingMode = Mode;
				}
				else if (UTSSSettingsSubsystem* S = GetSettings())
				{
					S->SetViewMode(Mode);
				}
				return FReply::Handled();
			}))
			.ButtonColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([this, Mode, bPendingChoice]()
			{
				const UTSSSettingsSubsystem* S = GetSettings();
				const ETSSViewMode Current = bPendingChoice ? PendingMode : (S ? S->GetViewMode() : Mode);
				return FSlateColor(Current == Mode ? MenuAccent : MenuIdle);
			}))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ MenuLabel(Title, 20, true) ]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10, 0, 0)[ MenuLabel(Body, 14, false, MenuMuted) ]
			]
		];
}

TSharedRef<SWidget> UTSSMenuWidget::FovRow(const FString& Text, ETSSViewMode Mode)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(210.f)[ MenuLabel(Text, 15, true) ]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(300.f)
			[
				SNew(SSlider)
				.Value(TAttribute<float>::CreateLambda([this, Mode]()
				{
					const UTSSSettingsSubsystem* S = GetSettings();
					return S ? (S->GetFOV(Mode) - TSSMinFOV) / (TSSMaxFOV - TSSMinFOV) : 0.5f;
				}))
				.StepSize(1.f / (TSSMaxFOV - TSSMinFOV))
				.MouseUsesStep(true)
				.OnValueChanged(FOnFloatValueChanged::CreateLambda([this, Mode](float V)
				{
					if (UTSSSettingsSubsystem* S = GetSettings())
					{
						S->SetFOV(Mode, TSSMinFOV + V * (TSSMaxFOV - TSSMinFOV));
					}
				}))
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16, 0, 0, 0)
		[
			SNew(SBox).WidthOverride(60.f)
			[
				SNew(STextBlock)
				.Text(TAttribute<FText>::CreateLambda([this, Mode]()
				{
					const UTSSSettingsSubsystem* S = GetSettings();
					return FText::FromString(FString::Printf(TEXT("%d\u00B0"), S ? FMath::RoundToInt(S->GetFOV(Mode)) : 0));
				}))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
			]
		];
}

TSharedRef<SWidget> UTSSMenuWidget::SprintRow()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(210.f)[ MenuLabel(TEXT("Sprint key"), 15, true) ]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(300.f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ButtonColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([this]()
				{
					const UTSSSettingsSubsystem* S = GetSettings();
					return FSlateColor((S && S->IsAwaitingSprintKey()) ? MenuAccent : MenuIdle);
				}))
				.OnClicked(FOnClicked::CreateLambda([this]()
				{
					if (UTSSSettingsSubsystem* S = GetSettings()) S->BeginAwaitSprintKey();
					return FReply::Handled();
				}))
				[
					SNew(STextBlock)
					.Text(TAttribute<FText>::CreateLambda([this]()
					{
						const UTSSSettingsSubsystem* S = GetSettings();
						if (!S) return FText::GetEmpty();
						return S->IsAwaitingSprintKey() ? FText::FromString(TEXT("Press a key...  (Esc cancels)")) : S->GetSprintKey().GetDisplayName();
					}))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
					.Justification(ETextJustify::Center)
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16, 0, 0, 0)
		[
			SNew(SButton)
			.OnClicked(FOnClicked::CreateLambda([this]()
			{
				if (UTSSSettingsSubsystem* S = GetSettings()) S->ResetSprintKey();
				return FReply::Handled();
			}))
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Reset (Shift)"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
			]
		];
}

// ---- pages ----

TSharedRef<SWidget> UTSSMenuWidget::BuildMain()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Header(TEXT("")) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6)[ MenuButton(TEXT("NEW GAME"), FOnClicked::CreateUObject(this, &UTSSMenuWidget::OnNewGame)) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6)[ MenuButton(TEXT("SETTINGS"), FOnClicked::CreateUObject(this, &UTSSMenuWidget::OnSettings)) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6)[ MenuButton(TEXT("QUIT"), FOnClicked::CreateUObject(this, &UTSSMenuWidget::OnQuit)) ];
}

TSharedRef<SWidget> UTSSMenuWidget::BuildViewSelect()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Header(TEXT("HOW WILL YOU EXPERIENCE\nYOUR JOURNEY?")) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 8)
		[
			ViewCard(ETSSViewMode::FirstPerson, TEXT("FIRST PERSON"), TEXT("Experience your store\nthrough your own eyes."), true)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 8)
		[
			ViewCard(ETSSViewMode::ThirdPerson, TEXT("THIRD PERSON"), TEXT("Watch your business grow\nfrom a new perspective."), true)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 14, 0, 18)
		[
			MenuLabel(TEXT("You can change this anytime\nin Settings."), 14, false, MenuMuted)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ MenuButton(TEXT("CONTINUE"), FOnClicked::CreateUObject(this, &UTSSMenuWidget::OnContinue)) ];
}

TSharedRef<SWidget> UTSSMenuWidget::BuildSettings()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Header(TEXT("SETTINGS")) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, 0, 0, 10)[ MenuLabel(TEXT("VIEW"), 18, true, MenuAccent) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)[ SNew(SBox).WidthOverride(300.f)[ ViewCard(ETSSViewMode::FirstPerson, TEXT("FIRST PERSON"), TEXT(""), false) ] ]
			+ SHorizontalBox::Slot().AutoWidth()[ SNew(SBox).WidthOverride(300.f)[ ViewCard(ETSSViewMode::ThirdPerson, TEXT("THIRD PERSON"), TEXT(""), false) ] ]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 6)[ FovRow(TEXT("First Person FOV"), ETSSViewMode::FirstPerson) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[ FovRow(TEXT("Third Person FOV"), ETSSViewMode::ThirdPerson) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, 24, 0, 10)[ MenuLabel(TEXT("CONTROLS"), 18, true, MenuAccent) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[ SprintRow() ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 28, 0, 0)[ MenuButton(TEXT("BACK"), FOnClicked::CreateUObject(this, &UTSSMenuWidget::OnBack)) ];
}

TSharedRef<SWidget> UTSSMenuWidget::BuildPause()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Header(TEXT("PAUSED")) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6)[ MenuButton(TEXT("RESUME"), FOnClicked::CreateUObject(this, &UTSSMenuWidget::OnResume)) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6)[ MenuButton(TEXT("SETTINGS"), FOnClicked::CreateUObject(this, &UTSSMenuWidget::OnSettings)) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6)[ MenuButton(TEXT("QUIT"), FOnClicked::CreateUObject(this, &UTSSMenuWidget::OnQuit)) ];
}

#undef LOCTEXT_NAMESPACE