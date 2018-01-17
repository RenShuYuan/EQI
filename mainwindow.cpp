// My
#include "mainwindow.h"
#include "eqi/eqiapplication.h"
#include "eqi/eqifractalmanagement.h"
#include "eqi/maptool/eqimaptoolpointtotk.h"
#include "eqi/pos/posdataprocessing.h"
#include "eqi/eqippinteractive.h"
#include "eqi/eqianalysisaerialphoto.h"
#include "ui_mainwindow.h"
#include "ui/toolTab/tab_mapbrowsing.h"
#include "ui/toolTab/tab_coordinatetransformation.h"
#include "ui/toolTab/tab_uavdatamanagement.h"
#include "ui/toolTab/tab_fractalmanagement.h"
#include "ui/toolTab/tab_datamanagement.h"
#include "ui/toolTab/tab_selectfeatures.h"
#include "ui/toolTab/tab_checkaerialphoto.h"
#include "ui/dialog/dialog_posloaddialog.h"
#include "ui/dialog/dialog_printtktoxy_txt.h"
#include "ui/dialog/dialog_prjtransformsetting.h"
#include "ui/dialog/dialog_possetting.h"
#include "ui/dialog/dialog_selectsetting.h"
#include "qgis/app/qgsstatusbarcoordinateswidget.h"
#include "qgis/app/qgsapplayertreeviewmenuprovider.h"
#include "qgis/app/qgsclipboard.h"
#include "qgis/app/qgisappstylesheet.h"
#include "qgis/app/qgsvisibilitypresets.h"
#include "qgis/app/qgsprojectproperties.h"
#include "qgis/app/qgsmeasuretool.h"
#include "qgis/ogr/qgsopenvectorlayerdialog.h"
#include "qgis/app/qgsvectorlayersaveasdialog.h"
#include "qgis/app/qgsmaptoolselectrectangle.h"
#include "qgis/app/qgsmaptoolselectpolygon.h"
#include "qgis/app/qgsmaptoolselectfreehand.h"

// Qt
#include <QAction>
#include <QTabBar>
#include <QLabel>
#include <QProgressBar>
#include <QCheckBox>
#include <QToolButton>
#include <QMessageBox>
#include <QGridLayout>
#include <QDockWidget>
#include <QCursor>
#include <QMenu>
#include <QFileDialog>

// QGis
#include "qgsscalecombobox.h"
#include "qgsdoublespinbox.h"
#include "qgslayertreeview.h"
#include "qgsmessagebar.h"
#include "qgslayertreemodel.h"
#include "qgsproject.h"
#include "qgslayertreenode.h"
#include "qgslayertreeregistrybridge.h"
#include "qgslayertreeviewdefaultactions.h"
#include "qgslayertreemapcanvasbridge.h"
#include "qgscustomlayerorderwidget.h"
#include "qgsmessagelogviewer.h"
#include "qgsmessagelog.h"
#include "qgsvectorlayer.h"
#include "qgslayertreegroup.h"
#include "qgslayertreelayer.h"
#include "qgslayertree.h"
#include "qgscoordinateutils.h"
#include "qgsrasterlayer.h"
#include "qgsvectordataprovider.h"
#include "qgsmaplayerregistry.h"
#include "qgssublayersdialog.h"
#include "qgsdatumtransformdialog.h"
#include "qgsmessageviewer.h"
#include "qgsmaptoolzoom.h"
#include "qgsmaptoolpan.h"

MainWindow *MainWindow::smInstance = nullptr;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
    , mScaleLabel( nullptr )
    , mScaleEdit( nullptr )
    , mCoordsEdit( nullptr )
    , mRotationLabel( nullptr )
    , mRotationEdit( nullptr )
    , mProgressBar( nullptr )
    , mRenderSuppressionCBox( nullptr )
    , mOnTheFlyProjectionStatusButton( nullptr )
    , mLayerTreeCanvasBridge( nullptr )
    , mInternalClipboard( nullptr )
    , mShowProjectionTab( false )
    , pPosdp( nullptr )
    , pPPInter( nullptr )
{
    if ( smInstance )
    {
        QMessageBox::critical( this, "EQI的多个实例", "检测到多个EQI应用对象的实例。" );
        abort();
    }
    smInstance = this;

    ui->setupUi(this);

    QWidget *centralWidget = this->centralWidget();
    QGridLayout *centralLayout = new QGridLayout( centralWidget );
    centralWidget->setLayout( centralLayout );
    centralLayout->setContentsMargins( 0, 0, 0, 0 );

    // 创建绘图区
    mMapCanvas = new QgsMapCanvas( centralWidget, "theMapCanvas" );
    mMapCanvas->setCanvasColor( QColor( 255, 255, 255 ) );
    connect( mMapCanvas, SIGNAL( messageEmitted( const QString&, const QString&, QgsMessageBar::MessageLevel ) ),
             this, SLOT( displayMessage( const QString&, const QString&, QgsMessageBar::MessageLevel ) ) );
    mMapCanvas->setWhatsThis( "地图画布." );

    centralLayout->addWidget( mMapCanvas, 1, 0, 1, 1 );
    mMapCanvas->setFocus();

    // 初始化信息显示条
     mInfoBar = new QgsMessageBar( centralWidget );
     mInfoBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
     centralLayout->addWidget( mInfoBar, 0, 0, 1, 1 );

    // 设置样式表制造商和应用保存或默认样式选项
    mStyleSheetBuilder = new QgisAppStyleSheet( this );
    connect( mStyleSheetBuilder, SIGNAL( appStyleSheetChanged( const QString& ) ),
             this, SLOT( setAppStyleSheet( const QString& ) ) );
    mStyleSheetBuilder->buildStyleSheet( mStyleSheetBuilder->defaultOptions() );

    // 设置图层管理面板
    mLayerTreeView = new QgsLayerTreeView( this );
    mLayerTreeView->setObjectName( "theLayerTreeView" );

    // create clipboard
    mInternalClipboard = new QgsClipboard;
    connect( mInternalClipboard, SIGNAL( changed() ), this, SLOT( clipboardChanged() ) );

    initActions();
    initTabTools();
    initStatusBar();
    initLayerTreeView();
//    initOverview();
    createCanvasTools();
    mMapCanvas->freeze();

    // 消息面板
    mLogViewer = new QgsMessageLogViewer( statusBar(), this );
    mLogDock = new QDockWidget( tr( "Log Messages Panel" ), this );
    mLogDock->setObjectName( "MessageLog" );
    mLogDock->setAllowedAreas( Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea );
    addDockWidget( Qt::BottomDockWidgetArea, mLogDock );
    mLogDock->setWidget( mLogViewer );
    mLogDock->hide();
    connect( mMessageButton, SIGNAL( toggled( bool ) ), mLogDock, SLOT( setVisible( bool ) ) );
    connect( mLogDock, SIGNAL( visibilityChanged( bool ) ), mMessageButton, SLOT( setChecked( bool ) ) );
    connect( QgsMessageLog::instance(), SIGNAL( messageReceived( bool ) ), this, SLOT( toggleLogMessageIcon( bool ) ) );
    connect( mMessageButton, SIGNAL( toggled( bool ) ), this, SLOT( toggleLogMessageIcon( bool ) ) );
    // end

    // 连接POS处理进度
    pPosdp = new posDataProcessing(this);
    connect( pPosdp, SIGNAL( startProcess() ), this, SLOT( canvasRefreshStarted ) );
    connect( pPosdp, SIGNAL( stopProcess() ), this, SLOT( canvasRefreshFinished() ) );
    upDataPosActions();
}

MainWindow::~MainWindow()
{
    delete ui;

    mMapCanvas->stopRendering();

    delete mInternalClipboard;

    delete mMapTools.mZoomIn;
    delete mMapTools.mZoomOut;
    delete mMapTools.mPan;

    delete mMapTools.mAddFeature;
    delete mMapTools.mAddPart;
    delete mMapTools.mAddRing;
    delete mMapTools.mFillRing;
    delete mMapTools.mAnnotation;
    delete mMapTools.mChangeLabelProperties;
    delete mMapTools.mDeletePart;
    delete mMapTools.mDeleteRing;
    delete mMapTools.mFeatureAction;
    delete mMapTools.mFormAnnotation;
    delete mMapTools.mHtmlAnnotation;
    delete mMapTools.mIdentify;
    delete mMapTools.mMeasureAngle;
    delete mMapTools.mMeasureArea;
    delete mMapTools.mMeasureDist;
    delete mMapTools.mMoveFeature;
    delete mMapTools.mMoveLabel;
    delete mMapTools.mNodeTool;
    delete mMapTools.mOffsetCurve;
    delete mMapTools.mPinLabels;
    delete mMapTools.mReshapeFeatures;
    delete mMapTools.mRotateFeature;
    delete mMapTools.mRotateLabel;
    delete mMapTools.mRotatePointSymbolsTool;
    delete mMapTools.mSelectFreehand;
    delete mMapTools.mSelectPolygon;
    delete mMapTools.mSelectRadius;
    delete mMapTools.mSelectFeatures;
    delete mMapTools.mShowHideLabels;
    delete mMapTools.mSimplifyFeature;
    delete mMapTools.mSplitFeatures;
    delete mMapTools.mSplitParts;
    delete mMapTools.mSvgAnnotation;
    delete mMapTools.mTextAnnotation;
    delete mMapTools.mCircularStringCurvePoint;
    delete mMapTools.mCircularStringRadius;

    delete pPosdp;
    delete pPPInter;

//    delete mOverviewMapCursor;

    // cancel request for FileOpen events
    QgsApplication::setFileOpenEventReceiver( nullptr );

    QgsApplication::exitQgis();

    delete QgsProject::instance();
}

QgsMapCanvas *MainWindow::mapCanvas()
{
    Q_ASSERT( mMapCanvas );
    return mMapCanvas;
}

QgsLayerTreeView *MainWindow::layerTreeView()
{
    return mLayerTreeView;
}

QgsClipboard *MainWindow::clipboard()
{
    return mInternalClipboard;
}

QgsMessageBar *MainWindow::messageBar()
{
    Q_ASSERT( mInfoBar );
    return mInfoBar;
}

int MainWindow::messageTimeout()
{
    QSettings settings;
    return settings.value( "/eqi/messageTimeout", 5 ).toInt();
}

void MainWindow::openMessageLog()
{
    mMessageButton->setChecked( true );
}

void MainWindow::addDockWidget(Qt::DockWidgetArea area, QDockWidget *dockwidget)
{
    QMainWindow::addDockWidget( area, dockwidget );
    // Make the right and left docks consume all vertical space and top
    // and bottom docks nest between them
    setCorner( Qt::TopLeftCorner, Qt::LeftDockWidgetArea );
    setCorner( Qt::BottomLeftCorner, Qt::LeftDockWidgetArea );
    setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
    setCorner( Qt::BottomRightCorner, Qt::RightDockWidgetArea );
    // add to the Panel submenu
//    mPanelMenu->addAction( dockwidget->toggleViewAction() );

    dockwidget->show();

    // refresh the map canvas
    mMapCanvas->refresh();
}

QgsVectorLayer *MainWindow::createrMemoryMap(const QString &layerName, const QString &geometric, const QStringList &fieldList)
{
    QString layerProperties = geometric+"?";		// 几何类型

    // 参照坐标系
    QString myCrsStr = mSettings.value( "/eqi/prjTransform/projectDefaultCrs", GEO_EPSG_CRS_AUTHID ).toString();
    QgsCoordinateReferenceSystem mCrs;
    mCrs.createFromOgcWmsCrs( myCrsStr );
    layerProperties.append("crs="+mCrs.authid()+"&");

    foreach (QString field, fieldList)
    {
        layerProperties.append(field+"&");          // 添加字段
    }

    layerProperties.append(QString( "index=yes&" ));// 创建索引

    layerProperties.append(QString( "memoryid=%1" ).arg( QUuid::createUuid().toString() ));	// 临时编码

    QgsVectorLayer* newLayer = new QgsVectorLayer(layerProperties, layerName, QString( "memory" ) );

    if (!newLayer->isValid())
    {
        messageBar()->pushMessage( "创建矢量图层","创建面图层失败, 运行已终止, 注意检查plugins文件夹!",
                                   QgsMessageBar::CRITICAL, messageTimeout() );
        QgsMessageLog::logMessage(QString("创建矢量图层 : \t创建面图层失败, 程序已终止, 注意检查plugins文件夹。"));
        return nullptr;
    }

    // 添加到地图
    QgsMapLayerRegistry::instance()->addMapLayer(newLayer);
    return newLayer;
}

