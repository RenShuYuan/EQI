﻿#include "eqianalysisaerialphoto.h"
#include "eqi/pos/posdataprocessing.h"
#include "mainwindow.h"

#include "qgsmessagelog.h"
#include "qgsvectordataprovider.h"

#include <QSet>
#include <QStringList>
#include <QDebug>

eqiAnalysisAerialphoto::eqiAnalysisAerialphoto(QObject *parent, QgsVectorLayer *layer, posDataProcessing *posdp)
    : QObject(parent)
    , mLayerMap(layer)
    , mPosdp(posdp)
    , mLayer_Omega(nullptr)
    , mLayer_Kappa(nullptr)
    , mLayer_Overlapping(nullptr)
    , isGroup(false)
{
    airLineGroup();
}

void eqiAnalysisAerialphoto::checkOverlapping()
{
    if (!isGroup)
    {
        QgsMessageLog::logMessage(QString("重叠度检查 : \t创建航带失败，无法进行旋片角检查。"));
    }

    // 重叠度限差
    double heading_Bg = 60.0;
    double heading_Ed = 80.0;
    double heading_Min = 53.0;
    double sideways_Bg = 15.0;
    double sideways_Ed = 60.0;
    double sideways_Min = 8.0;

    int lineSize = myOlp.getLineSize();
    if (!lineSize) return;
    int lineNumber = 0;

    while (++lineNumber <= lineSize)
    {
        QString currentPhoto;
        QString nextPhoto;
        QStringList lineList;
        QStringList nextlineList;

        myOlp.getLinePhoto(lineNumber, lineList);
        if (lineList.isEmpty()) return;

//        QgsMessageLog::logMessage(QString("第%1条航带共%2张相片：").arg(lineNumber).arg(lineList.size()));

        QStringList::const_iterator i = lineList.constBegin();
        if (i != lineList.constEnd())
        {
            currentPhoto = *i;
        }

        //! 行带内重叠度比较
        while (++i != lineList.constEnd())
        {
            nextPhoto = *i;
            myOlp.calculateOverlap(currentPhoto, nextPhoto, false);

            //! 与下一航带重叠度检查
            myOlp.getLinePhoto(lineNumber+1, nextlineList);

            QStringList::const_iterator i_next = nextlineList.constBegin();
            while (i_next != nextlineList.constEnd())
            {
                QString photoNo = *i_next++;
                int percentage = myOlp.calculateOverlap(currentPhoto, photoNo, true);
                if (percentage > sideways_Ed) break;
            }

            currentPhoto = nextPhoto;
        }
    }

    // 创建错误图层
    if ( !mLayer_Overlapping || !mLayer_Overlapping->isValid() )
    {
        mLayer_Overlapping = MainWindow::instance()->createrMemoryMap("重叠度检查",
                                                            "Polygon",
                                                            QStringList()
                                                            << "field=id:integer"
                                                            << "field=相片编号:string(50)"
                                                            << "field=与相邻相片重叠度:string(10)"
                                                            << "field=错误类型:string(100)");

        if (!mLayer_Overlapping && !mLayer_Overlapping->isValid())
        {
            MainWindow::instance()->messageBar()->pushMessage( "创建检查图层",
                "创建图层失败, 运行已终止, 注意检查plugins文件夹!",
                QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
            QgsMessageLog::logMessage(QString("创建检查图层 : \t创建图层失败, 程序已终止, 注意检查plugins文件夹。"));
            return;
        }
        mLayer_Overlapping->setCrs(mLayerMap->crs());
    }

    // 初始化渲染类
    eqiSymbol *mySymbol = new eqiSymbol(this, mLayer_Overlapping, "相片编号");

    // 对比超限值，输出错误矢量面
    int index = 0;
    QgsFeatureList featureList;
    QList<QString> list = myOlp.getAllPhotoNo();
    foreach (QString number, list)
    {
        //! 航带内
        QMap<QString, int> map = myOlp.getNextPhotoOverlapping(number);
        if (map.isEmpty())  // 没有重叠度
        {
            // 检查当前相片是否为该航线最后一张
            QStringList list;
            int lineNo = myOlp.getLineNumber(number);
            myOlp.getLinePhoto(lineNo, list);
            QString last = list.last();
            if (number != last)
            {
                QgsGeometry *errGeometry = myOlp.getFeature(number)->geometry();
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, "0%"
                                         , "与下张相片没有重叠度"
                                         , mySymbol, eqiSymbol::SeriousSrror);
            }
        }
        else
        {
            QMap<QString, int>::const_iterator it = map.constBegin();

            // 获得相交区域矢量面
            QgsGeometry *errGeometry = myOlp.getFeature(number)->geometry()
                    ->intersection(myOlp.getFeature(it.key())->geometry());

            if (it.value() < heading_Min)   // 与航带内下张相片重叠度低于最低标准
            {
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, QString("航带内%1%").arg(it.value())
                                         , QString("低于最低阈值%2%").arg(heading_Min)
                                         , mySymbol, eqiSymbol::SeriousSrror);
            }
            else if (it.value() < heading_Bg)   // 与航带内下张相片重叠度低于建议标准
            {
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, QString("航带内%1%").arg(it.value())
                                         , QString("低于建议阈值%2%").arg(heading_Bg)
                                         , mySymbol, eqiSymbol::warning);
            }
            else if (it.value() > heading_Ed)   // 与航带内下张相片重叠度大于最大建议值
            {
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, QString("航带内%1%").arg(it.value())
                                         , QString("超过建议阈值%2%").arg(heading_Ed)
                                         , mySymbol, eqiSymbol::warning);
            }
        }

        //! 航带间
        QMap<QString, int> mapNext = myOlp.getNextLineOverlapping(number);
        if (mapNext.isEmpty())  // 没有重叠度
        {
            QgsGeometry *errGeometry = myOlp.getFeature(number)->geometry();
            addOverLappingErrFeature(featureList, errGeometry, number
                                     , ++index, "航带间0%"
                                     , "与下航带相片没有重叠度"
                                     , mySymbol, eqiSymbol::SeriousSrror);
        }
        else
        {
            QMap<QString, int>::const_iterator itNext = mapNext.constBegin();

            // 利用获得最大重叠度进行比较
            QString maxNumber;
            int maxPercentage = 0;
            while (itNext != mapNext.constEnd())
            {
                if (itNext.value() > maxPercentage)
                {
                    maxPercentage = itNext.value();
                    maxNumber = itNext.key();
                }
                ++itNext;
            }

            // 获得相交区域矢量面
            QgsGeometry *errGeometry = myOlp.getFeature(number)->geometry()
                    ->intersection(myOlp.getFeature(maxNumber)->geometry());

            if (maxPercentage < sideways_Min)  // 低于最低标准
            {
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, QString("航带间%1%").arg(maxPercentage)
                                         , QString("低于最低阈值%2%").arg(sideways_Min)
                                         , mySymbol, eqiSymbol::SeriousSrror);
            }
            else if (maxPercentage < sideways_Bg) // 低于建议标准
            {
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, QString("航带间%1%").arg(maxPercentage)
                                         , QString("低于建议阈值%2%").arg(sideways_Bg)
                                         , mySymbol, eqiSymbol::warning);
            }
            else if (maxPercentage > sideways_Ed) // 大于最大建议
            {
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, QString("航带间%1%").arg(maxPercentage)
                                         , QString("超过建议阈值%2%").arg(sideways_Ed)
                                         , mySymbol, eqiSymbol::warning);
            }
        }
    }

    MainWindow::instance()->mapCanvas()->freeze();

    // 将要素添加到图层中
    mLayer_Overlapping->startEditing();
    mLayer_Overlapping->dataProvider()->addFeatures(featureList);
    mLayer_Overlapping->commitChanges();
    mLayer_Overlapping->updateExtents();

    MainWindow::instance()->mapCanvas()->freeze( false );
    MainWindow::instance()->refreshMapCanvas();
    mySymbol->updata();
}

