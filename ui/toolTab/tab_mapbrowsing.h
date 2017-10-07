#ifndef TAB_MAPBROWSING_H
#define TAB_MAPBROWSING_H

#include <QWidget>
#include <QToolBar>

namespace Ui {
class tab_mapBrowsing;
}

class tab_mapBrowsing : public QToolBar
{
    Q_OBJECT

public:
    explicit tab_mapBrowsing(QWidget *parent = 0);
    ~tab_mapBrowsing();

private:
    Ui::tab_mapBrowsing *ui;
};

#endif // TAB_MAPBROWSING_H