void MainWindow::pan()
{
    mMapCanvas->setMapTool( mMapTools.mPan );
}

void MainWindow::panToSelected()
{
    mMapCanvas->panToSelected();
}

void MainWindow::zoomIn()
{
    mMapCanvas->setMapTool( mMapTools.mZoomIn );
}

void MainWindow::zoomOut()
{
    mMapCanvas->setMapTool( mMapTools.mZoomOut );
}

void MainWindow::zoomFull()
{
    mMapCanvas->zoomToFullExtent();
}

void MainWindow::zoomActualSize()
{
    legendLayerZoomNative();
}

void MainWindow::zoomToSelected()
{
    mMapCanvas->zoomToSelected();
}

void MainWindow::zoomToLayerExtent()
{
    mLayerTreeView->defaultActions()->zoomToLayer( mMapCanvas );
}

void MainWindow::zoomToPrevious()
{
    mMapCanvas->zoomToPreviousExtent();
}

void MainWindow::zoomToNext()
{
    mMapCanvas->zoomToNextExtent();
}

void MainWindow::refreshMapCanvas()
{
    //停止任何当前的渲染
    mMapCanvas->stopRendering();
    mMapCanvas->refreshAllLayers();
}

void MainWindow::loadOGRSublayers(const QString &layertype, const QString &uri, const QStringList &list)
{
    // The uri must contain the actual uri of the vectorLayer from which we are
    // going to load the sublayers.
    QString fileName = QFileInfo( uri ).baseName();
    QList<QgsMapLayer *> myList;
    for ( int i = 0; i < list.size(); i++ )
    {
        QString composedURI;
        QStringList elements = list.at( i ).split( ':' );
        while ( elements.size() > 2 )
        {
            elements[0] += ':' + elements[1];
            elements.removeAt( 1 );
        }

        QString layerName = elements.value( 0 );
        QString layerType = elements.value( 1 );
        if ( layerType == "any" )
        {
            layerType = "";
            elements.removeAt( 1 );
        }

        if ( layertype != "GRASS" )
        {
            composedURI = uri + "|layername=" + layerName;
        }
        else
        {
            composedURI = uri + "|layerindex=" + layerName;
        }

        if ( !layerType.isEmpty() )
        {
            composedURI += "|geometrytype=" + layerType;
        }

        // addVectorLayer( composedURI, list.at( i ), "ogr" );

        QgsDebugMsg( "Creating new vector layer using " + composedURI );
        QString name = list.at( i );
        name.replace( ':', ' ' );
        QgsVectorLayer *layer = new QgsVectorLayer( composedURI, fileName + " " + name, "ogr", false );
        if ( layer && layer->isValid() )
        {
            myList << layer;
        }
        else
        {
            QString msg = tr( "%1 is not a valid or recognized data source" ).arg( composedURI );
            messageBar()->pushMessage( tr( "Invalid Data Source" ), msg, QgsMessageBar::CRITICAL, messageTimeout() );
            if ( layer )
                delete layer;
        }
    }

    if ( ! myList.isEmpty() )
    {
        // Register layer(s) with the layers registry
        QgsMapLayerRegistry::instance()->addMapLayers( myList );
        Q_FOREACH ( QgsMapLayer *l, myList )
        {
            bool ok;
            l->loadDefaultStyle( ok );
        }
    }
}

void MainWindow::autoSelectAddedLayer(QList<QgsMapLayer *> layers)
{
    if ( !layers.isEmpty() )
    {
        QgsLayerTreeLayer* nodeLayer = QgsProject::instance()->layerTreeRoot()->findLayer( layers[0]->id() );

        if ( !nodeLayer )
            return;

        QModelIndex index = mLayerTreeView->layerTreeModel()->node2index( nodeLayer );
        mLayerTreeView->setCurrentIndex( index );
    }
}

void MainWindow::activeLayerChanged(QgsMapLayer *layer)
{
    if ( mMapCanvas )
        mMapCanvas->setCurrentLayer( layer );
}

