#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt
#include <QMainWindow>
#include <QSettings>

class QAction;
class QTabBar;
class QLabel;
class QProgressBar;
class QCheckBox;
class QToolButton;
class QDockWidget;
class QCursor;

// QGis
#include "qgsmapcanvas.h"
#include "qgsmessagebar.h"

class QgsStatusBarCoordinatesWidget;
class QgsScaleComboBox;
class QgsDoubleSpinBox;
class QgisAppStyleSheet;
class QgsLayerTreeView;
class QgsLayerTreeMapCanvasBridge;
class QgsCustomLayerOrderWidget;
class QgsClipboard;
class QgsMessageLogViewer;
class QgsMapTool;
class QgsMapOverviewCanvas;

class posDataProcessing;
class eqiPPInteractive;

// 分幅图框字段名称
const static QString ThFieldName = "TH";

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QgsMapCanvas *mapCanvas();
    QgsLayerTreeView *layerTreeView();
    QgsClipboard *clipboard();
    QgsMessageBar *messageBar();

    //! 控制信息显示条显示的时间: 默认为5秒
    int messageTimeout();

    /** 打开消息面板 **/
    void openMessageLog();

    void addDockWidget( Qt::DockWidgetArea area, QDockWidget *dockwidget );

    // 创建临时图层
    // "point", "linestring", "polygon", "multipoint","multilinestring","multipolygon"
    QgsVectorLayer* createrMemoryMap( const QString& layerName,
                                      const QString& geometric,
                                      const QStringList& fieldList );

    static MainWindow *instance() { return smInstance; }
    QAction *actionHideAllLayers() { return mActionHideAllLayers; }
    QAction *actionShowAllLayers() { return mActionShowAllLayers; }
    QAction *actionHideSelectedLayers() { return mActionHideSelectedLayers; }
    QAction *actionShowSelectedLayers() { return mActionShowSelectedLayers; }
    QgsLayerTreeMapCanvasBridge *layerTreeCanvasBridge() { return mLayerTreeCanvasBridge; }
//    QgsMapOverviewCanvas* mapOverviewCanvas() { return mOverviewCanvas; }

public slots:
//    QMenu *panelMenu() { return mPanelMenu; }
    void pan();
    void panToSelected();
    void zoomIn();
    void zoomOut();
    void zoomFull();
    void zoomActualSize();
    void zoomToSelected();
    void zoomToLayerExtent();
    void zoomToPrevious();
    void zoomToNext();
    void refreshMapCanvas();
    void loadOGRSublayers( const QString& layertype, const QString& uri, const QStringList& list );

    //! 连接到图层树桥注册表，首先选择新添加的地图图层
    void autoSelectAddedLayer( QList<QgsMapLayer*> layers );
    void activeLayerChanged( QgsMapLayer *layer );
private:
    void initActions();
    void initTabTools();
    void initStatusBar();
    void initLayerTreeView();
    void createCanvasTools();
//    void initMenus();
//    void initOverview();

    //! 这种方法将打开的对话框，因此用户可以选择的OGR子层加载
    void askUserForOGRSublayers( QgsVectorLayer *layer );

    void saveAsVectorFileGeneral( QgsVectorLayer* vlayer = nullptr, bool symbologyOption = true );

    void upDataPosActions();

