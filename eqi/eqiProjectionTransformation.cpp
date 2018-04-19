#include <QDir>
#include <QFileInfo>

#include "mainwindow.h"
#include "eqiProjectionTransformation.h"
#include "qgscoordinatetransform.h"
#include "qgsmessagebar.h"
#include "sqlite3.h"

eqiProjectionTransformation::eqiProjectionTransformation()
    : QgsCoordinateTransform()
{

}

eqiProjectionTransformation::eqiProjectionTransformation(
        const QgsCoordinateReferenceSystem& theSource,
        const QgsCoordinateReferenceSystem& theDest) :
        QgsCoordinateTransform(theSource, theDest)
{

}

bool eqiProjectionTransformation::isValidGCS(const QString &authid)
{
    if ((authid == "EPSG:4490") ||   // CGCS2000
        (authid == "EPSG:4214") ||   // beijing1954
        (authid == "EPSG:4610"))    // xian1980
    {
       return true;
    }
    return false;
}

bool eqiProjectionTransformation::isValidPCS(const int postgisSrid)
{
    if (!(isSourceBeijing1954Prj(postgisSrid) ||
          isSourceCGCS2000Prj(postgisSrid) ||
          isSourceXian1980Prj(postgisSrid)))
    {
        return false;
    }
    else
    {
        return true;
    }
}

QgsPoint eqiProjectionTransformation::prjTransform(const QgsPoint p)
{
    // 创建目标投影
    if (!sourceCrs().isValid() || !destCRS().isValid())
    {
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                           "投影设置不正确, 运行已终止!",
                                                           QgsMessageBar::CRITICAL,
                                                           MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage("投影变换 : \t投影设置不正确, 运行已终止.");
        return QgsPoint();
    }

    // 创建转换关系
    QgsCoordinateTransform ct(sourceCrs(), destCRS());
    if (!ct.isInitialised())
    {
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                           "创建坐标转换关系失败, 运行已终止!",
                                                           QgsMessageBar::CRITICAL,
                                                           MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage("投影变换 : \t创建坐标转换关系失败, 运行已终止.");
        return QgsPoint();
    }

    // 坐标转换
    QgsPoint point = ct.transform(p);
    return point;
}

bool eqiProjectionTransformation::isSourceCGCS2000Prj(const int postgisSrid)
{
    if (postgisSrid > 4491 && postgisSrid < 4554)
    {
        return true;
    }
    return false;
}

bool eqiProjectionTransformation::isSourceXian1980Prj(const int postgisSrid)
{
    if (postgisSrid > 2327 && postgisSrid < 2390)
    {
        return true;
    }
    return false;
}

bool eqiProjectionTransformation::isSourceBeijing1954Prj(const int postgisSrid)
{
    if (postgisSrid > 2401 && postgisSrid < 2442)
    {
        return true;
    }
    return false;
}

void eqiProjectionTransformation::sjzToDfm(QgsPoint &p)
{
    double L = p.x();
    double B = p.y();
    sjzToDfm(L);
    sjzToDfm(B);
    p.set(L, B);
}

void eqiProjectionTransformation::sjzToDfm(double &num)
{
    double tmj, tmf, tmm;
    tmj = (int)num;
    tmf = (int)((num-tmj)*60);
    tmm = ((num-tmj)*60-tmf)*60;
    num = tmj + tmf/100 + tmm/10000;
}

void eqiProjectionTransformation::dfmToMM(QgsPoint &p)
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

int eqiProjectionTransformation::getBandwidth3(double djd)
{
    int itmp=(djd-1.5)/3+1;
    return itmp;
}

int eqiProjectionTransformation::getCentralmeridian3(double djd)
{
    int itmp=(djd-1.5)/3+1;
    return itmp*3;
}

