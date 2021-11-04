// Copyright (c) 2019-2020 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BOTTOMBAR_H
#define BOTTOMBAR_H

#include <QWidget>
#include "qt/qrax/pwidget.h"
#include "qt/qrax/lockunlock.h"
#include "amount.h"
#include <QTimer>
#include <QProgressBar>

class BalanceBubble;
class QRAXGUI;
class WalletModel;
class ClientModel;

namespace Ui {
class BottomBar;
}

class BottomBar : public PWidget
{
    Q_OBJECT

public:
    explicit BottomBar(QRAXGUI* _mainWindow, QWidget *parent = nullptr);
    ~BottomBar();

    void loadWalletModel() override;
    void loadClientModel() override;

    void openPassPhraseDialog(AskPassphraseDialog::Mode mode, AskPassphraseDialog::Context ctx);
    void encryptWallet();

    void run(int type) override;
    void onError(QString error, int type) override;
    void unlockWallet();

public Q_SLOTS:
    void updateBalances(const interfaces::WalletBalances& newBalance);
    void updateMultiMining(const interfaces::AssetsBalance& newMultimining);

    void updateDisplayUnit();

    void setNumConnections(int count);
    void setNumBlocks(int count);
    void setStakingStatusActive(bool fActive);
    void updateStakingStatus();
    void updateHDState(const bool& upgraded, const QString& upgradeError);
    void showUpgradeDialog(const QString& message);

Q_SIGNALS:
    void themeChanged(bool isLight);
    void walletSynced(bool isSync);
    void onShowHideColdStakingChanged(bool show);

protected:
    void resizeEvent(QResizeEvent *event) override;
private Q_SLOTS:
    void onBtnReceiveClicked();
    void onBtnBalanceInfoClicked();
    void onThemeClicked();
    void onBtnLockClicked();
    void lockDropdownMouseLeave();
    void lockDropdownClicked(const StateClicked&);
    void refreshStatus();
    void openLockUnlock();
    void onColdStakingClicked();
    void refreshProgressBarSize();
    void expandSync();
private:
    Ui::BottomBar *ui;
    LockUnlock *lockUnlockWidget = nullptr;
    QProgressBar* progressBar = nullptr;

    int nDisplayUnit = -1;
    QTimer* timerStakingIcon = nullptr;
    bool isInitializing = true;

    // info popup
    BalanceBubble* balanceBubble = nullptr;

    void updateTorIcon();
    void connectUpgradeBtnAndDialogTimer(const QString& message);
};

#endif // BOTTOMBAR_H
