#ifndef EQICORE_H
#define EQICORE_H

#include <QObject>
#include <QSettings>
#include <QStringList>

#include "qgspoint.h"
#include "qgscoordinatereferencesystem.h"

class eqiProjectionTransformation : public QObject
{
    Q_OBJECT
public:
    explicit eqiProjectionTransformation(QObject *parent = nullptr);

    QgsPoint prjTransform( const QgsPoint p, const QgsCoordinateReferenceSystem& mSourceCrs,
                                             const QgsCoordinateReferenceSystem& mTargetCrs );

private:
    // 创建目标投影参考坐标系
    bool createTargetCrs();

    // 利用系统数据库srs.db中获得的参照坐标系名称填充列表
    bool descriptionForDb(QStringList &list);

    // 利用用户数据库中获得的参照坐标系名称填充列表
    bool descriptionForUserDb(QStringList &list);

    // 输入中央经线，返回PROJ4格式的高斯投影字符串
    QString createProj4Cgcs2000Gcs( const double cm );
    QString createProj4Wgs84Gcs( const double cm );
signals:

public slots:

private:
    QSettings mSettings;

    QStringList descriptionList;
    QStringList descriptionUserList;

    QgsCoordinateReferenceSystem mSourceCrs;
    QgsCoordinateReferenceSystem mTargetCrs;
};

#endif // EQICORE_H
