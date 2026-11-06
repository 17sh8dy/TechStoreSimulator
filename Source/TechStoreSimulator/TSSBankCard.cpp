#include "TSSBankCard.h"
#include "TSSProducts.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

namespace
{
	constexpr float CardScale = 1.6f;
	constexpr float CardLen = 8.56f * CardScale;    // along local Y
	constexpr float CardWid = 5.40f * CardScale;    // along local X
	constexpr float CardThick = 0.08f;

	const TCHAR* const GBanks[] = { TEXT("HARBOR TRUST"), TEXT("SUMMIT FEDERAL"), TEXT("FIRST MERIDIAN"), TEXT("CEDAR VALLEY CU"), TEXT("PINECREST BANK"), TEXT("LAKESHORE SAVINGS") };
	// Invented card networks; the number after the leading 9 identifies them, like a real issuer number does.
	const TCHAR* const GNetworks[] = { TEXT("VELOCITY"), TEXT("ORBITAL"), TEXT("MERIDIAN PAY") };
	const TCHAR* const GFirst[] = { TEXT("AVA"), TEXT("NOAH"), TEXT("MIA"), TEXT("LIAM"), TEXT("ZOE"), TEXT("ELI"), TEXT("NINA"), TEXT("OMAR"), TEXT("SOFIA"), TEXT("JAMAL"), TEXT("CHLOE"), TEXT("DIEGO"), TEXT("MAYA"), TEXT("RUBEN"), TEXT("IRIS"), TEXT("KAI") };
	const TCHAR* const GLast[] = { TEXT("MARTINEZ"), TEXT("OKAFOR"), TEXT("NGUYEN"), TEXT("BROOKS"), TEXT("PATEL"), TEXT("HANSEN"), TEXT("ROSSI"), TEXT("KOWALSKI"), TEXT("WILLIAMS"), TEXT("TANAKA"), TEXT("DUBOIS"), TEXT("RAMIREZ"), TEXT("ADEYEMI"), TEXT("LINDQVIST"), TEXT("COHEN"), TEXT("SANTOS") };

	struct FDesign { FLinearColor Base; FColor Ink; };
	const FDesign GDesigns[] = {
		{ FLinearColor(0.012f, 0.03f, 0.12f), FColor(235, 240, 255) },   // navy
		{ FLinearColor(0.005f, 0.09f, 0.10f), FColor(225, 255, 250) },   // teal
		{ FLinearColor(0.12f, 0.012f, 0.03f), FColor(255, 235, 238) },   // burgundy
		{ FLinearColor(0.02f, 0.02f, 0.025f), FColor(240, 240, 245) },   // graphite
		{ FLinearColor(0.012f, 0.07f, 0.03f), FColor(230, 255, 235) },   // forest
		{ FLinearColor(0.07f, 0.02f, 0.12f), FColor(245, 235, 255) },    // plum
	};
}

FString FTSSBankCard::Spaced() const
{
	FString S;
	for (int32 i = 0; i < Number.Len(); ++i)
	{
		if (i > 0 && i % 4 == 0) S += TEXT(" ");
		S.AppendChar(Number[i]);
	}
	return S;
}

FString FTSSBankCard::Masked() const
{
	return FString::Printf(TEXT("**** **** **** %s"), *Last4());
}

bool FTSSBankCard::IsExpiredOn(const FDateTime& Date) const
{
	// Good through the end of the printed month.
	return Date.GetYear() > ExpYear || (Date.GetYear() == ExpYear && Date.GetMonth() > ExpMonth);
}

bool TSSCards::LuhnValid(const FString& Digits)
{
	int32 Sum = 0;
	bool bDouble = false;
	for (int32 i = Digits.Len() - 1; i >= 0; --i)
	{
		if (!FChar::IsDigit(Digits[i])) return false;
		int32 D = Digits[i] - TEXT('0');
		if (bDouble) { D *= 2; if (D > 9) D -= 9; }
		Sum += D;
		bDouble = !bDouble;
	}
	return Digits.Len() > 0 && Sum % 10 == 0;
}

