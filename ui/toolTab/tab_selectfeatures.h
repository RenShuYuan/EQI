#ifndef TAB_SELECTFEATURES_H
#define TAB_SELECTFEATURES_H

#include <QWidget>
#include <QToolBar>

namespace Ui {
class tab_selectFeatures;
}

class tab_selectFeatures : public QToolBar
{
    Q_OBJECT

public:
    explicit tab_selectFeatures(QWidget *parent = 0);
    ~tab_selectFeatures();

private:
    Ui::tab_selectFeatures *ui;
};

#endif // TAB_SELECTFEATURES_H