void MainWindow::initActions()
{
    /*--------------------------------------------地图浏览---------------------------------------------*/
    mActionPan = new QAction("平移地图", this);
    mActionPan->setStatusTip("平移地图");
    mActionPan->setIcon(eqiApplication::getThemeIcon("mActionPan.svg"));
    connect( mActionPan, SIGNAL( triggered() ), this, SLOT( pan() ) );

    mActionPanToSelected = new QAction("在视图中将\n选中部分居中", this);
    mActionPanToSelected->setStatusTip("在视图中将选中部分居中");
    mActionPanToSelected->setIcon(eqiApplication::getThemeIcon("mActionPanToSelected.svg"));
    connect( mActionPanToSelected, SIGNAL( triggered() ), this, SLOT( panToSelected() ) );

    mActionZoomIn = new QAction("放大", this);
    mActionZoomIn->setShortcut(tr("Ctrl++"));
    mActionZoomIn->setStatusTip("放大");
    mActionZoomIn->setIcon(eqiApplication::getThemeIcon("mActionZoomIn.svg"));
    connect( mActionZoomIn, SIGNAL( triggered() ), this, SLOT( zoomIn() ) );

    mActionZoomOut = new QAction("缩小", this);
    mActionZoomOut->setShortcut(tr("Ctrl+-"));
    mActionZoomOut->setStatusTip("缩小");
    mActionZoomOut->setIcon(eqiApplication::getThemeIcon("mActionZoomOut.svg"));
    connect( mActionZoomOut, SIGNAL( triggered() ), this, SLOT( zoomOut() ) );

    mActionZoomFullExtent = new QAction("全图显示(&F)", this);
    mActionZoomFullExtent->setShortcut(tr("Ctrl+Shift+F"));
    mActionZoomFullExtent->setStatusTip("全图显示(F)");
    mActionZoomFullExtent->setIcon(eqiApplication::getThemeIcon("mActionZoomFullExtent.svg"));
    connect( mActionZoomFullExtent, SIGNAL( triggered() ), this, SLOT( zoomFull() ) );

    mActionZoomActualSize = new QAction("缩放到原始\n分辨率(100%)", this);
    mActionZoomActualSize->setStatusTip("缩放到原始\n分辨率(100%)");
    mActionZoomActualSize->setIcon(eqiApplication::getThemeIcon("mActionZoomActual.svg"));
    connect( mActionZoomActualSize, SIGNAL( triggered() ), this, SLOT( zoomActualSize() ) );

<<<<<<< HEAD
    mActionZoomToSelected = new QAction("缩放到选择的区域(&S)", this);
=======
    mActionZoomToSelected = new QAction("缩放到\n选择的区域(&S)", this);
>>>>>>> 8408d22f60c6e2f266583bd79469a2a283413c43
    mActionZoomToSelected->setShortcut(tr("Ctrl+J"));
    mActionZoomToSelected->setStatusTip("缩放到选择的区域(S)");
    mActionZoomToSelected->setIcon(eqiApplication::getThemeIcon("mActionZoomToSelected.svg"));
    connect( mActionZoomToSelected, SIGNAL( triggered() ), this, SLOT( zoomToSelected() ) );

    mActionZoomToLayer = new QAction("缩放到图层(&L)", this);
    mActionZoomToLayer->setStatusTip("缩放到图层(L)");
    mActionZoomToLayer->setIcon(eqiApplication::getThemeIcon("mActionZoomToLayer.svg"));
    connect( mActionZoomToLayer, SIGNAL( triggered() ), this, SLOT( zoomToLayerExtent() ) );

    mActionZoomLast = new QAction("上一视图", this);
    mActionZoomLast->setStatusTip("上一视图");
    mActionZoomLast->setIcon(eqiApplication::getThemeIcon("mActionZoomLast.svg"));
    connect( mActionZoomLast, SIGNAL( triggered() ), this, SLOT( zoomToPrevious() ) );

<<<<<<< HEAD
    mActionZoomNext = new QAction("下一视图", this);
    mActionZoomNext->setStatusTip("下一视图");
    mActionZoomNext->setIcon(eqiApplication::getThemeIcon("mActionZoomNext.svg"));
    connect( mActionZoomNext, SIGNAL( triggered() ), this, SLOT( zoomToNext() ) );

    mActionDraw = new QAction("刷新", this);
    mActionDraw->setShortcut(tr("F5"));
    mActionDraw->setStatusTip("刷新");
    mActionDraw->setIcon(eqiApplication::getThemeIcon("mActionDraw.svg"));
    connect( mActionDraw, SIGNAL( triggered() ), this, SLOT( refreshMapCanvas() ) );
=======
//    mActionZoomNext = new QAction("下一视图", this);
//    mActionZoomNext->setStatusTip("下一视图");
//    mActionZoomNext->setIcon(eqiApplication::getThemeIcon("mActionZoomNext.svg"));
//    connect( mActionZoomNext, SIGNAL( triggered() ), this, SLOT( zoomToNext() ) );

//    mActionDraw = new QAction("刷新", this);
//    mActionDraw->setShortcut(tr("F5"));
//    mActionDraw->setStatusTip("刷新");
//    mActionDraw->setIcon(eqiApplication::getThemeIcon("mActionDraw.svg"));
//    connect( mActionDraw, SIGNAL( triggered() ), this, SLOT( refreshMapCanvas() ) );
>>>>>>> 8408d22f60c6e2f266583bd79469a2a283413c43

    mActionIdentify = new QAction("识别要素", this);
    mActionIdentify->setShortcut(tr("Ctrl+Shift+I"));
    mActionIdentify->setStatusTip("识别要素");
    mActionIdentify->setIcon(eqiApplication::getThemeIcon("mActionIdentify.svg"));
    connect( mActionIdentify, SIGNAL( triggered() ), this, SLOT( identify() ) );

    /*--------------------------------------------图层操作---------------------------------------------*/
    mActionRemoveLayer = new QAction("移除图层/组", this);
    mActionRemoveLayer->setShortcut(tr("Ctrl+D"));
    mActionRemoveLayer->setStatusTip("移除图层/组");
    mActionRemoveLayer->setIcon(eqiApplication::getThemeIcon("mActionRemoveLayer.svg"));
    connect( mActionRemoveLayer, SIGNAL( triggered() ), this, SLOT( removeLayer() ) );

    mActionShowAllLayers = new QAction("显示所有图层", this);
    mActionShowAllLayers->setShortcut(tr("Ctrl+Shift+U"));
    mActionShowAllLayers->setStatusTip("显示所有图层");
    mActionShowAllLayers->setIcon(eqiApplication::getThemeIcon("mActionShowAllLayers.png"));
    connect( mActionShowAllLayers, SIGNAL( triggered() ), this, SLOT( showAllLayers() ) );

    mActionHideAllLayers = new QAction("隐藏所有图层", this);
    mActionHideAllLayers->setShortcut(tr("Ctrl+Shift+H"));
    mActionHideAllLayers->setStatusTip("隐藏所有图层");
    mActionHideAllLayers->setIcon(eqiApplication::getThemeIcon("mActionHideAllLayers.png"));
    connect( mActionHideAllLayers, SIGNAL( triggered() ), this, SLOT( hideAllLayers() ) );

    mActionShowSelectedLayers = new QAction("显示选中的图层", this);
    mActionShowSelectedLayers->setStatusTip("显示选中的图层");
    mActionShowSelectedLayers->setIcon(eqiApplication::getThemeIcon("mActionShowSelectedLayers.png"));
    connect( mActionShowSelectedLayers, SIGNAL( triggered() ), this, SLOT( showSelectedLayers() ) );

    mActionHideSelectedLayers = new QAction("隐藏选中的图层", this);
    mActionHideSelectedLayers->setStatusTip("隐藏选中的图层");
    mActionHideSelectedLayers->setIcon(eqiApplication::getThemeIcon("mActionHideSelectedLayers.png"));
    connect( mActionHideSelectedLayers, SIGNAL( triggered() ), this, SLOT( hideSelectedLayers() ) );

    /*----------------------------------------------无人机数据管理-------------------------------------------*/
    mActionOpenPosFile = new QAction("载入曝光点文件", this);
    mActionOpenPosFile->setStatusTip("载入曝光点文件");
    mActionOpenPosFile->setIcon(eqiApplication::getThemeIcon("mActionCapturePoint.png"));
    connect( mActionOpenPosFile, SIGNAL( triggered() ), this, SLOT( openPosFile() ) );

    mActionPosTransform = new QAction("曝光点坐标转换", this);
    mActionPosTransform->setStatusTip("曝光点坐标转换");
    mActionPosTransform->setIcon(eqiApplication::getThemeIcon("mIconAtlas.svg"));
    connect( mActionPosTransform, SIGNAL( triggered() ), this, SLOT( posTransform() ) );

    mActionPosSketchMap = new QAction("创建航飞略图", this);
    mActionPosSketchMap->setStatusTip("创建航飞略图");
    mActionPosSketchMap->setIcon(eqiApplication::getThemeIcon("mActionCapturePolygon.png"));
    connect( mActionPosSketchMap, SIGNAL( triggered() ), this, SLOT( posSketchMap() ) );

    mActionPosSketchMapSwitch = new QAction("显示曝光点", this);
    mActionPosSketchMapSwitch->setStatusTip("切换显示曝光点与航飞略图。");
    mActionPosSketchMapSwitch->setIcon(eqiApplication::getThemeIcon("eqi/other/mActionSketchMapsurface.png"));
    connect( mActionPosSketchMapSwitch, SIGNAL( triggered() ), this, SLOT( posSketchMapSwitch() ) );

    mActionPPLinkPhoto = new QAction("PP动态联动", this);
    mActionPPLinkPhoto->setStatusTip("创建POS文件与photo相片之间的联动关系。");
    mActionPPLinkPhoto->setIcon(eqiApplication::getThemeIcon("mActionLink.svg"));
    connect( mActionPPLinkPhoto, SIGNAL( triggered() ), this, SLOT( posLinkPhoto() ) );

    mActionPosOneButton = new QAction("一键处理", this);
    mActionPosOneButton->setStatusTip("一键处理");
    mActionPosOneButton->setIcon(eqiApplication::getThemeIcon("mActionSelect.svg"));
    connect( mActionPosOneButton, SIGNAL( triggered() ), this, SLOT( posOneButton() ) );

    mActionPosExport = new QAction("导出曝光点文件", this);
    mActionPosExport->setStatusTip("导出曝光点文件");
    mActionPosExport->setIcon(eqiApplication::getThemeIcon("mActionSharingExport.svg"));
    connect( mActionPosExport, SIGNAL( triggered() ), this, SLOT( posExport() ) );

    mActionPosSetting = new QAction("参数设置", this);
    mActionPosSetting->setIcon(eqiApplication::getThemeIcon("propertyicons/settings.svg"));
    mActionPosSetting->setStatusTip("相机参数、POS一键处理设置。");
    connect( mActionPosSetting, SIGNAL( triggered() ), this, SLOT( posSetting() ));

    /*--------------------------------------------航摄检查---------------------------------------------*/
    mActionCheckOverlapping = new QAction("重叠度检查", this);
    mActionCheckOverlapping->setStatusTip("重叠度检查");
    mActionCheckOverlapping->setIcon(eqiApplication::getThemeIcon("mActionMoveItemsToTop.png"));
    connect( mActionCheckOverlapping, SIGNAL( triggered() ), this, SLOT( checkOverlapping() ) );

    mActionCheckOmega = new QAction("倾斜角检查", this);
    mActionCheckOmega = new QAction("倾斜角检查", this);
    mActionCheckOmega->setStatusTip("倾斜角检查");
    mActionCheckOmega->setIcon(eqiApplication::getThemeIcon("mActionInvertSelection.png"));
    connect( mActionCheckOmega, SIGNAL( triggered() ), this, SLOT( checkOmega() ));

    mActionCheckKappa = new QAction("旋片角检查", this);
    mActionCheckKappa->setStatusTip("旋片角检查");
    mActionCheckKappa->setIcon(eqiApplication::getThemeIcon("mActionNodeTool.png"));
    connect( mActionCheckKappa, SIGNAL( triggered() ), this, SLOT( checkKappa() ) );

    /*--------------------------------------------要素选择、航摄数据处理---------------------------------------------*/
    mActionSelectFeatures = new QAction("选择要素", this);
    mActionSelectFeatures->setStatusTip("选择要素");
    mActionSelectFeatures->setIcon(eqiApplication::getThemeIcon("mActionSelectRectangle.svg"));
    connect( mActionSelectFeatures, SIGNAL( triggered() ), this, SLOT( selectFeatures() ) );

    mActionSelectPolygon = new QAction("按多边形\n选择要素", this);
    mActionSelectPolygon->setStatusTip("按多边形选择要素");
    mActionSelectPolygon->setIcon(eqiApplication::getThemeIcon("mActionSelectPolygon.svg"));
    connect( mActionSelectPolygon, SIGNAL( triggered() ), this, SLOT( selectByPolygon() ) );

    mActionSelectFreehand = new QAction("自由手绘\n选择要素", this);
    mActionSelectFreehand->setStatusTip("自由手绘选择要素");
    mActionSelectFreehand->setIcon(eqiApplication::getThemeIcon("mActionSelectFreehand.svg"));
    connect( mActionSelectFreehand, SIGNAL( triggered() ), this, SLOT( selectByFreehand() ) );

    mActionDeselectAll = new QAction("取消选择要素", this);
    mActionDeselectAll->setShortcut(tr("Ctrl+Shift+A"));
    mActionDeselectAll->setStatusTip("在所有图层中取消选择要素");
    mActionDeselectAll->setIcon(eqiApplication::getThemeIcon("mActionDeselectAll.svg"));
    connect( mActionDeselectAll, SIGNAL( triggered() ), this, SLOT( deselectAll() ) );

    mActionSelectAll = new QAction("选择所有要素", this);
    mActionSelectAll->setShortcut(tr("Ctrl+A"));
    mActionSelectAll->setStatusTip("选择所有要素");
    mActionSelectAll->setIcon(eqiApplication::getThemeIcon("mActionSelectAll.svg"));
    connect( mActionSelectAll, SIGNAL( triggered() ), this, SLOT( selectAll() ) );

    mActionInvertSelection = new QAction("反选要素", this);
    mActionInvertSelection->setStatusTip("反选要素");
    mActionInvertSelection->setIcon(eqiApplication::getThemeIcon("mActionInvertSelection.svg"));
    connect( mActionInvertSelection, SIGNAL( triggered() ), this, SLOT( invertSelection() ) );

    mActionDelSelect = new QAction("删除航摄数据", this);
    mActionDelSelect->setStatusTip("将选择的略图、POS、相片数据删除，保持一套数据完整性。");
    mActionDelSelect->setIcon(eqiApplication::getThemeIcon("eqi/other/delSelect.png"));
    connect( mActionDelSelect, SIGNAL( triggered() ), this, SLOT( delSelect() ) );

    mActionSaveSelect = new QAction("保存航摄数据", this);
    mActionSaveSelect->setStatusTip("将选择的略图、POS、相片数据保存到指定路径中，保持一套数据完整性。");
    mActionSaveSelect->setIcon(eqiApplication::getThemeIcon("eqi/other/saveSelect.png"));
    connect( mActionSaveSelect, SIGNAL( triggered() ), this, SLOT( saveSelect() ) );

    mActionSelectSetting = new QAction("设置", this);
    mActionSelectSetting->setIcon(eqiApplication::getThemeIcon("propertyicons/settings.svg"));
    mActionSelectSetting->setStatusTip("删除、保存所选航飞数据的相关设置。");
    connect( mActionSelectSetting, SIGNAL( triggered() ), this, SLOT( selectSetting() ));

    /*----------------------------------------------坐标转换-------------------------------------------*/
    // 未实现
    mActionTextTranfrom = new QAction("坐标转换", this);
    mActionTextTranfrom->setIcon(eqiApplication::getThemeIcon("mActionShowPluginManager.svg"));
    mActionTextTranfrom->setStatusTip(" 坐标转换，仅支持文本格式。 ");

    // 未实现
    mActionDegreeMutual = new QAction("度<->度分秒", this);
    mActionDegreeMutual->setIcon(eqiApplication::getThemeIcon("mActionShowPluginManager.svg"));
    mActionDegreeMutual->setStatusTip(" 度与度分秒格式互转。 ");

    /*----------------------------------------------分幅管理-------------------------------------------*/
    // 未实现
    mActionCreateTK = new QAction("创建分幅图框", this);
    mActionCreateTK->setIcon(eqiApplication::getThemeIcon("eqi/other/mActionPtoTK.svg"));
    mActionCreateTK->setStatusTip("输入标准分幅图号创建图框。");
//    connect( mActionCreateTK, SIGNAL( triggered() ), this, SLOT() );

    mActionPtoTK = new QAction("根据坐标\n创建图框", this);
    mActionPtoTK->setIcon(eqiApplication::getThemeIcon("eqi/other/mActionPtoTK.svg"));
<<<<<<< HEAD
    mActionPtoTK->setStatusTip("从屏幕选取范围制作分幅图框。");
=======
    mActionPtoTK->setStatusTip("从屏幕选取或手动输入单点坐标制作图框，利用两个角点坐标可按范围制作图框。");
>>>>>>> 8408d22f60c6e2f266583bd79469a2a283413c43
    connect( mActionPtoTK, SIGNAL( triggered() ), this, SLOT( pointToTk()) );

    mActionTKtoXY = new QAction("输出图框\n坐标", this);
    mActionTKtoXY->setIcon(eqiApplication::getThemeIcon("eqi/other/mActionPtoTK.svg"));
    mActionTKtoXY->setStatusTip("选择一个包含标准图号的txt文本，计算并输出图框四个角点的投影坐标，并重新输出到txt文本中。");
    connect( mActionTKtoXY, SIGNAL( triggered() ), this, SLOT( TKtoXY() ) );

    mActionPtoTKSetting = new QAction("坐标参数设置", this);
    mActionPtoTKSetting->setIcon(eqiApplication::getThemeIcon("propertyicons/settings.svg"));
    mActionPtoTKSetting->setStatusTip("参照坐标系投影参数设置。");
    connect( mActionPtoTKSetting, SIGNAL( triggered() ), this, SLOT( prjtransformsetting() ) );

    /*----------------------------------------------数据管理-------------------------------------------*/
    mActionAddOgrLayer = new QAction("添加矢量图层...", this);
    mActionAddOgrLayer->setShortcut(tr("Ctrl+Shift+V"));
    mActionAddOgrLayer->setStatusTip("添加矢量图层...");
    mActionAddOgrLayer->setIcon(eqiApplication::getThemeIcon("eqi/1/mActionAddOgrLayer.png"));
    connect( mActionAddOgrLayer, SIGNAL( triggered() ), this, SLOT( addVectorLayer() ) );

    mActionLayerSaveAs = new QAction("另存为(&S)...", this);
    mActionLayerSaveAs->setStatusTip("另存为");
    mActionLayerSaveAs->setIcon(eqiApplication::getThemeIcon("eqi/1/mActionLayerSaveAs.png"));
    connect( mActionLayerSaveAs, SIGNAL( triggered() ), this, SLOT( saveAsFile() ) );
}

