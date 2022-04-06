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
#include <QThread>

class TooltipMenu;
class QRAXGUI;
class WalletModel;
class QmultiMiningTreeNode;
class QAssetsGraphicsScene;

namespace Ui {
class AssetsWidget;
}

static const unsigned int defaultPointDistance = 100;

class AssetWorker : public QObject {

	Q_OBJECT

private:
	bool loaded = false;
	WalletModel* walletModel = nullptr;
	std::map<CKeyID, QmultiMiningTreeNode*> structureNodes;
	std::vector<QAssetEdge*> structureEdges;
	unsigned int levelDepth = 0;
	QAssetsGraphicsScene  *structureScene;
	CAssetNode *rootTreeNode;
	void checkAndUpdateStructure();
	void addChildrensToStructure(CAssetNode *node, uint8_t &currentLevel, uint16_t &position);
	void updateNodePositions();
	void updateInternalNodePosition(QmultiMiningTreeNode *node, uint16_t &position);
	std::vector<QmultiMiningTreeNode *> getChildrensForNode(const CKeyID &parentId);

public:
	AssetWorker(){};

	~AssetWorker(){
	}

	void setWalletModel(WalletModel *_walletModel)
	{
		walletModel = _walletModel;
	}

	QAssetsGraphicsScene *getStructureScene() const;
	void setStructureScene(QAssetsGraphicsScene *value);
	void setLevelDepth(unsigned int value);

public Q_SLOTS:
	void n();
	void process();
	void createNode();

Q_SIGNALS:
	void s();
	void finished();
	void submitNode(QmultiMiningTreeNode* node);
	void submitEdge(QAssetEdge* enge);

};


class AssetsWidget : public PWidget
{
	Q_OBJECT

public:
	explicit AssetsWidget(QRAXGUI* _mainWindow, QWidget *parent = nullptr);
    ~AssetsWidget();

    void loadWalletModel() override;


public Q_SLOTS:

    void selectedItemChanged(QGraphicsItem *newItem, QGraphicsItem *oldItem);
    void updateDisplayUnit();
    void updateDisplayAssetPercent();
    void zoomIn();
    void zoomOut();
	void startRefreshTree();
	void finishRefresh();

private Q_SLOTS:
    void changeTheme(bool isLightTheme, QString &theme) override;

private:

    Ui::AssetsWidget *ui;
    int nDisplayUnit = -1;
    QAssetsGraphicsScene  *structureScene;
    int nDisplayAssetPercent = -1;
	bool threadIsRunning = false;
    TooltipMenu* menu = nullptr;

    void scaleView(qreal scaleFactor);
	AssetWorker worker;
	QThread thread;

	unsigned int levelDepth = 0;

};

#endif // ASSETSWIDGET_H
