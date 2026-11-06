#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TSSHudWidget.generated.h"

/** In-game overlay: cash, what is in your hands, the crosshair and the interaction prompt. */
UCLASS()
class UTSSHudWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FText MoneyText() const;
	FText HeldText() const;
	FText SalesText() const;
	FText ClockText() const;
	FText DayText() const;
	FText PromptText() const;
	EVisibility PromptVisibility() const;
	class ATSSCharacter* GetPlayer() const;
};
