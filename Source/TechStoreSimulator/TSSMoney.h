#pragma once

#include "CoreMinimal.h"

/**
 * United States currency, in whole cents. Everything that touches money at the checkout works in cents so that
 * sales tax, coins and change are exact. (The shop's shelf prices and wholesale costs are whole dollars; the register
 * multiplies them by 100.)
 *
 * Only US money exists today, but every piece (name, value, look) comes from the one table in TSSMoney.cpp, so another
 * currency is a second table rather than a rewrite.
 */
namespace TSSMoney
{
	/** Sales tax charged at the register. */
	constexpr float TaxRate = 0.08f;
	inline int32 TaxFor(int32 SubtotalCents) { return FMath::RoundToInt(SubtotalCents * TaxRate); }

	/** One kind of note or coin. */
	struct FDenom
	{
		int32 Cents;
		const TCHAR* Name;      // "Twenty dollar bill"
		const TCHAR* Short;     // "$20", "25c"
		bool bCoin;
		/** Real-world size in cm: notes are length x width, coins are diameter x thickness. */
		float SizeA, SizeB;
		FLinearColor Tint;      // body colour of the piece
		const TCHAR* Texture;   // /Game/TSS/Textures/<Texture>
	};

	/** Notes first ($1 .. $100, indices 0-5), then coins (1c, 5c, 10c, 25c, $1, indices 6-10). */
	constexpr int32 NumDenoms = 11;
	constexpr int32 NumBills = 6;
	const FDenom& Denom(int32 Index);
	/** Index into the table for a value, or INDEX_NONE. A $1 bill is 0; a $1 coin is 10. */
	int32 IndexOfBill(int32 Cents);
	int32 IndexOfCoin(int32 Cents);
	/** The table index for a value, preferring the note over the coin. */
	int32 IndexOf(int32 Cents);

	/** "$1,234.56" (negative: "-$1.50"). */
	FString Format(int32 Cents);
	/** "$20" for whole-dollar amounts, otherwise "$20.50": used where cents would only be noise. */
	FString FormatShort(int32 Cents);

	/** Pieces a drawer holds. */
	struct FStock
	{
		int32 Count[NumDenoms] = {};
		int32 TotalCents() const;
		int32 PieceCount() const;
	};
	/** What a drawer starts every day with. */
	FStock StartingFloat();

	/**
	 * What a customer hands over for a total: sometimes the exact amount, mostly a few notes that cover it,
	 * sometimes notes plus a few coins so the change comes out round. Pieces are returned largest first.
	 */
	TArray<int32> ChooseTender(int32 TotalCents);
	/** Value of a list of pieces. */
	int32 Sum(const TArray<int32>& Pieces);
	/**
	 * The fewest pieces that add up to `Cents` using only what the drawer holds (largest first; a note beats a coin of the
	 * same value). Returns false, leaving `Out` empty, if the drawer cannot make that amount.
	 */
	bool MakeChange(int32 Cents, const FStock& Drawer, TArray<int32>& Out);
}
