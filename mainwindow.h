#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt
#include <QMainWindow>

class QAction;
class QTabBar;
class QLabel;
class QProgressBar;
class QCheckBox;
class QToolButton;
class QDockWidget;

// QGis
class QgsMapCanvas;
class QgsStatusBarCoordinatesWidget;
class QgsScaleComboBox;
class QgsDoubleSpinBox;
class QgisAppStyleSheet;
class QgsLayerTreeView;
class QgsMessageBar;
class QgsLayerTreeMapCanvasBridge;
class QgsCustomLayerOrderWidget;
class QgsClipboard;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QgsLayerTreeView *layerTreeView();
    QgsClipboard *clipboard();

    static MainWindow *instance() { return smInstance; }
    QAction *actionHideAllLayers() { return mActionHideAllLayers; }
    QAction *actionShowAllLayers() { return mActionShowAllLayers; }
    QAction *actionHideSelectedLayers() { return mActionHideSelectedLayers; }
    QAction *actionShowSelectedLayers() { return mActionShowSelectedLayers; }
    QgsLayerTreeMapCanvasBridge *layerTreeCanvasBridge() { return mLayerTreeCanvasBridge; }

private:
    void initActions();
    void initTabTools();
    void initStatusBar();

    void initLayerTreeView();

private slots:
    void showRotation();

private:
    Ui::MainWindow *ui;
    static MainWindow *smInstance;

    QAction *mCTF_DtoD;
    QAction *mActionFilterLegend;
    QAction *mActionRemoveLayer;
    QAction *mActionShowAllLayers;
    QAction *mActionHideAllLayers;
    QAction *mActionShowSelectedLayers;
    QAction *mActionHideSelectedLayers;

    QLabel *mScaleLabel;
    QLabel *mRotationLabel;
    QTabWidget *toolTab;
    QProgressBar *mProgressBar;
    QCheckBox *mRenderSuppressionCBox;
    QToolButton *mMessageButton;
    QToolButton *mOnTheFlyProjectionStatusButton;
    QDockWidget *mLayerTreeDock;
    QDockWidget *mLayerOrderDock;

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
};

#ifdef ANDROID
#define QGIS_ICON_SIZE 32
#else
#define QGIS_ICON_SIZE 24
#endif

#endif // MAINWINDOW_H
