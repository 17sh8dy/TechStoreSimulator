#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TSSCheckoutTypes.h"
#include "TSSCheckoutScreens.generated.h"

class SBox;

class ATSSRegister;

/** The register monitor: itemised list, subtotal / tax / total, payment status. Drawn onto the screen mesh. */
UCLASS()
class UTSSMonitorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetRegister(ATSSRegister* R) { Reg = R; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FText RowName(int32 Row) const;
	FText RowAmount(int32 Row) const;
	FText StatusText() const;
	FText SubtotalText() const;
	FText TaxText() const;
	FText TotalText() const;
	FText FooterText() const;

	TWeakObjectPtr<ATSSRegister> Reg;
};

/** The handheld card terminal screen: amount and a colour-coded status. */
UCLASS()
class UTSSTerminalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetRegister(ATSSRegister* R) { Reg = R; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FText AmountText() const;
	FText CardText() const;
	FText StatusText() const;
	FSlateColor StatusColor() const;

	TWeakObjectPtr<ATSSRegister> Reg;
};

/** A printed till receipt: store name and address, date and time, itemised lines, tax and total, how it was paid, a barcode and a thank-you. */
UCLASS()
class UTSSReceiptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fills the paper in. Safe to call again for the next sale. */
	void SetReceipt(const FString& StoreName, const FTSSTransaction& Sale, const TArray<FTSSSaleLine>& Lines);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	TSharedPtr<SBox> Host;
};
