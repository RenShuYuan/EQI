#ifndef DIALOG_POSLOADDIALOG_H
#define DIALOG_POSLOADDIALOG_H

#include <QDialog>
#include <QSettings>
#include "ui_dialog_posloaddialog.h"

class QStringList;
class QgsDelimitedTextFile;

class dialog_posloaddialog : public QDialog
{

    Q_OBJECT

public:
    dialog_posloaddialog(QWidget *parent = 0);
    ~dialog_posloaddialog();

signals:
    void readFieldsList(QString &);

private slots:
    void on_buttonBox_accepted();

    // 选择POS文件
    void on_btnOpenPos_clicked();

    // POS文件路径lineEdit
    void on_lePosFile_textChanged();

    // 更新字段列表
    void updateFieldLists();

private:
    // 选择POS文件
    void getOpenFileName();

    // 从界面获得参数，并返回是否有效
    bool loadDelimitedFileDefinition();

    // 返回选择的分隔符
    QString selectedChars();

    // 保存和读取设置
    void loadSettings();
    void saveSettings();

private:
    Ui::dialog_posdialog ui;
    QgsDelimitedTextFile *mFile;
    QSettings mSettings;

    int mExampleRowCount;
    int mBadRowCount;
};

#endif // DIALOG_POSLOADDIALOG_H
