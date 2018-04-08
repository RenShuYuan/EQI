#ifndef EQIPCMFASTPICKSYSTEM_H
#define EQIPCMFASTPICKSYSTEM_H

#include <QObject>
#include <QStringList>
#include <QMap>
#include <QSettings>
#include "qgsmaptool.h"
#include "qgsrasterlayer.h"
#include "eqi/eqiinquiredemvalue.h"

class eqiPcmFastPickSystem : public QgsMapTool
{
    Q_OBJECT
public:
    eqiPcmFastPickSystem(QgsMapCanvas* canvas, QgsRasterLayer* rasterLayer, QStringList& demPaths);

    //! Overridden mouse release event
    virtual void canvasReleaseEvent( QgsMapMouseEvent* e ) override;

signals:
    void readPickPcm(QStringList &);

public slots:

private:
    // 用于保存像控快速拾取系统的DOM
    // 没用上
    QgsRasterLayer* pcm_rasterLayer;

    // 用于保存像控快速拾取系统的DEM路径
    QStringList pcm_demPaths;

    eqiInquireDemValue idv;

    QMap<int, QgsPoint> mapXY;
    QMap<int, qreal> mapZ;
    QStringList pcmList;

    QSettings mSettings;

    int index;
};

#endif // EQIPCMFASTPICKSYSTEM_H
