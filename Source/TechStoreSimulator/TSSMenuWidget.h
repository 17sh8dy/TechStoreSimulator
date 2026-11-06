#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TSSSettings.h"
#include "TSSMenuWidget.generated.h"

class SBox;

enum class ETSSMenuPage : uint8
{
	Main,
	ViewSelect,
	Settings,
	Pause
};

/** One widget, four pages (main menu, new-game view select, settings, pause), built in Slate. */
UCLASS()
class UTSSMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPage(ETSSMenuPage NewPage);
	ETSSMenuPage GetPage() const { return Page; }

	/** Esc/Tab handler. Returns true if it navigated back a page. */
	bool HandleBack();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	TSharedRef<SWidget> BuildMain();
	TSharedRef<SWidget> BuildViewSelect();
	TSharedRef<SWidget> BuildSettings();
	TSharedRef<SWidget> BuildPause();

	TSharedRef<SWidget> Header(const FString& Subtitle);
	TSharedRef<SWidget> MenuButton(const FString& Label, FOnClicked OnClicked, float Width = 320.f);
	TSharedRef<SWidget> ViewCard(ETSSViewMode Mode, const FString& Title, const FString& Body, bool bPendingChoice);
	TSharedRef<SWidget> FovRow(const FString& Label, ETSSViewMode Mode);
	TSharedRef<SWidget> SprintRow();

	UTSSSettingsSubsystem* GetSettings() const;

	FReply OnNewGame();
	FReply OnSettings();
	FReply OnQuit();
	FReply OnContinue();
	FReply OnResume();
	FReply OnBack();

	TSharedPtr<SBox> Content;
	ETSSMenuPage Page = ETSSMenuPage::Main;
	ETSSMenuPage SettingsReturnPage = ETSSMenuPage::Main;
	ETSSViewMode PendingMode = ETSSViewMode::FirstPerson;
};