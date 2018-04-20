#ifndef EQIPCMFASTPICKSYSTEM_H
#define EQIPCMFASTPICKSYSTEM_H

#include "head.h"
#include <QObject>
#include <QMap>
#include "qgsmaptool.h"
#include "qgsrasterlayer.h"
#include "eqi/eqiinquiredemvalue.h"

class eqiPcmFastPickSystem : public QgsMapTool
{
    Q_OBJECT
public:
    eqiPcmFastPickSystem(QgsMapCanvas* canvas, QgsVectorLayer* mLayer_pcm, QgsRasterLayer* rasterLayer, QStringList& demPaths);

    //! Overridden mouse release event
    virtual void canvasReleaseEvent( QgsMapMouseEvent* e ) override;

private:
    // 获得当前矢量图层，检查图层是否符合要求
    bool getCurrentVectorLayer();

private:
    // 用于保存像控快速拾取系统的DOM
    // 没用上***********
    QgsRasterLayer* pcm_rasterLayer;

    // 用于保存像控快速拾取系统的DEM路径
    QStringList pcm_demPaths;

    QgsVectorLayer* newLayer;

    eqiInquireDemValue idv;

    QSettings mSettings;

    int index;
    bool isAvailable;
};

#endif // EQIPCMFASTPICKSYSTEM_H
