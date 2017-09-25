#include "tab_uavdatamanagement.h"
#include "ui_tab_uavdatamanagement.h"

tab_uavDataManagement::tab_uavDataManagement(QWidget *parent) :
    QToolBar(parent),
    ui(new Ui::tab_uavDataManagement)
{
    ui->setupUi(this);
    setMinimumHeight(80);
}

tab_uavDataManagement::~tab_uavDataManagement()
{
    delete ui;
}
