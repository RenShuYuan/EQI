#ifndef TAB_INQUIRE_H
#define TAB_INQUIRE_H

#include <QWidget>
#include <QToolBar>

namespace Ui {
class tab_inquire;
}

class tab_inquire : public QToolBar
{
    Q_OBJECT

public:
    explicit tab_inquire(QWidget *parent = 0);
    ~tab_inquire();

private:
    Ui::tab_inquire *ui;
};

#endif // TAB_INQUIRE_H