bool eqiProjectionTransformation::createTargetCrs(const int cm)
{
    // 验证源参照坐标系
    if (!sourceCrs().isValid())
    {
        return false;
    }

    // 检查输入的是否是常用的地理坐标系
    if (!isValidGCS(sourceCrs().authid()))
    {
        MainWindow::instance()->messageBar()->pushMessage( "定义投影坐标系",
                                                           "项目指定了错误的地理坐标系, "
                                                           "目前仅支持CGCS2000、beijing1954、xian1980, 运行已终止!",
                                                           QgsMessageBar::CRITICAL,
                                                           MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage("定义投影坐标系 : \t项目指定了错误的地理坐标系,"
                                  "目前仅支持CGCS2000、beijing1954、xian1980, 运行已终止!");
        return false;
    }

    // 检查经度是否在正常范围内
    if ( !((cm>=75 && cm<=135) || (cm>=13 && cm<=45)) )
    {
        MainWindow::instance()->messageBar()->pushMessage( "定义投影坐标系",
                                                           "中央经线不在中国范围内, 运行已终止!",
                                                           QgsMessageBar::CRITICAL,
                                                           MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("定义投影坐标系 : \t中央经线 %1 不在中国范围内, 运行已终止!").arg(cm));
        return false;
    }

    // 需要考虑不同坐标系和6度分带----------------------------------------------------------------------
    QgsCoordinateReferenceSystem mTargetCrs;

    if (sourceCrs().authid() == "EPSG:4490")
    {
        mTargetCrs.createFromId(getPCSauthid_2000(cm, 3));
    }

    if ( !mTargetCrs.isValid() )
    {
        MainWindow::instance()->messageBar()->pushMessage( "定义投影坐标系",
                                                           QString("定义投影参考坐标系 %1 失败, 运行已终止!")
                                                           .arg(mTargetCrs.description()),
                                                           QgsMessageBar::CRITICAL,
                                                           MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("定义投影坐标系 : \t定义投影参考坐标系 %1 失败, 运行已终止!")
                                  .arg(mTargetCrs.description()));
        return false;
    }

    setDestCRS(mTargetCrs);
    QgsMessageLog::logMessage(QString("定义投影坐标系 : \t定义投影参考坐标系 %1 成功.").arg(mTargetCrs.description()));

    return true;
}

QgsCoordinateReferenceSystem eqiProjectionTransformation::getGCS(const QgsCoordinateReferenceSystem &sourceCrs)
{
    QgsCoordinateReferenceSystem crs;

    // 判断源参照坐标系是否是支持的地理坐标系
    int postgisSrid = sourceCrs.postgisSrid();
    if ( isValidPCS(postgisSrid) )
    {
        int iEPSG = 0;
        // 判断对应的是哪个投影坐标系
        if (isSourceBeijing1954Prj(postgisSrid))
            iEPSG = 4214;
        else if (isSourceXian1980Prj(postgisSrid))
            iEPSG = 4610;
        else if (isSourceCGCS2000Prj(postgisSrid))
            iEPSG = 4490;

        crs.createFromId(iEPSG);
    }
    else if ( isValidGCS(sourceCrs.authid()) )
    {
        return sourceCrs;
    }
    return crs;
}

bool eqiProjectionTransformation::descriptionForDb(QStringList &list)
{
    sqlite3      *myDatabase;
    const char   *myTail;
    sqlite3_stmt *myPreparedStatement;
    int           myResult;

    QString databaseFileName = QDir::currentPath() + "/Resources/srs.db";
    if ( !QFileInfo( databaseFileName ).exists() )
    {
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                        QString("没有找到srs.db数据库文件, 运行已终止!"),
                                                        QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("投影变换 : \t没有找到srs.db数据库文件, 运行已终止!"));
        return false;
    }

    // 检查数据库是否可用
    myResult = sqlite3_open_v2(databaseFileName.toUtf8().data(), &myDatabase, SQLITE_OPEN_READONLY, nullptr);
    if ( myResult )
    {
        QString errInfo = QString( "不能打开数据库: %1" ).arg( sqlite3_errmsg( myDatabase ) );
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                        QString("%1, 运行已终止!").arg(errInfo),
                                                        QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("投影变换 : \t%1, 运行已终止!").arg(errInfo));
        return false;
    }

    // 设置查询检索列表所需的投影信息
    QString mySql = "select description, srs_id, upper(auth_name||':'||auth_id), "
                    "is_geo, name, parameters, deprecated from vw_srs where 1 order by name,description";
    myResult = sqlite3_prepare( myDatabase, mySql.toUtf8(), mySql.toUtf8().length(), &myPreparedStatement, &myTail );

    if ( myResult == SQLITE_OK )
    {
        while ( sqlite3_step( myPreparedStatement ) == SQLITE_ROW )
        {
            QString strDescription = ( const char * )sqlite3_column_text( myPreparedStatement, 0 );
            list.append(strDescription);
        }
    }
    else
    {
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                        QString("查询系统数据库, 检索所需的投影信息失败, 运行已终止!"),
                                                        QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("投影变换 : \t查询系统数据库, 检索所需的投影信息失败, 运行已终止!"));
        sqlite3_finalize( myPreparedStatement );
        sqlite3_close( myDatabase );
        return false;
    }

    sqlite3_finalize( myPreparedStatement );
    sqlite3_close( myDatabase );
    return true;
}

