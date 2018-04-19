#include "overlappingprocessing.h"
#include "qgsgeometry.h"
#include "siftGPU.h"

OverlappingProcessing::OverlappingProcessing()
{

}

OverlappingProcessing::~OverlappingProcessing()
{
    QMapIterator<QString, Ol*> it(map);
    while (it.hasNext())
    {
        Ol* ol = it.next().value();
        delete ol;
        ol = nullptr;
    }
}

void OverlappingProcessing::addLineNumber(QString &photoNumber, int lineNumber)
{
    if (!map.contains(photoNumber))
    {
        map[photoNumber] = new Ol(lineNumber);
    }
    else
    {
        Ol* ol = map.value(photoNumber);
        ol->mLineNumber = lineNumber;
    }
}

void OverlappingProcessing::setQgsFeature(const QString &photoNumber, QgsFeature *f)
{
    if (!map.contains(photoNumber))
    {
        Ol* ol = new Ol();
        ol->mF = f;
        map[photoNumber] = ol;
    }
    else
    {
        Ol* ol = map.value(photoNumber);
        ol->mF = f;
    }
}

void OverlappingProcessing::setNextPhotoOl(const QString &currentNumber,
                                            QString nextNumber, const int n)
{
    if (n == 0) return;

    if (map.contains(currentNumber))
    {
        Ol* ol = map.value(currentNumber);
        ol->mMapNextPhoto.clear();
        ol->mMapNextPhoto.insert(nextNumber, n);
    }
}

void OverlappingProcessing::setNextPhotoKeys(const QString &currentNumber, QString nextNumber, const int n)
{
    if (n == 0) return;

    if (map.contains(currentNumber))
    {
        Ol* ol = map.value(currentNumber);
        ol->mMapNextPhotoKeys.clear();
        ol->mMapNextPhotoKeys.insert(nextNumber, n);
    }
}

void OverlappingProcessing::setNextLineOl(const QString &currentNumber
                                          , QString photoNumber, const int n)
{
    if (n == 0) return;

    if (map.contains(currentNumber))
    {
        Ol* ol = map.value(currentNumber);
        if (ol->mMapNextLine.size() == 0)
        {
            ol->mMapNextLine.insert(photoNumber, n);
        }
        else
        {
            QMap<QString, int>::const_iterator it = ol->mMapNextLine.begin();
            if (it.value() < n)
            {
                ol->mMapNextLine.clear();
                ol->mMapNextLine.insert(photoNumber, n);
            }
        }
    }
}

void OverlappingProcessing::setNextLineKeys(const QString &currentNumber, QString nextNumber, const int n)
{
    if (n == 0) return;

    if (map.contains(currentNumber))
    {
        Ol* ol = map.value(currentNumber);
        ol->mMapNextLineKeys.clear();
        ol->mMapNextLineKeys.insert(nextNumber, n);
    }
}

int OverlappingProcessing::getLineNumber(const QString &photoNumber)
{
    if (map.contains(photoNumber))
    {
        return map.value(photoNumber)->mLineNumber;
    }
    return -1;
}

void OverlappingProcessing::updataLineNumber()
{
    int lastLineNumber = 0;
    int currentLineNumber = 0;
    int actualNumber = 0;
    QMap<QString, Ol*>::const_iterator itBegin = map.constBegin();
    while (itBegin != map.constEnd())
    {
        // 当前相片航线号
        currentLineNumber = itBegin.value()->mLineNumber;

        if (lastLineNumber != currentLineNumber)
        {
            lastLineNumber = currentLineNumber;
            ++actualNumber;
        }

        if (currentLineNumber != actualNumber)
        {
            QString key = itBegin.key();
            addLineNumber(key, actualNumber);
        }
        ++itBegin;
    }
}