FTSSBankCard TSSCards::Generate(const FDateTime& Today, bool bExpired)
{
	FTSSBankCard C;
	const int32 NetIndex = FMath::RandRange(0, UE_ARRAY_COUNT(GNetworks) - 1);
	C.Network = GNetworks[NetIndex];
	C.Bank = GBanks[FMath::RandRange(0, UE_ARRAY_COUNT(GBanks) - 1)];
	C.Holder = FString::Printf(TEXT("%s %s"), GFirst[FMath::RandRange(0, UE_ARRAY_COUNT(GFirst) - 1)], GLast[FMath::RandRange(0, UE_ARRAY_COUNT(GLast) - 1)]);
	C.Design = FMath::RandRange(0, UE_ARRAY_COUNT(GDesigns) - 1);

	// 16 digits: 9, the network, five issuer digits, eight account digits, and the Luhn check digit.
	FString Body = FString::Printf(TEXT("9%d"), NetIndex + 1);
	while (Body.Len() < 15) Body += FString::FromInt(FMath::RandRange(0, 9));
	int32 Sum = 0;
	for (int32 i = Body.Len() - 1, k = 0; i >= 0; --i, ++k)
	{
		int32 D = Body[i] - TEXT('0');
		if (k % 2 == 0) { D *= 2; if (D > 9) D -= 9; }
		Sum += D;
	}
	C.Number = Body + FString::FromInt((10 - Sum % 10) % 10);

	C.Cvv = FString::Printf(TEXT("%03d"), FMath::RandRange(0, 999));
	C.Pin = FString::Printf(TEXT("%04d"), FMath::RandRange(0, 9999));

	// Expiry month and year.
	int32 MonthsAhead = bExpired ? -FMath::RandRange(1, 14) : FMath::RandRange(2, 47);
	int32 Index = Today.GetYear() * 12 + (Today.GetMonth() - 1) + MonthsAhead;
	C.ExpYear = Index / 12;
	C.ExpMonth = Index % 12 + 1;
	return C;
}

FString TSSCards::PinDots(int32 Typed)
{
	FString S;
	for (int32 i = 0; i < 4; ++i) S += (i < Typed ? TEXT("*") : TEXT("_")) + FString(i < 3 ? TEXT(" ") : TEXT(""));
	return S;
}

// ---- the card in the world ------------------------------------------------------------------------------

UTSSCardVisual::UTSSCardVisual()
{
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(this);
	Body->SetStaticMesh(TSSCubeMesh());
	Body->SetRelativeScale3D(FVector(CardWid / 100.f, CardLen / 100.f, CardThick / 100.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(false);

	Chip = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chip"));
	Chip->SetupAttachment(this);
	Chip->SetStaticMesh(TSSCubeMesh());
	Chip->SetRelativeLocation(FVector(-CardWid * 0.14f, -CardLen * 0.34f, CardThick * 0.5f + 0.01f));
	Chip->SetRelativeScale3D(FVector(1.4f / 100.f, 1.8f / 100.f, 0.02f / 100.f));
	Chip->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Chip->SetCastShadow(false);

	// Text lies flat on the face and reads from the +X side.
	const float Z = CardThick * 0.5f + 0.03f;
	BankText = MakeText(TEXT("BankText"), FVector(-CardWid * 0.36f, -CardLen * 0.17f, Z), 0.62f, FColor::White, EHTA_Center);
	NetworkText = MakeText(TEXT("NetworkText"), FVector(-CardWid * 0.36f, CardLen * 0.3f, Z), 0.7f, FColor::White, EHTA_Center);
	NumberText = MakeText(TEXT("NumberText"), FVector(CardWid * 0.1f, 0.f, Z), 0.95f, FColor::White, EHTA_Center);
	ExpiryText = MakeText(TEXT("ExpiryText"), FVector(CardWid * 0.26f, 0.f, Z), 0.55f, FColor::White, EHTA_Center);
	HolderText = MakeText(TEXT("HolderText"), FVector(CardWid * 0.4f, -CardLen * 0.18f, Z), 0.62f, FColor::White, EHTA_Center);
	KindText = MakeText(TEXT("KindText"), FVector(CardWid * 0.4f, CardLen * 0.34f, Z), 0.5f, FColor::White, EHTA_Center);
}

UTextRenderComponent* UTSSCardVisual::MakeText(const TCHAR* Name, FVector Loc, float Size, FColor Color, EHorizTextAligment Align)
{
	UTextRenderComponent* T = CreateDefaultSubobject<UTextRenderComponent>(Name);
	T->SetupAttachment(this);
	T->SetRelativeLocation(Loc);
	T->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	T->SetWorldSize(Size);
	T->SetHorizontalAlignment(Align);
	T->SetVerticalAlignment(EVRTA_TextCenter);
	T->SetTextRenderColor(Color);
	T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	T->SetCastShadow(false);
	return T;
}

void UTSSCardVisual::SetCard(const FTSSBankCard& Card)
{
	const FDesign& D = GDesigns[FMath::Clamp(Card.Design, 0, UE_ARRAY_COUNT(GDesigns) - 1)];
	TSSApplyColor(Body, D.Base);
	TSSApplyColor(Chip, FLinearColor(0.75f, 0.55f, 0.15f));
	auto Set = [&D](UTextRenderComponent* T, const FString& S) { T->SetText(FText::FromString(S)); T->SetTextRenderColor(D.Ink); };
	Set(BankText, Card.Bank);
	Set(NetworkText, Card.Network);
	Set(NumberText, Card.Spaced());
	Set(ExpiryText, FString::Printf(TEXT("VALID THRU  %s"), *Card.Expiry()));
	Set(HolderText, Card.Holder);
	Set(KindText, TEXT("DEBIT"));
}

void UTSSCardVisual::SetShown(bool bShown)
{
	SetVisibility(bShown, true);
}
