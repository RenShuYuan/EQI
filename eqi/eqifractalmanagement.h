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

    //根据图号计算出四角的经纬度, 并返回一个QgsPolygon对象: 西南0,1;西北2,3;东北4,5;东南6,7
    QgsPolygon dNToLal(const QString dNStr);

    // 根据一点生成图号
    QString pointToTh( const QgsPoint p );

    // 生成两地范围内所有图号
    QStringList rectToTh(const QgsPoint lastPoint, const QgsPoint nextPoint);

    // 设置分幅比例尺, 并同时设置对应的图幅经差与纬差
    void setBlc(const int mBlc);

    // 十进制转度
    void sjzToDfm(QgsPoint &p);
    void sjzToDfm(double &num);

    // 度分秒转秒
    void dfmToMM(QgsPoint &p);
signals:

public slots:

private:
    // 设置标准分幅经差与纬差,单位为秒
    void setJwc();

    // 设置比例尺对应字母
    void setBlcAbb();

    // 设置行列最大值
    void setRanks();

private:
    QSettings mSettings;

    int mBlc;           // 分幅比例尺
    struct jwc mJwc;    // 图幅经差与纬差
};

#endif // EQIFRACTALMANAGEMENT_H
