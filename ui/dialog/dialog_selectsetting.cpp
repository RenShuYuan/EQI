#include "dialog_selectsetting.h"
#include "ui_dialog_selectsetting.h"

dialog_selectSetting::dialog_selectSetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dialog_selectSetting)
{
    ui->setupUi(this);

    int iDelete = mSettings.value("/eqi/options/selectEdit/delete", DELETE_DIR).toInt();
    int iSave = mSettings.value("/eqi/options/selectEdit/save", SAVE_TEMPDIR).toInt();

    if (iDelete == DELETE_PHYSICAL)
    {
        ui->radioButton->setChecked(true);
    }
    else
    {
        ui->radioButton_2->setChecked(true);
    }

    if (iSave == SAVE_CUSTOMIZE)
    {
        ui->radioButton_3->setChecked(true);
    }
    else
    {
        ui->radioButton_4->setChecked(true);
    }
}

dialog_selectSetting::~dialog_selectSetting()
{
    delete ui;
}

void dialog_selectSetting::on_buttonBox_accepted()
{
    if (ui->radioButton->isChecked())
    {
        mSettings.setValue( "/eqi/options/selectEdit/delete", DELETE_PHYSICAL );
    }
    else
    {
        mSettings.setValue( "/eqi/options/selectEdit/delete", DELETE_DIR );
    }

    if (ui->radioButton_3->isChecked())
    {
        mSettings.setValue( "/eqi/options/selectEdit/save", SAVE_CUSTOMIZE );
    }
    else
    {
        mSettings.setValue( "/eqi/options/selectEdit/save", SAVE_TEMPDIR );
    }
}
