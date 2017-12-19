#include "eqiinquiredemvalue.h"
#include "eqiProjectionTransformation.h"
#include "qgsmessagelog.h"
#include "qgsrasterlayer.h"
#include "qgsrasterdataprovider.h"
#include "qgsrasteridentifyresult.h"

#include <QDir>

eqiInquireDemValue::eqiInquireDemValue(QObject *parent) : QObject(parent)
{
    mDefaultDemPath = QDir::currentPath() + "/Resources/DEM/";
}

eqiInquireDemValue::~eqiInquireDemValue()
{
    QMap< QString, QgsRasterLayer* >::iterator it = mRasterLayersMap.begin();
    while (it != mRasterLayersMap.end())
    {
        QgsRasterLayer *rasterLayer = it.value();
        if (rasterLayer)
        {
            delete rasterLayer;
            rasterLayer = nullptr;
        }
        ++it;
    }
}

void eqiInquireDemValue::setDemPath( const QString& demPath )
{
    mCustomizeDemPath = demPath;
}

eqiInquireDemValue::ErrorType eqiInquireDemValue::inquireElevation( const QgsPoint& point,
                                                                    qreal& elevation,
                                                                    const QgsCoordinateReferenceSystem* crs /*= nullptr*/ )
{
    QList< QgsPoint > points;
    QList< qreal > elevations;

    points << point;
    elevations << elevation;

    return inquireElevations(points, elevations, crs);
}

eqiInquireDemValue::ErrorType eqiInquireDemValue::inquireElevations( const QList< QgsPoint >& points,
                                                                     QList< qreal >& elevations,
                                                                     const QgsCoordinateReferenceSystem* crs /*= nullptr*/ )
{
    QgsMessageLog::logMessage(QString("自动匹配高程 : \t从默认DEM中匹配可用曝光点高程."));

    // 将点坐标转换到与DEM一致
    QList< QgsPoint > pointAfter;
    eqiInquireDemValue::ErrorType et = pointTransform(points, pointAfter, crs);
    if (et != eqiInquireDemValue::eOK)
        return et;

    // 计算涉及的所有DEM
    if (!involved(pointAfter))
    {
        QgsMessageLog::logMessage("\t\t没有找到相关的DEM数据，该问题极有可能是由于坐标系设置不正确造成.");
        return eqiInquireDemValue::eOther;
    }

    // 加载DEM数据
    loadDem();

    // 搜索高程值
    searchElevationValues(pointAfter, elevations);

    QgsMessageLog::logMessage(QString("\t\t高程匹配完成.\n"));

    return eqiInquireDemValue::eOK;
}

bool eqiInquireDemValue::involved(const QList< QgsPoint >& points)
{
    foreach (QgsPoint point, points)
    {
        mRasterLayersMap[defaultDemName(point)] = nullptr;
    }

    if (mRasterLayersMap.isEmpty())
        return false;
    else
        return true;
}

void eqiInquireDemValue::loadDem()
{
    QMap< QString, QgsRasterLayer* >::iterator it = mRasterLayersMap.begin();
    while (it != mRasterLayersMap.end())
    {
        // 创建栅格图层
        QString path = mDefaultDemPath + it.key();
        QgsRasterLayer* rasterLayer = new QgsRasterLayer(path);
        if (rasterLayer->isValid())
        {
            mRasterLayersMap[it.key()] = rasterLayer;
        }
        else
        {
            QgsMessageLog::logMessage(QString("\t\t加载%1失败, 将使用预设的平均高程计算地面分辨率.")
                                      .arg(QDir::toNativeSeparators(path)));
        }
        ++it;
    }
}

