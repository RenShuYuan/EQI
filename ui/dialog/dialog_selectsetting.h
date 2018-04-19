#ifndef DIALOG_SELECTSETTING_H
#define DIALOG_SELECTSETTING_H

#define DELETE_PHYSICAL 10001
#define DELETE_DIR 10002
#define SAVE_CUSTOMIZE 10003
#define SAVE_TEMPDIR 10004

#include "head.h"
#include <QDialog>

namespace Ui {
class dialog_selectSetting;
}

class dialog_selectSetting : public QDialog
{
    Q_OBJECT

public:
    explicit dialog_selectSetting(QWidget *parent = 0);
    ~dialog_selectSetting();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::dialog_selectSetting *ui;

    QSettings mSettings;
};

#endif // DIALOG_SELECTSETTING_H
