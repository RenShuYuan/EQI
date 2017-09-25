// My
#include "mainwindow.h"
#include "eqi/eqiapplication.h"
#include "ui_mainwindow.h"
#include "ui/toolTab/tab_coordinatetransformation.h"
#include "ui/toolTab/tab_uavdatamanagement.h"
#include "qgis/app/qgsstatusbarcoordinateswidget.h"
#include "qgis/app/qgsapplayertreeviewmenuprovider.h"
#include "qgis/app/qgsclipboard.h"
#include "qgis/app/qgisappstylesheet.h"
#include "qgis/app/qgsvisibilitypresets.h"
#include "qgis/app/qgsprojectproperties.h"
#include "qgis/app/qgsmeasuretool.h"

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

// QGis
#include "qgsmapcanvas.h"
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
#include "qgslayertreemapcanvasbridge.h"
#include "qgscustomlayerorderwidget.h"
#include "qgsmessagelogviewer.h"
#include "qgsmessagelog.h"
#include "qgsvectorlayer.h"
#include "qgslayertreegroup.h"
#include "qgslayertreelayer.h"
#include "qgslayertree.h"
#include "qgscoordinateutils.h"
#include "qgsmapoverviewcanvas.h"


MainWindow *MainWindow::smInstance = nullptr;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
//    , mScaleLabel( nullptr )
//    , mScaleEdit( nullptr )
//    , mCoordsEdit( nullptr )
//    , mRotationLabel( nullptr )
//    , mRotationEdit( nullptr )
//    , mProgressBar( nullptr )
//    , mRenderSuppressionCBox( nullptr )
//    , mOnTheFlyProjectionStatusButton( nullptr )
//    , mLayerTreeCanvasBridge( nullptr )
//    , mInternalClipboard( nullptr )
    , mShowProjectionTab( false )
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

}

