#include "eqianalysisaerialphoto.h"
#include "eqi/pos/posdataprocessing.h"
#include "mainwindow.h"

#include "qgsmessagelog.h"
#include "qgsvectordataprovider.h"

#include <QSet>
#include <QStringList>
#include <QDebug>
#include <QProgressDialog>

eqiAnalysisAerialphoto::eqiAnalysisAerialphoto(QObject *parent)
    : QObject(parent)
    , mLayerMap(nullptr)
    , mPosdp(nullptr)
    , mLayer_Omega(nullptr)
    , mLayer_Kappa(nullptr)
    , mLayer_Overlapping(nullptr)
    , isGroup(false)
{
    mField = "错误类型";
    updataChackValue();
}

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
    mField = "错误类型";

    updataChackValue();
}

void eqiAnalysisAerialphoto::checkOverlapping()
{
    if (!isGroup)
    {
        QgsMessageLog::logMessage(QString("重叠度检查 : \t创建航带失败，无法进行旋片角检查。"));
    }

    int lineSize = myOlp.getLineSize();
    if (!lineSize) return;
    int lineNumber = 0;

    //进度条
    QProgressDialog prDialog("检查重叠度...", "取消", 0, myOlp.getAllPhotoNo().size(), nullptr);
    prDialog.setWindowTitle("处理进度");              //设置窗口标题
    prDialog.setWindowModality(Qt::WindowModal);      //将对话框设置为模态
    prDialog.show();
    int prCount = 0;

    while (++lineNumber <= lineSize)
    {
        QString currentPhoto;
        QString nextPhoto;
        QStringList lineList;
        QStringList nextlineList;

        myOlp.getLinePhoto(lineNumber, lineList);
        if (lineList.isEmpty()) return;

        QStringList::const_iterator i = lineList.constBegin();
        if (i != lineList.constEnd())
        {
            currentPhoto = *i;

            //! 与下一航带重叠度检查
            myOlp.getLinePhoto(lineNumber+1, nextlineList);
            QStringList::const_iterator i_next = nextlineList.constBegin();
            while (i_next != nextlineList.constEnd())
            {
                QString photoNo = *i_next++;
                int percentage = myOlp.calculateOverlap(currentPhoto, photoNo);
                myOlp.setNextLineOl(currentPhoto, photoNo, percentage);
                if (percentage > sideways_Max) break;
            }

            prDialog.setValue(++prCount);
            QApplication::processEvents();
        }

        //! 行带内重叠度比较
        while (++i != lineList.constEnd())
        {
            nextPhoto = *i;
            int percentage = myOlp.calculateOverlap(currentPhoto, nextPhoto);
            myOlp.setNextPhotoOl(currentPhoto, nextPhoto, percentage);

            //! 与下一航带重叠度检查
            myOlp.getLinePhoto(lineNumber+1, nextlineList);

            QStringList::const_iterator i_next = nextlineList.constBegin();
            while (i_next != nextlineList.constEnd())
            {
                QString photoNo = *i_next++;
                int percentage = myOlp.calculateOverlap(nextPhoto, photoNo);
                myOlp.setNextLineOl(nextPhoto, photoNo, percentage);
                if (percentage > sideways_Max) break;
            }

            currentPhoto = nextPhoto;

            prDialog.setValue(++prCount);
            QApplication::processEvents();
        }

        //当按下取消按钮则中断
        if (prDialog.wasCanceled()) return;
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
    else
    {
        MainWindow::instance()->mapCanvas()->freeze();

        mLayer_Overlapping->startEditing();
        mLayer_Overlapping->selectAll();
        mLayer_Overlapping->deleteSelectedFeatures();
        mLayer_Overlapping->commitChanges();
        mLayer_Overlapping->updateExtents();

        MainWindow::instance()->mapCanvas()->freeze( false );
        MainWindow::instance()->refreshMapCanvas();
        eqiSymbol *mySymbol = new eqiSymbol(this, mLayer_Overlapping, mField);
        mySymbol->delAllSymbolItem();
    }

    // 初始化渲染类
    eqiSymbol *mySymbol = new eqiSymbol(this, mLayer_Overlapping, mField);

    // 对比超限值，输出错误矢量面
    int index = 0;
    QgsFeatureList featureList;
    QList<QString> list = myOlp.getAllPhotoNo();

    //进度条
    QProgressDialog prSymDialog("生成错误图层...", "取消", 0, list.size(), nullptr);
    prSymDialog.setWindowTitle("处理进度");              //设置窗口标题
    prSymDialog.setWindowModality(Qt::WindowModal);      //将对话框设置为模态
    prSymDialog.show();
    prCount = 0;

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
            if (number != list.last())
            {
                QgsGeometry *errGeometry = myOlp.getFeature(number)->geometry();
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, "0%"
                                         , errTpye_nextPhotoNoOverlap
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
                                         , ++index, QString("%1%").arg(it.value())
                                         , errTpye_nextPhotoMinOverlap
                                         , mySymbol, eqiSymbol::SeriousSrror);
            }
            else if (it.value() < heading_Gen)   // 与航带内下张相片重叠度低于建议标准
            {
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, QString("%1%").arg(it.value())
                                         , errTpye_nextPhotoGeneralOverlap
                                         , mySymbol, eqiSymbol::warning);
            }
            else if (it.value() > heading_Max)   // 与航带内下张相片重叠度大于最大建议值
            {
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, QString("%1%").arg(it.value())
                                         , errTpye_nextPhotoMaxOverlap
                                         , mySymbol, eqiSymbol::warning);
            }
        }

        //! 航带间
        QMap<QString, int> mapNext = myOlp.getNextLineOverlapping(number);
        if (mapNext.isEmpty())  // 没有重叠度
        {
            // 检查当前相片是否为最后一条航线
            int lineNo = myOlp.getLineNumber(number);
            int lineSize = myOlp.getLineSize();

            if (lineNo != lineSize)
            {
                QgsGeometry *errGeometry = myOlp.getFeature(number)->geometry();
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, "0%"
                                         , errTpye_nextLineNoOverlap
                                         , mySymbol, eqiSymbol::SeriousSrror);
            }
        }
        else
        {
            QMap<QString, int>::const_iterator itNext = mapNext.constBegin();
            QString maxNumber = itNext.key();
            int maxPercentage = itNext.value();

            // 获得相交区域矢量面
            QgsGeometry *currentGeometry = myOlp.getFeature(number)->geometry();
            QgsGeometry *maxGeometry = myOlp.getFeature(maxNumber)->geometry();
            QgsGeometry *errGeometry = nullptr;

            if (currentGeometry && maxGeometry)
            {
                errGeometry = currentGeometry->intersection(maxGeometry);
            }
            else
            {
                prSymDialog.setValue(++prCount);
                QApplication::processEvents();
                continue;
            }

            if (maxPercentage < sideways_Min)  // 低于最低标准
            {
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, QString("%1%").arg(maxPercentage)
                                         , errTpye_nextLineMinOverlap
                                         , mySymbol, eqiSymbol::SeriousSrror);
            }
            else if (maxPercentage < sideways_Min) // 低于建议标准
            {
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, QString("%1%").arg(maxPercentage)
                                         , errTpye_nextLineGeneralOverlap
                                         , mySymbol, eqiSymbol::warning);
            }
            else if (maxPercentage > sideways_Max) // 大于最大建议
            {
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, QString("%1%").arg(maxPercentage)
                                         , errTpye_nextLineMaxOverlap
                                         , mySymbol, eqiSymbol::warning);
            }
        }

        prSymDialog.setValue(++prCount);
        QApplication::processEvents();
        //当按下取消按钮则中断
        if (prDialog.wasCanceled()) return;
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