void eqiAnalysisAerialphoto::checkOmega()
{
    // 角度限差
    double angle_General = 5.0;
    double angle_Max = 12.0;
    double angle_Average = 8.0;

    // 保存超限的相片名称
    QStringList angle_General_List;
    QStringList angle_Max_List;
    QStringList angle_Average_List;

    const QStringList* noList = mPosdp->noList();
    for (int i = 0; i < noList->size(); ++i)
    {
        QString noName = noList->at(i);

        // 得到omega的绝对值
        QString str_omegaField = mPosdp->getPosRecord(noName, "omegaField");
        double angle = fabs(str_omegaField.toDouble());

        // 对比
        if (angle >= angle_Max) angle_Max_List.append(noName);
        else if (angle >= angle_Average) angle_Average_List.append(noName);
        else if (angle >= angle_General) angle_General_List.append(noName);
    }

    if ( !mLayer_Omega || !mLayer_Omega->isValid() )
    {
        mLayer_Omega = MainWindow::instance()->createrMemoryMap("倾角检查",
                                                            "Polygon",
                                                            QStringList()
                                                            << "field=id:integer"
                                                            << "field=相片编号:string(50)"
                                                            << "field=Omega:string(10)"
                                                            << "field=错误类型:string(100)");

        if (!mLayer_Omega && !mLayer_Omega->isValid())
        {
            MainWindow::instance()->messageBar()->pushMessage( "创建检查图层",
                "创建图层失败, 运行已终止, 注意检查plugins文件夹!",
                QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
            QgsMessageLog::logMessage(QString("创建检查图层 : \t创建图层失败, 程序已终止, 注意检查plugins文件夹。"));
            return;
        }
        mLayer_Omega->setCrs(mLayerMap->crs());
    }

    // 初始化渲染类
    eqiSymbol *mySymbol = new eqiSymbol(this, mLayer_Omega, "相片编号");

    int icount = 0;
    QString strExpression;
    QgsFeatureList featureList;

    // 构建条件语句 angle_General_List
    foreach (QString no, angle_General_List)
    {
        strExpression += QString("相片编号='%1' OR ").arg(no);
    }
    strExpression = strExpression.left(strExpression.size() - 3);
    extractFeature(featureList, strExpression, ++icount, "Omega"
                   , QString("超过一般限差%1%").arg(angle_General)
                   , mySymbol, eqiSymbol::warning);

    // 构建条件语句 angle_Max_List
    foreach (QString no, angle_Max_List)
    {
        strExpression += QString("相片编号='%1' OR ").arg(no);
    }
    strExpression = strExpression.left(strExpression.size() - 3);
    extractFeature(featureList, strExpression, ++icount, "Omega"
                   , QString("超过最大限差%1%").arg(angle_Max)
                   , mySymbol, eqiSymbol::error);

    if (((angle_Average_List.size()+angle_Max_List.size()) / noList->size()) > 0.1)
    {
        // 构建条件语句 angle_Average_List
        strExpression.clear();
        foreach (QString no, angle_Average_List)
        {
            strExpression += QString("相片编号='%1' OR ").arg(no);
        }
        strExpression = strExpression.left(strExpression.size() - 3);
        extractFeature(featureList, strExpression, ++icount, "Omega"
                       , QString("%1的片数多于总数10%").arg(angle_Max)
                       , mySymbol, eqiSymbol::SeriousSrror);
    }

    MainWindow::instance()->mapCanvas()->freeze();

    // 将要素添加到图层中
    mLayer_Omega->startEditing();
    mLayer_Omega->dataProvider()->addFeatures(featureList);
    mLayer_Omega->commitChanges();
    mLayer_Omega->updateExtents();

    MainWindow::instance()->mapCanvas()->freeze( false );
    MainWindow::instance()->refreshMapCanvas();
    mySymbol->updata();
}

void eqiAnalysisAerialphoto::checkKappa()
{
    if (!isGroup)
    {
        QgsMessageLog::logMessage(QString("旋片角检查 : \t创建航带失败，无法进行旋片角检查。"));
    }

    // 角度限差
    double angle_General = 15.0;
    double angle_Max = 30.0;
    double angle_Line = 20.0;

    // 保存超限的相片名称
    QStringList angle_General_List;
    QStringList angle_Max_List;
    QStringList angle_Line_List;
    QStringList angle_Average_List;

    QList< double > averageAngleList;

    int lineSize = myOlp.getLineSize();
    if (!lineSize) return;
    int lineNumber = 0;

    // 计算出每条行带的平均旋角
    while (++lineNumber <= lineSize)
    {
        double averageCount = 0;

        QStringList lineList;
        myOlp.getLinePhoto(lineNumber, lineList);
        if (lineList.isEmpty()) return;

        QStringList::const_iterator i = lineList.constBegin();
        while (i != lineList.constEnd())
        {
            QString str_kappaField = mPosdp->getPosRecord(*i++, "kappaField");
            averageCount += str_kappaField.toDouble();
        }

        averageAngleList << averageCount / lineList.size();
    }

    lineNumber = 0;
    QStringList angle_Line_List_sub;
    while (++lineNumber <= myOlp.getLineSize())
    {
        QStringList lineList;
        myOlp.getLinePhoto(lineNumber, lineList);
        if (lineList.isEmpty()) return;

        QStringList::const_iterator i = lineList.constBegin();
        while (i != lineList.constEnd())
        {
            QString noName = *i++;
            QString str_kappaField = mPosdp->getPosRecord(noName, "kappaField");
            double angle = str_kappaField.toDouble();

            // 对比
            if ((angle - averageAngleList.at(lineNumber-1)) >= angle_Max) angle_Max_List.append(noName);
            else if ((angle - averageAngleList.at(lineNumber-1)) >= angle_Line) angle_Line_List_sub.append(noName);
            else if ((angle - averageAngleList.at(lineNumber-1)) >= angle_General) angle_General_List.append(noName);
        }

        // 同一行带间超过20°的片子不应超过3片
        if ((angle_Line_List_sub.size() + angle_Max_List.size()) > 3)
            angle_Line_List += angle_Line_List_sub;
        else
            angle_Average_List += angle_Line_List_sub;
    }

    if ( !mLayer_Kappa || !mLayer_Kappa->isValid() )
    {
        mLayer_Kappa = MainWindow::instance()->createrMemoryMap("旋片角检查",
                                                            "Polygon",
                                                            QStringList()
                                                            << "field=id:integer"
                                                            << "field=相片编号:string(50)"
                                                            << "field=Kappa:string(10)"
                                                            << "field=错误类型:string(100)");

        if (!mLayer_Kappa && !mLayer_Kappa->isValid())
        {
            MainWindow::instance()->messageBar()->pushMessage( "创建检查图层",
                "创建图层失败, 运行已终止, 注意检查plugins文件夹!",
                QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
            QgsMessageLog::logMessage(QString("创建检查图层 : \t创建图层失败, 程序已终止, 注意检查plugins文件夹。"));
            return;
        }
        mLayer_Kappa->setCrs(mLayerMap->crs());
    }

    // 初始化渲染类
    eqiSymbol *mySymbol = new eqiSymbol(this, mLayer_Kappa, "相片编号");

    int icount = 0;
    QString strExpression;
    QgsFeatureList featureList;

    // 构建条件语句 angle_General_List
    foreach (QString no, angle_General_List)
    {
        strExpression += QString("相片编号='%1' OR ").arg(no);
    }
    strExpression = strExpression.left(strExpression.size() - 3);
    extractFeature(featureList, strExpression, ++icount, "Kappa"
                   , QString("超过一般限差%1%").arg(angle_General)
                   , mySymbol, eqiSymbol::warning);

    // 构建条件语句 angle_Max_List
    strExpression.clear();
    foreach (QString no, angle_Max_List)
    {
        strExpression += QString("相片编号='%1' OR ").arg(no);
    }
    strExpression = strExpression.left(strExpression.size() - 3);
    extractFeature(featureList, strExpression, ++icount, "Kappa"
                   , QString("超过最大限差%1%").arg(angle_Max)
                   , mySymbol, eqiSymbol::error);

    // 构建条件语句 angle_Line_List
    strExpression.clear();
    foreach (QString no, angle_Line_List)
    {
        strExpression += QString("相片编号='%1' OR ").arg(no);
    }
    strExpression = strExpression.left(strExpression.size() - 3);
    extractFeature(featureList, strExpression, ++icount, "Kappa"
                   , QString("同一航带中%1的片数多于3片").arg(angle_Line)
                   , mySymbol, eqiSymbol::SeriousSrror);

    // 构建条件语句
    if (((angle_Average_List.size()+angle_Max_List.size()+angle_General_List.size())
         / mPosdp->noList()->size()) > 0.1)
    {
        strExpression.clear();
        foreach (QString no, angle_Average_List)
        {
            strExpression += QString("相片编号='%1' OR ").arg(no);
        }
        strExpression = strExpression.left(strExpression.size() - 3);
        extractFeature(featureList, strExpression, ++icount, "Kappa"
                       , QString("%1的片数多余总数10%").arg(angle_General)
                       , mySymbol, eqiSymbol::SeriousSrror);
    }

    MainWindow::instance()->mapCanvas()->freeze();

    // 将要素添加到图层中
    mLayer_Kappa->startEditing();
    mLayer_Kappa->dataProvider()->addFeatures(featureList);
    mLayer_Kappa->commitChanges();
    mLayer_Kappa->updateExtents();

    MainWindow::instance()->mapCanvas()->freeze( false );
    MainWindow::instance()->refreshMapCanvas();
    mySymbol->updata();
}

void eqiAnalysisAerialphoto::airLineGroup()
{
    if (!mLayerMap) return;

    int lineCount = 0;
    const double LIMITVALUE = 45.0;	// 度
    double prev_kappaValue = 1000.0;

    QgsFeature f;
    QgsFeatureIterator it = mLayerMap->getFeatures();
    while (it.nextFeature(f))
    {
        if (f.isValid())
        {
            QString noName = f.attribute("相片编号").toString();
            double kappaValue = f.attribute("Kappa").toString().toDouble();
            QgsFeature *pf =new QgsFeature(f);
            myOlp.setQgsFeature(noName, pf);

            if ( abs(kappaValue - prev_kappaValue) < LIMITVALUE)
            {
                myOlp.addLineNumber(noName, lineCount);
            }
            else
            {
                myOlp.addLineNumber(noName, ++lineCount);
            }
            prev_kappaValue = kappaValue;
        }
    }

    isGroup = true;

    // 调试
//    const QStringList* noList = mPosdp->noList();
//    foreach (QString noName, *noList)
//    {
//        QgsMessageLog::logMessage(QString("%1=%2").arg(noName).arg(myOlp.getLineNumber(noName)));
//    }
}

void eqiAnalysisAerialphoto::extractFeature(QgsFeatureList &featureList
                                            , QString &strExpression
                                            , const int index
                                            , const QString &field
                                            , const QString &errType
                                            , eqiSymbol *mySymbol
                                            , eqiSymbol::linkedType type)
{
    QgsFeature f;
    QgsFeatureRequest reQuest;

    reQuest.setFilterExpression(strExpression);
    QgsFeatureIterator it = mLayerMap->getFeatures(reQuest);

    while (it.nextFeature(f))
    {
        QgsFeature MyFeature(f);
        MyFeature.setAttributes(QgsAttributes() << QVariant(index)
                                                << f.attribute("相片编号")
                                                << f.attribute(field)
                                                << QVariant(errType));
        featureList.append(MyFeature);

        mySymbol->addChangedItem(f.attribute("相片编号").toString(), type);
    }
    strExpression.clear();
}

void eqiAnalysisAerialphoto::addOverLappingErrFeature(QgsFeatureList &featureList
                                                      , QgsGeometry *errGeometry
                                                      , const QString &number
                                                      , const int index
                                                      , const QString &overlappingVelue
                                                      , const QString &errType
                                                      , eqiSymbol *mySymbol
                                                      , eqiSymbol::linkedType type)
{
    QgsFeature MyFeature;
    MyFeature.setGeometry(errGeometry);
    MyFeature.setAttributes(QgsAttributes() << QVariant(index)
                            << QVariant(number)
                            << QVariant(overlappingVelue)
                            << QVariant(errType));
    featureList.append(MyFeature);
    mySymbol->addChangedItem(number, type);
}


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
    if (map.contains(currentNumber))
    {
        Ol* ol = map.value(currentNumber);
        ol->mMapNextPhoto.clear();
        ol->mMapNextPhoto.insert(nextNumber, n);
    }
}

void OverlappingProcessing::setNextLineOl(const QString &currentNumber
                                          , QString photoNumber, const int n)
{
    if (map.contains(currentNumber))
    {
        Ol* ol = map.value(currentNumber);
        ol->mMapNextLine.insert(photoNumber, n);
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

int OverlappingProcessing::getLineSize()
{
    QMap<QString, Ol*>::const_iterator itBegin = map.constBegin();
    QMap<QString, Ol*>::const_iterator itEnd = map.constEnd();
    int iBengin = itBegin.value()->mLineNumber;
    int iEnd = (--itEnd).value()->mLineNumber;
    return iBengin > iEnd ? iBengin : iEnd;
}

void OverlappingProcessing::getLinePhoto(const int number, QStringList &list)
{
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

int OverlappingProcessing::calculateOverlap(const QString &currentNo, const QString &nextNo, const bool isSideways)
{
    QgsFeature *currentQgsFeature = nullptr;
    QgsFeature *nextQgsFeature = nullptr;

    currentQgsFeature = getFeature(currentNo);
    if (!currentQgsFeature)
    {
        QgsMessageLog::logMessage(QString("检查重叠度 : \t%1 last图形检索失败.").arg(currentNo));
        return -1;
    }

    nextQgsFeature = getFeature(nextNo);
    if (!nextQgsFeature)
    {
        QgsMessageLog::logMessage(QString("检查重叠度 : \t%1 current图形检索失败.").arg(nextNo));
        return -1;
    }

    QgsGeometry *currentGeometry = currentQgsFeature->geometry();
    QgsGeometry *nextGeometry = nextQgsFeature->geometry();

    if (currentGeometry->intersects(nextGeometry))
    {
        // 计算重叠百分比
        QgsGeometry *intersectGeometry = currentGeometry->intersection(nextGeometry);
        double intersectArea = intersectGeometry->area();
        double currentArea = currentGeometry->area();
        int percentage = (int)(intersectArea / currentArea * 100);
        if (isSideways)
        {
            setNextLineOl(currentNo, nextNo, percentage);
        }
        else
        {
            setNextPhotoOl(currentNo, nextNo, percentage);
        }
        return percentage;
    }
    return -1;
}

QMap<QString, int> OverlappingProcessing::getNextPhotoOverlapping(const QString &number)
{
    return map.value(number)->mMapNextPhoto;
}

QMap<QString, int> OverlappingProcessing::getNextLineOverlapping(const QString &number)
{
    return map.value(number)->mMapNextLine;
}
