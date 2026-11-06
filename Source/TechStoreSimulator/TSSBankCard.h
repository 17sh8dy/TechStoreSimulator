#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "TSSBankCard.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * A customer's debit card. All of it is invented: the banks and the card network do not exist, and the numbers start with
 * a 9 (a digit no real card network issues from), pass the same Luhn check that real numbers do, and are drawn from a
 * random generator, so a number can never belong to a real account.
 */
struct FTSSBankCard
{
	FString Number;      // 16 digits, no spaces
	FString Holder;      // "AVA MARTINEZ"
	FString Bank;        // "HARBOR TRUST"
	FString Network;     // "VELOCITY"
	FString Cvv;         // three digits (the back of the card, not drawn)
	FString Pin;         // four digits, what the customer types
	int32 ExpMonth = 1;  // 1-12
	int32 ExpYear = 2030;
	int32 Design = 0;    // which colour scheme

	bool IsValid() const { return Number.Len() == 16; }
	/** "9412 3456 7890 1234" */
	FString Spaced() const;
	FString Last4() const { return Number.Right(4); }
	/** "**** **** **** 1234" */
	FString Masked() const;
	/** "08/28" */
	FString Expiry() const { return FString::Printf(TEXT("%02d/%02d"), ExpMonth, ExpYear % 100); }
	/** A card is good through the last day of its expiry month. */
	bool IsExpiredOn(const FDateTime& Date) const;
};

namespace TSSCards
{
	/** The Luhn check digit rule used by real card numbers. */
	bool LuhnValid(const FString& Digits);
	/** A new random card. bExpired makes it one that ran out last month or earlier; otherwise it is good for one to four years. */
	FTSSBankCard Generate(const FDateTime& Today, bool bExpired);
	/** Four-digit PIN as dots, e.g. 2 typed of 4 = "* * _ _". */
	FString PinDots(int32 Typed);
}

/**
 * The card as an object in the world: a rounded plastic card lying flat, face up, with the bank, network, chip, number, expiry
 * and name printed on it. Its long side runs along local Y and the writing reads from the +X side (the shop owner's), so a card
 * laid in the register's frame can be read from behind the counter. Drawn at 1.6 times real size so it can be read.
 */
UCLASS()
class UTSSCardVisual : public USceneComponent
{
	GENERATED_BODY()

public:
	UTSSCardVisual();
	void SetCard(const FTSSBankCard& Card);
	void SetShown(bool bShown);

private:
	UTextRenderComponent* MakeText(const TCHAR* Name, FVector Loc, float Size, FColor Color, EHorizTextAligment Align);

	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Chip;
	UPROPERTY() TObjectPtr<UTextRenderComponent> BankText;
	UPROPERTY() TObjectPtr<UTextRenderComponent> NetworkText;
	UPROPERTY() TObjectPtr<UTextRenderComponent> NumberText;
	UPROPERTY() TObjectPtr<UTextRenderComponent> ExpiryText;
	UPROPERTY() TObjectPtr<UTextRenderComponent> HolderText;
	UPROPERTY() TObjectPtr<UTextRenderComponent> KindText;
};
