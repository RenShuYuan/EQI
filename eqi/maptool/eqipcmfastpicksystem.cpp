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

        /******************************
        // 读取保存的上一次路径
        QString strListPath = mSettings.value( "/eqi/pos/pathName", QDir::homePath()).toString();

        //浏览文件夹
        QString dir = QFileDialog::getExistingDirectory(nullptr, "选择文件夹", strListPath,
                                                        QFileDialog::ShowDirsOnly |
                                                        QFileDialog::DontResolveSymlinks);
        if (dir.isEmpty()) return;
        mSettings.setValue("/eqi/pos/pathName", dir);

        // 创建txt保存XYZ坐标
        QString path = dir + "/control.txt";
        QFile file(path);
        if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate))   //只写、文本、重写
        {
            MainWindow::instance()->messageBar()->pushMessage( "导出控制点成果表",
                QString("创建%1文件失败.").arg(QDir::toNativeSeparators(path)),
                QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
            QgsMessageLog::logMessage(QString("导出控制点成果表 : \t创建%1文件失败.").arg(QDir::toNativeSeparators(path)));
            return;
        }

        QTextStream out(&file);
        QMap<int, QgsPoint>::const_iterator it = mapXY.constBegin();
        while (it != mapXY.constEnd())
        {
            QString record;
            record = QString("%1\t%2\t%3\t%4\n")
                    .arg(it.key())
                    .arg(it.value().x(), 0, 'f', 3)
                    .arg(it.value().y(), 0, 'f', 3)
                    .arg(mapZ.value(it.key()), 0, 'f', 3);
            out << record;
            ++it;
        }
        file.close();
        ********************************/
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