void MainWindow::initTabTools()
{
    // 初始化“地图浏览”tab
    tab_mapBrowsing *m_MB = new tab_mapBrowsing(this);
    m_MB->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_MB->addAction(mActionPan);
    m_MB->addAction(mActionZoomIn);
    m_MB->addAction(mActionZoomOut);
    m_MB->addAction(mActionZoomLast);
    m_MB->addAction(mActionZoomNext);
    m_MB->addSeparator(); //---
    m_MB->addAction(mActionZoomFullExtent);
    m_MB->addAction(mActionZoomToLayer);
    m_MB->addAction(mActionPanToSelected);
    m_MB->addAction(mActionZoomToSelected);
    m_MB->addAction(mActionZoomActualSize);
<<<<<<< HEAD
    m_MB->addSeparator(); //---
    m_MB->addAction(mActionDraw);
=======
    m_MB->addAction(mActionZoomToSelected);
    m_MB->addAction(mActionZoomToLayer);
    m_MB->addAction(mActionZoomLast);
//    m_MB->addAction(mActionZoomNext);
//    m_MB->addAction(mActionDraw);
>>>>>>> 8408d22f60c6e2f266583bd79469a2a283413c43
    m_MB->addAction(mActionIdentify);

    // 初始化“无人机数据管理”tab
    tab_uavDataManagement *m_uDM = new tab_uavDataManagement(this);
    m_uDM->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_uDM->addAction(mActionOpenPosFile);
    m_uDM->addAction(mActionPosTransform);
    m_uDM->addAction(mActionPosSketchMap);
    m_uDM->addAction(mActionPPLinkPhoto);
    m_uDM->addAction(mActionPosExport);
    m_uDM->addSeparator(); //---
    m_uDM->addAction(mActionPosOneButton);
    m_uDM->addSeparator(); //---
    m_uDM->addAction(mActionPosSketchMapSwitch);
    m_uDM->addSeparator(); //---
    m_uDM->addAction(mActionPosSetting);

    // 初始化“航摄检查”tab
    tab_checkAerialPhoto *m_CAP = new tab_checkAerialPhoto(this);
    m_CAP->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_CAP->addAction(mActionCheckOverlapping);
    m_CAP->addAction(mActionCheckOmega);
    m_CAP->addAction(mActionCheckKappa);


    // 初始化“要素选择、编辑”tab
    tab_selectFeatures *m_SF = new tab_selectFeatures(this);
    m_SF->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_SF->addAction(mActionSelectFeatures);
    m_SF->addAction(mActionSelectPolygon);
    m_SF->addAction(mActionSelectFreehand);
    m_SF->addSeparator(); //---
    m_SF->addAction(mActionDeselectAll);
    m_SF->addAction(mActionSelectAll);
    m_SF->addAction(mActionInvertSelection);
    m_SF->addSeparator(); //---
    m_SF->addAction(mActionDelSelect);
    m_SF->addAction(mActionSaveSelect);
    m_SF->addSeparator(); //---
    m_SF->addAction(mActionSelectSetting);

    // 初始化“坐标转换”tab
    tab_coordinateTransformation *m_tCTF = new tab_coordinateTransformation(this);
    m_tCTF->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_tCTF->addAction(mActionTextTranfrom);
    m_tCTF->addAction(mActionDegreeMutual);

    // 初始化“分幅管理”tab
    tab_fractalManagement *m_tFM = new tab_fractalManagement(this);
    m_tFM->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_tFM->addAction(mActionCreateTK);
    m_tFM->addAction(mActionPtoTK);
    m_tFM->addAction(mActionTKtoXY);
    m_tFM->addSeparator(); //---
    m_tFM->addAction(mActionPtoTKSetting);

    // 初始化“数据管理”tab
    tab_dataManagement *m_tDM = new tab_dataManagement(this);
    m_tDM->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_tDM->addAction(mActionAddOgrLayer);
    m_tDM->addAction(mActionLayerSaveAs);

    // 添加tab到窗口
    toolTab = new QTabWidget;
    toolTab->addTab(m_MB, " 地图浏览 ");
    toolTab->addTab(m_uDM, "航摄数据管理");
    toolTab->addTab(m_CAP, " 航摄检查 ");
    toolTab->addTab(m_SF, " 要素选择 ");
    toolTab->addTab(m_tCTF, "　坐标转换　");
    toolTab->addTab(m_tFM, "　分幅管理　");
    toolTab->addTab(m_tDM, "　数据管理　");
    ui->mainToolBar->addWidget(toolTab);
}

void MainWindow::initStatusBar()
{
    // 删除窗口以下的子边界
    ui->statusBar->setStyleSheet( "QStatusBar::item {border: none;}" );

    // 将比例尺、坐标系、进度条面板添加到状态栏，还将显示复选框
    mProgressBar = new QProgressBar( ui->statusBar );
    mProgressBar->setObjectName( "mProgressBar" );
    mProgressBar->setMaximumWidth( 100 );
    mProgressBar->hide();
    mProgressBar->setWhatsThis( "显示渲染等时间密集型操作的状态进度条" );
    ui->statusBar->addPermanentWidget( mProgressBar, 1 );

    //当画布即将被渲染，显示繁忙进度条
    connect( mMapCanvas, SIGNAL( renderStarting() ), this, SLOT( canvasRefreshStarted() ) );
    connect( mMapCanvas, SIGNAL( mapCanvasRefreshed() ), this, SLOT( canvasRefreshFinished() ) );

    //显示坐标控件
    mCoordsEdit = new QgsStatusBarCoordinatesWidget( ui->statusBar );
    mCoordsEdit->setMapCanvas( mMapCanvas );
    ui->statusBar->addPermanentWidget( mCoordsEdit, 0 );

    // 添加一个标签显示比例尺
    mScaleLabel = new QLabel( QString(), ui->statusBar );
    mScaleLabel->setObjectName( "mScaleLable" );
    mScaleLabel->setMinimumWidth( 10 );
    mScaleLabel->setMargin( 3 );
    mScaleLabel->setAlignment( Qt::AlignCenter );
    mScaleLabel->setFrameStyle( QFrame::NoFrame );
    mScaleLabel->setText( "比例尺" );
    mScaleLabel->setToolTip( "当前地图比例尺" );
    ui->statusBar->addPermanentWidget( mScaleLabel, 0 );

    mScaleEdit = new QgsScaleComboBox( ui->statusBar );
    mScaleEdit->setObjectName( "mScaleEdit" );
    mScaleEdit->setMinimumWidth( 10 );
    mScaleEdit->setContentsMargins( 0, 0, 0, 0 );
    mScaleEdit->setWhatsThis( "显示当前地图的比例尺" );
    mScaleEdit->setToolTip( "当前地图的比例尺" );

    ui->statusBar->addPermanentWidget( mScaleEdit, 0 );
    connect( mScaleEdit, SIGNAL( scaleChanged() ), this, SLOT( userScale() ) );


    if ( QgsMapCanvas::rotationEnabled() )
    {
        // 添加一个控件用于 显示/设置 当前角度
        mRotationLabel = new QLabel( QString(), ui->statusBar );
        mRotationLabel->setObjectName( "mRotationLabel" );
        mRotationLabel->setMinimumWidth( 10 );
        mRotationLabel->setMargin( 3 );
        mRotationLabel->setAlignment( Qt::AlignCenter );
        mRotationLabel->setFrameStyle( QFrame::NoFrame );
        mRotationLabel->setText( "旋转角度" );
        mRotationLabel->setToolTip( "当前地图沿顺时针方向旋转的度数" );
        ui->statusBar->addPermanentWidget( mRotationLabel, 0 );

        mRotationEdit = new QgsDoubleSpinBox( ui->statusBar );
        mRotationEdit->setObjectName( "mRotationEdit" );
        mRotationEdit->setClearValue( 0.0 );
        mRotationEdit->setKeyboardTracking( false );
        mRotationEdit->setMaximumWidth( 120 );
        mRotationEdit->setDecimals( 1 );
        mRotationEdit->setRange( -180.0, 180.0 );
        mRotationEdit->setWrapping( true );
        mRotationEdit->setSingleStep( 5.0 );
        mRotationEdit->setWhatsThis( "当前地图沿顺时针方向旋转的度数,它还允许编辑设置旋转角度");
        mRotationEdit->setToolTip( "当前地图沿顺时针方向旋转的度数" );
        ui->statusBar->addPermanentWidget( mRotationEdit, 0 );
        connect( mRotationEdit, SIGNAL( valueChanged( double ) ), this, SLOT( userRotation() ) );

        showRotation();
    }

    // 渲染控制
    mRenderSuppressionCBox = new QCheckBox( "渲染", ui->statusBar );
    mRenderSuppressionCBox->setObjectName( "mRenderSuppressionCBox" );
    mRenderSuppressionCBox->setChecked( true );
    mRenderSuppressionCBox->setWhatsThis( "选中时，地图图层渲染响应命令和其他活动。如果未选中，没有渲染完成。"
                                          "这使您可以在添加大量层和渲染之前标志他们。" );
    mRenderSuppressionCBox->setToolTip( "切换地图渲染" );
    ui->statusBar->addPermanentWidget( mRenderSuppressionCBox, 0 );

    // 参照坐标系
    mOnTheFlyProjectionStatusButton = new QToolButton( ui->statusBar );
    mOnTheFlyProjectionStatusButton->setAutoRaise( true );
    mOnTheFlyProjectionStatusButton->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
    mOnTheFlyProjectionStatusButton->setObjectName( "mOntheFlyProjectionStatusButton" );

    mOnTheFlyProjectionStatusButton->setMaximumHeight( mScaleLabel->height() );
    mOnTheFlyProjectionStatusButton->setIcon( eqiApplication::getThemeIcon( "mIconProjectionEnabled.png" ) );
    mOnTheFlyProjectionStatusButton->setWhatsThis( "此图标显示动态投影是否启用。单击该图标，"
                                                   "在弹出项目属性对话框中改变这个行为" );
    mOnTheFlyProjectionStatusButton->setToolTip( tr( "CRS 状态 - 点击打开坐标参照系对话框" ) );
    connect( mOnTheFlyProjectionStatusButton, SIGNAL( clicked() ),
        this, SLOT( projectPropertiesProjections() ) );//单击时属性对话框中调用
    ui->statusBar->addPermanentWidget( mOnTheFlyProjectionStatusButton, 0 );
    ui->statusBar->showMessage( "准备" );

    // 消息按钮
    mMessageButton = new QToolButton( ui->statusBar );
    mMessageButton->setAutoRaise( true );
    mMessageButton->setIcon( eqiApplication::getThemeIcon("mMessageLog.svg") );
    mMessageButton->setToolTip( tr( "Messages" ) );
    mMessageButton->setWhatsThis( tr( "Messages" ) );
    mMessageButton->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
    mMessageButton->setObjectName( "mMessageLogViewerButton" );
    mMessageButton->setMaximumHeight( mScaleLabel->height() );
    mMessageButton->setCheckable( true );
    ui->statusBar->addPermanentWidget( mMessageButton, 0 );
}

void MainWindow::initLayerTreeView()
{
    mLayerTreeView->setWhatsThis( "显示当前在地图画布上的所有图层。点击复选框来开启或关闭图层。在图层上双击来定制它的外观和设置其他属性。" );

    mLayerTreeDock = new QDockWidget( "图层面板", this );
    mLayerTreeDock->setObjectName( "Layers" );
    mLayerTreeDock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );

    QgsLayerTreeModel* model = new QgsLayerTreeModel( QgsProject::instance()->layerTreeRoot(), this );

    model->setFlag( QgsLayerTreeModel::AllowNodeReorder );
    model->setFlag( QgsLayerTreeModel::AllowNodeRename );
    model->setFlag( QgsLayerTreeModel::AllowNodeChangeVisibility );
    model->setFlag( QgsLayerTreeModel::ShowLegendAsTree );
    model->setAutoCollapseLegendNodes( 10 );
    QFont fontLayer, fontGroup;
    fontLayer.setBold( true );
    fontGroup.setBold( false );
    model->setLayerTreeNodeFont( QgsLayerTreeNode::NodeLayer, fontLayer );
    model->setLayerTreeNodeFont( QgsLayerTreeNode::NodeGroup, fontGroup );

    mLayerTreeView->setModel( model );

    // 添加右键菜单
    mLayerTreeView->setMenuProvider( new QgsAppLayerTreeViewMenuProvider( mLayerTreeView, mMapCanvas ) );

    connect( mLayerTreeView, SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( layerTreeViewDoubleClicked( QModelIndex ) ) );
    connect( mLayerTreeView, SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( activeLayerChanged( QgsMapLayer* ) ) );
    connect( mLayerTreeView->selectionModel(), SIGNAL( currentChanged( QModelIndex, QModelIndex ) ), this, SLOT( updateNewLayerInsertionPoint() ) );
    connect( QgsProject::instance()->layerTreeRegistryBridge(), SIGNAL( addedLayersToLayerTree( QList<QgsMapLayer*> ) ),
        this, SLOT( autoSelectAddedLayer( QList<QgsMapLayer*> ) ) );

    // 添加组合
    QAction* actionAddGroup = new QAction( "添加组合", this );
    actionAddGroup->setIcon( eqiApplication::getThemeIcon( "mActionAddGroup.svg" ) );
    actionAddGroup->setToolTip( "添加组合" );
    connect( actionAddGroup, SIGNAL( triggered( bool ) ), mLayerTreeView->defaultActions(), SLOT( addGroup() ) );

    // 图层可见性
    QToolButton* btnVisibilityPresets = new QToolButton;
    btnVisibilityPresets->setAutoRaise( true );
    btnVisibilityPresets->setToolTip( "管理图层可见性" );
    btnVisibilityPresets->setIcon( eqiApplication::getThemeIcon( "mActionShowAllLayers.svg" ) );
    btnVisibilityPresets->setPopupMode( QToolButton::InstantPopup );
    btnVisibilityPresets->setMenu( QgsVisibilityPresets::instance()->menu() );

    // 图例过滤器
    mActionFilterLegend = new QAction( "按地图内容过滤图层", this );
    mActionFilterLegend->setCheckable( true );
    mActionFilterLegend->setToolTip( tr( "通过地图内容过滤图层" ) );
    mActionFilterLegend->setIcon( eqiApplication::getThemeIcon( "mActionFilter2.svg" ) );
    connect( mActionFilterLegend, SIGNAL( toggled( bool ) ), this, SLOT( updateFilterLegend() ) );

    //mLegendExpressionFilterButton = new QgsLegendFilterButton( this );
    //mLegendExpressionFilterButton->setToolTip( tr( "Filter legend by expression" ) );
    //connect( mLegendExpressionFilterButton, SIGNAL( toggled( bool ) ), this, SLOT( toggleFilterLegendByExpression( bool ) ) );

    // expand / collapse tool buttons
    QAction* actionExpandAll = new QAction( "全部展开", this );
    actionExpandAll->setIcon( eqiApplication::getThemeIcon( "mActionExpandTree.svg" ) );
    actionExpandAll->setToolTip( "全部展开" );
    connect( actionExpandAll, SIGNAL( triggered( bool ) ), mLayerTreeView, SLOT( expandAll() ) );
    QAction* actionCollapseAll = new QAction( "全部折叠", this );
    actionCollapseAll->setIcon( eqiApplication::getThemeIcon( "mActionCollapseTree.svg" ) );
    actionCollapseAll->setToolTip( "全部折叠" );
    connect( actionCollapseAll, SIGNAL( triggered( bool ) ), mLayerTreeView, SLOT( collapseAll() ) );

    QToolBar* toolbar = new QToolBar();
    toolbar->setIconSize( QSize( 16, 16 ) );
    toolbar->addAction( actionAddGroup );
    toolbar->addWidget( btnVisibilityPresets );
    toolbar->addAction( mActionFilterLegend );
    //toolbar->addWidget( mLegendExpressionFilterButton );
    toolbar->addAction( actionExpandAll );
    toolbar->addAction( actionCollapseAll );
    toolbar->addAction( mActionRemoveLayer );

    QVBoxLayout* vboxLayout = new QVBoxLayout;
    vboxLayout->setMargin( 0 );
    vboxLayout->addWidget( toolbar );
    vboxLayout->addWidget( mLayerTreeView );

    QWidget* w = new QWidget;
    w->setLayout( vboxLayout );
    mLayerTreeDock->setWidget( w );
    addDockWidget( Qt::LeftDockWidgetArea, mLayerTreeDock );

    mLayerTreeCanvasBridge = new QgsLayerTreeMapCanvasBridge( QgsProject::instance()->layerTreeRoot(), mMapCanvas, this );
    connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), mLayerTreeCanvasBridge, SLOT( writeProject( QDomDocument& ) ) );
    connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), mLayerTreeCanvasBridge, SLOT( readProject( QDomDocument ) ) );

    bool otfTransformAutoEnable = QSettings().value( "/Projections/otfTransformAutoEnable", true ).toBool();
    mLayerTreeCanvasBridge->setAutoEnableCrsTransform( otfTransformAutoEnable );

    mMapLayerOrder = new QgsCustomLayerOrderWidget( mLayerTreeCanvasBridge, this );
    mMapLayerOrder->setObjectName( "theMapLayerOrder" );

    mMapLayerOrder->setWhatsThis( "所有显示图层在地图绘制顺序." );
    mLayerOrderDock = new QDockWidget( "图层顺序面板", this );
    mLayerOrderDock->setObjectName( "LayerOrder" );
    mLayerOrderDock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );

    mLayerOrderDock->setWidget( mMapLayerOrder );
    addDockWidget( Qt::LeftDockWidgetArea, mLayerOrderDock );
    mLayerOrderDock->hide();

    connect( mMapCanvas, SIGNAL( mapCanvasRefreshed() ), this, SLOT( updateFilterLegend() ) );
}

