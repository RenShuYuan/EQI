#ifndef DIALOG_PCMSETTING_H
#define DIALOG_PCMSETTING_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class dialog_pcmSetting;
}

class dialog_pcmSetting : public QDialog
{
    Q_OBJECT

public:
    explicit dialog_pcmSetting(QWidget *parent = 0);
    ~dialog_pcmSetting();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::dialog_pcmSetting *ui;
    QSettings mSettings;
};

#endif // DIALOG_PCMSETTING_H
