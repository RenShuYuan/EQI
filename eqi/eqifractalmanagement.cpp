#include "eqifractalmanagement.h"
#include "qgsvectorlayer.h"
#include "qgsmessagelog.h"
#include "mainwindow.h"
#include <QDebug>

eqiFractalManagement::eqiFractalManagement(QObject *parent) : QObject(parent)
{
    // 获得分幅管理设置的坐标系
    QString myDefaultCrs = mSettings.value( "/eqi/prjTransform/projectDefaultCrs", GEO_EPSG_CRS_AUTHID ).toString();
    mSourceCrs.createFromOgcWmsCrs( myDefaultCrs );

    // 设置源坐标系
    eqiPrj.setSourceCrs(mSourceCrs);
    if (!eqiPrj.sourceCrs().isValid())
    {
        MainWindow::instance()->messageBar()->pushMessage( "分幅管理",
                                                           "源参照坐标系定义错误，请重新设置，否则相关结果不正确!",
                                                           QgsMessageBar::CRITICAL,
                                                           MainWindow::instance()->messageTimeout() );
    }

    // 设置目标坐标系
    if (!setGCS())
    {
        MainWindow::instance()->messageBar()->pushMessage( "分幅管理",
                                                           "目前只支持CGCS2000、XIAN 1980、BEIJING 1954，"
                                                           "请重新设置，否则相关结果不正确!",
                                                           QgsMessageBar::CRITICAL,
                                                           MainWindow::instance()->messageTimeout() );
    }
}

QVector<QgsPoint> eqiFractalManagement::dNToLal(const QString dNStr)
{
    int a=0, b=0, c=0, d=0;

    //得到百万图号的纬度
    QByteArray ba = dNStr.toLatin1();
    char ch = ba[0];
    int itmp=0;
    if(islower(ch))
        itmp=97;
    else
        itmp=65;

    for(int i=1;i<22;++i)
    {
        if(ch==itmp){
            a=i;
            break;
        }
        ++itmp;
    }

    //得到百万图号的经度
    QString tempStr = dNStr.mid(1,2);
    b = tempStr.toInt();

    //图幅的行号
    tempStr = dNStr.mid(4,3);
    c = tempStr.toInt();

    //图幅的列号
    tempStr = dNStr.mid(7,3);
    d = tempStr.toInt();

    //根据比例尺得到经差与纬差
    double djc = mJwc.djc/60;
    double dwc = mJwc.dwc/60;

    //计算出西南角的经纬度,十进制表示
    double djd=((b-31)*360+(d-1)*djc)/60;
    double dwd=((a-1)*240+(240/dwc-c)*dwc)/60;

    QVector<QgsPoint> list;
    list << QgsPoint( djd, dwd )                //西南
         << QgsPoint( djd, dwd+dwc/60 )         //西北
         << QgsPoint( djd+djc/60, dwd+dwc/60 )  //东北
         << QgsPoint( djd+djc/60, dwd );        //东南

    return list;
}

QVector<QgsPoint> eqiFractalManagement::dNToXy(const QString dNStr)
{
    QVector<QgsPoint> points = dNToLal(dNStr);

    eqiProjectionTransformation tmpPrj;
    QgsCoordinateReferenceSystem destcrs;

    // 定义源坐标系
    tmpPrj.setSourceCrs(eqiPrj.sourceCrs());

    // 计算图幅中央经线, 并创建对应的投影坐标系
    int dh = eqiProjectionTransformation::getCentralmeridian3(points.at(0).x());
    QString prj4Def = eqiProjectionTransformation::createProj4Cgcs2000Gcs(dh);
    destcrs.createFromProj4(prj4Def);
    tmpPrj.setDestCRS(destcrs);

    QVector<QgsPoint> pointsOut;
    pointsOut << tmpPrj.transform( points.at(0) );
    pointsOut << tmpPrj.transform( points.at(1) );
    pointsOut << tmpPrj.transform( points.at(2) );
    pointsOut << tmpPrj.transform( points.at(3) );

    return pointsOut;
}

QgsPolygon eqiFractalManagement::createTkPolygon(const QVector<QgsPoint> list)
{
    // 计算四个角点坐标，并生成QgsPolygon
    QgsPolyline polyline;
    QgsPolygon polyon;

    if (list.size() != 4)
    {
        return polyon;
    }

    // 检查源坐标系，是否需要坐标转换
    if (eqiPrj.sourceCrs().geographicFlag())
    {
        polyline = list;
        polyon << polyline;
    }
    else
    {
        polyline << eqiPrj.transform( list.at(0), QgsCoordinateTransform::ReverseTransform )    //西南
                 << eqiPrj.transform( list.at(1), QgsCoordinateTransform::ReverseTransform )    //西北
                 << eqiPrj.transform( list.at(2), QgsCoordinateTransform::ReverseTransform )    //东北
                 << eqiPrj.transform( list.at(3), QgsCoordinateTransform::ReverseTransform );   //东南
    }

    polyon << polyline;
    return polyon;
}

