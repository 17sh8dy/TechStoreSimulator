#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TSSPlayerController.generated.h"

class UTSSMenuWidget;
class UTSSHudWidget;

UCLASS()
class ATSSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void PlayerTick(float DeltaTime) override;

	bool IsGameActive() const { return bGameActive; }

	/** Called by the menu once the player has picked a view and pressed CONTINUE. */
	void StartNewGame();
	void ResumeGame();
	void QuitToDesktop();

	/** Debug: skip the menu, run the buy/stock/take/price loop through the real interaction code and log TSSTEST lines. */
	UFUNCTION(Exec) void TSSSelfTest();
	/** Debug: put the player at X Y Z facing Yaw/Pitch (for screenshots). */
	UFUNCTION(Exec) void TSSLook(float X, float Y, float Z, float Yaw, float Pitch);
	/** Debug: leave the menu and play. */
	UFUNCTION(Exec) void TSSStart();

protected:
	virtual void BeginPlay() override;

private:
	void ShowMenu(bool bMainMenu);
	void EnterGameInput();

	UPROPERTY() TObjectPtr<UTSSMenuWidget> Menu;
	UPROPERTY() TObjectPtr<UTSSHudWidget> Hud;
	bool bGameActive = false;
};