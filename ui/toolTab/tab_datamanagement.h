#ifndef TAB_DATAMANAGEMENT_H
#define TAB_DATAMANAGEMENT_H

#include <QWidget>
#include <QToolBar>

namespace Ui {
class tab_dataManagement;
}

class tab_dataManagement : public QToolBar
{
    Q_OBJECT

public:
    explicit tab_dataManagement(QWidget *parent = 0);
    ~tab_dataManagement();

private:
    Ui::tab_dataManagement *ui;
};

#endif // TAB_DATAMANAGEMENT_H