QStringList eqiAnalysisAerialphoto::delOverlapping()
{
    if ( !mLayer_Overlapping || !mLayer_Overlapping->isValid() )
    {
        MainWindow::instance()->messageBar()->pushMessage( "删除重叠度超限相片"
            ,"请先检查后再进行自动删除。", QgsMessageBar::INFO
            , MainWindow::instance()->messageTimeout() );
        return QStringList();
    }

    QStringList overrunHead;
    QStringList overrunSide;
    QStringList willDel;

    //进度条
    QProgressDialog prDialog("统计错误相片...", "取消", 0, mLayer_Overlapping->featureCount(), nullptr);
    prDialog.setWindowTitle("处理进度");              //设置窗口标题
    prDialog.setWindowModality(Qt::WindowModal);      //将对话框设置为模态
    prDialog.show();
    int prCount = 0;

    // 从错误图层中提取所有超过最大限差的相片集合
    QgsFeature f;
    QgsFeatureIterator it = mLayer_Overlapping->getFeatures();
    while (it.nextFeature(f))
    {
        if (f.isValid())
        {
            QString currentNo = f.attribute("相片编号").toString();
            QString strType = f.attribute("错误类型").toString();

            if (strType == errTpye_nextPhotoMaxOverlap)
                overrunHead << currentNo;
            else if (strType == errTpye_nextLineMaxOverlap)  // 航带间
                overrunSide << currentNo;
        }
        prDialog.setValue(++prCount);
        QApplication::processEvents();
    }

    prCount = 0;
    prDialog.setMaximum(overrunHead.size() + overrunSide.size());
    prDialog.setLabelText("判断航带内相片是否可删除...");

    // 遍历每张相片  航带内
    for (int i=0; i!=overrunHead.size(); ++i )
    {
        QString currentNo = overrunHead.at(i);
        QStringList list;
        int lineNo = myOlp.getLineNumber(currentNo);
        myOlp.getLinePhoto(lineNo, list);
        if (currentNo!=list.first() && currentNo!=list.last())
        {
            int index = list.indexOf(currentNo);
            QString lastNo = list.at(index-1);
            QString nextNo = list.at(index+1);

            // debug
            qDebug() << "==========》航带内检查：" << "lastNo： " << lastNo << "=currentNo：" << currentNo << "=nextNo：" << nextNo;

            int percentage = myOlp.calculateOverlap(lastNo, nextNo);
            if (percentage > heading_Gen)
            {
                // qdebug
                qDebug() << (QString("%1相片航带内重叠度%2 删除").arg(currentNo).arg(percentage));
                myOlp.delItem(currentNo);
                willDel << currentNo;
                overrunHead.removeOne(currentNo);
                ++i;
            }
        }

        prDialog.setValue(++prCount);
        QApplication::processEvents();
        //当按下取消按钮则中断
        if (prDialog.wasCanceled())
            return QStringList();
    }

    prDialog.setLabelText("判断航带间相片是否可删除...");

    // 遍历每张相片  航带间
    foreach (QString currentNo, overrunSide)
    {
        QStringList list;
        int lineNo = myOlp.getLineNumber(currentNo);
        myOlp.getLinePhoto(lineNo, list);
        int lineSize = myOlp.getLineSize();

        if ( (lineNo-1)>0 && (lineNo+1)<=lineSize ) // 保证上下还有航线
        {
            // 获得下条航带中的相片号
            QMap<QString, int> mapNext = myOlp.getNextLineOverlapping(currentNo);
            QMap<QString, int>::const_iterator itNext = mapNext.constBegin();
            QString maxNumber = itNext.key();

            // 遍历上一条航带中的每张相片，比较重叠度
            QStringList lastList;
            myOlp.getLinePhoto(lineNo-1, lastList);

            QStringList::const_iterator itLast = lastList.constBegin();
            while (itLast != lastList.constEnd())
            {
                QString lastPhoto = *itLast++;
                int percentage = myOlp.calculateOverlap(lastPhoto, maxNumber);

                if (percentage > sideways_Gen)
                {
                    // 删除本张相片
                    myOlp.delItem(currentNo);
                    willDel << currentNo;
                    // qdebug
                    qDebug() << (QString("%1相片航带间重叠度%2 删除").arg(currentNo).arg(percentage));
                    break;
                }
            }
        }

        prDialog.setValue(++prCount);
        QApplication::processEvents();
        //当按下取消按钮则中断
        if (prDialog.wasCanceled())
            return QStringList();
    }

    myOlp.updataLineNumber();
    myOlp.updataPhotoNumber();
    return willDel;
}

