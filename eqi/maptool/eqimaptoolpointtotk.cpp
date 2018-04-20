#include "mainwindow.h"
#include "qgis/app/qgsmaptoolselectutils.h"
#include "eqi/eqifractalmanagement.h"
#include "eqimaptoolpointtotk.h"

#include "qgsgeometry.h"
#include "qgsrubberband.h"
#include "qgsvectordataprovider.h"
#include <QMessageBox>

eqiMapToolPointToTk::eqiMapToolPointToTk(QgsMapCanvas *canvas)
        : QgsMapTool( canvas )
{
    mToolName = "pointToTk";
    count = 0;
    isAvailable = true;

    mRubberBand = nullptr;
    mCursor = Qt::ArrowCursor;
    mFillColor = QColor( 254, 178, 76, 63 );
    mBorderColour = QColor( 254, 58, 29, 100 );

    newLayer = getCurrentVectorLayer(canvas);
    if (!(newLayer && newLayer->isValid()))
    {
        QMessageBox::StandardButton sb = QMessageBox::information(
                    NULL, "创建图层",
                    "当前图层无法用于保存分幅图框，\n是否需要自动创建一个临时图层？\n或者取消后自行选择。",
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if ( sb == QMessageBox::Yes )
        {
            newLayer = MainWindow::instance()->createrMemoryMap("标准分幅略图",
                                                                "Polygon",
                                                                QStringList() << "field=TH:string(10)");
            if (!(newLayer && newLayer->isValid()))
            {
                MainWindow::instance()->messageBar()->pushMessage(
                            "创建图层",
                            "未知原因，图层创建失败，该操作已取消。",
                            QgsMessageBar::CRITICAL,
                            MainWindow::instance()->messageTimeout() );

                isAvailable = false;
                return;
            }
        }
        else
        {
            MainWindow::instance()->messageBar()->pushMessage(
                        "坐标生图框",
                        "由于没有合适的矢量图层，该操作已取消。",
                        QgsMessageBar::CRITICAL,
                        MainWindow::instance()->messageTimeout() );

            // 注销地图工具
            QgsMapTool *lastMapTool = canvas->mapTool();
            canvas->unsetMapTool( lastMapTool );

            isAvailable = false;
            return;
        }
    }

    mCursor = QCursor( Qt::CrossCursor );
}

eqiMapToolPointToTk::~eqiMapToolPointToTk()
{
    if (mRubberBand)
    {
        mRubberBand->reset( QGis::Polygon );
        delete mRubberBand;
        mRubberBand = nullptr;
    }
}

void eqiMapToolPointToTk::canvasReleaseEvent(QgsMapMouseEvent *e)
{
    if (e->button()==Qt::RightButton || !isAvailable)
    {
        if (mRubberBand)
        {
            mRubberBand->reset( QGis::Polygon );
            delete mRubberBand;
            mRubberBand = nullptr;
        }

        // 注销地图工具
        QgsMapTool *lastMapTool = canvas()->mapTool();
        canvas()->unsetMapTool( lastMapTool );
        return;
    }

    if (e->button()==Qt::LeftButton)
    {
        ++count;

        if (count==1)
        {
            lastPoint = toMapCoordinates( e->pos() );
        }
        else if (count==2)
        {
            nextPoint = toMapCoordinates( e->pos() );

            eqiFractalManagement fm(this);

            // 设置比例尺
            int scaleIndex = mSettings.value( "/FractalManagement/scale", 1).toInt();
            fm.setBlc( fm.getScale(scaleIndex) );

            QStringList thList = fm.rectToTh(lastPoint, nextPoint);
            if (thList.isEmpty())
            {
                mRubberBand->reset( QGis::Polygon );
                delete mRubberBand;
                mRubberBand = nullptr;
                return;
            }

            // 生成图幅要素
            QgsFeatureList featureList;
            foreach (QString th, thList)
            {
                QgsMessageLog::logMessage(th);

                QVector< QgsPoint > points = fm.dNToLal(th);
                QgsPolygon polygon = fm.createTkPolygon(points);
                QgsGeometry* mGeometry = QgsGeometry::fromPolygon(polygon);

                // 设置几何要素与属性
                QgsFeature MyFeature;
                MyFeature.setGeometry( mGeometry );
                MyFeature.setAttributes(QgsAttributes() << QVariant(th));
                featureList.append(MyFeature);
            }

            MainWindow::instance()->mapCanvas()->freeze();

            // 添加要素集到图层中
            newLayer->dataProvider()->addFeatures(featureList);

            // 更新范围
            newLayer->updateExtents();

            MainWindow::instance()->mapCanvas()->freeze( false );
            MainWindow::instance()->refreshMapCanvas();

            count = 0;
        }

        // 绘制橡皮筋
        if ( !mRubberBand )
        {
          mRubberBand = new QgsRubberBand( mCanvas, QGis::Polygon );
          mRubberBand->setFillColor( mFillColor );
          mRubberBand->setBorderColor( mBorderColour );
        }
        if ( count ==1 )
        {
          mRubberBand->addPoint( toMapCoordinates( e->pos() ) );
        }
        else
        {
          mRubberBand->reset( QGis::Polygon );
          delete mRubberBand;
          mRubberBand = nullptr;
        }
    }
}

QgsVectorLayer *eqiMapToolPointToTk::getCurrentVectorLayer(QgsMapCanvas *canvas)
{
    QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer *>( canvas->currentLayer() );
    if ( !vlayer )
    {
        MainWindow::instance()->messageBar()->pushMessage(
                    "创建图层",
                    "需要选择一个有效的矢量图层保存图框。",
                    QgsMessageBar::CRITICAL,
                    MainWindow::instance()->messageTimeout() );
        return nullptr;
    }

    // 检查图层矢量类型是否正确
   if (QGis::Polygon != vlayer->geometryType())
   {
       MainWindow::instance()->messageBar()->pushMessage(
                   "创建图层",
                   QString("当前图层%1矢量类型必须为\"面\"。").arg(vlayer->name()),
                   QgsMessageBar::CRITICAL,
                   MainWindow::instance()->messageTimeout() );
       return nullptr;
   }

   // 检查是否有TH字段
    QgsFields fields = vlayer->pendingFields();
    int filedIndex = fields.indexFromName(ThFieldName);
    if (filedIndex == -1)
    {
        MainWindow::instance()->messageBar()->pushMessage(
                    "创建图层",
                    "当前图层中未找到\"TH\"字段。",
                    QgsMessageBar::CRITICAL,
                    MainWindow::instance()->messageTimeout() );
        return nullptr;
    }

    // 检查字段类型和长度
    QgsField field = fields.field(filedIndex);
    if (!(field.type() == QVariant::String &&
        field.length() >= 10))
    {
        MainWindow::instance()->messageBar()->pushMessage(
                    "创建图层",
                    "当前图层中\"TH\"字段必须为字符型，且长度不低于10。",
                    QgsMessageBar::CRITICAL,
                    MainWindow::instance()->messageTimeout() );
        return nullptr;
    }

    return vlayer;
}

QList<QgsPoint> eqiMapToolPointToTk::getRect(const QgsPoint &lastPoint, const QgsPoint &nextPoint)
{
    QList<QgsPoint> ptLs;
    QgsPoint pt;

    pt.set( lastPoint.x()<nextPoint.x() ? lastPoint.x():nextPoint.x(),
            lastPoint.y()<nextPoint.y() ? lastPoint.y():nextPoint.y());
    ptLs.append(pt);

    pt.set( lastPoint.x()<nextPoint.x() ? lastPoint.x():nextPoint.x(),
            lastPoint.y()>nextPoint.y() ? lastPoint.y():nextPoint.y());
    ptLs.append(pt);

    pt.set( lastPoint.x()>nextPoint.x() ? lastPoint.x():nextPoint.x(),
            lastPoint.y()>nextPoint.y() ? lastPoint.y():nextPoint.y());
    ptLs.append(pt);

    pt.set( lastPoint.x()>nextPoint.x() ? lastPoint.x():nextPoint.x(),
            lastPoint.y()<nextPoint.y() ? lastPoint.y():nextPoint.y());
    ptLs.append(pt);

    return ptLs;
}

void eqiMapToolPointToTk::canvasMoveEvent(QgsMapMouseEvent *e)
{
    if ( !mRubberBand )
      return;

    mRubberBand->reset( QGis::Polygon );
    QList<QgsPoint> ptLs = getRect(lastPoint, toMapCoordinates( e->pos() ));
    mRubberBand->addPoint(ptLs.at(0));
    mRubberBand->addPoint(ptLs.at(1));
    mRubberBand->addPoint(ptLs.at(2));
    mRubberBand->addPoint(ptLs.at(3));
}
