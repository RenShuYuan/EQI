#ifndef FRACTALMANAGEMENT_H
#define FRACTALMANAGEMENT_H

#include <QWidget>
#include <QToolBar>

namespace Ui {
class fractalManagement;
}

class tab_fractalManagement : public QToolBar
{
    Q_OBJECT

public:
    explicit tab_fractalManagement(QWidget *parent = 0);
    ~tab_fractalManagement();

private:
    Ui::fractalManagement *ui;
};

#endif // FRACTALMANAGEMENT_H
