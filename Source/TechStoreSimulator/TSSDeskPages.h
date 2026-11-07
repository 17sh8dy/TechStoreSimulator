#pragma once

#include "CoreMinimal.h"
#include "TSSProducts.h"

class SWidget;
struct FButtonStyle;
struct FProgressBarStyle;
class UTSSStoreSubsystem;
class UTSSBusinessSubsystem;

/** Colours for the StoreDesk interface. Every control reads from here, so light and dark modes stay in step. */
struct FTSSDeskTheme
{
	FLinearColor Ink;       // main text
	FLinearColor Dim;       // secondary text
	FLinearColor Heading;   // titles, accents
	FLinearColor Good;      // positive numbers
	FLinearColor Bad;       // warnings, negative numbers
	FLinearColor Paper;     // panels and cards
	FLinearColor Panel2;    // panels nested inside panels
	FLinearColor PageBg;    // the page area behind the panels
	FLinearColor Window;    // window chrome and the side menu
	FLinearColor Border;    // thin outlines
	FLinearColor Button;    // button tint
	FLinearColor ButtonOff; // disabled button tint
	FLinearColor Select;    // selected row / menu item
	FLinearColor TitleBar;
	FLinearColor Backdrop;  // the screen behind the window
	FLinearColor StatusBar;
};

/** The active theme. The choice is saved with the user's settings. */
const FTSSDeskTheme& TSSDeskTheme();
bool TSSDeskDark();
void TSSDeskSetDark(bool bDark);

/** Everything a StoreDesk page can read or call. Pages hold no game data of their own. */
struct FTSSDeskContext
{
	UWorld* World = nullptr;
	UTSSStoreSubsystem* Store = nullptr;
	UTSSBusinessSubsystem* Biz = nullptr;

	// UI-only state that should survive a page rebuild
	TMap<ETSSProduct, int32> Cart;
	ETSSProduct Selected = ETSSProduct::Phone;
	FString NameDraft;   // what is typed in the rename box (survives page rebuilds)
	FString Message;
	bool bMessageIsError = false;

	/** Ask the window to rebuild the page that is showing (after a click changed something). */
	TFunction<void()> Refresh;
};

/**
 * One section of StoreDesk. To add a section: derive, implement Title/Build, and add it to TSSDeskPages()
 * in TSSDeskPages.cpp. The window, the side menu and the refresh handling need no changes.
 */
class FTSSDeskPage
{
public:
	virtual ~FTSSDeskPage() {}
	virtual FText Title() const = 0;
	virtual TSharedRef<SWidget> Build(FTSSDeskContext& Ctx) const = 0;
	/** Business level needed before this section works (0 = always open). */
	virtual int32 RequiredLevel() const { return 0; }
	/** Small tag shown after the name in the side menu (e.g. LOCKED). */
	virtual FString Badge(int32 PlayerLevel) const { return FString(); }
};

/** The sections, in side-menu order. */
const TArray<TSharedRef<FTSSDeskPage>>& TSSDeskPages();

/** Flat, light styles for StoreDesk controls (the default dark Slate button style would swallow the tint). */
const FButtonStyle& TSSDeskButtonStyle();
const FProgressBarStyle& TSSDeskBarStyle();
