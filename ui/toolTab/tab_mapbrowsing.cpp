#include "tab_mapbrowsing.h"
#include "ui_tab_mapbrowsing.h"

tab_mapBrowsing::tab_mapBrowsing(QWidget *parent) :
    QToolBar(parent),
    ui(new Ui::tab_mapBrowsing)
{
    ui->setupUi(this);
}

tab_mapBrowsing::~tab_mapBrowsing()
{
    delete ui;
}
