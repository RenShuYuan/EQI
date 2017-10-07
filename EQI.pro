#-------------------------------------------------
#
# Project created by QtCreator 2017-08-07T18:05:58
#
#-------------------------------------------------

QT       += core gui webkit xml network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = EQI
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RC_ICONS = plane.ico

# 加载头文件
INCLUDEPATH +=  C:/qgis-2.14.4/dev/include \
                C:/OSGeo4W/include \
                C:/OSGeo4W/include/qt4 \
                C:/OSGeo4W/include/libxml \
                C:/OSGeo4W/include/qwt \
                C:/OSGeo4W/include/spatialindex \
                C:/OSGeo4W/include/spatialite

# 加载lib文件
LIBS += -LC:/qgis-2.14.4/dev/lib    -lqgis_core \
                                    -lqgis_gui \
                                    -lqgis_networkanalysis \
                                    -lqgis_analysis \
                                    -lqgis_app

LIBS += -LC:/OSGeo4W/lib    -llibexpat \
                            -lgdal_i \
                            -lgeos_c \
                            -liconv \
                            -llibpq \
                            -llibxml2 \
                            -lproj_i \
                            -lqca \
                            -lqscintilla2 \
                            -lqwt \
                            -lspatialite_i \
                            -lsqlite3_i \
                            -lszip \
                            -lspatialindex_i \

DEFINES += NOMINMAX
DEFINES += WITH_QTWEBKIT
DEFINES += CORE_EXPORT=__declspec(dllimport)
DEFINES += GUI_EXPORT=_declspec(dllimport)
DEFINES += ANALYSIS_EXPORT=_declspec(dllimport)
DEFINES += APP_EXPORT=__declspec(dllexport)
DEFINES += PYTHON_EXPORT=__declspec(dllimport)

QMAKE_CXXFLAGS_RELEASE += /Zi
QMAKE_CXXFLAGS_RELEASE += /Od
QMAKE_LFLAGS_RELEASE += /DEBUG
DEFINES -=QT_NO_DEBUG_OUTPUT # enable debug output

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    ui/toolTab/tab_coordinatetransformation.cpp \
    qgis/app/qgsstatusbarcoordinateswidget.cpp \
    eqi/eqiapplication.cpp \
    qgis/app/qgisappstylesheet.cpp \
    ui/toolTab/tab_uavdatamanagement.cpp \
    qgis/app/qgsapplayertreeviewmenuprovider.cpp \
    qgis/app/qgsvisibilitypresets.cpp \
    qgis/app/qgsclipboard.cpp \
    qgis/app/qgsmaplayerstyleguiutils.cpp \
    qgis/app/qgsprojectproperties.cpp \
    qgis/app/qgsmeasuretool.cpp \
    qgis/app/qgsmeasuredialog.cpp \
    ui/toolTab/tab_mapbrowsing.cpp \
    ui/toolTab/tab_fractalmanagement.cpp \
    eqi/eqifractalmanagement.cpp \
    eqi/eqiProjectionTransformation.cpp \
    ui/dialog/dialog_prjtransformsetting.cpp \
    eqi/maptool/eqimaptoolpointtotk.cpp \
    qgis/app/qgsmaptoolselectutils.cpp \
    ui/toolTab/tab_datamanagement.cpp \
    qgis/ogr/qgsopenvectorlayerdialog.cpp \
    qgis/app/qgsvectorlayersaveasdialog.cpp \
    qgis/ogr/qgsogrhelperfunctions.cpp

HEADERS += \
        mainwindow.h \
    ui/toolTab/tab_coordinatetransformation.h \
    qgis/app/qgsstatusbarcoordinateswidget.h \
    eqi/eqiapplication.h \
    qgis/app/qgisappstylesheet.h \
    ui/toolTab/tab_uavdatamanagement.h \
    qgis/app/qgsapplayertreeviewmenuprovider.h \
    qgis/app/qgsvisibilitypresets.h \
    qgis/app/qgsclipboard.h \
    qgis/app/qgsmaplayerstyleguiutils.h \
    qgis/app/qgsprojectproperties.h \
    qgis/app/qgsmeasuretool.h \
    qgis/app/qgsmeasuredialog.h \
    ui/toolTab/tab_mapbrowsing.h \
    ui/toolTab/tab_fractalmanagement.h \
    eqi/eqifractalmanagement.h \
    eqi/eqiProjectionTransformation.h \
    ui/dialog/dialog_prjtransformsetting.h \
    eqi/maptool/eqimaptoolpointtotk.h \
    qgis/app/qgsmaptoolselectutils.h \
    ui/toolTab/tab_datamanagement.h \
    qgis/ogr/qgsopenvectorlayerdialog.h \
    qgis/app/qgsvectorlayersaveasdialog.h \
    qgis/ogr/qgsogrhelperfunctions.h

FORMS += \
        mainwindow.ui \
    ui/toolTab/tab_coordinatetransformation.ui \
    ui/toolTab/tab_uavdatamanagement.ui \
    ui/qgis/qgssymbolv2selectordialogbase.ui \
    ui/qgis/qgsmeasurebase.ui \
    ui/qgis/qgsprojectpropertiesbase.ui \
    ui/toolTab/tab_mapbrowsing.ui \
    ui/toolTab/tab_fractalmanagement.ui \
    ui/dialog/dialog_prjtransformsetting.ui \
    ui/toolTab/tab_datamanagement.ui \
    ui/qgis/qgsopenvectorlayerdialogbase.ui \
    ui/qgis/qgsvectorlayersaveasdialogbase.ui \
    ui/qgis/qgsextentgroupboxwidget.ui \
    ui/qgis/qgsdatumtransformdialogbase.ui

RESOURCES += \
    Resources/images/images.qrc
DISTFILES +=