void eqiAnalysisAerialphoto::checkOmega()
{
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
        if (angle >= omega_angle_Max) angle_Max_List.append(noName);
        else if (angle >= omega_angle_Average) angle_Average_List.append(noName);
        else if (angle >= omega_angle_General) angle_General_List.append(noName);
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
    else
    {
        MainWindow::instance()->mapCanvas()->freeze();

        mLayer_Omega->startEditing();
        mLayer_Omega->selectAll();
        mLayer_Omega->deleteSelectedFeatures();
        mLayer_Omega->commitChanges();
        mLayer_Omega->updateExtents();

        MainWindow::instance()->mapCanvas()->freeze( false );
        MainWindow::instance()->refreshMapCanvas();
        eqiSymbol *mySymbol = new eqiSymbol(this, mLayer_Omega, mField);
        mySymbol->delAllSymbolItem();
    }

    // 初始化渲染类
    eqiSymbol *mySymbol = new eqiSymbol(this, mLayer_Omega, mField);

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
                   , QString("超过一般限差%1%").arg(omega_angle_General)
                   , mySymbol, eqiSymbol::warning);

    // 构建条件语句 angle_Max_List
    foreach (QString no, angle_Max_List)
    {
        strExpression += QString("相片编号='%1' OR ").arg(no);
    }
    strExpression = strExpression.left(strExpression.size() - 3);
    extractFeature(featureList, strExpression, ++icount, "Omega"
                   , QString("超过最大限差%1%").arg(omega_angle_Max)
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
                       , QString("%1的片数多于总数10%").arg(omega_angle_Max)
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

    QgsMessageLog::logMessage("倾斜角检查完成。");
}

