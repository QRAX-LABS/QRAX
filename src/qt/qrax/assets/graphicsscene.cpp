// Copyright (c) 2021 The QRAX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "qt/qrax/assets/node.h"
#include "qt/qrax/assets/graphicsscene.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include "logging.h"
#include <QTransform>

class QmultiMiningTreeNode;


QAssetsGraphicsScene::QAssetsGraphicsScene(QObject *parent)
    : QGraphicsScene(parent)
{

}

void QAssetsGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    // Move pressed mouse

    QGraphicsScene::mouseMoveEvent(mouseEvent);
}

void QAssetsGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (mouseEvent->button() != Qt::LeftButton)
        return;

    QGraphicsItem *item = QGraphicsScene::itemAt(mouseEvent->scenePos(), QTransform());

    if (QmultiMiningTreeNode *citem = dynamic_cast<QmultiMiningTreeNode*>(item)) {
        setFocusItem(item, Qt::MouseFocusReason);
    } else {
        clearFocus();
    }

    QGraphicsScene::mousePressEvent(mouseEvent);
}

void QAssetsGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (mouseEvent->button() != Qt::LeftButton)
        return;

    QGraphicsScene::mouseReleaseEvent(mouseEvent);
}

void QAssetsGraphicsScene::wheelEvent(QGraphicsSceneWheelEvent *wheelEvent)
{

}

void QAssetsGraphicsScene::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->key() != Qt::Key_Control)
        return;

    isCtrlPressed = true;
    QGraphicsScene::keyPressEvent(keyEvent);
}

void QAssetsGraphicsScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->key() != Qt::Key_Control)
        return;


    isCtrlPressed = false;
    QGraphicsScene::keyReleaseEvent(keyEvent);
}

