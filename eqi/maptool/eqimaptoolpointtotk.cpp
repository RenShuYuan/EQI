#include "mainwindow.h"
#include "qgis/app/qgsmaptoolselectutils.h"
#include "eqi/eqifractalmanagement.h"
#include "eqimaptoolpointtotk.h"

#include "qgsmessagelog.h"
#include "qgsvectorlayer.h"
#include "qgsvectordataprovider.h"
#include "qgsmaplayerregistry.h"

eqiMapToolPointToTk::eqiMapToolPointToTk(QgsMapCanvas *canvas)
        : QgsMapTool( canvas )
{
    mToolName = "pointToTk";
    count = 0;
    fisrtMap = true;
}

void eqiMapToolPointToTk::canvasReleaseEvent(QgsMapMouseEvent *e)
{
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

            eqiFractalManagement fm;
            fm.setBlc(5000);
            QStringList thList = fm.rectToTh(lastPoint, nextPoint);

            // 是否需要创建矢量图层
            if (fisrtMap)
            {
                MainWindow::instance()->mapCanvas()->freeze();

                QString layerProperties = "Polygon?";													// 几何类型
                layerProperties.append(QString( "field=标准图号:string(20)" ));                         // 添加字段
                layerProperties.append(QString( "index=yes&" ));										// 创建索引
                layerProperties.append(QString( "memoryid=%1" ).arg( QUuid::createUuid().toString() ));	// 临时编码

                newLayer = new QgsVectorLayer(layerProperties, "标准分幅略图", QString( "memory" ) );

                if (!newLayer->isValid())
                {
                    MainWindow::instance()->messageBar()->pushMessage( "创建分幅略图",
                        "创建略图失败, 运行已终止, 注意检查plugins文件夹!",
                        QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
                    QgsMessageLog::logMessage(QString("创建分幅略图 : \t创建略图失败, 程序已终止, 注意检查plugins文件夹。"));
                    return;
                }

                // 添加到地图
                QgsMapLayerRegistry::instance()->addMapLayer(newLayer);
                dateProvider = newLayer->dataProvider();
                fisrtMap = false;
            }

            // 生成图幅要素
            QgsFeatureList featureList;
            foreach (QString th, thList)
            {
                QgsMessageLog::logMessage(th);

                QgsPolygon polygon = fm.dNToLal(th);
                QgsGeometry* mGeometry = QgsGeometry::fromPolygon(polygon);

                // 设置几何要素与属性
                QgsFeature MyFeature;
                MyFeature.setGeometry( mGeometry );
                MyFeature.setAttributes(QgsAttributes() << QVariant(th));
                featureList.append(MyFeature);
            }

            // 开始编辑
            newLayer->startEditing();

            // 添加要素集到图层中
            dateProvider->addFeatures(featureList);

            // 保存
            newLayer->commitChanges();

            // 更新范围
            newLayer->updateExtents();

            MainWindow::instance()->mapCanvas()->freeze( false );
            MainWindow::instance()->refreshMapCanvas();

            count = 0;
        }
    }

}