QStringList eqiAnalysisAerialphoto::delOmega(const bool isEdge)
{
    if ( !mLayer_Omega || !mLayer_Omega->isValid() )
    {
        MainWindow::instance()->messageBar()->pushMessage( "删除错误相片"
            ,"请先检查后再进行自动删除。", QgsMessageBar::INFO
            , MainWindow::instance()->messageTimeout() );
        return QStringList();
    }

    QStringList overrun;
    QStringList willDel;
    QString lastNo;
    QString nextNo;

    //进度条
    QProgressDialog prDialog("统计错误相片...", "取消", 0, mLayer_Omega->featureCount(), nullptr);
    prDialog.setWindowTitle("处理进度");              //设置窗口标题
    prDialog.setWindowModality(Qt::WindowModal);      //将对话框设置为模态
    prDialog.show();
    int prCount = 0;

    // 从错误图层中提取所有超过最大限差的相片集合
    QgsFeature f;
    QgsFeatureIterator it = mLayer_Omega->getFeatures();
    while (it.nextFeature(f))
    {
        if (f.isValid())
        {
            QString currentNo = f.attribute("相片编号").toString();
            double omegaValue = f.attribute("Omega").toString().toDouble();
            if (abs(omegaValue) > omega_angle_Max)
            {
                overrun << currentNo;

                // qdebug
                QgsMessageLog::logMessage(QString("%1相片超限 ").arg(currentNo));
            }
        }
        prDialog.setValue(++prCount);
        QApplication::processEvents();
    }

    prCount = 0;
    prDialog.setMaximum(overrun.size());
    prDialog.setLabelText("判断相片是否可删除...");

    // 遍历每张相片
    foreach (QString currentNo, overrun)
    {
        QStringList list;
        int lineNo = myOlp.getLineNumber(currentNo);
        myOlp.getLinePhoto(lineNo, list);
        if (currentNo==list.first() || currentNo==list.last())
        {
            // qdebug
            QgsMessageLog::logMessage(QString("相片位于航线两端 %1").arg(currentNo));

            // 相片位于航线两端,根据isEdge是否删除
            if (isEdge)
            {
                // 删除本张相片
                myOlp.delItem(currentNo);
                willDel << currentNo;
                overrun.removeOne(currentNo);

                // qdebug
                QgsMessageLog::logMessage(QString("相片位于航线两端 删除 %1").arg(currentNo));
            }
        }
        else
        {
            QStringList::const_iterator qit = list.constBegin();
            while (++qit != list.constEnd())
            {
                if (*qit == currentNo)
                {
                    lastNo = *(qit-1);
                    nextNo = *(qit+1);
                    break;
                }
            }
            int percentage = myOlp.calculateOverlap(lastNo, nextNo);
            // qdebug
            QgsMessageLog::logMessage(QString("%1相片相邻重叠度%2 ").arg(currentNo).arg(percentage));
            if (percentage > heading_Min)
            {
                // 删除本张相片
                myOlp.delItem(currentNo);
                willDel << currentNo;
                overrun.removeOne(currentNo);

                // qdebug
                QgsMessageLog::logMessage(QString("%1相片相邻重叠度%2 删除").arg(currentNo).arg(percentage));
            }
        }

        prDialog.setValue(++prCount);
        QApplication::processEvents();
        //当按下取消按钮则中断
        if (prDialog.wasCanceled())
            return QStringList();
    }

    myOlp.updataLineNumber();
    myOlp.updataPhotoNumber();
    return willDel;
}

