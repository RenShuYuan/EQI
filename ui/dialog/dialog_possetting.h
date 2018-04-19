#ifndef DIALOG_POSSETTING_H
#define DIALOG_POSSETTING_H

#include "head.h"
#include <QDialog>
#include "qgscoordinatereferencesystem.h"

class QgsProjectionSelectionWidget;

namespace Ui {
class dialog_posSetting;
}

class dialog_posSetting : public QDialog
{
    Q_OBJECT

public:
    explicit dialog_posSetting(QWidget *parent = 0);
    ~dialog_posSetting();

public slots:
    void crsChanged( const QgsCoordinateReferenceSystem& crs );

private slots:
    void on_buttonBox_accepted();

private:
    Ui::dialog_posSetting *ui;

    QSettings mSettings;
    QgsCoordinateReferenceSystem mLayerDefaultCrs;
    QgsProjectionSelectionWidget* leUavLayerGlobalCrs;
};

#endif // DIALOG_POSSETTING_H
