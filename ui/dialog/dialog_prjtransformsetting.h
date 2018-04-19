#ifndef DIALOG_PRJTRANSFORMSETTING_H
#define DIALOG_PRJTRANSFORMSETTING_H

#include "head.h"
#include <QDialog>
#include "qgsprojectionselectionwidget.h"
#include "qgscoordinatereferencesystem.h"

namespace Ui {
class dialog_PrjTransformSetting;
}

class dialog_PrjTransformSetting : public QDialog
{
    Q_OBJECT

public:
    explicit dialog_PrjTransformSetting(QWidget *parent = 0);
    ~dialog_PrjTransformSetting();

public slots:
    void crsChanged( const QgsCoordinateReferenceSystem& crs );
    void saveOptions();

private:
    Ui::dialog_PrjTransformSetting *ui;
    QSettings mSettings;
    QgsCoordinateReferenceSystem mDefaultCrs;
    QgsCoordinateReferenceSystem mLayerDefaultCrs;
    QgsProjectionSelectionWidget* leUavLayerGlobalCrs;
};

#endif // DIALOG_PRJTRANSFORMSETTING_H
