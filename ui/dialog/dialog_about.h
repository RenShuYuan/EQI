#ifndef DIALOG_ABOUT_H
#define DIALOG_ABOUT_H

#include <QWidget>

namespace Ui {
class dialog_about;
}

class dialog_about : public QWidget
{
    Q_OBJECT

public:
    explicit dialog_about(QWidget *parent = 0);
    ~dialog_about();

private slots:
    void on_pushButton_clicked();

private:
    Ui::dialog_about *ui;
};

#endif // DIALOG_ABOUT_H
