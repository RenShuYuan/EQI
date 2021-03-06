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

RC_FILE += my.rc

# 加载头文件
INCLUDEPATH +=	C:/qgis-2.18.18/dev/include \
                C:/qgis-2.18.18/qgis-2.18.18/src \
                C:/qgis-2.18.18/qgis-2.18.18/src/python \
                C:/qgis-2.18.18/qgis-2.18.18/src/plugins \
                C:/qgis-2.18.18/qgis-2.18.18/src/core \
                C:/qgis-2.18.18/qgis-2.18.18/src/core/auth \
                C:/qgis-2.18.18/qgis-2.18.18/src/core/composer \
                C:/qgis-2.18.18/qgis-2.18.18/src/core/dxf \
                C:/qgis-2.18.18/qgis-2.18.18/src/core/effects \
                C:/qgis-2.18.18/qgis-2.18.18/src/core/geometry \
                C:/qgis-2.18.18/qgis-2.18.18/src/core/layertree \
                C:/qgis-2.18.18/qgis-2.18.18/src/core/pal \
                C:/qgis-2.18.18/qgis-2.18.18/src/core/raster \
                C:/qgis-2.18.18/qgis-2.18.18/src/core/renderer \
                C:/qgis-2.18.18/qgis-2.18.18/src/core/symbology-ng \
                C:/qgis-2.18.18/qgis-2.18.18/src/core/gps/qextserialport \
                C:/qgis-2.18.18/qgis-2.18.18/src/gui \
                C:/qgis-2.18.18/qgis-2.18.18/src/app \
                C:/qgis-2.18.18/qgis-2.18.18/src/app/ogr \
                C:/qgis-2.18.18/qgis-2.18.18/src/app/pluginmanager \
                C:/OSGeo4W/include \
                C:/OSGeo4W/include/spatialindex \
                C:/OSGeo4W/include/spatialite \
                C:/OSGeo4W/include/qwt \
                C:/OSGeo4W/include/qt4 \
                C:/OSGeo4W/include/qt4/QtCrypto \
                C:/Qt/4.8.6/include/QtSvg \
                C:/Qt/4.8.6/include/QtGui \
                C:/Qt/4.8.6/include/QtXml \
                C:/Qt/4.8.6/include/QtSql \
                C:/Qt/4.8.6/include/QtNetwork \
                C:/Qt/4.8.6/include/QtCore \
                C:/siftgpu32

# 加载lib文件
LIBS += -LC:/qgis-2.18.18/dev/lib   -lqgis_core \
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

LIBS += -LC:/siftgpu32   -lSIFTGPU

#DEFINES += NOMINMAX
#DEFINES += WITH_QTWEBKIT
#DEFINES += CORE_EXPORT=__declspec(dllimport)
#DEFINES += GUI_EXPORT=_declspec(dllimport)
#DEFINES += GUI_EXPORT=__declspec(dllimport)
#DEFINES += ANALYSIS_EXPORT=_declspec(dllimport)
#DEFINES += APP_EXPORT=__declspec(dllexport)
#DEFINES += PYTHON_EXPORT=__declspec(dllimport)