private slots:
    void showRotation();

    //! 返回顶部消息
    void displayMessage( const QString& title, const QString& message, QgsMessageBar::MessageLevel level );

    //! 从settings设置应用程序样式表
    void setAppStyleSheet( const QString& stylesheet );

    //! 改变状态栏中消息按钮的图标
    void toggleLogMessageIcon( bool hasLogMessage );

    void measure();
    void measureArea();
    void identify();
    void saveAsFile();

    void removeLayer();
    void showAllLayers();
    void hideAllLayers();
    void showSelectedLayers();
    void hideSelectedLayers();

    void canvasRefreshStarted();
    void canvasRefreshFinished();
    void showProgress( int theProgress, int theTotalSteps );

    //! 处理用户输入的比例尺(slot)
    void userScale();

    //! 处理用户输入的旋转(slot)
    void userRotation();

    //! 打开项目属性对话框,并显示在投影状态(slot)
    void projectPropertiesProjections();

    //! 设置项目属性，包括地图单位(slot)
    void projectProperties();

    void updateMouseCoordinatePrecision();

    void showStatusMessage( const QString& theMessage );

    /** 缩放使光栅层的像素刚好占据一个屏幕像素。仅适用于栅格图层*/
    void legendLayerZoomNative();

    //! 添加一个矢量图层到地图
    void addVectorLayer();

    /** \这需要文件名列表，而不是提示用户的对话框，私有addLayer方法的简要重载版本
     @param enc 层编码类型
     @param dataSourceType OGR数据源的类型
     @returns 如果添加图层成功返回true
    */
    bool addVectorLayers( const QStringList &theLayerQStringList, const QString &enc, const QString &dataSourceType );

    //! 返回活动图层的指针
    QgsMapLayer *activeLayer();

    /************************* 面板 *************************/

    /************ 要素选择、编辑 ************/
    //! 选择要素
    void selectFeatures();

    //! 按多边形选择要素
    void selectByPolygon();

    //! 自由手绘选择要素
    void selectByFreehand();

    //! 取消选择要素
    void deselectAll();

    //! 选择所有要素
    void selectAll();

    //! 反选要素
    void invertSelection();

    //! 删除选择要素
    void delSelect();

    /************ 无人机数据管理 ************/
    //! 载入曝光点文件
    void openPosFile();

    //! 曝光点坐标转换
    bool posTransform();

    //! 创建航飞略图
    void posSketchMap();

    //! 切换显示略图
    void posSketchMapSwitch();

    //! 创建PP动态联动
    void posLinkPhoto();

    //! 一键处理
    void posOneButton();

    //! 导出曝光点文件
    void posExport();

    //! 相机设置
    void posSetting();

    /************ 分幅管理 ************/
    //! 根据坐标创建图框
    void pointToTk();

    //! 输出图框坐标
    void TKtoXY();

    //! 分幅管理的参数设置
    void prjtransformsetting();

signals:
    void layerSavedAs( QgsMapLayer* l, const QString& path );

private:
    Ui::MainWindow *ui;
    static MainWindow *smInstance;
    QSettings mSettings;
    posDataProcessing *posdp;
    eqiPPInteractive* ppInter;

    //! 标志，表示该项目属性对话框是怎么出现
    bool mShowProjectionTab;

    //! 地图浏览动作
    QAction *mActionPan;
    QAction *mActionPanToSelected;
    QAction *mActionZoomIn;
    QAction *mActionZoomOut;
    QAction *mActionZoomFullExtent;
    QAction *mActionZoomActualSize;
    QAction *mActionZoomToSelected;
    QAction *mActionZoomToLayer;
    QAction *mActionZoomLast;
    QAction *mActionZoomNext;
    QAction *mActionDraw;
    QAction *mActionIdentify;

    //! 要素选择、编辑
    QAction *mActionSelectFeatures;
    QAction *mActionSelectPolygon;
    QAction *mActionSelectFreehand;
    QAction *mActionDeselectAll;
    QAction *mActionSelectAll;
    QAction *mActionInvertSelection;
    QAction *mActionDelSelect;
    QAction *mActionSaveSelect;

    //! 图层管理动作
    QAction *mActionFilterLegend;
    QAction *mActionRemoveLayer;
    QAction *mActionShowAllLayers;
    QAction *mActionHideAllLayers;
    QAction *mActionShowSelectedLayers;
    QAction *mActionHideSelectedLayers;

    //! 无人机数据管理动作
    QAction *mOpenPosFile;
    QAction *mPosTransform;
    QAction *mPosSketchMap;
    QAction *mPosSketchMapSwitch;
    QAction *mPosOneButton;
    QAction *mPosExport;
    QAction *mPosSetting;

    //! 无人机数据联动
    QAction *mPPLinkPhoto;

    //! 坐标转换动作
    QAction *mActionTextTranfrom;
    QAction *mActionDegreeMutual;

    //! 分幅动作
    QAction *mActionPtoTK;
    QAction *mActionCreateTK;
    QAction *mActionTKtoXY;
    QAction *mActionPtoTKSetting;

    //! 数据管理动作
    QAction *mActionAddOgrLayer;
    QAction *mActionLayerSaveAs;

    QLabel *mScaleLabel;
    QLabel *mRotationLabel;
    QTabWidget *toolTab;
    QProgressBar *mProgressBar;
    QCheckBox *mRenderSuppressionCBox;
    QToolButton *mMessageButton;
    QToolButton *mOnTheFlyProjectionStatusButton;
    QDockWidget *mLayerTreeDock;
    QDockWidget *mLayerOrderDock;
    QDockWidget *mLogDock;
