// Copyright (c) 2019-2020 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include "qt/qrax/topbar.h"
#include "qt/qrax/forms/ui_topbar.h"
#include "qt/qrax/lockunlock.h"
#include "qt/qrax/qtutils.h"
#include "qt/qrax/receivedialog.h"
#include "qt/qrax/loadingdialog.h"
#include "askpassphrasedialog.h"

#include "bitcoinunits.h"
#include "qt/qrax/balancebubble.h"
#include "clientmodel.h"
#include "qt/guiconstants.h"
#include "qt/guiutil.h"
#include "optionsmodel.h"
#include "qt/platformstyle.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "guiinterface.h"

#include "masternode-sync.h"
#include "wallet/wallet.h"

#include <QPixmap>

#define REQUEST_UPGRADE_WALLET 1

class ButtonHoverWatcher : public QObject
{
public:
    explicit ButtonHoverWatcher(QObject* parent = nullptr) :
            QObject(parent) {}
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        QPushButton* button = qobject_cast<QPushButton*>(watched);
        if (!button) return false;

        if (event->type() == QEvent::Enter) {
            button->setIcon(QIcon("://ic-information-hover"));
            return true;
        }

        if (event->type() == QEvent::Leave){
            button->setIcon(QIcon("://ic-information"));
            return true;
        }
        return false;
    }
};

TopBar::TopBar(QRAXGUI* _mainWindow, QWidget *parent) :
    PWidget(_mainWindow, parent),
    ui(new Ui::TopBar)
{
    ui->setupUi(this);

    // Set parent stylesheet
    this->setStyleSheet(_mainWindow->styleSheet());
    /* Containers */
    ui->containerTop->setContentsMargins(10, 10, 10, 10);
    ui->containerTop->setProperty("cssClass", "container-topbar-no-image");

    std::initializer_list<QWidget*> lblTitles = {ui->labelTitle1, ui->mmAmountTitle};
    std::initializer_list<QWidget*> lblSubtitles = {ui->labelTitle3, ui->labelTitle4, ui->percentTitle, ui->structureBalanceTitle};
    setCssProperty(lblTitles, "text-title-topbar");
    setCssProperty(lblSubtitles, "text-subtitle-topbar");

    QFont font;
    font.setWeight(QFont::Light);
    for (QWidget* w : lblTitles) { w->setFont(font); }
    for (QWidget* w : lblSubtitles) { w->setFont(font); }

    // Amount information top
    //setCssProperty({ui->labelAmountTopPiv, ui->labelAmountTopShieldedPiv, ui->labelAmountTopMultiMiningPlc}, "amount-small-topbar");
    setCssProperty({ui->labelAmountPiv, ui->multiMiningAmount}, "amount-topbar");
    setCssProperty({ui->labelPendingPiv, ui->labelImmaturePiv, ui->structureBalance, ui->mmPercent}, "amount-small-topbar");


    setCssProperty(ui->qrContainer, "container-qr");
    setCssProperty(ui->pushButtonQR, "btn-qr");
    ui->mmAmountTitle->setText(tr("Assets"));

    // QR image
    QPixmap pixmap("://img-qr-test");
    ui->btnQr->setIcon(
                QIcon(pixmap.scaled(
                         70,
                         70,
                         Qt::KeepAspectRatio))
                );

    connect(ui->pushButtonQR, &QPushButton::clicked, this, &TopBar::onBtnReceiveClicked);
    connect(ui->btnQr, &QPushButton::clicked, this, &TopBar::onBtnReceiveClicked);

}

void TopBar::onBtnReceiveClicked()
{
	if (walletModel && !isReceiveDialogShowed) {

        QString addressStr = walletModel->getAddressTableModel()->getAddressToShow();
        if (addressStr.isNull()) {
            inform(tr("Error generating address"));
            return;
        }
		isReceiveDialogShowed = true;
        showHideOp(true);
        ReceiveDialog *receiveDialog = new ReceiveDialog(window);
        receiveDialog->updateQr(addressStr);
		if (openDialogWithOpaqueBackgroundY(receiveDialog, window, 3.5, 12)) {
            inform(tr("Address Copied"));
        }
        receiveDialog->deleteLater();
		isReceiveDialogShowed = false;
    }
}

TopBar::~TopBar()
{
    if (assetRefreshTimer) {
        assetRefreshTimer->stop();
    }
    delete ui;
}