void OverlappingProcessing::updataPhotoNumber()
{
    QMap<QString, Ol*>::iterator itBegin = map.constBegin();
    while (itBegin != map.constEnd())
    {
        Ol* ol = itBegin.value();

        if (!(ol->mMapNextPhoto.isEmpty()))
        {
            QMap<QString, int>::const_iterator it = ol->mMapNextPhoto.constBegin();
            if (!map.contains(it.key()))
            {
                ol->mMapNextPhoto.clear();
            }
        }

        if (!(ol->mMapNextPhotoKeys.isEmpty()))
        {
            QMap<QString, int>::const_iterator it = ol->mMapNextPhotoKeys.constBegin();
            if (!map.contains(it.key()))
            {
                ol->mMapNextPhotoKeys.clear();
            }
        }

        if (!(ol->mMapNextLine.isEmpty()))
        {
            QMap<QString, int>::const_iterator it = ol->mMapNextLine.constBegin();
            if (!map.contains(it.key()))
            {
                ol->mMapNextLine.clear();
            }
        }
        ++itBegin;
    }
}

int OverlappingProcessing::getLineSize()
{
    QMap<QString, Ol*>::const_iterator itEnd = map.constEnd();
    int iEnd = (--itEnd).value()->mLineNumber;
    return iEnd;
}

void OverlappingProcessing::getLinePhoto(const int number, QStringList &list)
{
    list.clear();

    QMapIterator<QString, Ol*> it(map);
    while (it.hasNext())
    {
        it.next();
        if (it.value()->mLineNumber == number)
        {
            list << it.key();
        }
    }
}

QgsFeature *OverlappingProcessing::getFeature(const QString &photoNumber)
{
    if (map.contains(photoNumber))
    {
        return map.value(photoNumber)->mF;
    }
    return nullptr;
}

QList<QString> OverlappingProcessing::getAllPhotoNo()
{
    return map.keys();
}

int OverlappingProcessing::calculateOverlap(const QString &currentNo, const QString &nextNo)
{
    QgsFeature *currentQgsFeature = nullptr;
    QgsFeature *nextQgsFeature = nullptr;

    currentQgsFeature = getFeature(currentNo);
    if (!currentQgsFeature || !currentQgsFeature->isValid())
    {
        QgsMessageLog::logMessage(QString("检查重叠度 : \t%1 last图形检索失败.").arg(currentNo));
        return 0;
    }

    nextQgsFeature = getFeature(nextNo);
    if (!nextQgsFeature || !nextQgsFeature->isValid())
    {
        QgsMessageLog::logMessage(QString("检查重叠度 : \t%1 next图形检索失败.").arg(currentNo));
        return 0;
    }

    QgsGeometry *currentGeometry = currentQgsFeature->geometry();
    QgsGeometry *nextGeometry = nextQgsFeature->geometry();

    QgsGeometry *intersectGeometry = nullptr;
    intersectGeometry = currentGeometry->intersection(nextGeometry);
    if (intersectGeometry && !intersectGeometry->isEmpty())
    {
        double intersectArea = intersectGeometry->area();
        double currentArea = currentGeometry->area();
        double nextArea = nextGeometry->area();
        int percentage = (int)((intersectArea/currentArea + intersectArea/nextArea)/2*100);
        return percentage;
    }

    return 0;
}

