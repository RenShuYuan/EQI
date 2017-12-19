#ifndef EQIFRACTALMANAGEMENT_H
#define EQIFRACTALMANAGEMENT_H

#include <QObject>
#include <QString>
#include <QSettings>
#include <QMouseEvent>
#include "qgspoint.h"
#include "qgsmaptool.h"
#include "qgsmapcanvas.h"
#include "qgsgeometry.h"
#include "eqiProjectionTransformation.h"
#include "qgscoordinatereferencesystem.h"
#include <QVector>

struct jwc{
    double djc;
    double dwc;
    int ranks;
    QChar blcAbb;
};

class eqiFractalManagement : public QObject
{
    Q_OBJECT
public:
    explicit eqiFractalManagement(QObject *parent = nullptr);

    // 根据图号计算出四角的经纬度, 西南0,1;西北2,3;东北4,5;东南6,7
    QVector< QgsPoint > dNToLal(const QString dNStr);

    // 根据图号计算出四角的投影坐标, 西南0,1;西北2,3;东北4,5;东南6,7
    QVector< QgsPoint > dNToXy(const QString dNStr);

    // 输入四个角点坐标，生成矢量面
    QgsPolygon createTkPolygon(const QVector< QgsPoint > list);

    // 根据一点生成图号
    QString pointToTh( const QgsPoint p );

    // 生成两地范围内所有图号
    QStringList rectToTh(const QgsPoint lastPoint, const QgsPoint nextPoint);

    // 设置分幅比例尺, 并同时设置对应的图幅经差与纬差
    void setBlc(const int mBlc);

    // 根据设置对话框的索引号返回比例尺
    int getScale(const int index);

signals:

public slots:

private:
    // 设置标准分幅经差与纬差,单位为秒
    void setJwc();

    // 设置比例尺对应字母
    void setBlcAbb();

    // 设置行列最大值
    void setRanks();

    // 将行列号不够三位的填充"0"
    void fill0( QString& str );

    // 根据定义的投影坐标系设置对应
    // 的地理坐标系统，目前只支持
    // CGCS2000、XIAN 1980、BEIJING 1954
    bool setGCS();

    // 检查经纬度坐标是否在中国范围内
    bool checkLBisCnExtent(const QgsPoint& point);

private:
    QSettings mSettings;
    eqiProjectionTransformation eqiPrj;

    QgsCoordinateReferenceSystem mSourceCrs;

    int mBlc;           // 分幅比例尺
    struct jwc mJwc;    // 图幅经差与纬差
};

#endif // EQIFRACTALMANAGEMENT_H
