#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt
#include <QMainWindow>
#include <QSettings>

// QGis
#include "qgsmapcanvas.h"
#include "qgsmessagebar.h"

class QAction;
class QTabBar;
class QLabel;
class QProgressBar;
class QCheckBox;
class QToolButton;
class QDockWidget;
class QCursor;

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
class QgsRasterLayer;

class posDataProcessing;
class eqiPPInteractive;
class eqiAnalysisAerialphoto;

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

    //! 显示图层属性
    void showLayerProperties( QgsMapLayer *ml );

    // 删除航摄数据，包括POS、略图、相片
    void deleteAerialPhotographyData(const QStringList& delList);

    /** Open a raster layer for the given file
      @returns false if unable to open a raster layer for rasterFile
      @note
      This is essentially a simplified version of the above
      */
    QgsRasterLayer *addRasterLayer( const QString &rasterFile, const QString &baseName, bool guiWarning = true );

public slots:
//    QMenu *panelMenu() { return mPanelMenu; }
    void about();
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
    void loadGDALSublayers( const QString& uri, const QStringList& list );

    //! 开启或关闭取决于当前地图图层类型的行动。从图例调用的时候，当前图例项已经改变
    void activateDeactivateLayerRelatedActions( QgsMapLayer *layer );

    //! 连接到图层树桥注册表，首先选择新添加的地图图层
    void autoSelectAddedLayer( QList<QgsMapLayer*> layers );
    void activeLayerChanged( QgsMapLayer *layer );

    //! mark project dirty
    void markDirty();
private:
    void initActions();
    void initTabTools();
    void initStatusBar();
    void initLayerTreeView();
    void createCanvasTools();
    void setupConnections();
//    void initMenus();
//    void initOverview();

    /** 栅格图层添加到地图（传过来的ptr）.
    * 它不会强制刷新。
    */
   bool addRasterLayer( QgsRasterLayer * theRasterLayer );

    /** 打开一个栅格图层 - 这是通用函数，它接受所有参数 */
    QgsRasterLayer* addRasterLayerPrivate( const QString & uri, const QString & baseName,
                                           const QString & providerKey, bool guiWarning,
                                           bool guiUpdate );

    /** 此方法将验证GDAL层是否包含子层
    */
    bool shouldAskUserForGDALSublayers( QgsRasterLayer *layer );

    /** This method will open a dialog so the user can select GDAL sublayers to load
     * @returns true if any items were loaded
     */
    bool askUserForZipItemLayers( QString path );

    /** 这种方法将打开对话框，因此用户可以选择的GDAL子层加载
    */
    void askUserForGDALSublayers( QgsRasterLayer *layer );

    //! 这种方法将打开的对话框，因此用户可以选择的OGR子层加载
    void askUserForOGRSublayers( QgsVectorLayer *layer );

    void saveAsVectorFileGeneral( QgsVectorLayer* vlayer = nullptr, bool symbologyOption = true );

    void upDataPosActions();

    //! 删除航摄略图中指定相片
    int deleteSketchMap(const QStringList &delList);

    QgsPoint vectorScale();

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

    //! 显示当前地图的比例尺(slot)
    void showScale( double theScale );

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

    //! 栅格图层添加到地图（将使用对话框提示用户输入文件名）
    void addRasterLayer();

    /** 重载版本，需要文件名列表，而不是与一个对话框，提示用户列表私有addRasterLayer（）方法。
     @returns 如果添加图层成功返回true
    */
    bool addRasterLayers( QStringList const &theFileNameQStringList, bool guiWarning = true );
    bool addRasterLayers( const QStringList &theFileNameQStringList, QgsRasterLayer*& rasterLayer, bool guiWarning = true );

    //! 返回活动图层的指针
    QgsMapLayer *activeLayer();

    /************************* 面板 *************************/

    /************ 要素选择 ************/
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

    /************ 无人机数据管理 ************/
    //! 载入曝光点文件
    void openPosFile();

    //! 曝光点坐标转换
    bool posTransform();

    //! 创建航飞略图
    void posSketchMap();

    //! 创建PP动态联动
    void posLinkPhoto();

    //! 一键处理
    void posOneButton();

    //! 导出曝光点文件
    void posExport();

    /**
    * @brief            切换航摄略图显示范围
    * @author           YuanLong
    * @warning          在正常范围与按比例缩小范围中切换。
    * @return
    */
    void pSwitchSketchMap();

    //! 切换显示航摄略图的编号
    void switchPosLabel();

    //! 相机设置
    void posSetting();

    /************ 航摄数据预处理 ************/
    //! 重叠度检查
    void checkOverlapping();

    //! 倾角检查
    void checkOmega();

    //! 旋片角检查
    void checkKappa();

    //! 删除重叠度超限相片
    void delOverlapIn();

    //! 删除倾角超限相片
    void delOmega();

    //! 删除旋片角超限相片
    void delKappa();

    //! 删除选择航飞数据
    void delSelect();

    //! 保存选择航飞数据
    void saveSelect();

    //! 修改POS文件
    void modifyPos();

    //! 修改相片数据
    void modifyPhoto();

    //! 设置
    void selectSetting();

    /************ 分幅管理 ************/
    //! 根据坐标创建图框
    void pointToTk();

    //! 输出图框坐标
    void TKtoXY();

    //! 分幅管理的参数设置
    void prjtransformsetting();

    /************ 像控快速拾取系统 ************/
    void addPcmDomLayer();

    void addPcmDemPath();

    void readPickPcm(QStringList &list);
    void pcmPicking();

    void printPcmToTxt();

    void pcmSetting();
