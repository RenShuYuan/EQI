#include "eqianalysisaerialphoto.h"
#include "eqi/pos/posdataprocessing.h"
#include "mainwindow.h"

#include "qgsmessagelog.h"
#include "qgsvectordataprovider.h"

#include <QStringList>

eqiAnalysisAerialphoto::eqiAnalysisAerialphoto(QObject *parent, QgsVectorLayer *layer, posDataProcessing *posdp)
    : QObject(parent)
    , mLayerMap(layer)
    , mPosdp(posdp)
    , mLayer_Omega(nullptr)
    , mLayer_Kappa(nullptr)
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
//    double heading_Bg = 60.0;
//    double heading_Ed = 80.0;
//    double heading_Min = 53.0;
//    double sideways_Bg = 15.0;
//    double sideways_Ed = 60.0;
//    double sideways_Min = 8.0;

    foreach (QStringList line, mAirLineGroup)
    {
        QgsMessageLog::logMessage("重叠度检查");

        //! 行带内重叠度检查
        QString lastPhoto;
        QString currentPhoto;
        QgsGeometry* lastGeometry = nullptr;
        QgsGeometry* currentGeometry = nullptr;

        QgsFeature f;
        QgsFeatureRequest request;

        QStringList::iterator it = line.begin();
        lastPhoto = *it;

        // 获得上个图形
        request.setFilterExpression(QString("相片编号='%1'").arg(lastPhoto));
        QgsFeatureIterator It_last = mLayerMap->getFeatures(request);
        while (It_last.nextFeature(f))
        {
            if (f.isValid())
                lastGeometry = f.geometry();
            break;
        }
        if (!lastGeometry || lastGeometry->isGeosEmpty())
        {
            QgsMessageLog::logMessage(QString("检查重叠度 : \t%1 last图形检索失败.").arg(lastPhoto));
            lastPhoto = currentPhoto;
            continue;
        }

//        while (++it != line.end())
//        {
//            // 获得当前图形
//            currentPhoto = *it;
//            request.setFilterExpression(QString("相片编号='%1'").arg(currentPhoto));
//            QgsFeatureIterator It_current = mLayerMap->getFeatures(request);
//            while (It_current.nextFeature(f))
//            {
//                if (f.isValid())
//                    currentGeometry = f.geometry();
//                break;
//            }
//            if (!currentGeometry || currentGeometry->isGeosEmpty())
//            {
//                QgsMessageLog::logMessage(QString("检查重叠度 : \t%1 current图形检索失败。").arg(currentPhoto));
//                break;
//            }
//            QgsMessageLog::logMessage(QString("检查重叠度 : \tlast图形面积：%1, current图形面积：%2").arg(lastGeometry->area()).arg(currentGeometry->area()));
//            QgsMessageLog::logMessage(QString("检查重叠度 : \tcurrent图形面积：%2").arg(currentGeometry->area()));

            // 判断两个图形是否有重叠
//            if (lastGeometry->intersects(currentGeometry))
//            {
////                shortestLine
//                QgsMessageLog::logMessage(QString("检查重叠度 : \t%1与%2图形之间有重叠度。")
//                                          .arg(lastPhoto)
//                                          .arg(currentPhoto));
//            }
//            else
//            {
//                QgsMessageLog::logMessage(QString("检查重叠度 : \t%1与%2图形之间没有重叠度。")
//                                          .arg(lastPhoto)
//                                          .arg(currentPhoto));      // err
//            }

//            lastGeometry = currentGeometry;
//        }

        // 根据航飞方向，提取相对的角点坐标

        // 利用垂点，计算最近距离，计算重叠度

        //! 行带间重叠度检查

        // 提取当前相片，与相邻行带所有相片进行重叠检查

        // 与上诉相同
    }
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
            QgsMessageLog::logMessage(QString("创建航摄略图 : \t创建略图失败, 程序已终止, 注意检查plugins文件夹。"));
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

    foreach (QStringList line, mAirLineGroup)
    {
        double averageAngle = 0;
        QStringList angle_Line_List_sub;

        // 航带平均角度值
        foreach (QString noName, line)
        {
            QString str_kappaField = mPosdp->getPosRecord(noName, "kappaField");
            averageAngle += str_kappaField.toDouble();
        }
        averageAngle = averageAngle / line.size();

        foreach (QString noName, line)
        {
            // 得到kappa的角度值
            QString str_kappaField = mPosdp->getPosRecord(noName, "kappaField");
            double angle = str_kappaField.toDouble();
            
            // 对比
            if ((angle - averageAngle) >= angle_Max) angle_Max_List.append(noName);
            else if ((angle - averageAngle) >= angle_Line) angle_Line_List_sub.append(noName);
            else if ((angle - averageAngle) >= angle_General) angle_General_List.append(noName);
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
            QgsMessageLog::logMessage(QString("创建航摄略图 : \t创建略图失败, 程序已终止, 注意检查plugins文件夹。"));
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
    const double LIMITVALUE = 45.0;	// 度

    QStringList childGroup;
    double prev_kappaValue = 0.0;

    const QStringList* noList = mPosdp->noList();
    if (!noList) return;

    // 设置初值
    prev_kappaValue = mPosdp->getPosRecord(noList->first(), "kappaField").toDouble();

    for (int i = 0; i < noList->size(); ++i)	// 遍历所有旋片角
    {
        double kappaValue = 0.0;
        QString noName = noList->at(i);
        kappaValue = mPosdp->getPosRecord(noName, "kappaField").toDouble();

        if ( abs(kappaValue - prev_kappaValue) < LIMITVALUE)
        {
            childGroup.append(noName);
        }
        else
        {
            mAirLineGroup.append(childGroup);
            childGroup.clear();
            childGroup.append(noName);
        }
        prev_kappaValue = kappaValue;
    }
    mAirLineGroup.append(childGroup);
    isGroup = true;

    // 调试
    foreach (QStringList list, mAirLineGroup)
    {
        QString outStr;
        foreach (QString str, list)
        {
            outStr += str + ", ";
        }
        QgsMessageLog::logMessage("航带: " + outStr);
    }
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

void OverlappingProcessing::setLineNumber(const QString &photoNumber, int lineNumber)
{
    if (!map.contains(photoNumber))
    {
        map[photoNumber] = new Ol(lineNumber);
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
