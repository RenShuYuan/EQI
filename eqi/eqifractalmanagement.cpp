#include "eqifractalmanagement.h"
#include "qgsvectorlayer.h"
#include "qgsmessagelog.h"
#include <QDebug>

eqiFractalManagement::eqiFractalManagement(QObject *parent) : QObject(parent)
{

}

QgsPolygon eqiFractalManagement::dNToLal(const QString dNStr)
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

//    //根据比例尺得到经差与纬差
//    double djc = mJwc.djc/60;
//    double dwc = mJwc.dwc/60;
    double djc = 1.875;
    double dwc = 1.25;

    //计算出西南角的经纬度,十进制表示
    double djd=((b-31)*360+(d-1)*djc)/60;
    double dwd=((a-1)*240+(240/dwc-c)*dwc)/60;

    // 计算四个角点坐标，并生成QgsPolygon
    QgsPolyline polyline;
    QgsPolygon polyon;
    polyline << QgsPoint( djd, dwd )                //西南
             << QgsPoint( djd, dwd+dwc/60 )         //西北
             << QgsPoint( djd+djc/60, dwd+dwc/60 )  //东北
             << QgsPoint( djd+djc/60, dwd );        //东南
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
    sjzToDfm( mP );
    dfmToMM(mP);

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
    {
        th += "0";  //填充0
        th += tmpStr;
    }
    else
        th += tmpStr;

    //比例尺
    th += mJwc.blcAbb;

    //行号及列号
    tmpStr = QString::number(c);
    if (tmpStr.size()==1)
    {
        th += "00";  //填充0
        th += tmpStr;
    }
    else if (tmpStr.size()==2)
    {
        th += "0";  //填充0
        th += tmpStr;
    }
    else
        th += tmpStr;

    tmpStr = QString::number(d);
    if (tmpStr.size()==1)
    {
        th += "00";  //填充0
        th += tmpStr;
    }
    else if (tmpStr.size()==2)
    {
        th += "0";  //填充0
        th += tmpStr;
    }
    else
        th += tmpStr;

    return th;
}

QStringList eqiFractalManagement::rectToTh(const QgsPoint lastPoint, const QgsPoint nextPoint)
{
    QStringList thList;

    // 分开保存最大经度、纬度，与最小经度、纬度
    QgsPoint maxPoint, minPoint;
    maxPoint.set( lastPoint.x()>nextPoint.x() ? lastPoint.x():nextPoint.x(),
                  lastPoint.y()>nextPoint.y() ? lastPoint.y():nextPoint.y());
    minPoint.set( lastPoint.x()<nextPoint.x() ? lastPoint.x():nextPoint.x(),
                  lastPoint.y()<nextPoint.y() ? lastPoint.y():nextPoint.y());

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

void eqiFractalManagement::sjzToDfm(QgsPoint &p)
{
    double L = p.x();
    double B = p.y();
    sjzToDfm(L);
    sjzToDfm(B);
    p.set(L, B);
}

void eqiFractalManagement::sjzToDfm(double &num)
{
    double tmj, tmf, tmm;
    tmj = (int)num;
    tmf = (int)((num-tmj)*60);
    tmm = ((num-tmj)*60-tmf)*60;
    num = tmj + tmf/100 + tmm/10000;
}

void eqiFractalManagement::dfmToMM(QgsPoint &p)
{
    double L = p.x();
    double B = p.y();

    double Lj, Lf, Lm;
    double Bj, Bf, Bm;

    Lj = (int)L;
    Lf = (int)((L-Lj)*100);
    Lm = ((L-Lj)*100-Lf)*100;

    Bj = (int)B;
    Bf = (int)((B-Bj)*100);
    Bm = ((B-Bj)*100-Bf)*100;

    p.setX(Lj*3600+Lf*60+Lm);
    p.setY(Bj*3600+Bf*60+Bm);
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
    {
        mJwc.ranks = 192;
    }
    else if (mBlc == 10000)
    {
        mJwc.ranks = 96;
    }
}
