#include <QStringList>
#include <QDebug>
#include <QApplication>

#include "eqigdalprogresstools.h"
#include "qgsmessagelog.h"

#include "cpl_string.h"
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "gdal/commonutils.h"
#include "gdal/gdal_utils_priv.h"

/************************************************************************/
/*                       GDALTranslateOptionsForBinaryNew()             */
/************************************************************************/

static GDALTranslateOptionsForBinary *GDALTranslateOptionsForBinaryNew(void)
{
    return (GDALTranslateOptionsForBinary*) CPLCalloc(  1, sizeof(GDALTranslateOptionsForBinary) );
}

/************************************************************************/
/*                       GDALTranslateOptionsForBinaryFree()            */
/************************************************************************/

static void GDALTranslateOptionsForBinaryFree( GDALTranslateOptionsForBinary* psOptionsForBinary )
{
    if( psOptionsForBinary )
    {
        CPLFree(psOptionsForBinary->pszSource);
        CPLFree(psOptionsForBinary->pszDest);
        CSLDestroy(psOptionsForBinary->papszOpenOptions);
        CPLFree(psOptionsForBinary->pszFormat);
        CPLFree(psOptionsForBinary);
    }
}

/**
* @brief 导出符号定义
*/
#ifndef STD_API
#define STD_API __stdcall
#endif
/**
* \brief 调用GDAL进度条接口
*
* 该函数用于将GDAL算法中的进度信息导出到CProcessBase基类中，供给界面显示
*
* @param dfComplete 完成进度值，其取值为 0.0 到 1.0 之间
* @param pszMessage 进度信息
* @param pProgressArg   CProcessBase的指针
*
* @return 返回TRUE表示继续计算，否则为取消
*/
int STD_API ALGTermProgress(double dfComplete, const char *pszMessage, void *pProgressArg)
{
    if(pProgressArg != NULL)
    {
        QProgressDialog * pProcess = (QProgressDialog*) pProgressArg;
        pProcess->setValue((int)(dfComplete*100));

        QApplication::processEvents();
        if (pProcess->wasCanceled())
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else
        return TRUE;
}

eqiGdalProgressTools::eqiGdalProgressTools()
{
    //添加环境变量
//    CPLSetConfigOption("GDAL_DATA", "C:\\gdal224-release-1600-dev\\release-1600\\data");

    //支持中文路径
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    proDialog = new QProgressDialog("影像处理中...", "取消", 0, 100, nullptr);
    proDialog->setWindowTitle("处理进度");
    proDialog->setWindowModality(Qt::WindowModal);
}

eqiGdalProgressTools::~eqiGdalProgressTools()
{
    // 卸载
//    GDALDestroyDriverManager();

    delete proDialog;
}

void eqiGdalProgressTools::eqiGDALTranslate(const QString &str)
{
    int argc = 0;
    char **argv = NULL;
    argv = QStringToChar(str, argc);

    if (argc==0)
    {
        QgsMessageLog::logMessage("影像裁切：参数分割失败，已终止。");
        return;
    }

    // 提前设置配置信息
    EarlySetConfigOptions(argc, argv);

    //注册所有影像格式
    GDALAllRegister();

    // GDAL通用命令行处理
    argc = GDALGeneralCmdLineProcessor(argc, &argv, 0);
    if (argc < 1)
    {
        QgsMessageLog::logMessage("影像裁切：GDAL参数处理失败，已终止。");
        return;
    }

    // 通过设置获得最佳性能
    if (CPLGetConfigOption("GDAL_MAX_DATASET_POOL_SIZE", NULL) == NULL)
    {
#if defined(__MACH__) && defined(__APPLE__)
        // On Mach, the default limit is 256 files per process
        // TODO We should eventually dynamically query the limit for all OS
        CPLSetConfigOption("GDAL_MAX_DATASET_POOL_SIZE", "100");
#else
        CPLSetConfigOption("GDAL_MAX_DATASET_POOL_SIZE", "450");
#endif
    }

    GDALTranslateOptionsForBinary* psOptionsForBinary = GDALTranslateOptionsForBinaryNew();
    GDALTranslateOptions *psOptions = GDALTranslateOptionsNew(argv, psOptionsForBinary);

    // 释放字符串列表
    CSLDestroy(argv);

    if (psOptions == NULL)
    {
        QgsMessageLog::logMessage("影像裁切：参数列表为空，已终止。");
        return;
    }

    if (psOptionsForBinary->pszSource == NULL)
    {
        QgsMessageLog::logMessage("影像裁切：数据源参数为空，已终止。");
        return;
    }

    if (psOptionsForBinary->pszDest == NULL)
    {
        QgsMessageLog::logMessage("影像裁切：数据目标参数为空，已终止。");
        return;
    }

    // 是否禁用标准输出
    if (strcmp(psOptionsForBinary->pszDest, "/vsistdout/") == 0)
    {
        psOptionsForBinary->bQuiet = TRUE;
    }
    if (!(psOptionsForBinary->bQuiet))
    {
//        qDebug("proDialog->value()=%d", proDialog->value());
        proDialog->show();
        GDALTranslateOptionsSetProgress(psOptions, ALGTermProgress, proDialog);
//        GDALTranslateOptionsSetProgress(psOptions, GDALTermProgress, NULL);
    }

    // 检查输出格式，如果不正确，则将列出所支持的格式
    if (psOptionsForBinary->pszFormat)
    {
        GDALDriverH hDriver = GDALGetDriverByName(psOptionsForBinary->pszFormat);
        if (hDriver == NULL)
        {
            QgsMessageLog::logMessage("影像裁切：不支持输出格式，请重新设置，已终止。");

            GDALTranslateOptionsFree(psOptions);
            GDALTranslateOptionsForBinaryFree(psOptionsForBinary);
            GDALDestroyDriverManager();
            return;
        }
    }

    GDALDatasetH hDataset, hOutDS;
    int bUsageError;

    // 尝试打开数据源
    hDataset = GDALOpenEx(psOptionsForBinary->pszSource, GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR, NULL,
        (const char* const*)psOptionsForBinary->papszOpenOptions, NULL);

    if (hDataset == NULL)
    {
        QgsMessageLog::logMessage("影像裁切：影像源无法打开，已终止。");

        GDALDestroyDriverManager();
        return;
    }

    // 处理子数据集
    if (!psOptionsForBinary->bCopySubDatasets
        && GDALGetRasterCount(hDataset) == 0
        && CSLCount(GDALGetMetadata(hDataset, "SUBDATASETS")) > 0)
    {
        QgsMessageLog::logMessage("影像裁切：输入的文件包含子数据集，请选择其中一个处理，已终止。");

        GDALClose(hDataset);
        GDALDestroyDriverManager();
        return;
    }

    if (psOptionsForBinary->bCopySubDatasets &&
        CSLCount(GDALGetMetadata(hDataset, "SUBDATASETS")) > 0)
    {
        char **papszSubdatasets = GDALGetMetadata(hDataset, "SUBDATASETS");
        char *pszSubDest = (char *)CPLMalloc(strlen(psOptionsForBinary->pszDest) + 32);
        int i;

        CPLString osPath = CPLGetPath(psOptionsForBinary->pszDest);
        CPLString osBasename = CPLGetBasename(psOptionsForBinary->pszDest);
        CPLString osExtension = CPLGetExtension(psOptionsForBinary->pszDest);
        CPLString osTemp;

        const char* pszFormat = NULL;
        if (CSLCount(papszSubdatasets) / 2 < 10)
        {
            pszFormat = "%s_%d";
        }
        else if (CSLCount(papszSubdatasets) / 2 < 100)
        {
            pszFormat = "%s_%002d";
        }
        else
        {
            pszFormat = "%s_%003d";
        }

        const char* pszDest = pszSubDest;

        for (i = 0; papszSubdatasets[i] != NULL; i += 2)
        {
            char* pszSource = CPLStrdup(strstr(papszSubdatasets[i], "=") + 1);
            osTemp = CPLSPrintf(pszFormat, osBasename.c_str(), i / 2 + 1);
            osTemp = CPLFormFilename(osPath, osTemp, osExtension);
            strcpy(pszSubDest, osTemp.c_str());
            hDataset = GDALOpenEx(pszSource, GDAL_OF_RASTER, NULL,
                (const char* const*)psOptionsForBinary->papszOpenOptions, NULL);
            CPLFree(pszSource);

            hOutDS = GDALTranslate(pszDest, hDataset, psOptions, &bUsageError);
            if (bUsageError)
                return;
            if (hOutDS == NULL)
                break;
            GDALClose(hOutDS);
        }

        GDALClose(hDataset);
        GDALTranslateOptionsFree(psOptions);
        GDALTranslateOptionsForBinaryFree(psOptionsForBinary);
        CPLFree(pszSubDest);

        GDALDestroyDriverManager();
        return;
    }

    // 开始正儿八经的处理了
    hOutDS = GDALTranslate(psOptionsForBinary->pszDest, hDataset, psOptions, &bUsageError);
    if (bUsageError == TRUE)
    {
        QgsMessageLog::logMessage("影像裁切：出现未知错误，已终止。");
        return;
    }

    GDALClose(hOutDS);
    GDALClose(hDataset);
    GDALTranslateOptionsFree(psOptions);
    GDALTranslateOptionsForBinaryFree(psOptionsForBinary);

    GDALDestroyDriverManager();
    qDebug() << "执行。。。。。。释放";
}

char** eqiGdalProgressTools::QStringToChar(const QString &str, int &doneSize)
{
    QStringList list = str.split(' ', QString::SkipEmptyParts);
    doneSize = list.size();
    if (doneSize==0)
        return NULL;

//    char **argv=(char **)malloc(sizeof(char*)*doneSize);
    char **argv=new char*[doneSize];
    for (int var = 0; var < doneSize; ++var)
    {
        std::string str = list.at(var).toStdString();
        const char* p = str.c_str();
        size_t cSize = strlen(p);
        char *c = new char[cSize];
        strncpy(c, p, cSize);
        c[cSize] = '\0';
        argv[var] = c;
    }

    return argv;
}
