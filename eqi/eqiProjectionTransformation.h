#ifndef EQICORE_H
#define EQICORE_H

#include "head.h"
#include <QObject>

#include "qgspoint.h"
#include "qgscoordinatetransform.h"
#include "qgscoordinatereferencesystem.h"

class eqiProjectionTransformation : public QgsCoordinateTransform
{
    Q_OBJECT
public:
    explicit eqiProjectionTransformation();
    explicit eqiProjectionTransformation(const QgsCoordinateReferenceSystem& theSource,
                                         const QgsCoordinateReferenceSystem& theDest);

    // 投影变换
    QgsPoint prjTransform(const QgsPoint p);

    // 检查是否是有效的地理、投影坐标系（CGCS2000、BJ1954、XIAN1980）
    static bool isValidGCS(const QString &authid);
    static bool isValidPCS(const int postgisSrid);

    // 是否对应投影坐标系
    static bool isSourceCGCS2000Prj(const int postgisSrid);
    static bool isSourceXian1980Prj(const int postgisSrid);
    static bool isSourceBeijing1954Prj(const int postgisSrid);

    // 输入中央经线，返回PROJ4格式的高斯投影字符串
    static QString createProj4Cgcs2000Gcs( const double cm );
    static QString createProj4Xian1980Gcs( const double cm );
    static QString createProj4Beijing1954Gcs( const double cm );

    // 十进制转度
    static void sjzToDfm(QgsPoint &p);
    static void sjzToDfm(double &num);

    // 度分秒转秒
    static void dfmToMM(QgsPoint &p);

    // 输入经度(十进制), 返回带号(3度分带)
    static int getBandwidth3(double djd);

    // 输入经度(十进制), 返回中央经线(3度分带)
    static int getCentralmeridian3(double djd);

    // 创建目标投影参考坐标系
    bool createTargetCrs(const int cm);

    // 返回地理坐标系，必须是支持的3种投影坐标系之一
    static QgsCoordinateReferenceSystem getGCS(const QgsCoordinateReferenceSystem& sourceCrs);

private:
    // 利用系统数据库srs.db中获得的参照坐标系名称填充列表
    bool descriptionForDb(QStringList &list);

    // 利用用户数据库中获得的参照坐标系名称填充列表
    bool descriptionForUserDb(QStringList &list);

    int getPCSauthid_2000(const int cm, const int bw);
    int getPCSauthid_1980();
    int getPCSauthid_1954();
signals:

public slots:

private:
    QSettings mSettings;

    QStringList descriptionList;
    QStringList descriptionUserList;
};

#endif // EQICORE_H
