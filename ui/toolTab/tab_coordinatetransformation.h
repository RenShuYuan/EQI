#ifndef TAB_COORDINATETRANSFORMATION_H
#define TAB_COORDINATETRANSFORMATION_H

#include <QWidget>

namespace Ui {
class tab_coordinateTransformation;
}

class tab_coordinateTransformation : public QWidget
{
    Q_OBJECT

public:
    explicit tab_coordinateTransformation(QWidget *parent = 0);
    ~tab_coordinateTransformation();

private:
    Ui::tab_coordinateTransformation *ui;
};

#endif // TAB_COORDINATETRANSFORMATION_H
