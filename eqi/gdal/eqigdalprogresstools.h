#ifndef EQIGDALPROGRESSTOOLS_H
#define EQIGDALPROGRESSTOOLS_H

#include <QString>
#include <QProgressDialog>

#define GL_RED                            0x1903
#define GL_RGB                            0x1907
#define GL_LUMINANCE                      0x1909
#define GL_STENCIL_INDEX                  0x1901
#define GL_DEPTH_COMPONENT                0x1902
#define GL_INTENSITY8                     0x804B
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_FLOAT                          0x1406

/*! 8U */
typedef unsigned char                     DT_8U;

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

    bool readRasterIO(const QString& rasterName, float **pDataBuffer, int &xSize, int &ySize);

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
    int QStringToChar(const QString& str, char ***argv);

    static QString enumToString(const int value);
private:
    QProgressDialog *proDialog;
};

#endif // EQIGDALPROGRESSTOOLS_H
