#ifndef UAVDATAMANAGEMENT_H
#define UAVDATAMANAGEMENT_H

#include <QWidget>
#include <QToolBar>

namespace Ui {
class tab_uavDataManagement;
}

class tab_uavDataManagement : public QToolBar
{
    Q_OBJECT

public:
    explicit tab_uavDataManagement(QWidget *parent = 0);
    ~tab_uavDataManagement();

private:
    Ui::tab_uavDataManagement *ui;
};

#endif // UAVDATAMANAGEMENT_H