void eqiInquireDemValue::searchElevationValues( const QList< QgsPoint >& points, QList< qreal >& elevations )
{
    foreach (QgsPoint point, points)
    {
        QgsRasterLayer *rasterLayer = mRasterLayersMap.value(defaultDemName(point));
        if (rasterLayer && rasterLayer->isValid())
        {
            // 读取高程值
            if (rasterLayer->bandCount() != 1)
                elevations.append(-9999.0);
            else
            {
                double value = 0.0;
                QgsRasterDataProvider* dataPro= rasterLayer->dataProvider();
                QgsRasterIdentifyResult result = dataPro->identify(point, QgsRaster::IdentifyFormatValue);
                value = result.results()[1].toDouble();
                elevations.append(value);
            }
        }
        else
        {
            elevations.append(-9999.0);
        }
    }
}

eqiInquireDemValue::ErrorType eqiInquireDemValue::pointTransform(const QList< QgsPoint >& pointFirst,
                                                                 QList< QgsPoint >& pointAfter,
                                                                 const QgsCoordinateReferenceSystem* crs)
{
    if (pointFirst.isEmpty())
    {
        QgsMessageLog::logMessage("\t\t传递的曝光点坐标为空, 将使用预设的平均高程计算地面分辨率.");
        return eqiInquireDemValue::eOther;
    }

    QgsCoordinateReferenceSystem sourceCrs;
    QgsCoordinateReferenceSystem mTargetCrs;

    // 设置源参照坐标系
    if (crs && crs->isValid())  // 首先调用输入的参数
        sourceCrs = *crs;
    else // 然后调用“设置”中的参数
    {
        QgsCoordinateReferenceSystem mSourceCrs;
        QString myDefaultCrs = mSettings.value( "/eqi/prjTransform/projectDefaultCrs",
                                                GEO_EPSG_CRS_AUTHID ).toString();
        mSourceCrs.createFromOgcWmsCrs( myDefaultCrs );

        if (mSourceCrs.isValid())
        {
            sourceCrs = mSourceCrs;
        }
        else
        {
            QgsMessageLog::logMessage("\t\t未定义源参照坐标系, 将使用预设的平均高程计算地面分辨率.");
            return eqiInquireDemValue::eOther;
        }
    }

    // 设置目标参照坐标系
    // 检查源参考坐标系是否为地理坐标系
    if (eqiProjectionTransformation::isValidGCS(sourceCrs.authid()))
    {
        // 必须是CGCS2000
        if (sourceCrs.authid()=="EPSG:4490")
        {
            pointAfter = pointFirst;
            return eqiInquireDemValue::eOK;
        }
        else
        {
            QgsMessageLog::logMessage("\t\t不受支持的坐标系（CGCS2000）, 将使用预设的平均高程计算地面分辨率.");
            return eqiInquireDemValue::eNotSupportCrs;
        }
    }
    else if (eqiProjectionTransformation::isSourceCGCS2000Prj(sourceCrs.postgisSrid()))
    {
        mTargetCrs = eqiProjectionTransformation::getGCS(sourceCrs);
        if (!mTargetCrs.isValid())
        {
            QgsMessageLog::logMessage("\t\t不受支持的坐标系, 将使用预设的平均高程计算地面分辨率.");
            return eqiInquireDemValue::eNotSupportCrs;
        }
    }
    else
    {
        QgsMessageLog::logMessage("\t\t不受支持的坐标系, 将使用预设的平均高程计算地面分辨率.");
        return eqiInquireDemValue::eNotSupportCrs;
    }

    eqiProjectionTransformation eqiPrj(sourceCrs, mTargetCrs);
    foreach (QgsPoint point, pointFirst)
    {
        QgsPoint p = eqiPrj.prjTransform(point);
        pointAfter.append(p);
    }

    return eqiInquireDemValue::eOK;
}

QString eqiInquireDemValue::defaultDemName( const QgsPoint &point )
{
    double longitude = point.x();
    double latitude = point.y();
    QString s_longitude;
    QString s_latitude;

    if (longitude < 100)
        s_longitude = "0" + QString::number(static_cast<int>(longitude));
    else
        s_longitude = QString::number(static_cast<int>(longitude));

    s_latitude = QString::number(static_cast<int>(latitude));

    QString str;
    str = "N" +  s_latitude + "E" + s_longitude + "_dem.tif";
    return str;
}
