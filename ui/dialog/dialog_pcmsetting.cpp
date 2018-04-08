#include "dialog_pcmsetting.h"
#include "ui_dialog_pcmsetting.h"

dialog_pcmSetting::dialog_pcmSetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dialog_pcmSetting)
{
    ui->setupUi(this);

    int size = 0;
    size = mSettings.value("/eqi/options/pcm/height", 500).toInt();
    ui->spinBox->setValue(size);
    size = mSettings.value("/eqi/options/pcm/width", 500).toInt();
    ui->spinBox_2->setValue(size);
    bool isEnable = mSettings.value("/eqi/options/pcm/make", false).toBool();
    ui->checkBox->setChecked(isEnable);
}

dialog_pcmSetting::~dialog_pcmSetting()
{
    delete ui;
}

void dialog_pcmSetting::on_buttonBox_accepted()
{
    mSettings.setValue("/eqi/options/pcm/height", ui->spinBox->value());
    mSettings.setValue("/eqi/options/pcm/width", ui->spinBox_2->value());
    mSettings.setValue("/eqi/options/pcm/make", ui->checkBox->isChecked());
}
