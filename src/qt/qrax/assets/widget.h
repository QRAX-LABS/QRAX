// Copyright (c) 2021 The QRAX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Copyright (c) 2019-2020 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ASSETSWIDGET_H
#define ASSETSWIDGET_H

#include "qt/qrax/pwidget.h"
#include "qt/qrax/tooltipmenu.h"
#include "qt/qrax/furabstractlistitemdelegate.h"
#include "qt/qrax/assets/graphicsscene.h"
#include "qt/qrax/assets/edge.h"
#include "qt/qrax/assets/node.h"
#include "uint256.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "guiinterface.h"
#include "bitcoinunits.h"
#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QMouseEvent>

class TooltipMenu;
class QRAXGUI;
class WalletModel;
class QmultiMiningTreeNode;
class QAssetsGraphicsScene;

namespace Ui {
class AssetsWidget;
}

static const unsigned int defaultPointDistance = 100;


class AssetsWidget : public PWidget
{
    Q_OBJECT

public:
    explicit AssetsWidget(QRAXGUI* _mainWindow, QWidget *parent = nullptr);
    ~AssetsWidget();

    void loadWalletModel() override;
    //void loadClientModel() override;

    bool AddChildrensToScene(CAssetNode *node, uint8_t &currentLevel, uint16_t &position);
    std::vector<QmultiMiningTreeNode *> getChildrensForNode(const CKeyID &parentId);
    bool findCommonParent(QmultiMiningTreeNode *first, QmultiMiningTreeNode *second, CKeyID &childFirst, CKeyID &childSecond);
    void updateInternalNodePosition(QmultiMiningTreeNode *node, uint16_t &position);


public Q_SLOTS:
    void checkUpdateForStructure();
    void selectedItemChanged(QGraphicsItem *newItem, QGraphicsItem *oldItem);
    void updateDisplayUnit();
    void updateDisplayAssetPercent();
    void sceneKeyPressEvent(QKeyEvent *event);
    void updateNodePositions();
    void zoomIn();
    void zoomOut();


private Q_SLOTS:
    void changeTheme(bool isLightTheme, QString &theme) override;

private:

    Ui::AssetsWidget *ui;
    int nDisplayUnit = -1;
    QAssetsGraphicsScene  *structureScene;
    CAssetNode *rootTreeNode;
    int nDisplayAssetPercent = -1;

    TooltipMenu* menu = nullptr;

    std::map<CKeyID, QmultiMiningTreeNode*> structureNodes;
    QTimer* timerUpdatePositions = nullptr;
    void scaleView(qreal scaleFactor);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
};

#endif // ASSETSWIDGET_H