void MainWindow::createCanvasTools()
{
    // create tools
    mMapTools.mZoomIn = new QgsMapToolZoom( mMapCanvas, false /* zoomIn */ );
    mMapTools.mZoomIn->setAction( mActionZoomIn );
    mMapTools.mZoomOut = new QgsMapToolZoom( mMapCanvas, true /* zoomOut */ );
    mMapTools.mZoomOut->setAction( mActionZoomOut );
    mMapTools.mPan = new QgsMapToolPan( mMapCanvas );
    mMapTools.mPan->setAction( mActionPan );
//    mMapTools.mIdentify = new QgsMapToolIdentifyAction( mMapCanvas );
//    mMapTools.mIdentify->setAction( mActionIdentify );
    //connect( mMapTools.mIdentify, SIGNAL( copyToClipboard( QgsFeatureStore & ) ),
    //	this, SLOT( copyFeatures( QgsFeatureStore & ) ) );
    //mMapTools.mFeatureAction = new QgsMapToolFeatureAction( mMapCanvas );
    //mMapTools.mFeatureAction->setAction( mActionFeatureAction );
    mMapTools.mMeasureDist = new QgsMeasureTool( mMapCanvas, false /* area */ );
//    mMapTools.mMeasureDist->setAction( mActionMeasure );
    mMapTools.mMeasureArea = new QgsMeasureTool( mMapCanvas, true /* area */ );
//    mMapTools.mMeasureArea->setAction( mActionMeasureArea );
    //mMapTools.mMeasureAngle = new QgsMapToolMeasureAngle( mMapCanvas );
    //mMapTools.mMeasureAngle->setAction( mActionMeasureAngle );
//	mMapTools.mTextAnnotation = new QgsMapToolTextAnnotation( mMapCanvas );
//	mMapTools.mTextAnnotation->setAction( mActionTextAnnotation );
//	mMapTools.mFormAnnotation = new QgsMapToolFormAnnotation( mMapCanvas );
//	mMapTools.mFormAnnotation->setAction( mActionFormAnnotation );
//	mMapTools.mHtmlAnnotation = new QgsMapToolHtmlAnnotation( mMapCanvas );
//	mMapTools.mHtmlAnnotation->setAction( mActionHtmlAnnotation );
//	mMapTools.mSvgAnnotation = new QgsMapToolSvgAnnotation( mMapCanvas );
//	mMapTools.mSvgAnnotation->setAction( mActionSvgAnnotation );
//	mMapTools.mAnnotation = new QgsMapToolAnnotation( mMapCanvas );
//	mMapTools.mAnnotation->setAction( mActionAnnotation );
//	mMapTools.mAddFeature = new QgsMapToolAddFeature( mMapCanvas );
//	mMapTools.mAddFeature->setAction( mActionAddFeature );
//	mMapTools.mCircularStringCurvePoint = new QgsMapToolCircularStringCurvePoint( dynamic_cast<QgsMapToolAddFeature*>( mMapTools.mAddFeature ), mMapCanvas );
//	mMapTools.mCircularStringCurvePoint->setAction( mActionCircularStringCurvePoint );
//	mMapTools.mCircularStringRadius = new QgsMapToolCircularStringRadius( dynamic_cast<QgsMapToolAddFeature*>( mMapTools.mAddFeature ), mMapCanvas );
//	mMapTools.mCircularStringRadius->setAction( mActionCircularStringRadius );
//	mMapTools.mMoveFeature = new QgsMapToolMoveFeature( mMapCanvas );
//	mMapTools.mMoveFeature->setAction( mActionMoveFeature );
//	mMapTools.mRotateFeature = new QgsMapToolRotateFeature( mMapCanvas );
//	mMapTools.mRotateFeature->setAction( mActionRotateFeature );
//	//need at least geos 3.3 for OffsetCurve tool
//#if defined(GEOS_VERSION_MAJOR) && defined(GEOS_VERSION_MINOR) && \
//	((GEOS_VERSION_MAJOR>3) || ((GEOS_VERSION_MAJOR==3) && (GEOS_VERSION_MINOR>=3)))
//	mMapTools.mOffsetCurve = new QgsMapToolOffsetCurve( mMapCanvas );
//	mMapTools.mOffsetCurve->setAction( mActionOffsetCurve );
//#else
//	mAdvancedDigitizeToolBar->removeAction( mActionOffsetCurve );
//	mEditMenu->removeAction( mActionOffsetCurve );
//	mMapTools.mOffsetCurve = 0;
//#endif //GEOS_VERSION
//	mMapTools.mReshapeFeatures = new QgsMapToolReshape( mMapCanvas );
//	mMapTools.mReshapeFeatures->setAction( mActionReshapeFeatures );
//	mMapTools.mSplitFeatures = new QgsMapToolSplitFeatures( mMapCanvas );
//	mMapTools.mSplitFeatures->setAction( mActionSplitFeatures );
//	mMapTools.mSplitParts = new QgsMapToolSplitParts( mMapCanvas );
//	mMapTools.mSplitParts->setAction( mActionSplitParts );
    mMapTools.mSelectFeatures = new QgsMapToolSelectFeatures( mMapCanvas );
    mMapTools.mSelectFeatures->setAction( mActionSelectFeatures );
    mMapTools.mSelectPolygon = new QgsMapToolSelectPolygon( mMapCanvas );
    mMapTools.mSelectPolygon->setAction( mActionSelectPolygon );
    mMapTools.mSelectFreehand = new QgsMapToolSelectFreehand( mMapCanvas );
    mMapTools.mSelectFreehand->setAction( mActionSelectFreehand );
//	mMapTools.mSelectRadius = new QgsMapToolSelectRadius( mMapCanvas );
//	mMapTools.mSelectRadius->setAction( mActionSelectRadius );
//	mMapTools.mAddRing = new QgsMapToolAddRing( mMapCanvas );
//	mMapTools.mAddRing->setAction( mActionAddRing );
//	mMapTools.mFillRing = new QgsMapToolFillRing( mMapCanvas );
//	mMapTools.mFillRing->setAction( mActionFillRing );
//	mMapTools.mAddPart = new QgsMapToolAddPart( mMapCanvas );
//	mMapTools.mAddPart->setAction( mActionAddPart );
//	mMapTools.mSimplifyFeature = new QgsMapToolSimplify( mMapCanvas );
//	mMapTools.mSimplifyFeature->setAction( mActionSimplifyFeature );
//	mMapTools.mDeleteRing = new QgsMapToolDeleteRing( mMapCanvas );
//	mMapTools.mDeleteRing->setAction( mActionDeleteRing );
//	mMapTools.mDeletePart = new QgsMapToolDeletePart( mMapCanvas );
//	mMapTools.mDeletePart->setAction( mActionDeletePart );
//	mMapTools.mNodeTool = new QgsMapToolNodeTool( mMapCanvas );
//	mMapTools.mNodeTool->setAction( mActionNodeTool );
//	mMapTools.mRotatePointSymbolsTool = new QgsMapToolRotatePointSymbols( mMapCanvas );
//	mMapTools.mRotatePointSymbolsTool->setAction( mActionRotatePointSymbols );
//	mMapTools.mPinLabels = new QgsMapToolPinLabels( mMapCanvas );
//	mMapTools.mPinLabels->setAction( mActionPinLabels );
//	mMapTools.mShowHideLabels = new QgsMapToolShowHideLabels( mMapCanvas );
//	mMapTools.mShowHideLabels->setAction( mActionShowHideLabels );
//	mMapTools.mMoveLabel = new QgsMapToolMoveLabel( mMapCanvas );
//	mMapTools.mMoveLabel->setAction( mActionMoveLabel );
//
//	mMapTools.mRotateLabel = new QgsMapToolRotateLabel( mMapCanvas );
//	mMapTools.mRotateLabel->setAction( mActionRotateLabel );
//	mMapTools.mChangeLabelProperties = new QgsMapToolChangeLabelProperties( mMapCanvas );
//	mMapTools.mChangeLabelProperties->setAction( mActionChangeLabelProperties );
    //ensure that non edit tool is initialized or we will get crashes in some situations
    mNonEditMapTool = mMapTools.mPan;
}

void MainWindow::askUserForOGRSublayers(QgsVectorLayer *layer)
{
    if ( !layer )
    {
        layer = qobject_cast<QgsVectorLayer *>( activeLayer() );
        if ( !layer || layer->dataProvider()->name() != "ogr" )
            return;
    }

    QStringList sublayers = layer->dataProvider()->subLayers();
    QString layertype = layer->dataProvider()->storageType();

    // 我们初始化一个选择对话框并显示它
    QgsSublayersDialog chooseSublayersDialog( QgsSublayersDialog::Ogr, "ogr", this );
    chooseSublayersDialog.populateLayerTable( sublayers );

    if ( chooseSublayersDialog.exec() )
    {
        QString uri = layer->source();
        //the separator char & was changed to | to be compatible
        //with url for protocol drivers
        if ( uri.contains( '|', Qt::CaseSensitive ) )
        {
            // If we get here, there are some options added to the filename.
            // A valid uri is of the form: filename&option1=value1&option2=value2,...
            // We want only the filename here, so we get the first part of the split.
            QStringList theURIParts = uri.split( '|' );
            uri = theURIParts.at( 0 );
        }
        QgsDebugMsg( "Layer type " + layertype );
        // the user has done his choice
        loadOGRSublayers( layertype, uri, chooseSublayersDialog.selectionNames() );
    }
}

