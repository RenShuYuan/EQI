#include "mainwindow.h"
#include "eqiProjectionTransformation.h"
#include "qgscoordinatetransform.h"
#include "qgsmessagebar.h"
#include "qgsmessagelog.h"
#include "sqlite3.h"

eqiProjectionTransformation::eqiProjectionTransformation(QObject *parent) : QObject(parent)
{

}

QgsPoint eqiProjectionTransformation::prjTransform(const QgsPoint p, const QgsCoordinateReferenceSystem& mSourceCrs,
                                                                     const QgsCoordinateReferenceSystem& mTargetCrs)
{
    // 创建目标投影
    if (!mSourceCrs.isValid() || !mTargetCrs.isValid())
    {
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                           "投影设置不正确, 运行已终止!",
                                                           QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage("投影变换 : \t投影设置不正确, 运行已终止.");
        return QgsPoint();
    }

    // 创建转换关系
    QgsCoordinateTransform ct(mSourceCrs, mTargetCrs);
    if (!ct.isInitialised())
    {
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                           "创建坐标转换关系失败, 运行已终止!",
                                                           QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage("投影变换 : \t创建坐标转换关系失败, 运行已终止.");
        return QgsPoint();
    }

    // 坐标转换
    QgsPoint point = ct.transform(p);
    return point;
}

bool eqiProjectionTransformation::createTargetCrs()
{
    // 获得当前参照坐标系
    QString myDefaultCrs = mSettings.value( "/eqi/prjTransform/projectDefaultCrs", GEO_EPSG_CRS_AUTHID ).toString();
    mSourceCrs.createFromOgcWmsCrs( myDefaultCrs );

    // 验证源参照坐标系
    if (!mSourceCrs.isValid())
    {
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                           "项目没有指定正确的参照坐标系, 运行已终止!",
                                                           QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage("投影变换 : \t项目没有指定正确的参照坐标系, 运行已终止!");
        return false;
    }

    // 检查输入的是否是2种常用的地理坐标系
    if ( !( (mSourceCrs.authid() == "EPSG:4326") ||		// WGS84
        (mSourceCrs.authid() == "EPSG:4490") ) )		// CGCS2000
    {
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                           "项目指定了错误的参照坐标系, 目前仅支持WGS84、CGCS2000, 运行已终止!",
                                                           QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage("投影变换 : \t项目指定了错误的参照坐标系, 目前仅支持WGS84、CGCS2000, 运行已终止!");
        return false;
    }

    // 获得曝光点文件中的中央经线
//    double cm = getCentralMeridian();
    double cm = 87;

    // 检查经度是否在正常范围内
    if ( !((cm>74 && cm<136) || (cm>24 && cm<46) || (cm>12 && cm<24)) )
    {
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                           "中央经线不在中国范围内, 运行已终止!",
                                                           QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("投影变换 : \t中央经线 %1 不在中国范围内, 运行已终止!").arg(cm));
        return false;
    }

    // 创建WKT格式投影坐标系
    QString wkt;
    QString strDescription;
    if (mSourceCrs.authid() == "EPSG:4326")
    {
        wkt = createProj4Wgs84Gcs(cm);

        // 投影名称 不加带号
        if (cm>74 && cm<136)
            strDescription = QString("WGS 84 / Gauss-Kruger CM %1E").arg(cm);
        // 投影名称 加带号
        if (cm>12 && cm<46)
            strDescription = QString("WGS 84 / Gauss-Kruger zone %1").arg(cm);
    }
    else if (mSourceCrs.authid() == "EPSG:4490")
    {
        wkt = createProj4Cgcs2000Gcs(cm);

        // 投影名称 不加带号
        if (cm>74 && cm<136)
            strDescription = QString("CGCS2000 / Gauss-Kruger CM %1E").arg(cm);
        // 投影名称 加带号
        if (cm>12 && cm<46)
            strDescription = QString("CGCS2000 / Gauss-Kruger zone %1").arg(cm);
    }

    mTargetCrs.createFromProj4(wkt);

    if ( !mTargetCrs.isValid() )
    {
        MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                                                           QString("中央经线为%1, 创建投影参考坐标系失败, 运行已终止!").arg(cm),
                                                           QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("投影变换 : \t中央经线为%1, 创建投影参考坐标系失败, 运行已终止!").arg(cm));
        return false;
    }

    QgsMessageLog::logMessage(QString("投影变换 : \t中央经线为%1, 创建投影参考坐标系\"%2\"成功.").arg(cm).arg(mTargetCrs.description()));

    // 填充参照坐标系名称列表
    if (descriptionList.isEmpty())
    {
        if (!descriptionForDb(descriptionList))
            return false;
    }
    if (descriptionUserList.isEmpty())
    {
        if (!descriptionForUserDb(descriptionUserList))
            return false;
    }

    // 如果数据库中没有这个参照坐标系，则写入
    if (descriptionList.contains(strDescription) || descriptionUserList.contains(strDescription))
        return true;
    else
    {
        long return_id = mTargetCrs.saveAsUserCRS( strDescription );
        if (!(return_id == -1))
        {
            descriptionUserList.clear();
            if (!descriptionForUserDb(descriptionUserList))
                return false;
        }
        else
        {
            MainWindow::instance()->messageBar()->pushMessage( "投影变换",
                QString("向数据库中写入 %1 参考坐标系失败, 运行已终止!").arg(strDescription),
                QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
            QgsMessageLog::logMessage(QString("投影变换 : \t向数据库中写入 %1 参考坐标系失败, 运行已终止!").arg(strDescription));
            return false;
        }
    }

    return true;
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

QString eqiProjectionTransformation::createProj4Cgcs2000Gcs(const double cm)
{
    QString pszCGCS_2000;

    //3度分带，不加带号
    if (cm>74 && cm<136)
    {
        pszCGCS_2000 = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 +x_0=500000 +y_0=0 +ellps=GRS80 +units=m +no_defs").arg(cm);
    }
    //3度分带，加带号
    if (cm>24 && cm<46)
    {
        pszCGCS_2000 = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 +x_0=%2 +y_0=0 +ellps=GRS80 +units=m +no_defs").arg(cm*3).arg(cm*1000000+500000);
    }
    //6度分带，加带号
    if (cm>12 && cm<24)
    {
        pszCGCS_2000 = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 +x_0=%2 +y_0=0 +ellps=GRS80 +units=m +no_defs").arg(cm*6-3).arg(cm*1000000+500000);
    }

    return pszCGCS_2000;
}

QString eqiProjectionTransformation::createProj4Wgs84Gcs(const double cm)
{
    QString pszCGCS_84;

    //3度分带，不加带号
    if (cm>74 && cm<136)
    {
        pszCGCS_84 = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 +x_0=500000 +y_0=0 +datum=WGS84 +units=m +no_defs").arg(cm);
    }
    //3度分带，加带号
    if (cm>24 && cm<46)
    {
        pszCGCS_84 = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 +x_0=%2 +y_0=0 +datum=WGS84 +units=m +no_defs").arg(cm*3).arg(cm*1000000+500000);
    }
    //6度分带，加带号
    if (cm>12 && cm<24)
    {
        pszCGCS_84 = QString("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1 +x_0=%2 +y_0=0 +datum=WGS84 +units=m +no_defs").arg(cm*6-3).arg(cm*1000000+500000);
    }

    return pszCGCS_84;
}
