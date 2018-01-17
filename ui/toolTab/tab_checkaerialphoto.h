#ifndef TAB_CHECKAERIALPHOTO_H
#define TAB_CHECKAERIALPHOTO_H

#include <QWidget>
#include <QToolBar>

namespace Ui {
class tab_checkAerialPhoto;
}

class tab_checkAerialPhoto : public QToolBar
{
    Q_OBJECT

public:
    explicit tab_checkAerialPhoto(QWidget *parent = 0);
    ~tab_checkAerialPhoto();

private:
    Ui::tab_checkAerialPhoto *ui;
};

#endif // TAB_CHECKAERIALPHOTO_H
