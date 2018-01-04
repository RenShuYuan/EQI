#include "tab_selectfeatures.h"
#include "ui_tab_selectfeatures.h"

tab_selectFeatures::tab_selectFeatures(QWidget *parent) :
    QToolBar(parent),
    ui(new Ui::tab_selectFeatures)
{
    ui->setupUi(this);
}

tab_selectFeatures::~tab_selectFeatures()
{
    delete ui;
}
