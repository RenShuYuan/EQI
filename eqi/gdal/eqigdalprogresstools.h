#ifndef EQIGDALPROGRESSTOOLS_H
#define EQIGDALPROGRESSTOOLS_H

#include <QString>
#include <QProgressDialog>

class eqiGdalProgressTools
{
public:
    eqiGdalProgressTools();
    ~eqiGdalProgressTools();

    /**
    * \brief 调用GDALTranslate函数处理数据
    *
    * 该函数与GDALTranslate实用工具功能一致。
    *
    * @param QString 输入与GDALTranslate实用工具一致的参数
    *
    * @return
    */
    void eqiGDALTranslate(const QString &str);

    /**
    * \brief 分割QString字符串
    *
    * 该函数用于将QString字符串以空格分割，并复制到char**中返回。
    *
    * @param QString 以空格分开的字符串
    * @param doneSize 成功分割的字符串数量
    *
    * @return 返回char**，类似于main()参数。
    */
    static char **QStringToChar(const QString& str, int &doneSize);

private:
    QProgressDialog *proDialog;
};

#endif // EQIGDALPROGRESSTOOLS_H