bool eqiProjectionTransformation::descriptionForUserDb(QStringList &list)
{
    QString databaseFileName = QDir::currentPath() + "/Resources/qgis.db";
    if ( !QFileInfo( databaseFileName ).exists() )
    {
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                        "没有找到qgis.db数据库文件, 运行已终止!",
                                                        QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("投影变换 : \t没有找到qgis.db数据库文件, 运行已终止!"));
        return false;
    }

    sqlite3      *database;
    const char   *tail;
    sqlite3_stmt *stmt;
    // 检查数据库是否可用
    int result = sqlite3_open_v2( databaseFileName.toUtf8().constData(), &database, SQLITE_OPEN_READONLY, nullptr );
    if ( result )
    {
        QString errInfo = QString( "不能打开数据库: %1" ).arg( sqlite3_errmsg( database ) );
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                        QString("%1, 运行已终止!").arg(errInfo),
                                                        QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("投影变换 : \t%1, 运行已终止!").arg(errInfo));
        return false;
    }

    // 设置查询以检索来填充列表所需的投影信息
    QString sql = QString( "select description, srs_id from vw_srs where 1" );

    result = sqlite3_prepare( database, sql.toUtf8(), sql.toUtf8().length(), &stmt, &tail );

    if ( result == SQLITE_OK )
    {
        while ( sqlite3_step( stmt ) == SQLITE_ROW )
        {
            QString strDescription = ( const char * )sqlite3_column_text( stmt, 0 );
            list.append(strDescription);
        }
    }
    else
    {
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                        QString("查询系统数据库, 检索所需的投影信息失败, 运行已终止!"),
                                                        QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("投影变换 : \t查询系统数据库, 检索所需的投影信息失败, 运行已终止!"));
        sqlite3_finalize( stmt );
        sqlite3_close( database );
        return false;
    }

    sqlite3_finalize( stmt );
    sqlite3_close( database );
    return true;
}

int eqiProjectionTransformation::getPCSauthid_2000(const int cm, const int bw)
{
    int id = 0;
    if (bw==3)
    {
        if (cm >= 75 && cm <= 135)
        {
            id = 4534;
            int cmJz = 75;
            int cmIn = getCentralmeridian3(cm);
            while (cmJz != cmIn)
            {
                cmJz += 3;
                ++id;
            }
        }
        else if (cm >= 25 && cm <= 45)
        {
            id = 4513;
            int cmJz = 25;
            int cmIn = getBandwidth3(cm);
            while (cmJz != cmIn)
            {
                ++cmJz;
                ++id;
            }
        }
    }
    else if (bw==6)
    {

    }
    return id;
}

QString eqiProjectionTransformation::createProj4Cgcs2000Gcs(const double cm)
{
    QString pszCGCS;

    //3度分带，不加带号
    if (cm>74 && cm<136)
    {
        pszCGCS = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 "
                          "+x_0=500000 +y_0=0 +ellps=GRS80 +units=m +no_defs").arg(cm);
    }
    //3度分带，加带号
    if (cm>24 && cm<46)
    {
        pszCGCS = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 "
                          "+x_0=%2 +y_0=0 +ellps=GRS80 +units=m +no_defs").arg(cm*3).arg(cm*1000000+500000);
    }
    //6度分带，加带号
    if (cm>12 && cm<24)
    {
        pszCGCS = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 "
                          "+x_0=%2 +y_0=0 +ellps=GRS80 +units=m +no_defs").arg(cm*6-3).arg(cm*1000000+500000);
    }

    return pszCGCS;
}

QString eqiProjectionTransformation::createProj4Xian1980Gcs(const double cm)
{
    QString pszCGCS;

    //3度分带，不加带号
    if (cm>74 && cm<136)
    {
        pszCGCS = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 +x_0=500000 +y_0=0 "
                          "+a=6378140 +b=6356755.288157528 +units=m +no_defs").arg(cm);
    }
    //3度分带，加带号
    if (cm>24 && cm<46)
    {
        pszCGCS = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 +x_0=%2 +y_0=0 "
                          "+a=6378140 +b=6356755.288157528 +units=m +no_defs").arg(cm*3).arg(cm*1000000+500000);
    }
    //6度分带，加带号
    if (cm>12 && cm<24)
    {
        pszCGCS = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 +x_0=%2 +y_0=0 "
                          "+a=6378140 +b=6356755.288157528 +units=m +no_defs").arg(cm*6-3).arg(cm*1000000+500000);
    }

    return pszCGCS;
}

QString eqiProjectionTransformation::createProj4Beijing1954Gcs(const double cm)
{
    QString pszCGCS;

    //3度分带，不加带号
    if (cm>74 && cm<136)
    {
        pszCGCS = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 "
                          "+x_0=500000 +y_0=0 +ellps=krass +towgs84=15.8,-154.4,-82.3,0,0,0,0 "
                          "+units=m +no_defs").arg(cm);
    }
    //3度分带，加带号
    if (cm>24 && cm<46)
    {
        pszCGCS = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 "
                          "+x_0=%2 +y_0=0 +ellps=krass +towgs84=15.8,-154.4,-82.3,0,0,0,0 "
                          "+units=m +no_defs").arg(cm*3).arg(cm*1000000+500000);
    }
    //6度分带，加带号
    if (cm>12 && cm<24)
    {
        pszCGCS = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 "
                          "+x_0=%2 +y_0=0 +ellps=krass +towgs84=15.8,-154.4,-82.3,0,0,0,0 "
                          "+units=m +no_defs").arg(cm*6-3).arg(cm*1000000+500000);
    }

    return pszCGCS;
}
