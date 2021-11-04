// Copyright (c) 2021 The QRAX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Copyright (c) 2019-2020 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/qrax/assets/widget.h"
#include "qt/qrax/assets/forms/ui_assetswidget.h"
#include "qt/qrax/assets/graphicsscene.h"
#include "qt/qrax/assets/qassetgraphics.h"
#include "qt/qrax/assets/node.h"
#include "qt/qrax/assets/edge.h"
#include "qt/qrax/addresslabelrow.h"
#include "qt/qrax/addnewaddressdialog.h"
#include "qt/qrax/tooltipmenu.h"
#include "qt/qrax/qraxgui.h"
#include "qt/guiutil.h"
#include "qt/qrax/qtutils.h"
#include "qt/walletmodel.h"

#include <QRegExpValidator>
#include <QGraphicsItem>
#include <QtConcurrent/QtConcurrent>

AssetsWidget::AssetsWidget(QRAXGUI* _mainWindow, QWidget *parent) :
    PWidget(_mainWindow, parent),
    ui(new Ui::AssetsWidget)
{

    ui->setupUi(this);
    this->setMouseTracking(true);
    this->setStyleSheet(_mainWindow->styleSheet());

    /* Containers */
    setCssProperty(ui->left, "container");
    ui->left->setContentsMargins(0,20,0,20);
    setCssProperty(ui->right, "container-right");
    ui->right->setContentsMargins(20,20,20,20);

    /* Light Font */
    QFont fontLight;
    fontLight.setWeight(QFont::Light);

    /* Title */
    setCssProperty({ui->labelTitle, ui->labelInfoTitle}, "text-title-screen");
    ui->labelTitle->setFont(fontLight);

    setCssProperty({ui->labelSubtitle, ui->labelBalance, ui->labelAssetBalance, ui->labelNodePercent, ui->labelPayment, ui->labelType, ui->labelHash, ui->labelLevel}, "text-subtitle");
    setCssProperty({ui->labelBalanceValue, ui->labelAssetValue, ui->labelPercentValue, ui->labelTypeValue, ui->labelPaymentValue, ui->labelHashValue, ui->labelLevelValue}, "amount-small-topbar");

    structureScene = new QAssetsGraphicsScene(this);
    ui->graphicsView->setScene(structureScene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
	ui->graphicsView->setCacheMode(QGraphicsView::CacheBackground);
    ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    ui->graphicsView->scale(qreal(1), qreal(1));


    setCssProperty(ui->pushZoomIn, "btn-check");
    setCssProperty(ui->pushZoomOut, "btn-check");

    QObject::connect(structureScene,
                      SIGNAL(focusItemChanged(QGraphicsItem*, QGraphicsItem*, Qt::FocusReason)),
                      this,
                      SLOT(selectedItemChanged(QGraphicsItem*, QGraphicsItem*)));

    connect(ui->pushZoomIn, &QPushButton::clicked, this, &AssetsWidget::zoomIn);
    connect(ui->pushZoomOut, &QPushButton::clicked, this, &AssetsWidget::zoomOut);

    /* Containers */
    setCssProperty(ui->left, "container");
    ui->left->setContentsMargins(0,20,0,20);
    setCssProperty(ui->right, "container-right");
    ui->right->setContentsMargins(20,10,20,20);

}


void AssetsWidget::zoomOut()
{
    qreal zoom = 1 / qreal(1.1);
    scaleView(zoom);
}


void AssetsWidget::zoomIn()
{
    qreal zoom = qreal(1.1);
    scaleView(zoom);
}

void AssetsWidget::scaleView(qreal scaleFactor)
{
    qreal factor = ui->graphicsView->transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();

    if (factor < 0.07 || factor > 100)
        return;

    ui->graphicsView->scale(scaleFactor, scaleFactor);
}

void AssetsWidget::sceneKeyPressEvent(QKeyEvent *event)
{

}

void AssetsWidget::selectedItemChanged(QGraphicsItem *newItem, QGraphicsItem *oldItem)
{
    if (walletModel) {
        if (QmultiMiningTreeNode *citem = dynamic_cast<QmultiMiningTreeNode*>(newItem)) {
            CAssetNode *node = walletModel->GetInformationNode(citem->getKeyid());
            if (node == nullptr) return;
            double percent = node->GetWalletPercent();
            switch (nDisplayAssetPercent) {
                case 1:
                    percent = percent * 30.41;
                    break;
                case 2:
                    percent = percent * 365;
                    break;
                default:
                    break;
            }
            ui->labelHashValue->setText(QString::fromStdString(node->getKeyId().ToString()).left(10) + "..." + QString::fromStdString(node->getKeyId().ToString()).right(10));
            ui->labelLevelValue->setText(QString("%1").arg(citem->getLevel()));
            ui->labelBalanceValue->setText(GUIUtil::formatBalance(node->totalBalance, nDisplayUnit));
            ui->labelAssetValue->setText(GUIUtil::formatBalance(node->structureBalance, nDisplayUnit));
            ui->labelPercentValue->setText(GUIUtil::formatPercent(percent));
            QDateTime date = QDateTime::fromTime_t(static_cast<uint>(node->GetUpdateTime()));
            ui->labelPaymentValue->setText(GUIUtil::dateTimeStr(date));
            ui->labelTypeValue->setText(node->isMasterNode ? "Mastenode" : "Node");
        } else {
            ui->labelHashValue->setText("N/A");
            ui->labelLevelValue->setText("N/A");
            ui->labelBalanceValue->setText("N/A");
            ui->labelAssetValue->setText("N/A");
            ui->labelPercentValue->setText("N/A");
            ui->labelPaymentValue->setText("N/A");
            ui->labelTypeValue->setText("N/A");
        }
    }
}

void AssetsWidget::updateDisplayUnit()
{
    if (walletModel && walletModel->getOptionsModel()) {
        int displayUnitPrev = nDisplayUnit;
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
    }
}

void AssetsWidget::loadWalletModel()
{
    if (walletModel) {
        structureScene->setWalletModel(walletModel);
        connect(walletModel, &WalletModel::multiminingStructureChanged, this, &AssetsWidget::checkUpdateForStructure);
        connect(walletModel->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &AssetsWidget::updateDisplayUnit);
        connect(walletModel->getOptionsModel(), &OptionsModel::changeDisplayAssetPercent, this, &AssetsWidget::updateDisplayAssetPercent);

        updateDisplayUnit();
        updateDisplayAssetPercent();

        timerUpdatePositions = new QTimer(this);
        //timerUpdatePositions->setSingleShot(true);
        connect(timerUpdatePositions, &QTimer::timeout, this, &AssetsWidget::updateNodePositions);
        timerUpdatePositions->start(1000 / 25);
    }
}

std::atomic<bool> processingDrawStructure{false};

std::atomic<bool> needUpdateStructure{false};

void AssetsWidget::updateNodePositions()
{
    uint16_t curLevel = 0;
    if (processingDrawStructure) return;
    if (!needUpdateStructure) return;

    QmultiMiningTreeNode *parentNode = nullptr;
    std::map<CKeyID, QmultiMiningTreeNode*>::iterator structureIt = structureNodes.find(rootTreeNode->getKeyId());
    if (structureIt != structureNodes.end()) {
        uint16_t pos = 0;
        updateInternalNodePosition(structureIt->second, pos);
    }

    bool haveNodes = true;
    while (haveNodes) {
        haveNodes = false;
        QList<QGraphicsItem *> allItems = structureScene->items(Qt::AscendingOrder);
        for (QGraphicsItem* item: allItems)
        {
            if (QmultiMiningTreeNode *nodeItem = dynamic_cast<QmultiMiningTreeNode*>(item)) {
                if (curLevel == nodeItem->getLevel()) {
                    haveNodes = true;

                    QVector<QmultiMiningTreeNode *> childs = nodeItem->getChildNodes();
                    uint16_t parentCount = childs.size();

                    uint16_t yOffsetFromRoot = ((parentCount / 4) * 0.5 * defaultPointDistance) + defaultPointDistance;
                    uint16_t xOffsetFromRoot = defaultPointDistance * (parentCount / 2);
                    if (parentCount > 1 && parentCount % 2 == 0) {
                        xOffsetFromRoot = xOffsetFromRoot - defaultPointDistance / 2;
                    }

                    QPointF p;
                    QPointF parentF = nodeItem->mapToScene(p);

                    int16_t totalPosition = 0;
                    for(QmultiMiningTreeNode *child : childs) {
                        totalPosition = - xOffsetFromRoot + child->getPosition() * defaultPointDistance;
                        child->setPos(totalPosition + parentF.x(), yOffsetFromRoot + parentF.y());
                    }

                }
            }
        }
        curLevel++;
    }

    //structureScene->update();

    curLevel = 100;
    while(curLevel > 0)
    {
        QList<QGraphicsItem *> allItems = structureScene->items(Qt::AscendingOrder);
        for (QGraphicsItem* item: allItems)
        {
            if (QmultiMiningTreeNode *nodeItem = dynamic_cast<QmultiMiningTreeNode*>(item)) {
                if (nodeItem->getParentNode() == nullptr) continue;
                if (curLevel == nodeItem->getLevel()) {
                    QVector<QmultiMiningTreeNode *> childs = nodeItem->getChildNodes();
                    uint16_t childCount = childs.size();
                    if (childCount > 1) {
                        int16_t offsetX = (childCount/ 2) * defaultPointDistance;
                        if (nodeItem->x() > nodeItem->getParentNode()->x()) {
                            nodeItem->moveBy(offsetX, 0);
                            nodeItem->moveSiblingNodes(offsetX);
                        } else if (nodeItem->x() < nodeItem->getParentNode()->x()) {
                            nodeItem->moveBy(-offsetX, 0);
                            nodeItem->moveSiblingNodes(-offsetX);
                        } else if (nodeItem->x() == nodeItem->getParentNode()->x())
                        {
                            nodeItem->moveSiblingNodes(offsetX /2);
                            nodeItem->moveSiblingNodes(-offsetX /2);
                        }
                    }
                }
            }
        }

        curLevel--;
    }

    //structureScene->update();

    bool isCollided = true;
    while (isCollided)
    {
        isCollided = false;
        QList<QGraphicsItem *> allItems = structureScene->items(Qt::AscendingOrder);
        for (QGraphicsItem* item: allItems)
        {
            if (QmultiMiningTreeNode *nodeItem1 = dynamic_cast<QmultiMiningTreeNode*>(item)) {

                QList<QGraphicsItem *> collidings = structureScene->collidingItems(nodeItem1);
                for (QGraphicsItem* colItem: collidings) {

                  if (QmultiMiningTreeNode *nodeItem2 = dynamic_cast<QmultiMiningTreeNode*>(colItem)) {
                        if (nodeItem1->getLevel() == 0 || nodeItem2->getLevel() == 0) continue;
                        if (nodeItem1->getKeyid() == nodeItem2->getKeyid()) continue;
                        if (nodeItem1->getParentNode() == nullptr || nodeItem2->getParentNode() == nullptr) continue;
                        if (nodeItem1->getParentId() == nodeItem2->getParentId()) continue;

                        //if (nodeItem1->pos() == nodeItem2->pos())
                        //{

                            if (nodeItem1->getParentNode()->x() > nodeItem2->getParentNode()->x())
                            {
                                isCollided = true;
                                nodeItem1->getParentNode()->moveBy(25, 0);
                                nodeItem1->getParentNode()->moveSiblingNodes(25);
                                nodeItem2->getParentNode()->moveBy(-25, 0);
                                nodeItem2->getParentNode()->moveSiblingNodes(-25);

                            } else if (nodeItem1->getParentNode()->x() < nodeItem2->getParentNode()->x())
                            {
                                isCollided = true;
                                nodeItem1->getParentNode()->moveBy(-25, 0);
                                nodeItem1->getParentNode()->moveSiblingNodes(-25);
                                nodeItem2->getParentNode()->moveBy(25, 0);
                                nodeItem2->getParentNode()->moveSiblingNodes(25);
                            } else {

                                if (nodeItem1->getParentNode() == nullptr || nodeItem2->getParentNode() == nullptr) continue;
                                QmultiMiningTreeNode *tmp1 = nodeItem1->getParentNode();
                                QmultiMiningTreeNode *tmp2 = nodeItem2->getParentNode();

                                /*while (tmp1->pos() == tmp2->pos() && tmp1->getParentNode() != nullptr && tmp2->getParentNode() != nullptr) {
                                    tmp1 = tmp1->getParentNode();
                                    tmp2 = tmp2->getParentNode();
                                }*/
                                if (tmp1->x() > tmp2->x())
                                {
                                    isCollided = true;
                                    tmp1->moveSiblingNodes(25);
                                    tmp2->moveSiblingNodes(-25);
                                } else if (tmp1->x() < tmp2->x()) {
                                    isCollided = true;
                                    tmp1->moveSiblingNodes(-25);
                                    tmp2->moveSiblingNodes(25);
                                }
                            }

                        /*} else if (nodeItem1->getParentId() != nodeItem2->getParentId()) {
                            CKeyID first;
                            CKeyID second;
                            if (findCommonParent(nodeItem1, nodeItem2, first, second))
                            {

                                QmultiMiningTreeNode *firstN = nullptr;
                                std::map<CKeyID, QmultiMiningTreeNode*>::iterator structureIt1 = structureNodes.find(first);
                                if (structureIt1 != structureNodes.end()) {
                                    firstN = structureIt1->second;
                                }

                                QmultiMiningTreeNode *secondN = nullptr;
                                std::map<CKeyID, QmultiMiningTreeNode*>::iterator structureIt2 = structureNodes.find(second);
                                if (structureIt2 != structureNodes.end()) {
                                    secondN = structureIt2->second;
                                }

                                if (firstN != nullptr && secondN != nullptr) {
                                    if (firstN->getPosition() > secondN->getPosition()) {
                                        isCollided = true;
                                        firstN->moveSiblingNodes(-25);
                                        secondN->moveSiblingNodes(25);
                                    } else if (secondN->getPosition() > firstN->getPosition()) {
                                        isCollided = true;
                                        firstN->moveSiblingNodes(25);
                                        secondN->moveSiblingNodes(-25);
                                    }
                                }
                            }
                        }*/

                  }
                }

            }
        }
    }
    structureScene->update();
    needUpdateStructure = false;
}

void AssetsWidget::updateDisplayAssetPercent()
{
    if (walletModel && walletModel->getOptionsModel()) {
         nDisplayAssetPercent = walletModel->getOptionsModel()->getAssetPercentPeriod();
    }
}

void AssetsWidget::checkUpdateForStructure()
{

    rootTreeNode = walletModel->GetMultiMiningOwnStructure();
    if (rootTreeNode == nullptr) return;
    processingDrawStructure = true;

    uint8_t currentLevel = 0;
    uint16_t currentPosition = 0;
    if (structureNodes.find(rootTreeNode->getKeyId()) == structureNodes.end()) {
        QmultiMiningTreeNode *rootNode = new QmultiMiningTreeNode(this);
        rootNode->setKeyid(rootTreeNode->getKeyId());
        rootNode->setPosition(0);
        rootNode->setLevel(0);
        rootNode->setZValue(0.1);
        rootNode->setPos(0, 0);
        structureScene->addItem(rootNode);
        structureNodes.emplace(rootTreeNode->getKeyId(), rootNode);
    }

    for (CAssetNode* childNode : rootTreeNode->childrens) {
        AddChildrensToScene(childNode, currentLevel, currentPosition);
        currentPosition++;
    }

    QList<QGraphicsItem *> items = structureScene->items(Qt::DescendingOrder);
    for (QGraphicsItem* item: items) {
      if (QmultiMiningTreeNode *citem = dynamic_cast<QmultiMiningTreeNode*>(item)) {
        if (citem->isVisible() == false) structureScene->removeItem(citem);
      }
    }

    structureScene->update();
    processingDrawStructure = false;

}

void AssetsWidget::updateInternalNodePosition(QmultiMiningTreeNode* node, uint16_t &position)
{
    node->setPosition(position);

    std::vector<QmultiMiningTreeNode*> childs = getChildrensForNode(node->getKeyid());
    uint16_t curPos = 0;
    for(QmultiMiningTreeNode *child : childs) {
       updateInternalNodePosition(child, curPos);
       curPos++;
    }
}

std::vector<QmultiMiningTreeNode*> AssetsWidget::getChildrensForNode(const CKeyID &parentId)
{
    std::vector<QmultiMiningTreeNode*> nodes;

    QList<QGraphicsItem *> allItems = structureScene->items(Qt::AscendingOrder);
    for (QGraphicsItem* item: allItems)
    {
        if (QmultiMiningTreeNode *nodeItem = dynamic_cast<QmultiMiningTreeNode*>(item)) {

            if (nodeItem->getParentId() == parentId) nodes.push_back(nodeItem);
        }
    }

    return nodes;
}

bool AssetsWidget::findCommonParent(QmultiMiningTreeNode *first, QmultiMiningTreeNode* second, CKeyID &childFirst, CKeyID &childSecond)
{
    bool isCommonParent = false;
    QmultiMiningTreeNode *_first = first;
    QmultiMiningTreeNode *_second = second;


    while (isCommonParent == false) {
        if (_first->getParentNode() == nullptr || _second->getParentNode() == nullptr) break;
        if (_first->getParentNode()->getKeyid() == _second->getParentNode()->getKeyid()) {
            isCommonParent = true;
            childFirst = _first->getKeyid();
            childSecond = _second->getKeyid();
        } else {
            if (_first->getLevel() > _second->getLevel()) {
                _first = _first->getParentNode();
            } else if (_second->getLevel() > _first->getLevel()) {
                _second = _second->getParentNode();
            } else {
                _first = _first->getParentNode();
                _second = _second->getParentNode();
            }
        }
    }

    return isCommonParent;
}

bool AssetsWidget::AddChildrensToScene(CAssetNode* node, uint8_t &currentLevel, uint16_t &position)
{

    QmultiMiningTreeNode *parentNode = nullptr;
    std::map<CKeyID, QmultiMiningTreeNode*>::iterator structureIt = structureNodes.find(node->getParentId());
    if (structureIt != structureNodes.end()) {
        parentNode = structureIt->second;
    } else {
        return false;
    }

    uint16_t parentCount = node->getCountChildrenNodes();
    parentNode->setChildCount(parentCount);

    currentLevel = parentNode->getLevel() + 1;
    std::map<CKeyID, QmultiMiningTreeNode*>::iterator existNodeIt = structureNodes.find(node->getKeyId());
    if (existNodeIt == structureNodes.end()) {

        if (currentLevel > 100) {
            return true;
        }

        QmultiMiningTreeNode *treeNode = new QmultiMiningTreeNode(this);

        treeNode->setKeyid(node->getKeyId());
        treeNode->setPosition(position);
        treeNode->setLevel(currentLevel);
        treeNode->setParentId(node->getParentId());
        treeNode->setParentNode(parentNode);
        treeNode->setZValue(0.1);

        structureScene->addItem(treeNode);

        QAssetEdge *lineItem = new QAssetEdge(parentNode, treeNode);
        parentNode->AddChildEdge(lineItem);
        parentNode->addChildNode(treeNode);

        structureScene->addItem(lineItem);

        structureNodes.emplace(node->getKeyId(), treeNode);
        needUpdateStructure = true;
    }

    uint16_t currentPosition = 0;
    for (CAssetNode* childNode : node->childrens) {
        AddChildrensToScene(childNode, currentLevel, currentPosition);
        currentPosition++;
    }

    return true;

}

AssetsWidget::~AssetsWidget()
{
    delete ui;
}


void AssetsWidget::changeTheme(bool isLightTheme, QString& theme)
{

}

void AssetsWidget::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
}

