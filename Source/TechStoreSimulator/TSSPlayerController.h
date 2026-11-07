#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TSSPlayerController.generated.h"

class UTSSMenuWidget;
class UTSSHudWidget;
class UTSSStoreDeskWidget;

UCLASS()
class ATSSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void PlayerTick(float DeltaTime) override;

	bool IsGameActive() const { return bGameActive; }

	/** StoreDesk: the business computer's full-screen interface. Gameplay input is off while it is open. */
	void OpenStoreDesk();
	void CloseStoreDesk();
	bool IsDeskOpen() const { return bDeskOpen; }
	UTSSStoreDeskWidget* GetDesk() const { return Desk; }
	/** Debug: switch the open StoreDesk to a page (opens it if needed). */
	UFUNCTION(Exec) void TSSDeskPage(int32 Page);
	/** Debug: run the StoreDesk end-to-end test (ordering, delivery, upgrades, save) and log TSSTEST lines. */
	UFUNCTION(Exec) void TSSDeskTest();

	/** Called by the menu once the player has picked a view and pressed CONTINUE. */
	void StartNewGame();
	void ResumeGame();
	void QuitToDesktop();

	/** Debug: skip the menu, run the buy/stock/take/price loop through the real interaction code and log TSSTEST lines. */
	UFUNCTION(Exec) void TSSSelfTest();
	/** Debug: send a customer through the door right now. */
	UFUNCTION(Exec) void TSSCustomer();
	/** Developer: sends an empty delivery truck down the road to the shop (no order, nothing delivered). */
	UFUNCTION(Exec) void TSSTruck();
	/** Developer: opens the Settings page (for screenshots). */
	UFUNCTION(Exec) void TSSSettingsPage();
	/** Developer: sets the in-game clock to this hour of the day, e.g. `TSSTime 21.5` for 9:30 PM. */
	UFUNCTION(Exec) void TSSTime(float Hour);
	/** Debug: run two customers through the real shop (one buys, one balks at the price) and log TSSTEST lines. */
	UFUNCTION(Exec) void TSSCustomerTest();
	/** Debug: put the player at X Y Z facing Yaw/Pitch (for screenshots). */
	UFUNCTION(Exec) void TSSLook(float X, float Y, float Z, float Yaw, float Pitch);
	/** Debug: parks one of each kind of car on the road (traffic off) so they can be photographed. */
	UFUNCTION(Exec) void TSSTownDemo();
	/** Debug: looks at demo car N from a good angle. Which: 0 front 3/4, 1 side, 2 rear 3/4, 3 street view down the road, 4 high. */
	UFUNCTION(Exec) void TSSTownCam(int32 Car, int32 Which);
	/** Debug: leave the menu and play. */
	/** Debug: one customer pays by chip and PIN while the camera photographs the typing, then the receipt. */
	UFUNCTION(Exec) void TSSPayDemo();
	UFUNCTION(Exec) void TSSStart();

protected:
	virtual void BeginPlay() override;

private:
	void ShowMenu(bool bMainMenu);
	void EnterGameInput();

	UPROPERTY() TObjectPtr<UTSSMenuWidget> Menu;
	UPROPERTY() TObjectPtr<UTSSHudWidget> Hud;
	UPROPERTY() TObjectPtr<UTSSStoreDeskWidget> Desk;
	bool bDeskOpen = false;
	bool bGameActive = false;
};