QString eqiFractalManagement::pointToTh(const QgsPoint p)
{
    // 保存计算的图号各部分
    int a=0, b=0, c=0, d=0;
    QgsPoint mP = p;

    //计算出1:100万图幅所在的字符对应的数字码
    a = mP.x()/6+31;
    b = mP.y()/4+1;

    //将十进制经纬度转换为度分秒,并全部转换为秒方便计算
    eqiProjectionTransformation::sjzToDfm( mP );
    eqiProjectionTransformation::dfmToMM(mP);

    //计算出1:100万图号后的行、列号
    c=(4*3600)/mJwc.dwc-(int)(((int)mP.y()%(4*3600))/mJwc.dwc);
    d=(int)(((int)mP.x()%(6*3600))/mJwc.djc)+1;

    //=============组合成完整标准图号=============//
    QString th;
    //百万行号
    char ch='A';
    for (int i = 1; i < 22; ++i)
    {
        if( b==i )
        {
            th = ch;
            break;
        }
        else
            ++ch;
    }

    //百万列号
    QString tmpStr = QString::number(a);
    if (tmpStr.size()==1)
        th += tmpStr.prepend("0");
    else
        th += tmpStr;

    //比例尺
    th += mJwc.blcAbb;

    //行号及列号
    tmpStr = QString::number(c);
    fill0(tmpStr);
    th += tmpStr;

    tmpStr = QString::number(d);
    fill0(tmpStr);
    th += tmpStr;

    return th;
}

QStringList eqiFractalManagement::rectToTh(const QgsPoint lastPoint, const QgsPoint nextPoint)
{
    QStringList thList;
    QgsPoint lastPointD;
    QgsPoint nextPointD;

    // 如果是投影坐标系，则转换为经纬度
    if (eqiPrj.sourceCrs().geographicFlag() || (checkLBisCnExtent(lastPoint) && checkLBisCnExtent(nextPoint)))
    {
        lastPointD = lastPoint;
        nextPointD = nextPoint;
    }
    else
    {
        lastPointD = eqiPrj.transform(lastPoint);
        nextPointD = eqiPrj.transform(nextPoint);
    }

    qDebug() << "lastPointD=" << lastPointD.toString() << ", nextPointD=" << nextPointD.toString(); ///

    // 分开保存最大经度、纬度，与最小经度、纬度
    QgsPoint maxPoint, minPoint;
    maxPoint.set( lastPointD.x()>nextPointD.x() ? lastPointD.x():nextPointD.x(),
                  lastPointD.y()>nextPointD.y() ? lastPointD.y():nextPointD.y());
    minPoint.set( lastPointD.x()<nextPointD.x() ? lastPointD.x():nextPointD.x(),
                  lastPointD.y()<nextPointD.y() ? lastPointD.y():nextPointD.y());

    qDebug() << "4.开始生成单幅图框";///
    QString maxTh = pointToTh(maxPoint);
    QString minTh = pointToTh(minPoint);
    qDebug() << "4.结束生成单幅图框" << maxTh << " : " << minTh ;///
    // 如果两幅图一致则返回
    if (maxTh == minTh)
    {
        return thList << maxTh;
    }

    QString minThRow = minTh;
    while (true)
    {
        qDebug() << "4.开始遍历行";///
        // 遍历行
        if ( (maxTh.at(0)!=minTh.at(0)) || (maxTh.mid(4,3)!=minTh.mid(4,3)) )
        {
            thList << minTh;
            qDebug() << minTh;///
            int row = (minTh.mid(4,3)).toInt();
            if (row > 1)
            {
                QString tmpStr = QString::number(--row);
                if (tmpStr.size()==1)
                    minTh.replace(4, 3, "00"+tmpStr);
                else if (tmpStr.size()==2)
                    minTh.replace(4, 3, "0"+tmpStr);
                else
                    minTh.replace(4, 3, tmpStr);
            }
            else
            {
                QByteArray ba = minTh.toLatin1();
                char ch = ba[0];
                minTh.replace(0, 1, ++ch);
                minTh.replace(4, 3, "192");
            }
        }
        else
        {
            qDebug() << "4.开始遍历列";///
            thList << minTh;

            if ( (maxTh.mid(1,2)!=minTh.mid(1,2)) || (maxTh.mid(7,3)!=minTh.mid(7,3)) )
            {
                int column = (minTh.mid(7,3)).toInt();
                if (column < mJwc.ranks)
                {
                    QString tmpStr = QString::number(++column);
                    if (tmpStr.size()==1)
                        minTh.replace(7, 3, "00"+tmpStr);
                    else if (tmpStr.size()==2)
                        minTh.replace(7, 3, "0"+tmpStr);
                    else
                        minTh.replace(7, 3, tmpStr);
                }
                else
                {
                    int iColumn = (minTh.mid(1,2)).toInt();
                    QString tmpStr = QString::number(++iColumn);
                    if (tmpStr.size()==1)
                        minTh.replace(1, 2, "0"+tmpStr);
                    else if (tmpStr.size()==2)
                        minTh.replace(1, 2, tmpStr);
                    minTh.replace(7, 3, "001");
                }

                minTh.replace(0, 1, minThRow.mid(0, 1));
                minTh.replace(4, 3, minThRow.mid(4, 3));
            }
            else
                break;
        }
    }

    return thList;
}