signals:
    void layerSavedAs( QgsMapLayer* l, const QString& path );

private:
    Ui::MainWindow *ui;
    static MainWindow *smInstance;
    QSettings mSettings;

    // POS类
    posDataProcessing *pPosdp;

    // 动态联动类
    eqiPPInteractive* pPPInter;

    // 空间分析类
    eqiAnalysisAerialphoto* pAnalysis;

    // 保存航摄略图的比例显示状态
    bool isSmSmall;

    // 保存航摄略图的标注显示状态
    bool isPosLabel;

    // 用于保存航飞略图
    QgsVectorLayer* sketchMapLayer;

    // 用于保存像控快速拾取系统的DOM
    QgsRasterLayer* pcm_rasterLayer;

    // 用于保存像控快速拾取系统的DEM路径
    QStringList pcm_demPaths;

    // 用于保存拾取到的控制点
    QStringList pcmList;

    // 含有辅助光栅文件格式适合于FileDialog的文件过滤字符串。内置构造函数.
    QString mRasterFileFilter;

    // 标志，表示该项目属性对话框是怎么出现
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

    //! 信息查询
    QAction *mActionIdentify;
    QAction *mActionMeasure;
    QAction *mActionMeasureArea;

    //! 图层管理动作
    QAction *mActionFilterLegend;
    QAction *mActionRemoveLayer;
    QAction *mActionShowAllLayers;
    QAction *mActionHideAllLayers;
    QAction *mActionShowSelectedLayers;
    QAction *mActionHideSelectedLayers;

    //! 要素选择
    QAction *mActionSelectFeatures;
    QAction *mActionSelectPolygon;
    QAction *mActionSelectFreehand;
    QAction *mActionDeselectAll;
    QAction *mActionSelectAll;
    QAction *mActionInvertSelection;

    //! 航摄数据管理动作
    QAction *mActionOpenPosFile;
    QAction *mActionPosTransform;
    QAction *mActionPosSketchMap;
    QAction *mActionSketchMapSwitch;
    QAction *mActionPosLabelSwitch;
    QAction *mActionPosOneButton;
    QAction *mActionPosExport;
    QAction *mActionPosSetting;

    //! 航摄数据联动
    QAction *mActionPPLinkPhoto;

    //! 航摄数据预处理
    QAction *mActionCheckOverlapIn;
    QAction *mActionCheckOverlapBetween;
    QAction *mActionCheckOmega;
    QAction *mActionCheckKappa;
    QAction *mActionDelSelect;
    QAction *mActionSaveSelect;
    QAction *mActionSelectSetting;
    QAction *mActionDelOmega;
    QAction *mActionDelKappa;
    QAction *mActionDelOverlapIn;
    QAction *mActionDelOverlapBetween;
    QAction *mActionModifyPos;
    QAction *mActionModifyPhoto;

    //! 像控点快速拾取
    QAction *pcm_mActionAddDOMLayer;
    QAction *pcm_mActionAddDEMLayer;
    QAction *pcm_mActionPickContrelPoint;
    QAction *pcm_mActionOutContrelPoint;
    QAction *pcm_mActionsetting;

    //! 坐标转换动作
    QAction *mActionTextTranfrom;
    QAction *mActionDegreeMutual;

    //! 分幅动作
    QAction *mActionPtoTK;
    QAction *mActionCreateTK;
    QAction *mActionTKtoXY;
    QAction *mActionExTKtoXY;
    QAction *mActionPtoTKSetting;

    //! 数据管理动作
    QAction *mActionAddOgrLayer;
    QAction *mActionAddOgrRaster;
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

    QAction *mActionAbout;

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
