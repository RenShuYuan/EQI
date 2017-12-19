#include "dialog_prjtransformsetting.h"
#include "ui_dialog_prjtransformsetting.h"

dialog_PrjTransformSetting::dialog_PrjTransformSetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dialog_PrjTransformSetting)
{
    ui->setupUi(this);
    setWindowTitle("坐标参数设置");

    ui->comboBox->setCurrentIndex(mSettings.value( "/FractalManagement/scale", 1).toInt() );

    // 添加参照坐标系选择小组件
    leUavLayerGlobalCrs = new QgsProjectionSelectionWidget(this);
    QString mylayerDefaultCrs = mSettings.value( "/eqi/prjTransform/projectDefaultCrs", GEO_EPSG_CRS_AUTHID ).toString();
    mLayerDefaultCrs.createFromOgcWmsCrs( mylayerDefaultCrs );
    leUavLayerGlobalCrs->setCrs( mLayerDefaultCrs );
    leUavLayerGlobalCrs->setOptionVisible( QgsProjectionSelectionWidget::DefaultCrs, false );
    ui->gridLayout->addWidget(leUavLayerGlobalCrs, 0, 1, 1, 2);

    connect( leUavLayerGlobalCrs, SIGNAL( crsChanged(const QgsCoordinateReferenceSystem&) ),
             this, SLOT( crsChanged(const QgsCoordinateReferenceSystem&) ) );
    connect( this, SIGNAL( accepted() ), this, SLOT( saveOptions() ) );
}

dialog_PrjTransformSetting::~dialog_PrjTransformSetting()
{
    delete ui;
}

void dialog_PrjTransformSetting::crsChanged(const QgsCoordinateReferenceSystem &crs)
{
    mLayerDefaultCrs = crs;
}

void dialog_PrjTransformSetting::saveOptions()
{
    mSettings.setValue( "/eqi/prjTransform/projectDefaultCrs", mLayerDefaultCrs.authid() );
    mSettings.setValue( "/FractalManagement/scale", ui->comboBox->currentIndex() );
}