void eqiAnalysisAerialphoto::checkKappa()
{
    if (!isGroup)
    {
        QgsMessageLog::logMessage(QString("旋片角检查 : \t创建航带失败，无法进行旋片角检查。"));
    }

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
        double averageCount = 0.0;

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
            if ((angle - averageAngleList.at(lineNumber-1)) >= kappa_angle_Max) angle_Max_List.append(noName);
            else if ((angle - averageAngleList.at(lineNumber-1)) >= kappa_angle_Line) angle_Line_List_sub.append(noName);
            else if ((angle - averageAngleList.at(lineNumber-1)) >= kappa_angle_General) angle_General_List.append(noName);
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
    else
    {
        MainWindow::instance()->mapCanvas()->freeze();

        mLayer_Kappa->startEditing();
        mLayer_Kappa->selectAll();
        mLayer_Kappa->deleteSelectedFeatures();
        mLayer_Kappa->commitChanges();
        mLayer_Kappa->updateExtents();

        MainWindow::instance()->mapCanvas()->freeze( false );
        MainWindow::instance()->refreshMapCanvas();
        eqiSymbol *mySymbol = new eqiSymbol(this, mLayer_Kappa, mField);
        mySymbol->delAllSymbolItem();
    }

    // 初始化渲染类
    eqiSymbol *mySymbol = new eqiSymbol(this, mLayer_Kappa, mField);

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
                   , QString("超过一般限差%1%").arg(kappa_angle_General)
                   , mySymbol, eqiSymbol::warning);

    // 构建条件语句 angle_Max_List
    strExpression.clear();
    foreach (QString no, angle_Max_List)
    {
        strExpression += QString("相片编号='%1' OR ").arg(no);
    }
    strExpression = strExpression.left(strExpression.size() - 3);
    extractFeature(featureList, strExpression, ++icount, "Kappa"
                   , QString("超过最大限差%1%").arg(kappa_angle_Max)
                   , mySymbol, eqiSymbol::error);

    // 构建条件语句 angle_Line_List
    strExpression.clear();
    foreach (QString no, angle_Line_List)
    {
        strExpression += QString("相片编号='%1' OR ").arg(no);
    }
    strExpression = strExpression.left(strExpression.size() - 3);
    extractFeature(featureList, strExpression, ++icount, "Kappa"
                   , QString("同一航带中%1的片数多于3片").arg(kappa_angle_Line)
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
                       , QString("%1的片数多余总数10%").arg(kappa_angle_General)
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

    QgsMessageLog::logMessage("旋偏角检查完成。");
}

