#include "tab_fractalmanagement.h"
#include "ui_fractalmanagement.h"

tab_fractalManagement::tab_fractalManagement(QWidget *parent) :
    QToolBar(parent),
    ui(new Ui::fractalManagement)
{
    ui->setupUi(this);
}

tab_fractalManagement::~tab_fractalManagement()
{
    delete ui;
}
