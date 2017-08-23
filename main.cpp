#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>
#include <QTranslator>
#include <QSettings>
#include <QDir>

#include "qgsapplication.h"
#include "qgseditorwidgetregistry.h"

int main(int argc, char *argv[])
{
    /* 设置成支持中文 */
    QTextCodec *codec=QTextCodec::codecForName("System");
    QTextCodec::setCodecForLocale(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForTr(codec);

    //! 使用 QSettings
    QCoreApplication::setOrganizationName( "YuanLong" );
    QCoreApplication::setApplicationName( "EQI" );

    QgsApplication a(argc, argv, true);
//    a.setWindowIcon( QIcon( "plane.ico" ) );
    QgsApplication::setDefaultSvgPaths( QStringList( QDir::currentPath()+"/Resources/images/svg" ) );
    QgsApplication::setPrefixPath( QDir::currentPath(), true );
    QgsApplication::init(QDir::currentPath()+"/Resources");
    QgsApplication::initQgis();

    // 使用翻译文件
    QTranslator qtTranslator;
    qtTranslator.load("qgis_zh-Hans.qm");
    a.installTranslator(&qtTranslator);

    MainWindow m;
    m.setWindowState(Qt::WindowMaximized);
    m.setWindowTitle("EQI");
    m.show();

    QgsEditorWidgetRegistry::initEditors();


    return a.exec();
}
