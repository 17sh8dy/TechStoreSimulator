#include "TSSMoney.h"

namespace
{
	using namespace TSSMoney;

	// Notes: 6.14 x 2.61 in (15.6 x 6.63 cm) for every denomination. Coins: diameter x thickness from the US Mint's specifications.
	const FDenom GDenoms[NumDenoms] = {
		{ 100,   TEXT("One dollar bill"),     TEXT("$1"),   false, 15.6f, 6.63f, FLinearColor(0.50f, 0.66f, 0.50f), TEXT("T_Cash_1") },
		{ 500,   TEXT("Five dollar bill"),    TEXT("$5"),   false, 15.6f, 6.63f, FLinearColor(0.58f, 0.52f, 0.66f), TEXT("T_Cash_5") },
		{ 1000,  TEXT("Ten dollar bill"),     TEXT("$10"),  false, 15.6f, 6.63f, FLinearColor(0.78f, 0.62f, 0.40f), TEXT("T_Cash_10") },
		{ 2000,  TEXT("Twenty dollar bill"),  TEXT("$20"),  false, 15.6f, 6.63f, FLinearColor(0.46f, 0.72f, 0.50f), TEXT("T_Cash_20") },
		{ 5000,  TEXT("Fifty dollar bill"),   TEXT("$50"),  false, 15.6f, 6.63f, FLinearColor(0.78f, 0.55f, 0.62f), TEXT("T_Cash_50") },
		{ 10000, TEXT("Hundred dollar bill"), TEXT("$100"), false, 15.6f, 6.63f, FLinearColor(0.55f, 0.68f, 0.80f), TEXT("T_Cash_100") },
		{ 1,     TEXT("Penny"),               TEXT("1c"),   true,  1.905f, 0.152f, FLinearColor(0.72f, 0.38f, 0.20f), TEXT("T_Coin_Penny") },
		{ 5,     TEXT("Nickel"),              TEXT("5c"),   true,  2.121f, 0.195f, FLinearColor(0.68f, 0.70f, 0.72f), TEXT("T_Coin_Nickel") },
		{ 10,    TEXT("Dime"),                TEXT("10c"),  true,  1.791f, 0.135f, FLinearColor(0.78f, 0.80f, 0.82f), TEXT("T_Coin_Dime") },
		{ 25,    TEXT("Quarter"),             TEXT("25c"),  true,  2.426f, 0.175f, FLinearColor(0.78f, 0.80f, 0.82f), TEXT("T_Coin_Quarter") },
		{ 100,   TEXT("Dollar coin"),         TEXT("$1c"),  true,  2.650f, 0.200f, FLinearColor(0.85f, 0.72f, 0.28f), TEXT("T_Coin_Dollar") },
	};

	/** Indices in the order change is counted out: biggest value first, a note before a coin of the same value. */
	const int32 GChangeOrder[NumDenoms] = { 5, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6 };
}

const TSSMoney::FDenom& TSSMoney::Denom(int32 Index)
{
	return GDenoms[FMath::Clamp(Index, 0, NumDenoms - 1)];
}

int32 TSSMoney::IndexOfBill(int32 Cents)
{
	for (int32 i = 0; i < NumBills; ++i) if (GDenoms[i].Cents == Cents) return i;
	return INDEX_NONE;
}

int32 TSSMoney::IndexOfCoin(int32 Cents)
{
	for (int32 i = NumBills; i < NumDenoms; ++i) if (GDenoms[i].Cents == Cents) return i;
	return INDEX_NONE;
}

int32 TSSMoney::IndexOf(int32 Cents)
{
	const int32 B = IndexOfBill(Cents);
	return B != INDEX_NONE ? B : IndexOfCoin(Cents);
}

FString TSSMoney::Format(int32 Cents)
{
	const int32 A = FMath::Abs(Cents);
	FString Dollars = FString::FromInt(A / 100);
	for (int32 i = Dollars.Len() - 3; i > 0; i -= 3) Dollars.InsertAt(i, TEXT(","));
	return FString::Printf(TEXT("%s$%s.%02d"), Cents < 0 ? TEXT("-") : TEXT(""), *Dollars, A % 100);
}

FString TSSMoney::FormatShort(int32 Cents)
{
	if (Cents % 100 == 0)
	{
		const int32 A = FMath::Abs(Cents) / 100;
		FString Dollars = FString::FromInt(A);
		for (int32 i = Dollars.Len() - 3; i > 0; i -= 3) Dollars.InsertAt(i, TEXT(","));
		return FString::Printf(TEXT("%s$%s"), Cents < 0 ? TEXT("-") : TEXT(""), *Dollars);
	}
	return Format(Cents);
}