void MainWindow::saveAsVectorFileGeneral(QgsVectorLayer *vlayer, bool symbologyOption)
{
    if ( !vlayer )
    {
        vlayer = qobject_cast<QgsVectorLayer *>( activeLayer() ); // FIXME: output of multiple layers at once?
    }

    if ( !vlayer )
        return;

    QgsCoordinateReferenceSystem destCRS;

    int options = QgsVectorLayerSaveAsDialog::AllOptions;
    if ( !symbologyOption )
    {
        options &= ~QgsVectorLayerSaveAsDialog::Symbology;
    }

    QgsVectorLayerSaveAsDialog *dialog = new QgsVectorLayerSaveAsDialog( vlayer->crs().srsid(), vlayer->extent(), vlayer->selectedFeatureCount() != 0, options, this );

    dialog->setCanvasExtent( mMapCanvas->mapSettings().visibleExtent(), mMapCanvas->mapSettings().destinationCrs() );
    dialog->setIncludeZ( QgsWKBTypes::hasZ( QGis::fromOldWkbType( vlayer->wkbType() ) ) );

    if ( dialog->exec() == QDialog::Accepted )
    {
        QString encoding = dialog->encoding();
        QString vectorFilename = dialog->filename();
        QString format = dialog->format();
        QStringList datasourceOptions = dialog->datasourceOptions();
        bool autoGeometryType = dialog->automaticGeometryType();
        QgsWKBTypes::Type forcedGeometryType = dialog->geometryType();

        QgsCoordinateTransform* ct = nullptr;
        destCRS = QgsCoordinateReferenceSystem( dialog->crs(), QgsCoordinateReferenceSystem::InternalCrsId );

        if ( destCRS.isValid() && destCRS != vlayer->crs() )
        {
            ct = new QgsCoordinateTransform( vlayer->crs(), destCRS );

            //ask user about datum transformation
            QSettings settings;
            QList< QList< int > > dt = QgsCoordinateTransform::datumTransformations( vlayer->crs(), destCRS );
            if ( dt.size() > 1 && settings.value( "/Projections/showDatumTransformDialog", false ).toBool() )
            {
                QgsDatumTransformDialog d( vlayer->name(), dt );
                if ( d.exec() == QDialog::Accepted )
                {
                    QList< int > sdt = d.selectedDatumTransform();
                    if ( !sdt.isEmpty() )
                    {
                        ct->setSourceDatumTransform( sdt.at( 0 ) );
                    }
                    if ( sdt.size() > 1 )
                    {
                        ct->setDestinationDatumTransform( sdt.at( 1 ) );
                    }
                    ct->initialise();
                }
            }
        }

        // ok if the file existed it should be deleted now so we can continue...
        QApplication::setOverrideCursor( Qt::WaitCursor );

        QgsVectorFileWriter::WriterError error;
        QString errorMessage;
        QString newFilename;
        QgsRectangle filterExtent = dialog->filterExtent();
        error = QgsVectorFileWriter::writeAsVectorFormat(
            vlayer, vectorFilename, encoding, ct, format,
            dialog->onlySelected(),
            &errorMessage,
            datasourceOptions, dialog->layerOptions(),
            dialog->skipAttributeCreation(),
            &newFilename,
            static_cast< QgsVectorFileWriter::SymbologyExport >( dialog->symbologyExport() ),
            dialog->scaleDenominator(),
            dialog->hasFilterExtent() ? &filterExtent : nullptr,
            autoGeometryType ? QgsWKBTypes::Unknown : forcedGeometryType,
            dialog->forceMulti(),
            dialog->includeZ()
            );

        delete ct;

        QApplication::restoreOverrideCursor();

        if ( error == QgsVectorFileWriter::NoError )
        {
            if ( dialog->addToCanvas() )
            {
                addVectorLayers( QStringList( newFilename ), encoding, "file" );
            }
            emit layerSavedAs( vlayer, vectorFilename );
            messageBar()->pushMessage( tr( "Saving done" ),
                tr( "Export to vector file has been completed" ),
                QgsMessageBar::INFO, messageTimeout() );
        }
        else
        {
            QgsMessageViewer *m = new QgsMessageViewer( nullptr );
            m->setWindowTitle( tr( "Save error" ) );
            m->setMessageAsPlainText( tr( "Export to vector file failed.\nError: %1" ).arg( errorMessage ) );
            m->exec();
        }
    }

    delete dialog;
}

void MainWindow::upDataPosActions()
{
    if ( pPosdp->isValid() )
    {
        mActionPosTransform->setEnabled(true);
        mActionPosSketchMap->setEnabled(true);
        mActionPosOneButton->setEnabled(true);
        mActionPPLinkPhoto->setEnabled(true);
        mActionPosExport->setEnabled(true);
    }
    else
    {
        mActionPosTransform->setEnabled(false);
        mActionPosSketchMap->setEnabled(false);
        mActionPosOneButton->setEnabled(false);
        mActionPPLinkPhoto->setEnabled(false);
        mActionPosExport->setEnabled(false);
    }
}

//void MainWindow::initMenus()
//{
//    // Panel and Toolbar Submenus
//    mPanelMenu = new QMenu( tr( "Panels" ), this );
//    mPanelMenu->setObjectName( "mPanelMenu" );
//    mToolbarMenu = new QMenu( tr( "Toolbars" ), this );
//    mToolbarMenu->setObjectName( "mToolbarMenu" );

//    //! 视图菜单
//    ui.mViewMenu->addAction(mActionPan);
//    ui.mViewMenu->addAction(mActionPanToSelected);
//    ui.mViewMenu->addAction(mActionZoomIn);
//    ui.mViewMenu->addAction(mActionZoomOut);
//    ui.mViewMenu->addSeparator();

//    QMenu *menuSelect = ui.mViewMenu->addMenu("选择");
//    menuSelect->addAction(mActionSelectFeatures);
//    menuSelect->addAction(mActionSelectPolygon);
//    menuSelect->addAction(mActionDeselectAll);
//    menuSelect->addAction(mActionSelectAll);
//    menuSelect->addAction(mActionInvertSelection);

//    ui.mViewMenu->addAction(mActionIdentify);

//    QMenu *menuMeasure = ui.mViewMenu->addMenu("测量");
//    menuMeasure->addAction(mActionMeasure);
//    menuMeasure->addAction(mActionMeasureArea);

//    ui.mViewMenu->addAction(mActionStatisticalSummary);
//    ui.mViewMenu->addSeparator();
//    ui.mViewMenu->addAction(mActionZoomFullExtent);
//    ui.mViewMenu->addAction(mActionZoomToLayer);
//    ui.mViewMenu->addAction(mActionZoomToSelected);
//    ui.mViewMenu->addAction(mActionZoomLast);
//    ui.mViewMenu->addAction(mActionZoomNext);
//    ui.mViewMenu->addAction(mActionZoomActualSize);
//    ui.mViewMenu->addSeparator();
//    ui.mViewMenu->addAction(mActionMapTips);
//    ui.mViewMenu->addAction(mActionDraw);

//    //! 图层菜单
//    QMenu *mNewLayerMenu = ui.mLayerMenu->addMenu("创建图层");
//    mNewLayerMenu->addAction(mActionNewVectorLayer);

//    QMenu *mAddLayerMenu = ui.mLayerMenu->addMenu("添加图层");
//    mAddLayerMenu->addAction(mActionAddOgrLayer);
//    mAddLayerMenu->addAction(mActionAddRasterLayer);
//    mAddLayerMenu->addAction(mActionAddDelimitedText);

//    ui.mLayerMenu->addSeparator();
//    ui.mLayerMenu->addAction(mActionCopyStyle);
//    ui.mLayerMenu->addAction(mActionPasteStyle);
//    ui.mLayerMenu->addSeparator();
//    ui.mLayerMenu->addAction(mActionOpenTable);
//    ui.mLayerMenu->addSeparator();
//    ui.mLayerMenu->addAction(mActionLayerSaveAs);
//    ui.mLayerMenu->addAction(mActionRemoveLayer);
//    ui.mLayerMenu->addAction(mActionSetLayerCRS);
//    ui.mLayerMenu->addAction(mActionSetProjectCRSFromLayer);
//    ui.mLayerMenu->addAction(mActionLayerProperties);
//    ui.mLayerMenu->addAction(mActionLayerSubsetString);
//    ui.mLayerMenu->addAction(mActionLabeling);
//    ui.mLayerMenu->addSeparator();
//    ui.mLayerMenu->addAction(mActionAddToOverview);
//    ui.mLayerMenu->addAction(mActionAddAllToOverview);
//    ui.mLayerMenu->addAction(mActionRemoveAllFromOverview);
//    ui.mLayerMenu->addSeparator();
//    ui.mLayerMenu->addAction(mActionShowAllLayers);
//    ui.mLayerMenu->addAction(mActionHideAllLayers);
//    ui.mLayerMenu->addAction(mActionShowSelectedLayers);
//    ui.mLayerMenu->addAction(mActionHideSelectedLayers);

//    mToolbarMenu = new QMenu( "工具栏", this );
//    mToolbarMenu->setObjectName( "mToolbarMenu" );
//}
/*
void MainWindow::initOverview()
{
    // overview canvas
    mOverviewCanvas = new QgsMapOverviewCanvas( nullptr, mMapCanvas );

    //set canvas color to default
    QSettings settings;
    int red = settings.value( "/eqi/default_canvas_color_red", 255 ).toInt();
    int green = settings.value( "/eqi/default_canvas_color_green", 255 ).toInt();
    int blue = settings.value( "/eqi/default_canvas_color_blue", 255 ).toInt();
    mOverviewCanvas->setBackgroundColor( QColor( red, green, blue ) );

    mOverviewCanvas->setWhatsThis( "鹰眼地图画布。此画布可以用来显示一个地图定位器，"
                                   "显示在地图上画布的当前程度。当前范围被示为红色矩形。"
                                   "地图上的任何层都可以被添加到鹰眼画布。" );

//    mOverviewMapCursor = new QCursor( Qt::OpenHandCursor );
//    mOverviewCanvas->setCursor( *mOverviewMapCursor );
    mOverviewDock = new QDockWidget( "鹰眼图面板", this );
    mOverviewDock->setObjectName( "Overview" );
    mOverviewDock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    mOverviewDock->setWidget( mOverviewCanvas );
    mOverviewDock->hide();
    addDockWidget( Qt::LeftDockWidgetArea, mOverviewDock );
    // add to the Panel submenu
//    mPanelMenu->addAction( mOverviewDock->toggleViewAction() );

    mMapCanvas->enableOverviewMode( mOverviewCanvas );

    // 移动到这里以将反锯齿设置为地图画布和概览
    // QGIS 1.7 默认情况下启用了抗锯齿
    mMapCanvas->enableAntiAliasing( settings.value( "/eqi/enable_anti_aliasing", true ).toBool() );

    int action = settings.value( "/eqi/wheel_action", 2 ).toInt();
    double zoomFactor = settings.value( "/eqi/zoom_factor", 2 ).toDouble();
    mMapCanvas->setWheelAction( static_cast< QgsMapCanvas::WheelAction >( action ), zoomFactor );

    mMapCanvas->setCachingEnabled( settings.value( "/eqi/enable_render_caching", true ).toBool() );

    mMapCanvas->setParallelRenderingEnabled( settings.value( "/eqi/parallel_rendering", false ).toBool() );

    mMapCanvas->setMapUpdateInterval( settings.value( "/eqi/map_update_interval", 250 ).toInt() );
}
*/
void MainWindow::showRotation()
{
    // 更新当前状态栏的旋转标签。
    double myrotation = mMapCanvas->rotation();
    mRotationEdit->setValue( myrotation );
}

void MainWindow::displayMessage(const QString &title, const QString &message, QgsMessageBar::MessageLevel level)
{
    messageBar()->pushMessage( title, message, level, messageTimeout() );
}

void MainWindow::setAppStyleSheet(const QString &stylesheet)
{
    setStyleSheet( stylesheet );
}

void MainWindow::toggleLogMessageIcon(bool hasLogMessage)
{
    if ( hasLogMessage && !mLogDock->isVisible() )
    {
        mMessageButton->setIcon( eqiApplication::getThemeIcon( "mMessageLog.svg" ) );
    }
    else
    {
        mMessageButton->setIcon( eqiApplication::getThemeIcon( "mMessageLogRead.svg" ) );
    }
}

