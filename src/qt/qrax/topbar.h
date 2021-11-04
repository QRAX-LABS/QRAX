// Copyright (c) 2019-2020 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TOPBAR_H
#define TOPBAR_H

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
class TopBar;
}

class TopBar : public PWidget
{
    Q_OBJECT

public:
    explicit TopBar(QRAXGUI* _mainWindow, QWidget *parent = nullptr);
    ~TopBar();

    void loadWalletModel() override;

    void run(int type) override;
    void onError(QString error, int type) override;

private Q_SLOTS:
    void onBtnReceiveClicked();

public Q_SLOTS:
    void updateBalances(const interfaces::WalletBalances& newBalance);
    void updateMultiMining();
    void refreshAssets(const interfaces::AssetsBalance &newMultimining);
    void updateDisplayAssetPercent();
    void updateDisplayUnit();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::TopBar *ui;

    int nDisplayUnit = -1;
    int nDisplayAssetPercent = -1;

    QTimer* timerStakingIcon = nullptr;
    bool isInitializing = true;
    QTimer* assetRefreshTimer = nullptr;
	bool isReceiveDialogShowed = false;

};

#endif // TOPBAR_H