void eqiFractalManagement::setBlc(const int blc)
{
    this->mBlc = blc;   //设置比例尺
    setJwc();           //设置经差与纬差
    setRanks();         //设置行列最大值
    setBlcAbb();        //设置比例尺对应字母
}

int eqiFractalManagement::getScale(const int index)
{
    switch (index) {
    case 0:
        return 5000;  break;
    case 1:
        return 10000;  break;
    case 2:
        return 25000;  break;
    case 3:
        return 50000;  break;
    case 4:
        return 100000;  break;
    case 5:
        return 250000;  break;
    case 6:
        return 500000;  break;
    default:
        return 10000;  break;
    }
}

void eqiFractalManagement::setJwc()
{
    if (mBlc==500000)
    {
        mJwc.djc = 10800;
        mJwc.dwc = 7200;
    }
    else if (mBlc==250000)
    {
        mJwc.djc=5400;
        mJwc.dwc=3600;
    }
    else if (mBlc==100000)
    {
        mJwc.djc=1800;
        mJwc.dwc=1200;
    }
    else if (mBlc==50000)
    {
        mJwc.djc=900;
        mJwc.dwc=600;
    }
    else if (mBlc==25000)
    {
        mJwc.djc=450;
        mJwc.dwc=300;
    }
    else if (mBlc==10000)
    {
        mJwc.djc=225;
        mJwc.dwc=150;
    }
    else if (mBlc==5000)
    {
        mJwc.djc=112.5;
        mJwc.dwc=75.0;
    }
    else
    {
        mJwc.djc=0;
        mJwc.dwc=0;
    }
}

void eqiFractalManagement::setBlcAbb()
{
    if( mBlc==500000 )
        mJwc.blcAbb = 'B';
    else if( mBlc==250000 )
        mJwc.blcAbb = 'C';
    else if( mBlc==100000 )
        mJwc.blcAbb = 'D';
    else if( mBlc==50000 )
        mJwc.blcAbb = 'E';
    else if( mBlc==25000 )
        mJwc.blcAbb = 'F';
    else if( mBlc==10000 )
        mJwc.blcAbb = 'G';
    else if( mBlc==5000 )
        mJwc.blcAbb = 'H';
}

void eqiFractalManagement::setRanks()
{
    if (mBlc == 5000)
        mJwc.ranks = 192;
    else if (mBlc == 10000)
        mJwc.ranks = 96;
    else if (mBlc == 25000)
        mJwc.ranks = 48;
    else if (mBlc == 50000)
        mJwc.ranks = 24;
    else if (mBlc == 100000)
        mJwc.ranks = 12;
    else if (mBlc == 250000)
        mJwc.ranks = 4;
    else if (mBlc == 500000)
        mJwc.ranks = 2;
}

void eqiFractalManagement::fill0(QString &str)
{
    if (str.size()==1)
    {
        str = str.prepend("00");
    }
    else if (str.size()==2)
    {
        str = str.prepend("0");
    }
}

bool eqiFractalManagement::setGCS()
{   
    QgsCoordinateReferenceSystem crs;
    crs = eqiPrj.getGCS(eqiPrj.sourceCrs());

    if ( crs.isValid() )
    {
        eqiPrj.setDestCRS(crs);
        return true;
    }
    else
        return false;
}

bool eqiFractalManagement::checkLBisCnExtent(const QgsPoint &point)
{
    if (point.x()>=73 && point.x()<=136 &&
        point.y()>=3 && point.y()<=54)
    {
        return true;
    }
    else
    {
        return false;
    }
}
