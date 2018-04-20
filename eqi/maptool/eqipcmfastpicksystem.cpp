#include <QFileDialog>
#include <QMessageBox>

#include "eqipcmfastpicksystem.h"
#include "qgsvectordataprovider.h"
#include "qgsgeometry.h"
#include "qgsmapcanvas.h"
#include "mainwindow.h"

eqiPcmFastPickSystem::eqiPcmFastPickSystem(QgsMapCanvas *canvas, QgsVectorLayer *mLayer_pcm, QgsRasterLayer* rasterLayer, QStringList& demPaths)
    : QgsMapTool( canvas )
    , pcm_rasterLayer(rasterLayer)
    , newLayer(mLayer_pcm)
    , pcm_demPaths(demPaths)
{
//    idv.setDemPath(pcm_demPaths);
    index = 0;
    isAvailable = true;

    isAvailable = getCurrentVectorLayer();

    if (isAvailable)
        mCursor = QCursor( Qt::CrossCursor );
}

void eqiPcmFastPickSystem::canvasReleaseEvent(QgsMapMouseEvent *e)
{
    if (!isAvailable)
        return;

    if (e->button()==Qt::RightButton)
    {
        // 注销地图工具
        QgsMapTool *lastMapTool = canvas()->mapTool();
        canvas()->unsetMapTool( lastMapTool );
        return;
    }

    if (e->button()==Qt::LeftButton)
    {
        QgsPoint point = toMapCoordinates( e->pos() );

        QList< QgsPoint > points;
        QList< qreal > elevations;
        points << point;
        double elev = 0.0;
        if (eqiInquireDemValue::eOK == idv.inquireElevations(points, elevations))
        {
            elev = elevations.at(0);
        }

        // 设置几何要素与属性
        QgsFeature MyFeature;
        QgsGeometry* mGeometry = QgsGeometry::fromPoint(point);
        MyFeature.setGeometry( mGeometry );
        MyFeature.setAttributes(QgsAttributes()
                                << QVariant(++index)
                                << QVariant(point.x())
                                << QVariant(point.y())
                                << QVariant(elev));

        MainWindow::instance()->mapCanvas()->freeze();

        // 添加要素集到图层中
        newLayer->dataProvider()->addFeatures( QgsFeatureList() << MyFeature );

        MainWindow::instance()->mapCanvas()->freeze( false );
        MainWindow::instance()->refreshMapCanvas();
    }
}

bool eqiPcmFastPickSystem::getCurrentVectorLayer()
{
    if ( !(newLayer && newLayer->isValid()))
    {
        MainWindow::instance()->messageBar()->pushMessage(
                    "创建图层",
                    "需要选择一个有效的矢量图层保存图框。",
                    QgsMessageBar::CRITICAL,
                    MainWindow::instance()->messageTimeout() );
        return false;
    }

    // 检查图层矢量类型是否正确
   if (QGis::Point != newLayer->geometryType())
   {
       MainWindow::instance()->messageBar()->pushMessage(
                   "创建图层",
                   QString("当前图层%1矢量类型必须为\"点\"。").arg(newLayer->name()),
                   QgsMessageBar::CRITICAL,
                   MainWindow::instance()->messageTimeout() );

       return false;
   }

   // 检查字段
    QgsFields fields = newLayer->pendingFields();
    int filedNameIndex = fields.indexFromName("点号");
    int filedXIndex = fields.indexFromName("X");
    int filedYIndex = fields.indexFromName("Y");
    int filedZIndex = fields.indexFromName("Z");
    if (filedNameIndex == -1 || filedXIndex == -1
        || filedYIndex == -1 || filedZIndex == -1)
    {
        MainWindow::instance()->messageBar()->pushMessage(
                    "创建图层",
                    "像控点位矢量图层需要\"点号\"、\"X\"、\"Y\"、\"Z\"字段。",
                    QgsMessageBar::CRITICAL,
                    MainWindow::instance()->messageTimeout() );
        return false;
    }

    // 检查字段类型和长度
    QgsField fieldName = fields.field(filedNameIndex);
    QgsField fieldX = fields.field(filedXIndex);
    QgsField fieldY = fields.field(filedYIndex);
    QgsField fieldZ = fields.field(filedZIndex);
    if (!((fieldName.type() == QVariant::String && fieldName.length() >= 10)
        && (fieldX.type() == QVariant::Double)
        && (fieldY.type() == QVariant::Double)
        && (fieldZ.type() == QVariant::Double)))
    {
        MainWindow::instance()->messageBar()->pushMessage(
                    "创建图层",
                    "像控点位矢量图层中\"点号\"字段必须为字符型，且长度不低于10。X、Y、Z字段为double类型。",
                    QgsMessageBar::CRITICAL,
                    MainWindow::instance()->messageTimeout() );
        return false;
    }

    return true;
}