void MainWindow::measure()
{
    mMapCanvas->setMapTool( mMapTools.mMeasureDist );
}

void MainWindow::measureArea()
{
    mMapCanvas->setMapTool( mMapTools.mMeasureArea );
}

void MainWindow::identify()
{
    mMapCanvas->setMapTool( mMapTools.mIdentify );
}

void MainWindow::saveAsFile()
{
    QgsMapLayer* layer = activeLayer();
    if ( !layer )
        return;

    QgsMapLayer::LayerType layerType = layer->type();
    if ( layerType == QgsMapLayer::RasterLayer )
    {
        //saveAsRasterFile();
    }
    else if ( layerType == QgsMapLayer::VectorLayer )
    {
        saveAsVectorFileGeneral();
    }
}

void MainWindow::selectFeatures()
{
    mMapCanvas->setMapTool( mMapTools.mSelectFeatures );
}

void MainWindow::selectByPolygon()
{
    mMapCanvas->setMapTool( mMapTools.mSelectPolygon );
}

void MainWindow::selectByFreehand()
{
    mMapCanvas->setMapTool( mMapTools.mSelectFreehand );
}

void MainWindow::deselectAll()
{
    // 关闭渲染以提高速度
    bool renderFlagState = mMapCanvas->renderFlag();
    if ( renderFlagState )
        mMapCanvas->setRenderFlag( false );

    QMap<QString, QgsMapLayer*> layers = QgsMapLayerRegistry::instance()->mapLayers();
    for ( QMap<QString, QgsMapLayer*>::iterator it = layers.begin(); it != layers.end(); ++it )
    {
        QgsVectorLayer *vl = qobject_cast<QgsVectorLayer *>( it.value() );
        if ( !vl )
            continue;

        vl->removeSelection();
    }

    // 打开渲染
    if ( renderFlagState )
        mMapCanvas->setRenderFlag( true );
}

void MainWindow::selectAll()
{
    QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( mMapCanvas->currentLayer() );
    if ( !vlayer )
    {
        messageBar()->pushMessage( "没有活动的矢量图层", "要选择所有，请在图层管理器中选择一个矢量图层",
                                   QgsMessageBar::INFO, messageTimeout() );
        return;
    }

    // 关闭渲染以提高速度
    bool renderFlagState = mMapCanvas->renderFlag();
    if ( renderFlagState )
        mMapCanvas->setRenderFlag( false );

    vlayer->selectAll();

    // 打开渲染
    if ( renderFlagState )
        mMapCanvas->setRenderFlag( true );
}

void MainWindow::invertSelection()
{
    QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( mMapCanvas->currentLayer() );
    if ( !vlayer )
    {
        messageBar()->pushMessage( "没有活动的矢量图层",
                                   "要反向选择，请在图层管理器中选择一个矢量图层",
                                   QgsMessageBar::INFO, messageTimeout() );
        return;
    }

    // 关闭渲染以提高速度
    bool renderFlagState = mMapCanvas->renderFlag();
    if ( renderFlagState )
        mMapCanvas->setRenderFlag( false );

    vlayer->invertSelection();

    // 打开渲染
    if ( renderFlagState )
        mMapCanvas->setRenderFlag( true );
}

void MainWindow::delSelect()
{
    QString delPath;
    int iDelete = mSettings.value("/eqi/options/selectEdit/delete", DELETE_DIR).toInt();

    if (iDelete == DELETE_DIR) // 移动到临时文件夹中
    {
        QString autoPath = mSettings.value("/eqi/pos/lePosFile", "").toString();
        autoPath = QFileInfo(autoPath).path();

        QDateTime current_date_time = QDateTime::currentDateTime();
        QString current_date = current_date_time.toString("yyyy-MM-dd");
        autoPath = QString("%1/删除的相片-%3").arg(autoPath).arg(current_date);

        if (!QDir(autoPath).exists())
        {
            QDir dir;
            if ( !dir.mkpath(autoPath) )
            {
                QgsMessageLog::logMessage(QString("删除航摄数据 : \t自动创建文件夹失败，请手动指定。"));
                return;
            }
        }

        delPath = autoPath;
    }

    if (pPPInter)
    {
        QgsMessageLog::logMessage("\n");
        pPPInter->delSelect(delPath);
    }
}

void MainWindow::saveSelect()
{
    QString savePath;
    int iSave = mSettings.value("/eqi/options/selectEdit/save", SAVE_TEMPDIR).toInt();

    if (iSave == SAVE_CUSTOMIZE) // 选择文件夹
    {
        // 读取保存的上一次路径
        QString strListPath = mSettings.value( "/eqi/pos/pathName", QDir::homePath()).toString();

        //浏览文件夹
        QString dir = QFileDialog::getExistingDirectory(this, "选择文件夹", strListPath,
                                                        QFileDialog::ShowDirsOnly |
                                                        QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty())
        {
            mSettings.setValue("/eqi/pos/pathName", dir);
            savePath = dir;
        }
    }
    else // 自动创建临时文件夹
    {
        QString autoPath = mSettings.value("/eqi/pos/lePosFile", "").toString();
        autoPath = QFileInfo(autoPath).path();

        QDateTime current_date_time = QDateTime::currentDateTime();
        QString current_date = current_date_time.toString("yyyy-MM-dd-hh-mm");
        autoPath = QString("%1/航摄数据-%3").arg(autoPath).arg(current_date);

        if (!QDir(autoPath).exists())
        {
            QDir dir;
            if ( !dir.mkpath(autoPath) )
            {
                QgsMessageLog::logMessage(QString("保存航摄数据 : \t自动创建文件夹失败，请手动指定。"));
                return;
            }
        }

        savePath = autoPath;
    }

    // 检查
    if (savePath.isEmpty())
    {
        return;
    }
    if (!QDir(savePath).exists())
    {
        messageBar()->pushMessage( "保存航摄数据",
                                   "选择的是一个无效文件夹路径。",
                                   QgsMessageBar::INFO, messageTimeout() );
        return;
    }

    // 保存数据
    if (pPPInter)
    {
        QgsMessageLog::logMessage("\n");

        // 保存略图与相片
        pPPInter->saveSelect(savePath);
    }
}

void MainWindow::selectSetting()
{
    dialog_selectSetting *selectDialog = new dialog_selectSetting(this);
    selectDialog->exec();
}