DEFINES += WIN32
DEFINES += _WINDOWS
DEFINES += NOMINMAX
DEFINES += WITH_QTWEBKIT
DEFINES += CORE_EXPORT=__declspec(dllimport)
DEFINES += GUI_EXPORT=_declspec(dllimport)
DEFINES += ANALYSIS_EXPORT=_declspec(dllimport)
DEFINES += APP_EXPORT=__declspec(dllexport)
DEFINES += PYTHON_EXPORT=__declspec(dllimport)
DEFINES += SPATIALITE_VERSION_GE_4_0_0
DEFINES += SPATIALITE_VERSION_G_4_1_1
DEFINES += SPATIALITE_HAS_INIT_EX
DEFINES += NDEBUG
DEFINES += QGISDEBUG=1
DEFINES += CUSTOMWIDGETS_EXPORT=__declspec(dllimport)
DEFINES += SERVER_EXPORT=__declspec(dllimport)
DEFINES += QT_DLL
DEFINES += QT_NO_DEBUG
DEFINES += QT_SVG_LIB
DEFINES += QT_GUI_LIB
DEFINES += QT_XML_LIB
DEFINES += QT_SQL_LIB
DEFINES += QT_NETWORK_LIB
DEFINES += QT_CORE_LIB
DEFINES += ENABLE_TESTS
DEFINES += QT_NO_CAST_TO_ASCII
DEFINES += _USE_MATH_DEFINES
DEFINES += _CRT_SECURE_NO_WARNINGS
DEFINES += _CRT_NONSTDC_NO_WARNINGS
DEFINES += _TTY_WIN_
DEFINES += _HAVE_WINDOWS_H_
DEFINES += qgis_core_EXPORTS

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
    ui/dialog/dialog_prjtransformsetting.cpp \
    eqi/maptool/eqimaptoolpointtotk.cpp \
    qgis/app/qgsmaptoolselectutils.cpp \
    ui/toolTab/tab_datamanagement.cpp \
    qgis/app/qgsvectorlayersaveasdialog.cpp \
    qgis/ogr/qgsogrhelperfunctions.cpp \
    ui/dialog/dialog_printtktoxy_txt.cpp \
    ui/dialog/dialog_posloaddialog.cpp \
    eqi/pos/posdataprocessing.cpp \
    eqi/eqiinquiredemvalue.cpp \
    eqi/eqiProjectionTransformation.cpp \
    qgis/app/delimitedtext/qgsdelimitedtextfile.cpp \
    ui/dialog/dialog_possetting.cpp \
    eqi/eqippinteractive.cpp \
    eqi/eqisymbol.cpp \
    ui/toolTab/tab_selectfeatures.cpp \
    qgis/app/qgsmaptoolselectfreehand.cpp \
    qgis/app/qgsmaptoolselectpolygon.cpp \
    qgis/app/qgsmaptoolselectrectangle.cpp \
    ui/dialog/dialog_selectsetting.cpp \
    ui/toolTab/tab_checkaerialphoto.cpp \
    eqi/eqianalysisaerialphoto.cpp \
    ui/dialog/dialog_about.cpp \
    qgis/app/qgsmaptoolidentifyaction.cpp \
    qgis/app/qgsidentifyresultsdialog.cpp \
    qgis/app/qgsrasterlayerproperties.cpp \
    qgis/app/qgsvectorlayerproperties.cpp \
    qgis/app/qgsjoindialog.cpp \
    qgis/app/qgsdiagramproperties.cpp \
    qgis/app/qgslabelengineconfigdialog.cpp \
    qgis/app/qgsfieldcalculator.cpp \
    qgis/app/qgsfieldsproperties.cpp \
    qgis/app/qgslabeldialog.cpp \
    qgis/app/qgslabelingwidget.cpp \
    qgis/app/qgspluginmetadata.cpp \
    qgis/app/qgspluginregistry.cpp \
    qgis/app/qgssavestyletodbdialog.cpp \
    qgis/app/qgsloadstylefromdbdialog.cpp \
    qgis/app/qgslabelinggui.cpp \
    qgis/app/qgsaddtaborgroup.cpp \
    qgis/app/qgslabelpreview.cpp \
    qgis/app/qgsrulebasedlabelingwidget.cpp \
    qgis/ogr/qgsrulebasedlabelingwidget.cpp \
    qgis/app/qgsaddattrdialog.cpp \
    qgis/app/qgsdelattrdialog.cpp \
    qgis/app/qgsattributetabledialog.cpp \
    qgis/app/qgsattributetypedialog.cpp \
    qgis/app/qgsfeatureaction.cpp \
    qgis/app/qgsguivectorlayertools.cpp \
    ui/toolTab/tab_inquire.cpp \
    qgis/ogr/qgsopenvectorlayerdialog.cpp \
    eqi/maptool/eqipcmfastpicksystem.cpp \
    qgis/app/qgsattributeactiondialog.cpp \
    qgis/app/qgsattributeactionpropertiesdialog.cpp \
    eqi/gdal/eqigdalprogresstools.cpp \
    ui/dialog/dialog_pcmsetting.cpp \
    eqi/overlappingprocessing.cpp

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
    ui/dialog/dialog_prjtransformsetting.h \
    eqi/maptool/eqimaptoolpointtotk.h \
    qgis/app/qgsmaptoolselectutils.h \
    ui/toolTab/tab_datamanagement.h \
    qgis/app/qgsvectorlayersaveasdialog.h \
    ui/dialog/dialog_printtktoxy_txt.h \
    ui/dialog/dialog_posloaddialog.h \
    eqi/pos/posdataprocessing.h \
    eqi/eqiinquiredemvalue.h \
    eqi/eqiProjectionTransformation.h \
    qgis/app/delimitedtext/qgsdelimitedtextfile.h \
    ui/dialog/dialog_possetting.h \
    eqi/eqippinteractive.h \
    eqi/eqisymbol.h \
    ui/toolTab/tab_selectfeatures.h \
    qgis/app/qgsmaptoolselectfreehand.h \
    qgis/app/qgsmaptoolselectpolygon.h \
    qgis/app/qgsmaptoolselectrectangle.h \
    ui/dialog/dialog_selectsetting.h \
    ui/toolTab/tab_checkaerialphoto.h \
    eqi/eqianalysisaerialphoto.h \
    ui/dialog/dialog_about.h \
    qgis/app/qgsmaptoolidentifyaction.h \
    qgis/app/qgsidentifyresultsdialog.h \
    qgis/app/qgsrasterlayerproperties.h \
    qgis/app/qgsvectorlayerproperties.h \
    qgis/app/qgsjoindialog.h \
    qgis/plugins/qgsapplydialog.h \
    qgis/app/qgsdiagramproperties.h \
    qgis/app/qgslabelengineconfigdialog.h \
    qgis/app/qgsfieldcalculator.h \
    qgis/app/qgsfieldsproperties.h \
    qgis/app/qgslabeldialog.h \
    qgis/app/qgslabelingwidget.h \
    qgis/app/qgspluginmetadata.h \
    qgis/app/qgspluginregistry.h \
    qgis/app/qgssavestyletodbdialog.h \
    qgis/app/qgsloadstylefromdbdialog.h \
    qgis/app/qgslabelinggui.h \
    qgis/app/qgsaddtaborgroup.h \
    qgis/app/qgslabelpreview.h \
    qgis/app/qgsrulebasedlabelingwidget.h \
    qgis/ogr/qgsogrhelperfunctions.h \
    qgis/ogr/qgsopenvectorlayerdialog.h \
    qgis/ogr/qgsrulebasedlabelingwidget.h \
    qgis/app/qgsaddattrdialog.h \
    qgis/app/qgsdelattrdialog.h \
    qgis/app/qgsattributetabledialog.h \
    qgis/app/qgsattributetypedialog.h \
    qgis/app/qgsfeatureaction.h \
    qgis/app/qgsguivectorlayertools.h \
    ui/toolTab/tab_inquire.h \
    eqi/maptool/eqipcmfastpicksystem.h \
    gdal/commonutils.h \
    gdal/gdal_utils_priv.h \
    qgis/app/qgsattributeactiondialog.h \
    qgis/app/qgsattributeactionpropertiesdialog.h \
    gdal/commonutils.h \
    gdal/gdal_utils_priv.h \
    eqi/gdal/eqigdalprogresstools.h \
    ui/dialog/dialog_pcmsetting.h \
    head.h \
    eqi/overlappingprocessing.h

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
    ui/qgis/qgsdatumtransformdialogbase.ui \
    ui/dialog/dialog_printtktoxy_txt.ui \
    ui/dialog/dialog_posloaddialog.ui \
    ui/dialog/dialog_possetting.ui \
    ui/toolTab/tab_selectfeatures.ui \
    ui/dialog/dialog_selectsetting.ui \
    ui/toolTab/tab_checkaerialphoto.ui \
    ui/dialog/dialog_about.ui \
    ui/qgis/qgsjoindialogbase.ui \
    ui/qgis/qgsattributeactiondialogbase.ui \
    ui/qgis/qgsengineconfigdialog.ui \
    ui/qgis/qgsdiagrampropertiesbase.ui \
    ui/qgis/qgsfieldcalculatorbase.ui \
    ui/qgis/qgsaddattrdialogbase.ui \
    ui/qgis/qgsattributetabledialog.ui \
    ui/qgis/qgsattributetypeedit.ui \
    ui/qgis/qgsdelattrdialogbase.ui \
    ui/qgis/qgsdualviewbase.ui \
    ui/qgis/qgsfieldconditionalformatwidget.ui \
    ui/qgis/qgsidentifyresultsbase.ui \
    ui/qgis/qgslabeldialogbase.ui \
    ui/qgis/qgslabelingwidget.ui \
    ui/qgis/qgsloadstylefromdbdialog.ui \
    ui/qgis/qgsmultibandcolorrendererwidgetbase.ui \
    ui/qgis/qgspalettedrendererwidgetbase.ui \
    ui/qgis/qgsrasterhistogramwidgetbase.ui \
    ui/qgis/qgsrasterlayerpropertiesbase.ui \
    ui/qgis/qgsrasterminmaxwidgetbase.ui \
    ui/qgis/qgssavetodbdialog.ui \
    ui/qgis/qgssinglebandgrayrendererwidgetbase.ui \
    ui/qgis/qgssinglebandpseudocolorrendererwidgetbase.ui \
    ui/qgis/qgsvectorlayerpropertiesbase.ui \
    ui/qgis/qgsrulebasedlabelingwidget.ui \
    ui/qgis/qgsfieldspropertiesbase.ui \
    ui/qgis/qgslabelingguibase.ui \
    ui/qgis/qgscolordialog.ui \
    ui/qgis/qgsaddtaborgroupbase.ui \
    ui/qgis/qgsmapunitscaledialog.ui \
    ui/qgis/qgsrendererv2propsdialogbase.ui \
    ui/qgis/qgsunitselectionwidget.ui \
    ui/qgis/qgseffectstackpropertieswidgetbase.ui \
    ui/qgis/qgscharacterselectdialogbase.ui \
    ui/qgis/qgslabelingrulepropsdialog.ui \
    ui/symbollayer/qgs25drendererwidgetbase.ui \
    ui/symbollayer/qgsdashspacedialogbase.ui \
    ui/symbollayer/qgsgeometrygeneratorwidgetbase.ui \
    ui/symbollayer/qgsheatmaprendererwidgetbase.ui \
    ui/symbollayer/widget_centroidfill.ui \
    ui/symbollayer/widget_ellipse.ui \
    ui/symbollayer/widget_fontmarker.ui \
    ui/symbollayer/widget_gradientfill.ui \
    ui/symbollayer/widget_layerproperties.ui \
    ui/symbollayer/widget_linepatternfill.ui \
    ui/symbollayer/widget_markerline.ui \
    ui/symbollayer/widget_pointpatternfill.ui \
    ui/symbollayer/widget_rasterfill.ui \
    ui/symbollayer/widget_set_dd_value.ui \
    ui/symbollayer/widget_shapeburstfill.ui \
    ui/symbollayer/widget_simplefill.ui \
    ui/symbollayer/widget_simpleline.ui \
    ui/symbollayer/widget_simplemarker.ui \
    ui/symbollayer/widget_size_scale.ui \
    ui/symbollayer/widget_svgfill.ui \
    ui/symbollayer/widget_svgmarker.ui \
    ui/symbollayer/widget_svgselector.ui \
    ui/symbollayer/widget_symbolslist.ui \
    ui/symbollayer/widget_vectorfield.ui \
    ui/qgis/qgsattributeloadfrommap.ui \
    ui/toolTab/tab_inquire.ui \
    ui/qgis/qgsnewogrconnectionbase.ui \
    ui/qgis/qgssnappingdialogbase.ui \
    ui/qgis/qgspluginmanagerbase.ui \
    ui/qgis/qgsbookmarksbase.ui \
    ui/qgis/qgsattributeactionpropertiesdialogbase.ui \
    ui/dialog/dialog_pcmsetting.ui

RESOURCES += \
    Resources/images/images.qrc
DISTFILES +=
