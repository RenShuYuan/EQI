#include "dialog_selectsetting.h"
#include "ui_dialog_selectsetting.h"

dialog_selectSetting::dialog_selectSetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dialog_selectSetting)
{
    ui->setupUi(this);

    // 航飞质量参数
    double value_f = 0.0;
    int value_i = 0;
    value_i = mSettings.value("/eqi/options/imagePreprocessing/heading_Min", 53).toInt();
    ui->spinBox->setValue(value_i);
    value_i = mSettings.value("/eqi/options/imagePreprocessing/heading_Max", 80).toInt();
    ui->spinBox_2->setValue(value_i);
    value_i = mSettings.value("/eqi/options/imagePreprocessing/heading_Gen", 60).toInt();
    ui->spinBox_3->setValue(value_i);
    value_i = mSettings.value("/eqi/options/imagePreprocessing/sideways_Min", 8).toInt();
    ui->spinBox_4->setValue(value_i);
    value_i = mSettings.value("/eqi/options/imagePreprocessing/sideways_Max", 60).toInt();
    ui->spinBox_5->setValue(value_i);
    value_i = mSettings.value("/eqi/options/imagePreprocessing/sideways_Gen", 15).toInt();
    ui->spinBox_6->setValue(value_i);

    value_f = mSettings.value("/eqi/options/imagePreprocessing/omega_angle_General", 5.0).toDouble();
    ui->doubleSpinBox->setValue(value_f);
    value_f = mSettings.value("/eqi/options/imagePreprocessing/omega_angle_Max", 12.0).toDouble();
    ui->doubleSpinBox_2->setValue(value_f);
    value_f = mSettings.value("/eqi/options/imagePreprocessing/omega_angle_Average", 8.0).toDouble();
    ui->doubleSpinBox_3->setValue(value_f);
    value_f = mSettings.value("/eqi/options/imagePreprocessing/kappa_angle_General", 15.0).toDouble();
    ui->doubleSpinBox_4->setValue(value_f);
    value_f = mSettings.value("/eqi/options/imagePreprocessing/kappa_angle_Max", 30.0).toDouble();
    ui->doubleSpinBox_5->setValue(value_f);
    value_f = mSettings.value("/eqi/options/imagePreprocessing/kappa_angle_Line", 20.0).toDouble();
    ui->doubleSpinBox_6->setValue(value_f);

    // 航摄数据删除与保存
    int iDelete = mSettings.value("/eqi/options/selectEdit/delete", DELETE_DIR).toInt();
    int iSave = mSettings.value("/eqi/options/selectEdit/save", SAVE_TEMPDIR).toInt();

    if (iDelete == DELETE_PHYSICAL)
        ui->radioButton->setChecked(true);
    else
        ui->radioButton_2->setChecked(true);

    if (iSave == SAVE_CUSTOMIZE)
        ui->radioButton_3->setChecked(true);
    else
        ui->radioButton_4->setChecked(true);
}

dialog_selectSetting::~dialog_selectSetting()
{
    delete ui;
}

void dialog_selectSetting::on_buttonBox_accepted()
{
    mSettings.setValue("/eqi/options/imagePreprocessing/heading_Min", ui->spinBox->value());
    mSettings.setValue("/eqi/options/imagePreprocessing/heading_Max", ui->spinBox_2->value());
    mSettings.setValue("/eqi/options/imagePreprocessing/heading_Gen", ui->spinBox_3->value());
    mSettings.setValue("/eqi/options/imagePreprocessing/sideways_Min", ui->spinBox_4->value());
    mSettings.setValue("/eqi/options/imagePreprocessing/sideways_Max", ui->spinBox_5->value());
    mSettings.setValue("/eqi/options/imagePreprocessing/sideways_Gen", ui->spinBox_6->value());

    mSettings.setValue("/eqi/options/imagePreprocessing/omega_angle_General", ui->doubleSpinBox->value());
    mSettings.setValue("/eqi/options/imagePreprocessing/omega_angle_Max", ui->doubleSpinBox_2->value());
    mSettings.setValue("/eqi/options/imagePreprocessing/omega_angle_Average", ui->doubleSpinBox_3->value());
    mSettings.setValue("/eqi/options/imagePreprocessing/kappa_angle_General", ui->doubleSpinBox_4->value());
    mSettings.setValue("/eqi/options/imagePreprocessing/kappa_angle_Max", ui->doubleSpinBox_5->value());
    mSettings.setValue("/eqi/options/imagePreprocessing/kappa_angle_Line", ui->doubleSpinBox_6->value());

    if (ui->radioButton->isChecked())
        mSettings.setValue( "/eqi/options/selectEdit/delete", DELETE_PHYSICAL );
    else
        mSettings.setValue( "/eqi/options/selectEdit/delete", DELETE_DIR );

    if (ui->radioButton_3->isChecked())
        mSettings.setValue( "/eqi/options/selectEdit/save", SAVE_CUSTOMIZE );
    else
        mSettings.setValue( "/eqi/options/selectEdit/save", SAVE_TEMPDIR );
}
