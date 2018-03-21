#include "dialog_possetting.h"
#include "ui_dialog_possetting.h"
#include "qgsprojectionselectionwidget.h"

#include <QDebug>

dialog_posSetting::dialog_posSetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dialog_posSetting)
{
    ui->setupUi(this);

    double tmpDouble = 0.0;
    int tmpInt = 0;
    bool blchk = false;

    // 添加参照坐标系选择小组件
    leUavLayerGlobalCrs = new QgsProjectionSelectionWidget(this);
    QString mylayerDefaultCrs = mSettings.value( "/eqi/prjTransform/projectDefaultCrs",
                                                  GEO_EPSG_CRS_AUTHID ).toString();
    mLayerDefaultCrs.createFromOgcWmsCrs( mylayerDefaultCrs );
    leUavLayerGlobalCrs->setCrs( mLayerDefaultCrs );
    leUavLayerGlobalCrs->setOptionVisible( QgsProjectionSelectionWidget::DefaultCrs, false );
    ui->gridLayout_2->addWidget(leUavLayerGlobalCrs, 0, 1, 1, 3);
    connect( leUavLayerGlobalCrs, SIGNAL( crsChanged(const QgsCoordinateReferenceSystem&) ),
             this, SLOT( crsChanged(const QgsCoordinateReferenceSystem&) ) );

    // 相机参数 初始化
    tmpDouble = mSettings.value("/eqi/options/imagePreprocessing/leFocal", 0.0).toDouble();
    ui->lineEdit_2->setText(QString::number(tmpDouble, 'f', 9));
    tmpDouble = mSettings.value("/eqi/options/imagePreprocessing/lePixelSize", 0.0).toDouble();
    ui->lineEdit_3->setText(QString::number(tmpDouble, 'f', 3));
    tmpInt = mSettings.value("/eqi/options/imagePreprocessing/leHeight", 0).toInt();
    ui->lineEdit_4_1->setText(QString::number(tmpInt));
    tmpInt = mSettings.value("/eqi/options/imagePreprocessing/leWidth", 0).toInt();
    ui->lineEdit_4_2->setText(QString::number(tmpInt));
    tmpDouble = mSettings.value("/eqi/options/imagePreprocessing/leAverageEle", 0.0).toDouble();
    ui->lineEdit_5->setText(QString::number(tmpDouble, 'f', 2));

    // 曝光点一键处理 初始化
    blchk = mSettings.value("/eqi/options/imagePreprocessing/chkTransform", true).toBool();
    ui->checkBox->setChecked(blchk);
    blchk = mSettings.value("/eqi/options/imagePreprocessing/chkSketchMap", true).toBool();
    ui->checkBox_2->setChecked(blchk);
    blchk = mSettings.value("/eqi/options/imagePreprocessing/chkLinkPhoto", true).toBool();
    ui->checkBox_3->setChecked(blchk);
    blchk = mSettings.value("/eqi/options/imagePreprocessing/chkPosExport", true).toBool();
    ui->checkBox_4->setChecked(blchk);
    QString strPhotoName = mSettings.value("/eqi/options/imagePreprocessing/lePhotoFolder", "photo").toString();
    ui->lineEdit_6->setText(strPhotoName);
    ui->label_6->setEnabled(ui->checkBox_3->isChecked());
    ui->lineEdit_6->setEnabled(ui->checkBox_3->isChecked());

    connect(ui->checkBox_3, SIGNAL(toggled (bool) ), ui->label_6, SLOT( setEnabled (bool) ));
    connect(ui->checkBox_3, SIGNAL(toggled (bool) ), ui->lineEdit_6, SLOT( setEnabled (bool) ));

    // 航摄略图显示参数 初始化
    double scale = mSettings.value("/eqi/options/imagePreprocessing/dspScale", 0.1).toDouble();
    ui->doubleSpinBox->setValue(scale);
}

dialog_posSetting::~dialog_posSetting()
{
    delete ui;
}

void dialog_posSetting::crsChanged(const QgsCoordinateReferenceSystem &crs)
{
    mLayerDefaultCrs = crs;
}

void dialog_posSetting::on_buttonBox_accepted()
{
    // 保存影像预处理参数
    mSettings.setValue( "/eqi/options/imagePreprocessing/leFocal", ui->lineEdit_2->text().toDouble() );
    mSettings.setValue( "/eqi/options/imagePreprocessing/lePixelSize", ui->lineEdit_3->text().toDouble() );
    mSettings.setValue( "/eqi/options/imagePreprocessing/leHeight", ui->lineEdit_4_1->text().toInt() );
    mSettings.setValue( "/eqi/options/imagePreprocessing/leWidth", ui->lineEdit_4_2->text().toInt() );
    mSettings.setValue( "/eqi/options/imagePreprocessing/leAverageEle", ui->lineEdit_5->text().toDouble() );

    mSettings.setValue("/eqi/options/imagePreprocessing/chkTransform", ui->checkBox->isChecked());
    mSettings.setValue("/eqi/options/imagePreprocessing/chkSketchMap", ui->checkBox_2->isChecked());
    mSettings.setValue("/eqi/options/imagePreprocessing/chkLinkPhoto", ui->checkBox_3->isChecked());
    mSettings.setValue("/eqi/options/imagePreprocessing/chkPosExport", ui->checkBox_4->isChecked());
    mSettings.setValue("/eqi/options/imagePreprocessing/lePhotoFolder", ui->lineEdit_6->text());

    mSettings.setValue( "/eqi/prjTransform/projectDefaultCrs", mLayerDefaultCrs.authid() );

    mSettings.setValue("/eqi/options/imagePreprocessing/dspScale", ui->doubleSpinBox->value());
}
