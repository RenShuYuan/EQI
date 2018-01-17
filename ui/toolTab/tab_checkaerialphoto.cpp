#include "tab_checkaerialphoto.h"
#include "ui_tab_checkaerialphoto.h"

tab_checkAerialPhoto::tab_checkAerialPhoto(QWidget *parent) :
    QToolBar(parent),
    ui(new Ui::tab_checkAerialPhoto)
{
    ui->setupUi(this);
}

tab_checkAerialPhoto::~tab_checkAerialPhoto()
{
    delete ui;
}
