#include "TSSPasserby.h"
#include "TSSHumanComponent.h"

ATSSPasserby::ATSSPasserby()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Human = CreateDefaultSubobject<UTSSHumanComponent>(TEXT("Human"));
	Human->SetupAttachment(Root);
	Human->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));   // the kit's meshes face +Y
}

void ATSSPasserby::Begin(const TArray<FVector>& InPath, float InSpeed, int32 PauseAt, float PauseSeconds, const FVector& LookTarget)
{
	Path = InPath;
	Speed = InSpeed;
	PauseIndex = PauseAt;
	PauseTotal = PauseSeconds;
	Look = LookTarget;
	Index = Path.Num() > 1 ? 1 : 0;
	if (Path.Num() > 0)
	{
		SetActorLocation(Path[0]);
		if (Path.Num() > 1) SetActorRotation(FRotator(0.f, (Path[1] - Path[0]).Rotation().Yaw, 0.f));
	}
	auto Pick = [](const TArray<FString>& V) { return V[FMath::RandRange(0, V.Num() - 1)]; };
	Human->Build(FMath::RandRange(1, UTSSHumanComponent::NumIdentities()), Pick(UTSSHumanComponent::Tops()), Pick(UTSSHumanComponent::Bottoms()), Pick(UTSSHumanComponent::Shoes()));
}

void ATSSPasserby::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!Path.IsValidIndex(Index)) { Destroy(); return; }

	if (PauseLeft > 0.f)
	{
		PauseLeft -= Dt;
		Human->SetSpeed(0.f);
		Human->LookAt(Look);
		const FVector To = Look - GetActorLocation();
		FRotator Want(0.f, To.Rotation().Yaw, 0.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), Want, Dt, 4.f));
		if (PauseLeft <= 0.f) Human->StopLooking();
		return;
	}

	const FVector Goal = Path[Index];
	const FVector Delta = FVector(Goal.X, Goal.Y, 0.f) - FVector(GetActorLocation().X, GetActorLocation().Y, 0.f);
	const float Dist = Delta.Size();
	const float Step = Speed * Dt;
	if (Dist <= Step)
	{
		SetActorLocation(Goal);
		if (Index == PauseIndex && !bPauseDone)
		{
			bPauseDone = true;
			PauseLeft = PauseTotal;
		}
		++Index;
		return;
	}
	const FVector Dir = Delta / Dist;
	SetActorLocation(GetActorLocation() + FVector(Dir.X, Dir.Y, 0.f) * Step);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Dir.Rotation().Yaw, 0.f), Dt, 7.f));
	Human->SetSpeed(Speed);
}
