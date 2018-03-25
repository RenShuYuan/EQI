#include "tab_inquire.h"
#include "ui_tab_inquire.h"

tab_inquire::tab_inquire(QWidget *parent) :
    QToolBar(parent),
    ui(new Ui::tab_inquire)
{
    ui->setupUi(this);
}

tab_inquire::~tab_inquire()
{
    delete ui;
}
