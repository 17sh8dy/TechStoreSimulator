#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TSSDeskPages.h"
#include "TSSStoreDeskWidget.generated.h"

class SBox;
class UTSSBusinessSubsystem;
class UTSSStoreSubsystem;

/**
 * The StoreDesk application window: a dated grey-and-navy desktop program with a side menu and a page area.
 * It only hosts pages (see TSSDeskPages.h); all data comes from the store and business subsystems.
 */
UCLASS()
class UTSSStoreDeskWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPage(int32 Index);
	int32 GetPage() const { return PageIndex; }
	/** Called by the Log Off button / close box. */
	void SetOnClose(TFunction<void()> Handler) { OnClose = MoveTemp(Handler); }
	FTSSDeskContext& GetContext() { return Ctx; }
	/** Light / dark appearance (the pill in the title bar). Saved with the user settings. */
	void SetDark(bool bDark);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void MarkDirty() { bDirty = true; }
	/** Rebuilds the visible page if something changed. Driven by a Slate timer, so it never depends on UMG ticking. */
	void RefreshPage();
	TSharedRef<SWidget> BuildWindow();

	FTSSDeskContext Ctx;
	int32 PageIndex = 0;
	bool bDirty = true;
	TSharedPtr<SBox> RootHost;
	TSharedPtr<SBox> PageHost;
	TFunction<void()> OnClose;
	FDelegateHandle StoreHandle;
	FDelegateHandle BizHandle;
};

/** What the CRT in the world shows while nobody is logged in. */
UCLASS()
class UTSSDeskIdleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetSystems(UTSSStoreSubsystem* InStore, UTSSBusinessSubsystem* InBiz) { Store = InStore; Biz = InBiz; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FText StatusLine() const;
	FText Prompt() const;

	TWeakObjectPtr<UTSSStoreSubsystem> Store;
	TWeakObjectPtr<UTSSBusinessSubsystem> Biz;
};