MainWindow::~MainWindow()
{
    delete ui;

//    mMapCanvas->stopRendering();

//    delete mInternalClipboard;

//    delete mMapTools.mZoomIn;
//    delete mMapTools.mZoomOut;
//    delete mMapTools.mPan;

//    delete mMapTools.mAddFeature;
//    delete mMapTools.mAddPart;
//    delete mMapTools.mAddRing;
//    delete mMapTools.mFillRing;
//    delete mMapTools.mAnnotation;
//    delete mMapTools.mChangeLabelProperties;
//    delete mMapTools.mDeletePart;
//    delete mMapTools.mDeleteRing;
//    delete mMapTools.mFeatureAction;
//    delete mMapTools.mFormAnnotation;
//    delete mMapTools.mHtmlAnnotation;
//    delete mMapTools.mIdentify;
//    delete mMapTools.mMeasureAngle;
//    delete mMapTools.mMeasureArea;
//    delete mMapTools.mMeasureDist;
//    delete mMapTools.mMoveFeature;
//    delete mMapTools.mMoveLabel;
//    delete mMapTools.mNodeTool;
//    delete mMapTools.mOffsetCurve;
//    delete mMapTools.mPinLabels;
//    delete mMapTools.mReshapeFeatures;
//    delete mMapTools.mRotateFeature;
//    delete mMapTools.mRotateLabel;
//    delete mMapTools.mRotatePointSymbolsTool;
//    delete mMapTools.mSelectFreehand;
//    delete mMapTools.mSelectPolygon;
//    delete mMapTools.mSelectRadius;
//    delete mMapTools.mSelectFeatures;
//    delete mMapTools.mShowHideLabels;
//    delete mMapTools.mSimplifyFeature;
//    delete mMapTools.mSplitFeatures;
//    delete mMapTools.mSplitParts;
//    delete mMapTools.mSvgAnnotation;
//    delete mMapTools.mTextAnnotation;
//    delete mMapTools.mCircularStringCurvePoint;
//    delete mMapTools.mCircularStringRadius;

//    delete mOverviewMapCursor;

//    // cancel request for FileOpen events
//    QgsApplication::setFileOpenEventReceiver( nullptr );

//    QgsApplication::exitQgis();

//    delete QgsProject::instance();
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

//void MainWindow::addDockWidget(Qt::DockWidgetArea area, QDockWidget *dockwidget)
//{
//    QMainWindow::addDockWidget( area, dockwidget );
//    // Make the right and left docks consume all vertical space and top
//    // and bottom docks nest between them
//    setCorner( Qt::TopLeftCorner, Qt::LeftDockWidgetArea );
//    setCorner( Qt::BottomLeftCorner, Qt::LeftDockWidgetArea );
//    setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
//    setCorner( Qt::BottomRightCorner, Qt::RightDockWidgetArea );
//    // add to the Panel submenu
//    mPanelMenu->addAction( dockwidget->toggleViewAction() );

//    dockwidget->show();

//    // refresh the map canvas
//    mMapCanvas->refresh();
//}

void MainWindow::initActions()
{
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

    /*-----------------------------------------------------------------------------------------*/
    mActionCTF = new QAction("加载POS文件", this);
    mActionCTF->setIcon(eqiApplication::getThemeIcon("mActionShowPluginManager.svg"));
    mActionCTF->setStatusTip(" 度与度分秒格式互转，仅支持文本格式。 ");
}

void MainWindow::initTabTools()
{
    // 初始化“坐标转换”tab
    tab_coordinateTransformation *m_tCTF = new tab_coordinateTransformation(this);
    m_tCTF->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_tCTF->addAction(mActionCTF);

    // 初始化“无人机数据管理”tab
    tab_uavDataManagement *m_uDM = new tab_uavDataManagement(this);
    m_uDM->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_uDM->addAction(mActionCTF);

    // 添加tab到窗口
    toolTab = new QTabWidget;
    toolTab->addTab(m_uDM, "无人机数据管理");
    toolTab->addTab(m_tCTF, "　　坐标转换　　");
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
    ui->statusBar->addPermanentWidget( mCoordsEdit );

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
    mMessageButton = new QToolButton( this );
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

//void MainWindow::initOverview()
//{
//    // overview canvas
//    mOverviewCanvas = new QgsMapOverviewCanvas( nullptr, mMapCanvas );

//    //set canvas color to default
//    QSettings settings;
//    int red = settings.value( "/eqi/default_canvas_color_red", 255 ).toInt();
//    int green = settings.value( "/eqi/default_canvas_color_green", 255 ).toInt();
//    int blue = settings.value( "/eqi/default_canvas_color_blue", 255 ).toInt();
//    mOverviewCanvas->setBackgroundColor( QColor( red, green, blue ) );

//    mOverviewCanvas->setWhatsThis( "鹰眼地图画布。此画布可以用来显示一个地图定位器，"
//                                   "显示在地图上画布的当前程度。当前范围被示为红色矩形。"
//                                   "地图上的任何层都可以被添加到鹰眼画布。" );

//    mOverviewMapCursor = new QCursor( Qt::OpenHandCursor );
//    mOverviewCanvas->setCursor( *mOverviewMapCursor );
//    mOverviewDock = new QDockWidget( "鹰眼图面板", this );
//    mOverviewDock->setObjectName( "Overview" );
////    QFont myFont( "微软雅黑", 9 );
////    mOverviewDock->setFont(myFont);
//    mOverviewDock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
//    mOverviewDock->setWidget( mOverviewCanvas );
//    mOverviewDock->hide();
//    addDockWidget( Qt::LeftDockWidgetArea, mOverviewDock );
//    // add to the Panel submenu
//    mPanelMenu->addAction( mOverviewDock->toggleViewAction() );

//    mMapCanvas->enableOverviewMode( mOverviewCanvas );

//    // moved here to set anti aliasing to both map canvas and overview
//    QSettings mySettings;
//    // Anti Aliasing enabled by default as of QGIS 1.7
//    mMapCanvas->enableAntiAliasing( mySettings.value( "/eqi/enable_anti_aliasing", true ).toBool() );

//    int action = mySettings.value( "/eqi/wheel_action", 2 ).toInt();
//    double zoomFactor = mySettings.value( "/eqi/zoom_factor", 2 ).toDouble();
//    mMapCanvas->setWheelAction( static_cast< QgsMapCanvas::WheelAction >( action ), zoomFactor );

//    mMapCanvas->setCachingEnabled( mySettings.value( "/eqi/enable_render_caching", true ).toBool() );

//    mMapCanvas->setParallelRenderingEnabled( mySettings.value( "/eqi/parallel_rendering", false ).toBool() );

//    mMapCanvas->setMapUpdateInterval( mySettings.value( "/eqi/map_update_interval", 250 ).toInt() );
//}

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
        //only call show if not already hidden to reduce flicker 只有调用show如果尚未隐藏，以减少闪烁
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
