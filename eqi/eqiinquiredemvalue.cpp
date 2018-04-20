#include "eqiinquiredemvalue.h"
#include "eqiProjectionTransformation.h"
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
    // 将点坐标转换到与DEM一致
    QList< QgsPoint > pointAfter;
    eqiInquireDemValue::ErrorType et = pointTransform(points, pointAfter, crs);
    if (et != eqiInquireDemValue::eOK)
        return et;

    // 计算涉及的所有DEM
    if (!involved(pointAfter))
    {
        return eqiInquireDemValue::eOther;
    }

    // 加载DEM数据
    loadDem();

    // 搜索高程值
    searchElevationValues(pointAfter, elevations);

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
        QString myDefaultCrs = mSettings.value( globalCrs, GEO_EPSG_CRS_AUTHID ).toString();
        mSourceCrs.createFromOgcWmsCrs( myDefaultCrs );

        // debug
        qDebug() << "然后调用“设置”中的参数.mSourceCrs=" << mSourceCrs.description();

        if (mSourceCrs.isValid())
        {
            // debug
            qDebug() << "将mSourceCrs=sourceCrs。";

            sourceCrs = mSourceCrs;
        }
        else
        {
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
            return eqiInquireDemValue::eNotSupportCrs;
        }
    }
    else if (eqiProjectionTransformation::isSourceCGCS2000Prj(sourceCrs.postgisSrid()))
    {
        // debug
        qDebug() << "目标坐标系为2000投影坐标系。";

        mTargetCrs = eqiProjectionTransformation::getGCS(sourceCrs);
        if (!mTargetCrs.isValid())
        {
            return eqiInquireDemValue::eNotSupportCrs;
        }
    }
    else
    {
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