void TopBar::loadWalletModel()
{
    connect(walletModel, &WalletModel::balanceChanged, this, &TopBar::updateBalances);
    connect(walletModel, &WalletModel::multiminingChanged, this, &TopBar::refreshAssets);
    connect(walletModel->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &TopBar::updateDisplayUnit);
    connect(walletModel->getOptionsModel(), &OptionsModel::changeDisplayAssetPercent, this, &TopBar::updateDisplayAssetPercent);

    updateDisplayUnit();
    updateDisplayAssetPercent();

    assetRefreshTimer = new QTimer(this);
    connect(assetRefreshTimer, &QTimer::timeout, this, &TopBar::updateMultiMining);
    assetRefreshTimer->start(1000);

    isInitializing = false;
}

void TopBar::updateDisplayAssetPercent()
{
    if (walletModel && walletModel->getOptionsModel()) {
        int displayAssetPrev = nDisplayAssetPercent;
        nDisplayAssetPercent = walletModel->getOptionsModel()->getAssetPercentPeriod();
        if (displayAssetPrev != nDisplayAssetPercent) {
            refreshAssets(walletModel->GetAssetBalances());
        }
    }
}

void TopBar::updateDisplayUnit()
{
    if (walletModel && walletModel->getOptionsModel()) {
        int displayUnitPrev = nDisplayUnit;
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if (displayUnitPrev != nDisplayUnit) {
            updateBalances(walletModel->GetWalletBalances());
        }
    }
}

void TopBar::refreshAssets(const interfaces::AssetsBalance &newMultimining)
{

    double currentPercent = newMultimining.percent;
    double currentBalance = newMultimining.assets;
    double timePeriod = GetAdjustedTime() - newMultimining.time;
    double balance = currentBalance * currentPercent * timePeriod / 864000000;
    CAmount totalBalance = newMultimining.profit + balance;

    if (newMultimining.time == 0) totalBalance = 0;

    QString mmAmount = GUIUtil::formatBalance(totalBalance, nDisplayUnit);
    ui->multiMiningAmount->setText(mmAmount);

    QString mmStructure = GUIUtil::formatBalance(newMultimining.structure, nDisplayUnit);
    ui->structureBalance->setText(mmStructure);

    switch (nDisplayAssetPercent) {
        case 1:
            ui->percentTitle->setText("Percent / month");
            currentPercent = currentPercent * 30.41;
            break;
        case 2:
            ui->percentTitle->setText("Percent / year");
            currentPercent = currentPercent * 365;
            break;
        default:
            ui->percentTitle->setText("Percent / day");
            break;
    }

    QString percent = GUIUtil::formatPercent(currentPercent);

    ui->mmPercent->setText(percent);
}

void TopBar::updateMultiMining()
{
    if (walletModel) {
        refreshAssets(walletModel->GetAssetBalances());
    }
}

void TopBar::updateBalances(const interfaces::WalletBalances& newBalance)
{
    // Locked balance. //TODO move this to the signal properly in the future..
    CAmount nLockedBalance = 0;
    if (walletModel) {
        nLockedBalance = walletModel->getLockedBalance();
    }
    ui->labelTitle1->setText(nLockedBalance > 0 ? tr("Available (Locked included)") : tr("Available"));

	QString totalQrax = GUIUtil::formatBalance(newBalance.balance, nDisplayUnit);
    QString totalTransparent = GUIUtil::formatBalance(newBalance.balance - newBalance.shielded_balance);
    QString totalShielded = GUIUtil::formatBalance(newBalance.shielded_balance);

	ui->labelAmountPiv->setText(totalQrax);
    ui->labelPendingPiv->setText(GUIUtil::formatBalance(newBalance.unconfirmed_balance + newBalance.unconfirmed_shielded_balance, nDisplayUnit));
    ui->labelImmaturePiv->setText(GUIUtil::formatBalance(newBalance.immature_balance, nDisplayUnit));
}

void TopBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void TopBar::run(int type)
{
    if (type == REQUEST_UPGRADE_WALLET) {
        std::string upgradeError;
        bool ret = this->walletModel->upgradeWallet(upgradeError);
        QMetaObject::invokeMethod(this,
                "updateHDState",
                Qt::QueuedConnection,
                Q_ARG(bool, ret),
                Q_ARG(QString, QString::fromStdString(upgradeError))
        );
    }
}

void TopBar::onError(QString error, int type)
{
    if (type == REQUEST_UPGRADE_WALLET) {
        warn(tr("Upgrade Wallet Error"), error);
    }
}
