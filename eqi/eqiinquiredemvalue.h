#ifndef EQIINQUIREDEMVALUE_H
#define EQIINQUIREDEMVALUE_H

#include "head.h"
#include <QObject>
#include <QList>
#include <QMap>
#include "qgscoordinatereferencesystem.h"
#include "qgspoint.h"

class QString;
class QgsRasterLayer;
class QgsCoordinateReferenceSystem;

class eqiInquireDemValue : public QObject
{
    Q_OBJECT
public:
    enum ErrorType
    {
        eOK,						// 有效
        eInvalidDefaultDemPath,		// 无效的默认DEM路径
        eNotSupportCrs,				// 不支持的源参考坐标系
        eTransformFailed,			// 创建坐标转换关系失败
        eOther,						// 其他错误
    };

    explicit eqiInquireDemValue(QObject *parent = nullptr);
    ~eqiInquireDemValue();

    /**
    * @brief                    显示指定DEM文件路径
    * @author                   YuanLong
    * @param demPath			传递DEM文件完整路径
    * @warning					如果传递的是一个有效DEM路径，则会基于该DEM查询高程信息，
    *							否则调用默认DEM数据。该函数只有在需要使用自己的DEM数据
    *							时才调用。
    * @return
    */
    void setDemPath(const QString& demPath);

    /**
    * @brief                    查询单个坐标或坐标集对应的高程信息
    * @author                   YuanLong
    * @param point				平面坐标（坐标集）
    * @param elevation			保存查询的高程信息
    * @param crs				传递参考坐标系
    * @warning					如果没有指定crs，则默认调用setTargetCrs()设置
    *							的crs，以上参数都未设置则会发送错误信息。
    *							该函数内部将会调用一系列函数来完成高程查询工作。
    * @return
    */
    eqiInquireDemValue::ErrorType inquireElevation(const QgsPoint& point,
                                                   qreal& elevation,
                                                   const QgsCoordinateReferenceSystem* crs = nullptr);
    eqiInquireDemValue::ErrorType inquireElevations(const QList< QgsPoint >& points,
                                                    QList< qreal >& elevations,
                                                    const QgsCoordinateReferenceSystem* crs = nullptr);

private:
    /**
    * @brief                    将点坐标转换到与DEM相同坐标系
    * @author                   YuanLong
    * @param pointFirst		转换前的平面坐标（坐标集）
    * @param pointAfter		转换后的平面坐标（坐标集）
    * @param crs				传递转换前参考坐标系
    * @return
    */
    eqiInquireDemValue::ErrorType pointTransform(const QList< QgsPoint >& pointFirst,
                                                 QList< QgsPoint >& pointAfter,
                                                 const QgsCoordinateReferenceSystem* crs);

    /**
    * @brief                    根据坐标计算所涉及的DEM范围
    * @author                   YuanLong
    * @param points				坐标集
    * @return					如果找到有效DEM则返回true
    */
    bool involved(const QList< QgsPoint >& points);

    /**
    * @brief                    加载所涉及的DEM数据
    * @author                   YuanLong
    * @warning					利用involved()计算出的DEM范围，从默认路径（或setDemPath()）
    *							下加载DEM，并保存其指针。
    * @return
    */
    void loadDem();

    /**
    * @brief                    搜索高程值
    * @author                   YuanLong
    * @param point				平面坐标
    * @warning					该函数是在crs、DEM数据都准备好的情况下调用，
    *							将返回坐标对应到DEM上的高程值。
    * @return
    */
    void searchElevationValues(const QList< QgsPoint >& points, QList< qreal >& elevations);

    /**
    * @brief                    返回默认DEM文件名称
    * @author                   YuanLong
    * @param point				平面坐标
    * @return					返回通过组合固定格式获得DEM文件名称
    */
    QString defaultDemName(const QgsPoint &point);

private:
    QList< QgsPoint > mPoints;
    QSettings mSettings;
    QString mDefaultDemPath;
    QString mCustomizeDemPath;
    QMap< QString, QgsRasterLayer* > mRasterLayersMap;
};

#endif // EQIINQUIREDEMVALUE_H
