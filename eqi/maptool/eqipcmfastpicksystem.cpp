#include <QFileDialog>

#include "eqipcmfastpicksystem.h"
#include "qgsmessagelog.h"
#include "qgsmapcanvas.h"
#include "mainwindow.h"

eqiPcmFastPickSystem::eqiPcmFastPickSystem(QgsMapCanvas *canvas, QgsRasterLayer* rasterLayer, QStringList& demPaths)
    : QgsMapTool( canvas )
    , pcm_rasterLayer(rasterLayer)
    , pcm_demPaths(demPaths)
{
//    idv.setDemPath(pcm_demPaths);
    index = 0;
}

void eqiPcmFastPickSystem::canvasReleaseEvent(QgsMapMouseEvent *e)
{
    if (e->button()==Qt::RightButton)
    {
        // 注销地图工具
        QgsMapTool *lastMapTool = canvas()->mapTool();
        canvas()->unsetMapTool( lastMapTool );

        //
        if (mapXY.isEmpty()) return;

        QMap<int, QgsPoint>::const_iterator it = mapXY.constBegin();
        while (it != mapXY.constEnd())
        {
            QString record;
            record = QString("%1\t%2\t%3\t%4\n")
                    .arg(it.key())
                    .arg(it.value().x(), 0, 'f', 3)
                    .arg(it.value().y(), 0, 'f', 3)
                    .arg(mapZ.value(it.key()), 0, 'f', 3);
            pcmList << record;
            ++it;
        }
        emit readPickPcm( pcmList );
        return;
    }

    if (e->button()==Qt::LeftButton)
    {
        QgsPoint point = toMapCoordinates( e->pos() );

        QList< QgsPoint > points;
        QList< qreal > elevations;
        points << point;
        if (eqiInquireDemValue::eOK == idv.inquireElevations(points, elevations))
        {
            ++index;
            mapXY[index] = point;
            mapZ[index] = elevations.at(0);

            QgsMessageLog::logMessage(QString("x = %1 , y = %2, z = %3")
                                      .arg(point.x(), 0, 'f', 3)
                                      .arg(point.y(), 0, 'f', 3)
                                      .arg(elevations.at(0), 0, 'f', 3));
        }

    }
}
