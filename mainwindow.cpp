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


MainWindow *MainWindow::smInstance = nullptr;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    if ( smInstance )
    {
        QMessageBox::critical( this, "EQI的多个实例", "检测到多个EQI应用对象的实例。" );
        abort();
    }
    smInstance = this;

    ui->setupUi(this);

    //创建动作
     initActions();

    QWidget *centralWidget = this->centralWidget();
    QGridLayout *centralLayout = new QGridLayout( centralWidget );
    centralWidget->setLayout( centralLayout );
    centralLayout->setContentsMargins( 0, 0, 0, 0 );

    // "theMapCanvas" used to find this canonical instance later
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

    //初始化工具栏
    initTabTools();

    // 创建状态栏
    initStatusBar();

    initLayerTreeView();

    mMapCanvas->freeze();
}

MainWindow::~MainWindow()
{
    delete ui;
}

QgsLayerTreeView *MainWindow::layerTreeView()
{
    return mLayerTreeView;
}

QgsClipboard *MainWindow::clipboard()
{
    return mInternalClipboard;
}

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
    mCTF_DtoD = new QAction("加载POS文件", this);
    mCTF_DtoD->setIcon(QIcon(":/Resources/images/themes/default/mActionShowPluginManager.svg"));
    mCTF_DtoD->setStatusTip(" 度与度分秒格式互转，仅支持文本格式。 ");
}

void MainWindow::initTabTools()
{
    // 初始化“坐标转换”tab
    tab_coordinateTransformation *m_tCTF = new tab_coordinateTransformation(this);
    m_tCTF->addAction(mCTF_DtoD);

    // 初始化“无人机数据管理”tab
    tab_uavDataManagement *m_uDM = new tab_uavDataManagement(this);
    m_uDM->addAction(mCTF_DtoD);

    // 添加tab到窗口
    toolTab = new QTabWidget;
    toolTab->addTab(m_uDM, "无人机数据管理");
    toolTab->addTab(m_tCTF, "坐标转换");
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
    //mScaleLabel->setMaximumHeight( 20 );
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
        //mRotationLabel->setMaximumHeight( 20 );
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
    mMessageButton->setIcon( eqiApplication::getThemeIcon( "mMessageLogRead.svg" ) );
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

void MainWindow::showRotation()
{
    // 更新当前状态栏的旋转标签。
    double myrotation = mMapCanvas->rotation();
    mRotationEdit->setValue( myrotation );
}
