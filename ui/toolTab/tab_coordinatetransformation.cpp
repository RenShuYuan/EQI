#include "tab_coordinatetransformation.h"
#include "ui_tab_coordinatetransformation.h"

tab_coordinateTransformation::tab_coordinateTransformation(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::tab_coordinateTransformation)
{
    ui->setupUi(this);
}

tab_coordinateTransformation::~tab_coordinateTransformation()
{
    delete ui;
}
