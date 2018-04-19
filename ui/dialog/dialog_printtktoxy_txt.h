#ifndef DIALOG_PRINTTKTOXY_TXT_H
#define DIALOG_PRINTTKTOXY_TXT_H

#include "head.h"
#include <QDialog>

namespace Ui {
class dialog_printTKtoXY_txt;
}

class dialog_printTKtoXY_txt : public QDialog
{
    Q_OBJECT

public:
    explicit dialog_printTKtoXY_txt(QWidget *parent = 0);
    ~dialog_printTKtoXY_txt();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_buttonBox_accepted();

private:
    Ui::dialog_printTKtoXY_txt *ui;

};

#endif // DIALOG_PRINTTKTOXY_TXT_H