QStringList eqiAnalysisAerialphoto::delKappa(const bool isEdge)
{
    if ( !mLayer_Kappa || !mLayer_Kappa->isValid() )
    {
        MainWindow::instance()->messageBar()->pushMessage( "删除错误相片"
            ,"请先检查后再进行自动删除。", QgsMessageBar::INFO
            , MainWindow::instance()->messageTimeout() );
        return QStringList();
    }

    QStringList overrun;
    QStringList willDel;
    QString lastNo;
    QString nextNo;

    //进度条
    QProgressDialog prDialog("统计错误相片...", "取消", 0, mLayer_Kappa->featureCount(), nullptr);
    prDialog.setWindowTitle("处理进度");              //设置窗口标题
    prDialog.setWindowModality(Qt::WindowModal);      //将对话框设置为模态
    prDialog.show();
    int prCount = 0;

    // 从错误图层中提取所有超过最大限差的相片集合
    QgsFeature f;
    QgsFeatureIterator it = mLayer_Kappa->getFeatures();
    while (it.nextFeature(f))
    {
        if (f.isValid())
        {
            QString currentNo = f.attribute("相片编号").toString();
            double kappaValue = f.attribute("Kappa").toString().toDouble();
            if (abs(kappaValue) > kappa_angle_Max)
            {
                overrun << currentNo;

                // qdebug
                QgsMessageLog::logMessage(QString("%1相片超限 ").arg(currentNo));
            }
        }
        prDialog.setValue(++prCount);
        QApplication::processEvents();
    }

    prCount = 0;
    prDialog.setMaximum(overrun.size());
    prDialog.setLabelText("判断相片是否可删除...");

    // 遍历每张相片
    foreach (QString currentNo, overrun)
    {
        QStringList list;
        int lineNo = myOlp.getLineNumber(currentNo);
        myOlp.getLinePhoto(lineNo, list);
        if (currentNo==list.first() || currentNo==list.last())
        {
            // qdebug
            QgsMessageLog::logMessage(QString("相片位于航线两端 %1").arg(currentNo));

            // 相片位于航线两端,根据isEdge是否删除
            if (isEdge)
            {
                // 删除本张相片
                myOlp.delItem(currentNo);
                willDel << currentNo;
                overrun.removeOne(currentNo);

                // qdebug
                QgsMessageLog::logMessage(QString("相片位于航线两端 删除 %1").arg(currentNo));
            }
        }
        else
        {
            QStringList::const_iterator qit = list.constBegin();
            while (++qit != list.constEnd())
            {
                if (*qit == currentNo)
                {
                    lastNo = *(qit-1);
                    nextNo = *(qit+1);
                    break;
                }
            }
            int percentage = myOlp.calculateOverlap(lastNo, nextNo);

            // qdebug
            QgsMessageLog::logMessage(QString("%1相片相邻重叠度%2 ").arg(currentNo).arg(percentage));

            if (percentage > heading_Min)
            {
                // 删除本张相片
                myOlp.delItem(currentNo);
                willDel << currentNo;
                overrun.removeOne(currentNo);

                // qdebug
                QgsMessageLog::logMessage(QString("%1相片相邻重叠度%2 删除").arg(currentNo).arg(percentage));
            }
        }

        prDialog.setValue(++prCount);
        QApplication::processEvents();
        //当按下取消按钮则中断
        if (prDialog.wasCanceled())
            return QStringList();
    }

    myOlp.updataLineNumber();
    myOlp.updataPhotoNumber();
    return willDel;
}