int32 TSSMoney::FStock::TotalCents() const
{
	int32 T = 0;
	for (int32 i = 0; i < NumDenoms; ++i) T += Count[i] * GDenoms[i].Cents;
	return T;
}

int32 TSSMoney::FStock::PieceCount() const
{
	int32 N = 0;
	for (int32 i = 0; i < NumDenoms; ++i) N += Count[i];
	return N;
}

TSSMoney::FStock TSSMoney::StartingFloat()
{
	FStock S;
	// Notes: ones, fives, tens, twenties, a couple of fifties, one hundred. Coins: a roll or so of each.
	const int32 Start[NumDenoms] = { 20, 12, 10, 12, 3, 1, 60, 40, 50, 50, 15 };
	for (int32 i = 0; i < NumDenoms; ++i) S.Count[i] = Start[i];
	return S;
}

int32 TSSMoney::Sum(const TArray<int32>& Pieces)
{
	int32 T = 0;
	for (int32 P : Pieces) T += GDenoms[FMath::Clamp(P, 0, NumDenoms - 1)].Cents;
	return T;
}

bool TSSMoney::MakeChange(int32 Cents, const FStock& Drawer, TArray<int32>& Out)
{
	Out.Reset();
	if (Cents < 0) return false;
	FStock Left = Drawer;
	int32 Remaining = Cents;
	for (int32 k = 0; k < NumDenoms && Remaining > 0; ++k)
	{
		const int32 Idx = GChangeOrder[k];
		while (Remaining >= GDenoms[Idx].Cents && Left.Count[Idx] > 0)
		{
			Out.Add(Idx);
			--Left.Count[Idx];
			Remaining -= GDenoms[Idx].Cents;
		}
	}
	if (Remaining != 0) { Out.Reset(); return false; }
	return true;
}

TArray<int32> TSSMoney::ChooseTender(int32 TotalCents)
{
	// Pays `Cents` with the fewest pieces, notes for whole dollars and coins for the rest. Unlimited wallet.
	auto Pay = [](int32 Cents) -> TArray<int32>
	{
		TArray<int32> Out;
		FStock Wallet;
		for (int32 i = 0; i < NumDenoms; ++i) Wallet.Count[i] = 1000;
		// Customers do not pay in dollar coins: they use notes, then quarters, dimes, nickels, pennies.
		Wallet.Count[10] = 0;
		MakeChange(Cents, Wallet, Out);
		return Out;
	};

	const float Roll = FMath::FRand();
	int32 Amount = TotalCents;
	if (Roll < 0.18f)
	{
		// Exactly the total: notes and every cent in coins.
		Amount = TotalCents;
	}
	else if (Roll < 0.62f)
	{
		// Notes that cover it: the next five / ten / twenty / fifty / hundred.
		TArray<int32> Steps = { 500, 1000, 2000, 5000, 10000 };
		TArray<int32> Options;
		for (int32 Step : Steps) Options.AddUnique(((TotalCents + Step - 1) / Step) * Step);
		// Not an absurd overpayment: drop anything more than a few multiples above the cheapest option.
		const int32 Cheapest = Options[0];
		Options.RemoveAll([Cheapest](int32 O) { return O > Cheapest * 4 + 2000; });
		Amount = Options[FMath::RandRange(0, Options.Num() - 1)];
	}
	else
	{
		// A whole-dollar amount plus a few coins, chosen so that the change is a tidy number of quarters.
		const int32 Dollars = (TotalCents + 99) / 100 * 100;
		int32 BestExtra = 0, BestPieces = TNumericLimits<int32>::Max();
		for (int32 Extra = 0; Extra < 100; ++Extra)
		{
			const int32 Change = Dollars + Extra - TotalCents;
			FStock Wallet;
			for (int32 i = 0; i < NumDenoms; ++i) Wallet.Count[i] = 1000;
			TArray<int32> C, Coins;
			MakeChange(Change, Wallet, C);
			MakeChange(Extra, Wallet, Coins);
			const int32 Pieces = C.Num() + Coins.Num();
			if (Pieces < BestPieces) { BestPieces = Pieces; BestExtra = Extra; }
		}
		// Whole dollars paid up to the next ten when that is a big jump, so the note count stays natural.
		Amount = Dollars + BestExtra;
		if (Dollars >= 2000) Amount = ((Dollars + 999) / 1000) * 1000 + BestExtra;
	}
	return Pay(Amount);
}
