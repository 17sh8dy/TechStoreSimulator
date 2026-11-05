#pragma once

#include "CoreMinimal.h"
#include "TSSProducts.h"

class SWidget;
struct FButtonStyle;
struct FProgressBarStyle;
class UTSSStoreSubsystem;
class UTSSBusinessSubsystem;

/** Everything a StoreDesk page can read or call. Pages hold no game data of their own. */
struct FTSSDeskContext
{
	UWorld* World = nullptr;
	UTSSStoreSubsystem* Store = nullptr;
	UTSSBusinessSubsystem* Biz = nullptr;

	// UI-only state that should survive a page rebuild
	TMap<ETSSProduct, int32> Cart;
	ETSSProduct Selected = ETSSProduct::Phone;
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
};

/** The sections, in side-menu order. */
const TArray<TSharedRef<FTSSDeskPage>>& TSSDeskPages();

/** Flat, light styles for StoreDesk controls (the default dark Slate button style would swallow the tint). */
const FButtonStyle& TSSDeskButtonStyle();
const FProgressBarStyle& TSSDeskBarStyle();