void MainWindow::removeLayer()
{
    if ( !mLayerTreeView )
    {
        return;
    }

    foreach ( QgsMapLayer * layer, mLayerTreeView->selectedLayers() )
    {
        QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer*>( layer );
//        if ( vlayer && vlayer->isEditable() && !toggleEditing( vlayer, true ) )
//            return;
        if ( vlayer && vlayer->isEditable() )
            return;
    }

    QList<QgsLayerTreeNode*> selectedNodes = mLayerTreeView->selectedNodes( true );

    //validate selection
    if ( selectedNodes.isEmpty() )
    {
        messageBar()->pushMessage( tr( "No legend entries selected" ),
            tr( "Select the layers and groups you want to remove in the legend." ),
            QgsMessageBar::INFO, messageTimeout() );
        return;
    }

    bool promptConfirmation = QSettings().value( "eqi/askToDeleteLayers", true ).toBool();
    //display a warning
    if ( promptConfirmation && QMessageBox::warning( this, tr( "Remove layers and groups" ),
                                                     tr( "Remove %n legend entries?",
                                                         "number of legend items to remove",
                                                     selectedNodes.count() ),
                                                     QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Cancel )
    {
        return;
    }

    Q_FOREACH ( QgsLayerTreeNode* node, selectedNodes )
    {
        QgsLayerTreeGroup* parentGroup = qobject_cast<QgsLayerTreeGroup*>( node->parent() );
        if ( parentGroup )
            parentGroup->removeChildNode( node );
    }

    showStatusMessage( tr( "%n legend entries removed.", "number of removed legend entries", selectedNodes.count() ) );

    mMapCanvas->refresh();
}

void MainWindow::showAllLayers()
{
    foreach ( QgsLayerTreeLayer* nodeLayer, mLayerTreeView->layerTreeModel()->rootGroup()->findLayers() )
        nodeLayer->setVisible( Qt::Checked );
}

void MainWindow::hideAllLayers()
{
    foreach ( QgsLayerTreeLayer* nodeLayer, mLayerTreeView->layerTreeModel()->rootGroup()->findLayers() )
        nodeLayer->setVisible( Qt::Unchecked );
}

void MainWindow::showSelectedLayers()
{
    foreach ( QgsLayerTreeNode* node, mLayerTreeView->selectedNodes() )
    {
        if ( QgsLayerTree::isGroup( node ) )
            QgsLayerTree::toGroup( node )->setVisible( Qt::Checked );
        else if ( QgsLayerTree::isLayer( node ) )
            QgsLayerTree::toLayer( node )->setVisible( Qt::Checked );
    }
}

void MainWindow::hideSelectedLayers()
{
    foreach ( QgsLayerTreeNode* node, mLayerTreeView->selectedNodes() )
    {
        if ( QgsLayerTree::isGroup( node ) )
            QgsLayerTree::toGroup( node )->setVisible( Qt::Unchecked );
        else if ( QgsLayerTree::isLayer( node ) )
            QgsLayerTree::toLayer( node )->setVisible( Qt::Unchecked );
    }
}

void MainWindow::canvasRefreshStarted()
{
    showProgress( -1, 0 ); // 使进度条显示繁忙指示器
}

void MainWindow::canvasRefreshFinished()
{
    showProgress( 0, 0 ); // 停止忙碌指示
}

void MainWindow::showProgress(int theProgress, int theTotalSteps)
{
    if ( theProgress == theTotalSteps )
    {
        mProgressBar->reset();
        mProgressBar->hide();
    }
    else
    {
        // 只有调用show如果尚未隐藏，以减少闪烁
        if ( !mProgressBar->isVisible() )
        {
            mProgressBar->show();
        }
        mProgressBar->setMaximum( theTotalSteps );
        mProgressBar->setValue( theProgress );

        if ( mProgressBar->maximum() == 0 )
        {
            // 繁忙指示灯（当最小等于最大值）Qt的风格（在KDE中）有一些问题，并没有启动的忙指示动画。
            // 这是一个简单的修复，迫使以某种方式继续动画临时进度条的创建。
            // 注意：在看下面的代码可以在胃中引入轻度疼痛。
            if ( strcmp( QApplication::style()->metaObject()->className(), "Oxygen::Style" ) == 0 )
            {
                QProgressBar pb;
                pb.setAttribute( Qt::WA_DontShowOnScreen ); // 没有视觉干扰
                pb.setMaximum( 0 );
                pb.show();
                qApp->processEvents();
            }
        }
    }
}

void MainWindow::userScale()
{
    mMapCanvas->zoomScale( 1.0 / mScaleEdit->scale() );
}

void MainWindow::userRotation()
{
    double degrees = mRotationEdit->value();
    mMapCanvas->setRotation( degrees );
    mMapCanvas->refresh();
}

void MainWindow::projectPropertiesProjections()
{
    // 驱动程序，以显示该项目对话框并切换到投影页
    mShowProjectionTab = true;
    projectProperties();
}

void MainWindow::projectProperties()
{
    /* 显示该项目的属性表*/
    // 项目属性对话框可能正在构建，改为等待光标
    QApplication::setOverrideCursor( Qt::WaitCursor );
    QgsProjectProperties *pp = new QgsProjectProperties( mMapCanvas );
    // 如果从状态栏调用，显示投影页
    if ( mShowProjectionTab )
    {
        pp->showProjectionsTab();
        mShowProjectionTab = false;
    }
    qApp->processEvents();

    // 如果在项目属性对话框中改变显示精度等，对应修改状态栏
    connect( pp, SIGNAL( displayPrecisionChanged() ), this,
        SLOT( updateMouseCoordinatePrecision() ) );

    connect( pp, SIGNAL( scalesChanged( const QStringList & ) ), mScaleEdit,
        SLOT( updateScales( const QStringList & ) ) );
    QApplication::restoreOverrideCursor();	//恢复光标

    //通过任意刷新信号关闭到画布
    connect( pp, SIGNAL( refresh() ), mMapCanvas, SLOT( refresh() ) );

    // 显示模态对话框。
    pp->exec();

    qobject_cast<QgsMeasureTool*>( mMapTools.mMeasureDist )->updateSettings();
    qobject_cast<QgsMeasureTool*>( mMapTools.mMeasureArea )->updateSettings();

    // 删除属性表对象
    delete pp;
}

void MainWindow::updateMouseCoordinatePrecision()
{
    mCoordsEdit->setMouseCoordinatesPrecision(
                QgsCoordinateUtils::calculateCoordinatePrecision(
                    mapCanvas()->mapUnitsPerPixel(),
                    mapCanvas()->mapSettings().destinationCrs() ) );
}

void MainWindow::showStatusMessage(const QString &theMessage)
{
    statusBar()->showMessage( theMessage );
}

void MainWindow::legendLayerZoomNative()
{
    if ( !mLayerTreeView )
        return;

    //找到当前图层
    QgsMapLayer* currentLayer = mLayerTreeView->currentLayer();
    if ( !currentLayer )
        return;

    QgsRasterLayer *layer =  qobject_cast<QgsRasterLayer *>( currentLayer );
    if ( layer )
    {
        QgsDebugMsg( "Raster units per pixel  : " + QString::number( layer->rasterUnitsPerPixelX() ) );
        QgsDebugMsg( "MapUnitsPerPixel before : " + QString::number( mMapCanvas->mapUnitsPerPixel() ) );

        if ( mMapCanvas->hasCrsTransformEnabled() )
        {
            // 得到中央的画布像素宽度的长度源栅格CRS
            QgsRectangle e = mMapCanvas->extent();
            QSize s = mMapCanvas->mapSettings().outputSize();
            QgsPoint p1( e.center().x(), e.center().y() );
            QgsPoint p2( e.center().x() + e.width() / s.width(), e.center().y() + e.height() / s.height() );
            QgsCoordinateTransform ct( mMapCanvas->mapSettings().destinationCrs(), layer->crs() );
            p1 = ct.transform( p1 );
            p2 = ct.transform( p2 );
            double width = sqrt( p1.sqrDist( p2 ) ); // 投影像素的宽度（实际对角线）
            mMapCanvas->zoomByFactor( sqrt( layer->rasterUnitsPerPixelX() * layer->rasterUnitsPerPixelX() +
                                            layer->rasterUnitsPerPixelY() * layer->rasterUnitsPerPixelY() ) / width );
        }
        else
        {
            mMapCanvas->zoomByFactor( qAbs( layer->rasterUnitsPerPixelX() / mMapCanvas->mapUnitsPerPixel() ) );
        }
        mMapCanvas->refresh();
        QgsDebugMsg( "MapUnitsPerPixel after  : " + QString::number( mMapCanvas->mapUnitsPerPixel() ) );
    }
}

void MainWindow::addVectorLayer()
{
    mMapCanvas->freeze();
    QgsOpenVectorLayerDialog *ovl = new QgsOpenVectorLayerDialog( this );

    if ( ovl->exec() )
    {
        QStringList selectedSources = ovl->dataSources();
        QString enc = ovl->encoding();
        if ( !selectedSources.isEmpty() )
        {
            addVectorLayers( selectedSources, enc, ovl->dataSourceType() );
        }
    }

    mMapCanvas->freeze( false );
    mMapCanvas->refresh();

    delete ovl;
}

bool MainWindow::addVectorLayers(const QStringList &theLayerQStringList, const QString &enc, const QString &dataSourceType)
{
    bool wasfrozen = mMapCanvas->isFrozen();
    QList<QgsMapLayer *> myList;
    foreach ( QString src, theLayerQStringList )
    {
        src = src.trimmed();
        QString base;
        if ( dataSourceType == "file" )
        {
          QFileInfo fi( src );
          base = fi.completeBaseName();
        }
        else if ( dataSourceType == "database" )
        {
          base = src;
        }
        else //directory //protocol
        {
          QFileInfo fi( src );
          base = fi.completeBaseName();
        }

        QgsDebugMsg( "completeBaseName: " + base );

        // 创建一个矢量图层
        QgsVectorLayer *layer = new QgsVectorLayer( src, base, "ogr", false );
        Q_CHECK_PTR( layer );

        if ( ! layer )
        {
            mMapCanvas->freeze( false );
            return false;
        }

        if ( layer->isValid() )
        {
            layer->setProviderEncoding( enc );

            QStringList sublayers = layer->dataProvider()->subLayers();
            QgsDebugMsg( QString( "got valid layer with %1 sublayers" ).arg( sublayers.count() ) );

            // 如果新创建的层具有多个子图层数据，我们显示子层选择对话框，这样用户可以选择实际加载的子层
            if ( sublayers.count() > 1 )
            {
                askUserForOGRSublayers( layer );

                // 装在第一层不是在这种情况下是有用的。如果他要加载它，用户可以在列表中选择它
                delete layer;
            }
            else if ( !sublayers.isEmpty() ) // 只有单个可用数据
            {
                QStringList sublayers = layer->dataProvider()->subLayers();
                QStringList elements = sublayers.at( 0 ).split( ':' );

                Q_ASSERT( elements.size() >= 4 );
                if ( layer->name() != elements.at( 1 ) )
                {
                    layer->setLayerName( QString( "%1 %2 %3" ).arg( layer->name(), elements.at( 1 ), elements.at( 3 ) ) );
                }

                myList << layer;
            }
            else
            {
                QString msg = QString("%1 不具有任何图层").arg( src );
                messageBar()->pushMessage( "无效数据源", msg, QgsMessageBar::CRITICAL, messageTimeout() );
                delete layer;
            }
        }
        else
        {
            QString msg = QString("%1 不是有效或被支持的数据源").arg( src );
            messageBar()->pushMessage( "无效数据源", msg, QgsMessageBar::CRITICAL, messageTimeout() );

            // since the layer is bad, stomp on it
            delete layer;
        }

    }

    // 确保至少一个图层已成功添加
    if ( myList.isEmpty() )
    {
        return false;
    }

    // 在图层注册表中注册这个图层
    QgsMapLayerRegistry::instance()->addMapLayers( myList );

    foreach ( QgsMapLayer *l, myList )
    {
        bool ok;
        l->loadDefaultStyle( ok );
    }

    // 只有当我们在此方法冻结更新地图
    if ( !wasfrozen )
    {
        mMapCanvas->freeze( false );
        mMapCanvas->refresh();
    }
    mMapCanvas->setCurrentLayer( myList.first() );

    return true;
}

QgsMapLayer *MainWindow::activeLayer()
{
    return mLayerTreeView ? mLayerTreeView->currentLayer() : nullptr;
}

void MainWindow::openPosFile()
{
    dialog_posloaddialog *posDialog = new dialog_posloaddialog(this);
    connect( posDialog, SIGNAL( readFieldsList( QString & ) ), pPosdp, SLOT( readFieldsList( QString & ) ) );
    int result = posDialog->exec();
    if (result == QDialog::Accepted)
    {
        openMessageLog();

        const QStringList errList = pPosdp->checkPosSettings();
        if (!errList.isEmpty())
        {
            QString err;
            foreach (QString str, errList)
                err += str + "、";

            messageBar()->pushMessage( "曝光点相关参数检查",
                                       QString("程序检测到%1参数设置可能不正确，请确认参数是否已正确初始化。").arg(err),
                                       QgsMessageBar::CRITICAL, messageTimeout() );
            QgsMessageLog::logMessage(QString("曝光点相关参数检查 : \t程序检测到%1参数设置可能不正确，"
                                              "请确认参数是否已正确初始化。").arg(err));
        }
        else
        {
            if (pPosdp->isValid())
            {
                const QStringList* noList = pPosdp->noList();
                messageBar()->pushMessage( "曝光点加载",
                                           QString("成功加载%1条记录，%2条记录加载失败。")
                                           .arg(noList->size())
                                           .arg(pPosdp->getInvalidLineSize()),
                                           QgsMessageBar::SUCCESS, messageTimeout() );
                QgsMessageLog::logMessage(QString("曝光点加载 : \t成功加载%1条记录，%2条记录加载失败。")
                                          .arg(noList->size())
                                          .arg(pPosdp->getInvalidLineSize()));
                upDataPosActions();
            }
            else
            {
                messageBar()->pushMessage( "曝光点加载",
                                           QString("加载曝光点记录失败，请仔细排查错误信息。"),
                                           QgsMessageBar::CRITICAL, messageTimeout() );
                QgsMessageLog::logMessage("曝光点加载 : \t加载曝光点记录失败，请仔细排查错误信息。");
            }
        }
    }
    delete posDialog;
}

bool MainWindow::posTransform()
{
    if (!pPosdp->isValid()) return false;

    QgsMessageLog::logMessage("\n");
    return pPosdp->autoPosTransform();
}

void MainWindow::posSketchMap()
{
    if (!pPosdp->isValid()) return;

    QgsMessageLog::logMessage("\n");
    //! 用于保存航飞略图
    QgsVectorLayer* sketchMapLayer = pPosdp->autoSketchMap();
    if (!sketchMapLayer)
        return;
    pPPInter = new eqiPPInteractive(this, sketchMapLayer, pPosdp);
    pAnalysis = new eqiAnalysisAerialphoto(this, sketchMapLayer, pPosdp);
}

void MainWindow::posSketchMapSwitch()
{
//    ppInter->testSwitch();
    QgsMessageLog::logMessage("test");
}

void MainWindow::posLinkPhoto()
{
    QgsMessageLog::logMessage("\n");

    if (!pPPInter)
    {
        messageBar()->pushMessage( "动态联动",
            "必须在航飞略图成功创建后才能启动联动功能, 联动功能启动失败...",
            QgsMessageBar::CRITICAL, messageTimeout() );
        QgsMessageLog::logMessage(QString("动态联动 : \t必须在航飞略图成功创建后才能启动联动功能, 联动功能启动失败..."));
        return;
    }

    if (pPPInter->islinked())
    {
        // 断开连接并更改QAction
        pPPInter->upDataUnLinkedSymbol();

        QgsMessageLog::logMessage("动态联动 : \t已成功断开连接.");
        messageBar()->pushMessage( "动态联动", "成功断开联动关系.",QgsMessageBar::SUCCESS, messageTimeout() );

        mActionPPLinkPhoto->setText("PP动态联动");
        mActionPPLinkPhoto->setIcon(eqiApplication::getThemeIcon("mActionlink.svg"));
    }
    else
    {
        if (!pPPInter->isValid())
        {
            messageBar()->pushMessage( "动态联动",
                "必须在曝光点文件解析成功后才能启动联动功能, 联动功能启动失败...",
                QgsMessageBar::CRITICAL, messageTimeout() );
            QgsMessageLog::logMessage(QString("动态联动 : \t必须在曝光点文件解析成功后才能启动联动功能, 联动功能启动失败..."));
            return;
        }

        // 创建联动关系
        pPPInter->upDataLinkedSymbol();

        if (!pPPInter->islinked())
            return;

        QgsMessageLog::logMessage(QString("动态联动 : \t成功创建联动关系."));
        messageBar()->pushMessage( "动态联动", "成功创建联动关系.",QgsMessageBar::SUCCESS, messageTimeout() );

        mActionPPLinkPhoto->setText("断开PP动态联动");
        mActionPPLinkPhoto->setStatusTip("断开PP动态联动");
        mActionPPLinkPhoto->setIcon(eqiApplication::getThemeIcon("mActionUnlink.svg"));
    }
}

void MainWindow::posOneButton()
{
    // 坐标转换
    if (mSettings.value("/eqi/options/imagePreprocessing/chkTransform", true).toBool())
    {
        if (!posTransform())
            return;
    }

    // 创建略图
    if (mSettings.value("/eqi/options/imagePreprocessing/chkSketchMap", true).toBool())
    {
        posSketchMap();
    }

    // PP联动
    if (mSettings.value("/eqi/options/imagePreprocessing/chkLinkPhoto", true).toBool())
    {
        posLinkPhoto();
    }
}

void MainWindow::posExport()
{
    if (!pPosdp->isValid()) return;

    pPosdp->posExport();
}

void MainWindow::posSetting()
{
    dialog_posSetting *posDialog = new dialog_posSetting(this);
    posDialog->exec();
}

void MainWindow::checkOverlapping()
{
    pAnalysis->checkOverlapping();
}

void MainWindow::checkOmega()
{
    pAnalysis->checkOmega();
}

void MainWindow::checkKappa()
{
    pAnalysis->checkKappa();
}

void MainWindow::pointToTk()
{
    eqiMapToolPointToTk *gcd = new eqiMapToolPointToTk(mMapCanvas);
    mMapCanvas->setMapTool(gcd);
}

void MainWindow::TKtoXY()
{
    dialog_printTKtoXY_txt *printxy = new dialog_printTKtoXY_txt(this);
    printxy->exec();
}

void MainWindow::prjtransformsetting()
{
    dialog_PrjTransformSetting *ptfs = new dialog_PrjTransformSetting(this);
    ptfs->exec();
}
