#include "dialog_printtktoxy_txt.h"
#include "ui_dialog_printtktoxy_txt.h"
#include "eqi/eqifractalmanagement.h"
#include "eqi/eqiProjectionTransformation.h"
#include <QFileDialog>
#include <QSettings>

dialog_printTKtoXY_txt::dialog_printTKtoXY_txt(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dialog_printTKtoXY_txt)
{
    ui->setupUi(this);
}

dialog_printTKtoXY_txt::~dialog_printTKtoXY_txt()
{
    delete ui;
}

void dialog_printTKtoXY_txt::on_pushButton_clicked()
{
    // 输入
    QString fileName = QFileDialog::getOpenFileName(this, "选择标准图号文本", "", "文本文件 (*.txt)");
    if (!fileName.isEmpty())
    {
        ui->lineEdit->setText(QDir::toNativeSeparators(fileName));     // 将"/"转换为"\\"
    }
    else
        return;
}

void dialog_printTKtoXY_txt::on_pushButton_2_clicked()
{
    // 输出
    QString fileName = QFileDialog::getOpenFileName(this, "输出结果保存文本", "", "文本文件 (*.txt)");
    if (!fileName.isEmpty())
    {
        ui->lineEdit_2->setText(QDir::toNativeSeparators(fileName));     // 将"/"转换为"\\"
    }
    else
        return;
}

void dialog_printTKtoXY_txt::on_buttonBox_accepted()
{
    QSettings mSettings;
    QStringList thLists;
    eqiFractalManagement fm(this);

    // 读取文件内容
    QFile file(ui->lineEdit->text());
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return;
    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine();
        thLists.append(line);
    }
    file.close();

    // 写入文件
    file.setFileName(ui->lineEdit_2->text());
    if (!file.open(QFile::WriteOnly))
        return;
    QTextStream out(&file);

    foreach (QString th, thLists)
    {
        // 设置比例尺

        int scaleIndex = mSettings.value( "/FractalManagement/scale", 1).toInt();
        fm.setBlc( fm.getScale(scaleIndex) );

        QVector< QgsPoint > points = fm.dNToXy(th);
        out << th << QString( " %1 %2 %3 %4 %5 %6 %7 %8\n" ).arg(QString::number(points.at(0).x(), 10, 3))
                                                            .arg(QString::number(points.at(0).y(), 10, 3))
                                                            .arg(QString::number(points.at(1).x(), 10, 3))
                                                            .arg(QString::number(points.at(1).y(), 10, 3))
                                                            .arg(QString::number(points.at(2).x(), 10, 3))
                                                            .arg(QString::number(points.at(2).y(), 10, 3))
                                                            .arg(QString::number(points.at(3).x(), 10, 3))
                                                            .arg(QString::number(points.at(3).y(), 10, 3));
    }
    file.close();
}