void eqiAnalysisAerialphoto::updataChackValue()
{
    // qdebug
    QgsMessageLog::logMessage("updataChackValue()");

    QSettings mSettings;

    // 重叠度限差
    heading_Gen = mSettings.value("/eqi/options/imagePreprocessing/heading_Gen", 60).toInt();
    heading_Max = mSettings.value("/eqi/options/imagePreprocessing/heading_Max", 80).toInt();
    heading_Min = mSettings.value("/eqi/options/imagePreprocessing/heading_Min", 53).toInt();
    sideways_Gen = mSettings.value("/eqi/options/imagePreprocessing/sideways_Gen", 15).toInt();
    sideways_Max = mSettings.value("/eqi/options/imagePreprocessing/sideways_Max", 60).toInt();
    sideways_Min = mSettings.value("/eqi/options/imagePreprocessing/sideways_Min", 8).toInt();

    // Kappa角度限差
    kappa_angle_General = mSettings.value("/eqi/options/imagePreprocessing/kappa_angle_General", 15.0).toDouble();
    kappa_angle_Max = mSettings.value("/eqi/options/imagePreprocessing/kappa_angle_Max", 30.0).toDouble();
    kappa_angle_Line = mSettings.value("/eqi/options/imagePreprocessing/kappa_angle_Line", 20.0).toDouble();

    // 倾斜角度限差
    omega_angle_General = mSettings.value("/eqi/options/imagePreprocessing/omega_angle_General", 5.0).toDouble();
    omega_angle_Max = mSettings.value("/eqi/options/imagePreprocessing/omega_angle_Max", 12.0).toDouble();
    omega_angle_Average = mSettings.value("/eqi/options/imagePreprocessing/omega_angle_Average", 8.0).toDouble();

    errTpye_nextPhotoNoOverlap = "与下张相片没有重叠度";
    errTpye_nextPhotoMinOverlap = QString("航带内重叠度低于最低阈值%2%").arg(heading_Min);
    errTpye_nextPhotoGeneralOverlap = QString("航带内重叠度低于建议阈值%2%").arg(heading_Gen);
    errTpye_nextPhotoMaxOverlap = QString("航带内重叠度超过建议阈值%2%").arg(heading_Max);
    errTpye_nextLineNoOverlap = "与下航带相片没有重叠度";
    errTpye_nextLineMinOverlap = QString("航带间重叠度低于最低阈值%2%").arg(sideways_Min);
    errTpye_nextLineGeneralOverlap = QString("航带间重叠度低于建议阈值%2%").arg(sideways_Gen);
    errTpye_nextLineMaxOverlap = QString("航带间重叠度超过建议阈值%2%").arg(sideways_Max);
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

    //  debug
//    const QStringList* noList = mPosdp->noList();
//    foreach (QString noName, *noList)
//    {

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

        mySymbol->addChangedItem(f.attribute(mField).toString(), type);
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
    mySymbol->addChangedItem(errType, type);
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
//        qDebug() << "delete:" << it.key();
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
    if (n == 0)
    {
        return;
    }

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
    if (n == 0)
    {
        return;
    }

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

//    if (currentGeometry->intersects(nextGeometry))
//    {
//        // 计算重叠百分比
//        QgsGeometry *intersectGeometry = currentGeometry->intersection(nextGeometry);
//        double intersectArea = intersectGeometry->area();
//        double currentArea = currentGeometry->area();
//        double nextArea = nextGeometry->area();
//        int percentage = (int)((intersectArea/currentArea + intersectArea/nextArea)/2*100);
//        return percentage;
//    }
    return 0;
}

QMap<QString, int> OverlappingProcessing::getNextPhotoOverlapping(const QString &number)
{
    return map.value(number)->mMapNextPhoto;
}

QMap<QString, int> OverlappingProcessing::getNextLineOverlapping(const QString &number)
{
    return map.value(number)->mMapNextLine;
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