//    QDockWidget *mOverviewDock;
//    QCursor *mOverviewMapCursor;
//    QMenu *mPanelMenu;

    // QGis
    QgsMapCanvas *mMapCanvas;
    QgsStatusBarCoordinatesWidget *mCoordsEdit;
    QgsScaleComboBox *mScaleEdit;
    QgsDoubleSpinBox *mRotationEdit;
    QgisAppStyleSheet *mStyleSheetBuilder;
    QgsLayerTreeView *mLayerTreeView;
    QgsMessageBar* mInfoBar;
    QgsLayerTreeMapCanvasBridge *mLayerTreeCanvasBridge;
    QgsCustomLayerOrderWidget *mMapLayerOrder;
    QgsClipboard *mInternalClipboard;
    QgsMessageLogViewer *mLogViewer;
//    QgsMapOverviewCanvas *mOverviewCanvas;

    QgsMapTool *mNonEditMapTool;

    class Tools
    {
    public:

        Tools()
            : mZoomIn( nullptr )
            , mZoomOut( nullptr )
            , mPan( nullptr )
            , mIdentify( nullptr )
            , mFeatureAction( nullptr )
            , mMeasureDist( nullptr )
            , mMeasureArea( nullptr )
            , mMeasureAngle( nullptr )
            , mAddFeature( nullptr )
            , mCircularStringCurvePoint( nullptr )
            , mCircularStringRadius( nullptr )
            , mMoveFeature( nullptr )
            , mOffsetCurve( nullptr )
            , mReshapeFeatures( nullptr )
            , mSplitFeatures( nullptr )
            , mSplitParts( nullptr )
            , mSelect( nullptr )
            , mSelectFeatures( nullptr )
            , mSelectPolygon( nullptr )
            , mSelectFreehand( nullptr )
            , mSelectRadius( nullptr )
            , mVertexAdd( nullptr )
            , mVertexMove( nullptr )
            , mVertexDelete( nullptr )
            , mAddRing( nullptr )
            , mFillRing( nullptr )
            , mAddPart( nullptr )
            , mSimplifyFeature( nullptr )
            , mDeleteRing( nullptr )
            , mDeletePart( nullptr )
            , mNodeTool( nullptr )
            , mRotatePointSymbolsTool( nullptr )
            , mAnnotation( nullptr )
            , mFormAnnotation( nullptr )
            , mHtmlAnnotation( nullptr )
            , mSvgAnnotation( nullptr )
            , mTextAnnotation( nullptr )
            , mPinLabels( nullptr )
            , mShowHideLabels( nullptr )
            , mMoveLabel( nullptr )
            , mRotateFeature( nullptr )
            , mRotateLabel( nullptr )
            , mChangeLabelProperties( nullptr )
        {}

        QgsMapTool *mZoomIn;
        QgsMapTool *mZoomOut;
        QgsMapTool *mPan;
        QgsMapTool *mIdentify;
        QgsMapTool *mFeatureAction;
        QgsMapTool *mMeasureDist;
        QgsMapTool *mMeasureArea;
        QgsMapTool *mMeasureAngle;
        QgsMapTool *mAddFeature;
        QgsMapTool *mCircularStringCurvePoint;
        QgsMapTool *mCircularStringRadius;
        QgsMapTool *mMoveFeature;
        QgsMapTool *mOffsetCurve;
        QgsMapTool *mReshapeFeatures;
        QgsMapTool *mSplitFeatures;
        QgsMapTool *mSplitParts;
        QgsMapTool *mSelect;
        QgsMapTool *mSelectFeatures;
        QgsMapTool *mSelectPolygon;
        QgsMapTool *mSelectFreehand;
        QgsMapTool *mSelectRadius;
        QgsMapTool *mVertexAdd;
        QgsMapTool *mVertexMove;
        QgsMapTool *mVertexDelete;
        QgsMapTool *mAddRing;
        QgsMapTool *mFillRing;
        QgsMapTool *mAddPart;
        QgsMapTool *mSimplifyFeature;
        QgsMapTool *mDeleteRing;
        QgsMapTool *mDeletePart;
        QgsMapTool *mNodeTool;
        QgsMapTool *mRotatePointSymbolsTool;
        QgsMapTool *mAnnotation;
        QgsMapTool *mFormAnnotation;
        QgsMapTool *mHtmlAnnotation;
        QgsMapTool *mSvgAnnotation;
        QgsMapTool *mTextAnnotation;
        QgsMapTool *mPinLabels;
        QgsMapTool *mShowHideLabels;
        QgsMapTool *mMoveLabel;
        QgsMapTool *mRotateFeature;
        QgsMapTool *mRotateLabel;
        QgsMapTool *mChangeLabelProperties;
    } mMapTools;
};

#ifdef ANDROID
#define QGIS_ICON_SIZE 32
#else
#define QGIS_ICON_SIZE 24
#endif

#endif // MAINWINDOW_H
