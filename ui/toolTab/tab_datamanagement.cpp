#include "tab_datamanagement.h"
#include "ui_tab_datamanagement.h"

tab_dataManagement::tab_dataManagement(QWidget *parent) :
    QToolBar(parent),
    ui(new Ui::tab_dataManagement)
{
    ui->setupUi(this);
}

tab_dataManagement::~tab_dataManagement()
{
    delete ui;
}
