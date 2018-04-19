#include <QApplication>
#include "eqigdalprogresstools.h"

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

    proDialog = new QProgressDialog("影像处理中...", "取消", 0, 100, nullptr);
    proDialog->setWindowTitle("处理进度");
    proDialog->setWindowModality(Qt::WindowModal);
}

eqiGdalProgressTools::~eqiGdalProgressTools()
{    
    delete proDialog;
}

void eqiGdalProgressTools::eqiGDALTranslate(const QString &str)
{   
    QStringList list = str.split(' ', QString::SkipEmptyParts);
    int argc = list.size();
    if ( argc==0 )
        return;

    char **argv=(char **)malloc(sizeof(char*)*argc);
    for (int var = 0; var < argc; ++var)
    {
        std::string str = list.at(var).toStdString();
        const char* p = str.c_str();
        size_t cSize = strlen(p);
        char *c = (char*)malloc(sizeof(char*)*cSize);
        strncpy(c, p, cSize);
        c[cSize] = '\0';
        argv[var] = c;
    }

    if (argc==0)
    {
        QgsMessageLog::logMessage("影像裁切：参数分割失败，已终止。");
        return;
    }

    // 提前设置配置信息
    EarlySetConfigOptions(argc, argv);

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
        proDialog->show();
        GDALTranslateOptionsSetProgress(psOptions, ALGTermProgress, proDialog);
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
            return;
        }
    }

    GDALDatasetH hDataset, hOutDS;
    int bUsageError;

    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    // 尝试打开数据源
    hDataset = GDALOpenEx(psOptionsForBinary->pszSource, GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR, NULL,
        (const char* const*)psOptionsForBinary->papszOpenOptions, NULL);

    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");

    if (hDataset == NULL)
    {
        QgsMessageLog::logMessage("影像裁切：影像源无法打开，已终止。");

        return;
    }

    // 处理子数据集
    if (!psOptionsForBinary->bCopySubDatasets
        && GDALGetRasterCount(hDataset) == 0
        && CSLCount(GDALGetMetadata(hDataset, "SUBDATASETS")) > 0)
    {
        QgsMessageLog::logMessage("影像裁切：输入的文件包含子数据集，请选择其中一个处理，已终止。");

        GDALClose(hDataset);
        return;
    }

    if (psOptionsForBinary->bCopySubDatasets &&
        CSLCount(GDALGetMetadata(hDataset, "SUBDATASETS")) > 0)
    {
        char **papszSubdatasets = GDALGetMetadata(hDataset, "SUBDATASETS");
        char *pszSubDest = (char *)CPLMalloc(strlen(psOptionsForBinary->pszDest) + 32);

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

        for (int i = 0; papszSubdatasets[i] != NULL; i += 2)
        {
            char* pszSource = CPLStrdup(strstr(papszSubdatasets[i], "=") + 1);
            osTemp = CPLSPrintf(pszFormat, osBasename.c_str(), i / 2 + 1);
            osTemp = CPLFormFilename(osPath, osTemp, osExtension);
            strcpy(pszSubDest, osTemp.c_str());

            CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

            hDataset = GDALOpenEx(pszSource, GDAL_OF_RASTER, NULL,
                (const char* const*)psOptionsForBinary->papszOpenOptions, NULL);
            CPLFree(pszSource);

            hOutDS = GDALTranslate(pszDest, hDataset, psOptions, &bUsageError);

            CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");

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

        return;
    }

    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    // 开始正儿八经的处理了
    hOutDS = GDALTranslate(psOptionsForBinary->pszDest, hDataset, psOptions, &bUsageError);

    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");

    if (bUsageError == TRUE)
    {
        QgsMessageLog::logMessage("影像裁切：出现未知错误，已终止。");
        return;
    }

    GDALClose(hOutDS);
    GDALClose(hDataset);
    GDALTranslateOptionsFree(psOptions);
    GDALTranslateOptionsForBinaryFree(psOptionsForBinary);
}

bool eqiGdalProgressTools::readRasterIO(const QString &rasterName, float **pDataBuffer, int &xSize, int &ySize)
{
    // 尝试打开数据源
    GDALDataset* poDataset = NULL;

    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    poDataset = (GDALDataset*)GDALOpenEx(rasterName.toStdString().c_str(), GDAL_OF_RASTER, NULL, NULL, NULL);

    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");

    if (poDataset != NULL)
    {
        if (poDataset->GetRasterCount()>0)
        {
            GDALRasterBand* pBand = poDataset->GetRasterBand(1);
            xSize = pBand->GetXSize();
            ySize = pBand->GetYSize();
            *pDataBuffer = new float[xSize*ySize];
            int err = pBand->RasterIO(GF_Read, 0, 0, xSize, ySize, *pDataBuffer, xSize, ySize, GDT_Float32, 0, 0);
            if (err == CE_None)
            {
                if (poDataset != NULL)
                    GDALClose((GDALDatasetH)poDataset);
                return true;
            }
            else
            {
                if (*pDataBuffer != NULL)
                {
                    delete *pDataBuffer; *pDataBuffer = NULL;
                }
            }
        }
    }
    QgsMessageLog::logMessage(QString("特征匹配：\t影像%1无法读取，已忽略。").arg(rasterName));
    if (poDataset!=NULL)
        GDALClose((GDALDatasetH)poDataset);
    return false;
}

int eqiGdalProgressTools::QStringToChar(const QString& str, char ***argv)
{
    int doneSize = 0;
    QStringList list = str.split(' ', QString::SkipEmptyParts);
    doneSize = list.size();
    if ( doneSize==0 )
        return doneSize;

    char **mArgv= *argv;
    for (int var = 0; var < doneSize; ++var)
    {
        std::string str = list.at(var).toStdString();
        const char* p = str.c_str();
        size_t cSize = strlen(p);
        char *c = new char[cSize];
        strncpy(c, p, cSize);
        c[cSize] = '\0';
        mArgv[var] = c;
    }

    return doneSize;
}

bool eqiGdalProgressTools::isCompression(const QString &str)
{
    // 尝试打开数据源
    GDALDataset* poDataset = NULL;

    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    poDataset = (GDALDataset*)GDALOpenEx(str.toStdString().c_str(), GDAL_OF_RASTER, NULL, NULL, NULL);

    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");

    if (poDataset != NULL)
    {
        char** info = poDataset->GetMetadata("Image_Structure");
        if( info != NULL && *info != NULL )
        {
            for( int i = 0; info[i] != NULL; i++ )
            {
                QString str = info[i];
                QStringList list = str.split('=');
                if (!list.isEmpty() && list.at(0)=="COMPRESSION")
                {
                    return true;
                }
            }
        }
        return false;
    }

    if (poDataset!=NULL)
        GDALClose((GDALDatasetH)poDataset);
    return false;
}

QString eqiGdalProgressTools::enumToString(const int value)
{
    switch (value) {
    case 1:
        return "Byte";
    case 2:
        return "UInt16";
    case 3:
        return "Int16";
    case 4:
        return "UInt32";
    case 5:
        return "Int32";
    case 6:
        return "Float32";
    case 7:
        return "Float64";
    case 8:
        return "CInt16";
    case 9:
        return "CInt32";
    case 10:
        return "CFloat32";
    case 11:
        return "CFloat64";
    default:
        return "Byte";
    }
}
