// Copyright (c) 2021 The QRAX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef ASSETSGRAPHICSSCENE_H
#define ASSETSGRAPHICSSCENE_H

#include <QGraphicsScene>

QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

class QAssetsGraphicsScene : public QGraphicsScene
{
 private:
    WalletModel *walletModel = nullptr;
    bool isCtrlPressed = false;

 public:
     explicit QAssetsGraphicsScene(QObject *parent = nullptr);

     void setWalletModel(WalletModel *_walletModel)
     {
         walletModel = _walletModel;
     }

 protected:
     void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
     void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
     void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
     void wheelEvent(QGraphicsSceneWheelEvent *wheelEvent) override;
     void keyPressEvent(QKeyEvent *keyEvent) override;
     void keyReleaseEvent(QKeyEvent *keyEvent) override;


/*Q_SIGNALS:
     void fillNodeInformation(const uint256 &hash);*/
};

#endif // ASSETSGRAPHICSSCENE_H