int OverlappingProcessing::calculateOverlapSiftGPU(const QString &currentNo, const QString &nextNo)
{
    SiftGPU  *sift = new SiftGPU;
    SiftMatchGPU *matcher = new SiftMatchGPU(4096);
    vector<float> descriptors1(1), descriptors2(1);

    char * argv[] = {"-fo", "-1",  "-v", "1", "-tc", "2000"};
//    char * argv[] = {"-fo", "-1",  "-v", "1"};
    int argc = sizeof(argv)/sizeof(char*);
    sift->ParseParam(argc, argv);

    // 检查硬件是否支持SiftGPU
    int support = sift->CreateContextGL();
    if ( support != SiftGPU::SIFTGPU_FULL_SUPPORTED )
    {
        QgsMessageLog::logMessage("当前硬件不支持GPU计算功能!");
        return 0;
    }

    gdalTools.isCompression(nextNo);

    // 检查是否已匹配过
    if (mapKeyDescriptors.contains(currentNo))
    {
        descriptors1 = mapKeyDescriptors.value(currentNo);
    }
    else
    {
        bool isDone = false;
//        if ( !gdalTools.isCompression(currentNo) )
        if ( true )
        {
            QgsMessageLog::logMessage("影像特征匹配 : 非压缩格式，sitf直接读取影像:" + currentNo);
            if (sift->RunSIFT(currentNo.toStdString().c_str()))
                isDone = true;
        }
        else
        {
            // 使用GDAL读取影像
            float *image1;
            int xSize1 = 0;
            int ySize1 = 0;
            bool isReadImage1 = gdalTools.readRasterIO(currentNo, &image1, xSize1, ySize1);
            if (isReadImage1)
            {
                if(sift->RunSIFT(xSize1, ySize1, image1, GL_LUMINANCE, GL_FLOAT))
                {
                    isDone = true;
                    QgsMessageLog::logMessage("影像特征匹配 : 压缩格式，使用GDAL读取影像:" + currentNo);
                }
                delete[] image1;
            }
        }
        if (isDone)
        {
            int num1 = sift->GetFeatureNum();
            vector<SiftGPU::SiftKeypoint> keys1(num1);
            descriptors1.resize(128*num1);
            sift->GetFeatureVector(&keys1[0], &descriptors1[0]);
            mapKeyDescriptors[currentNo] = descriptors1;
        }
        else
        {
            QgsMessageLog::logMessage("影像特征匹配 : 读取匹配失败：" + currentNo);
        }
    }

    if (mapKeyDescriptors.contains(nextNo))
    {
        descriptors2 = mapKeyDescriptors.value(nextNo);
    }
    else
    {
        bool isDone = false;
//        if (!gdalTools.isCompression(nextNo))
        if (true)
        {
            QgsMessageLog::logMessage("影像特征匹配 : 非压缩格式，sitf直接读取影像:" + nextNo);
            if (sift->RunSIFT(nextNo.toStdString().c_str()))
                isDone = true;
        }
        else
        {
            // 使用GDAL读取影像
            float *image2;
            int xSize2 = 0;
            int ySize2 = 0;
            bool isReadImage2 = gdalTools.readRasterIO(nextNo, &image2, xSize2, ySize2);
            if (isReadImage2)
            {
                if(sift->RunSIFT(xSize2, ySize2, image2, GL_LUMINANCE, GL_FLOAT))
                {
                    isDone = true;
                    QgsMessageLog::logMessage("影像特征匹配 :压缩格式，使用GDAL读取影像：" + nextNo);
                }
                delete[] image2;
            }
        }
        if (isDone)
        {
            int num2 = sift->GetFeatureNum();
            vector<SiftGPU::SiftKeypoint> keys2(num2);
            descriptors2.resize(128*num2);
            sift->GetFeatureVector(&keys2[0], &descriptors2[0]);
            mapKeyDescriptors[nextNo] = descriptors2;
        }
        else
        {
            QgsMessageLog::logMessage("影像特征匹配 : 读取失败：" + nextNo);
        }
    }

    int num1 = descriptors1.size()/128;
    int num2 = descriptors2.size()/128;
    matcher->VerifyContextGL();
    matcher->SetDescriptors(0, num1, &descriptors1[0]);
    matcher->SetDescriptors(1, num2, &descriptors2[0]);
    int (*match_buf)[2] = new int[num1][2];
    int num_match = matcher->GetSiftMatch(num1, match_buf);

    delete[] match_buf;
    delete sift;
    delete matcher;
    return num_match;
}

QMap<QString, int> OverlappingProcessing::getNextPhotoOverlapping(const QString &number)
{
    return map.value(number)->mMapNextPhoto;
}

QMap<QString, int> OverlappingProcessing::getNextPhotoKeys(const QString &number)
{
    return map.value(number)->mMapNextPhotoKeys;
}

QMap<QString, int> OverlappingProcessing::getNextLineOverlapping(const QString &number)
{
    return map.value(number)->mMapNextLine;
}

QMap<QString, int> OverlappingProcessing::getNextLineKeys(const QString &number)
{
    return map.value(number)->mMapNextLineKeys;
}

void OverlappingProcessing::delItem(const QString &photoNumber)
{
    if (map.contains(photoNumber))
    {
        Ol* ol = map.value(photoNumber);
        delete ol;
        ol = nullptr;

        map.remove(photoNumber);
    }
}
