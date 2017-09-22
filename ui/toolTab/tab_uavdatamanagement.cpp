#include "tab_uavdatamanagement.h"
#include "ui_tab_uavdatamanagement.h"

tab_uavDataManagement::tab_uavDataManagement(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::tab_uavDataManagement)
{
    ui->setupUi(this);
}

tab_uavDataManagement::~tab_uavDataManagement()
{
    delete ui;
}
