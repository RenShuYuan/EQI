#include "eqianalysisaerialphoto.h"
#include "mainwindow.h"

#include "qgsmapcanvas.h"
#include "qgsvectordataprovider.h"

#include <QSet>
#include <QProgressDialog>

eqiAnalysisAerialphoto::eqiAnalysisAerialphoto()
{

}

eqiAnalysisAerialphoto::eqiAnalysisAerialphoto(QObject *parent, QgsVectorLayer *layer, posDataProcessing *posdp, eqiPPInteractive *pp)
    : QObject(parent)
    , mLayerMap(layer)
    , mPosdp(posdp)
    , mPp(pp)
    , isGroup(false)
{
    airLineGroup();
    mField = "错误类型";

    updataChackValue();
}

void eqiAnalysisAerialphoto::checkoverlappingIn(QgsVectorLayer *mLayer_OverlapIn)
{
    if (!isGroup)
    {
        QgsMessageLog::logMessage(QString("重叠度检查 : \t创建航带失败，无法进行重叠度检查。"));
    }

    int lineSize = myOlp.getLineSize();
    if (!lineSize) return;
    int lineNumber = 0;

    // 检查是否需要特征匹配以及能否达到匹配条件
    bool isEnable = false;
    isEnable = mSetting.value("/eqi/options/imagePreprocessing/match", true).toBool();
    if (isEnable)
    {
        if (!(mPp && mPp->islinked()))
        {
            isEnable = false;
            QgsMessageLog::logMessage("重叠度检查 : \tPOS未与相片创建联动关系,将忽略特征点匹配。");
        }
    }

    //进度条
    QProgressDialog prDialog("检查重叠度...", "取消", 0, myOlp.getAllPhotoNo().size(), nullptr);
    prDialog.setWindowTitle("处理进度");              //设置窗口标题
    prDialog.setWindowModality(Qt::WindowModal);      //将对话框设置为模态
    prDialog.show();
    int prCount = 0;

    // 遍历每条航线
    while (++lineNumber <= lineSize)
    {
        QString currentPhoto;
        QString nextPhoto;
        QStringList lineList;

        // 获得当前航线上所有相片号集合
        myOlp.getLinePhoto(lineNumber, lineList);
        if (lineList.isEmpty()) return;

        QStringList::const_iterator i = lineList.constBegin();

        // 获得第一张相片
        if (i != lineList.constEnd())
        {
            currentPhoto = *i;
            prDialog.setValue(++prCount);
            QApplication::processEvents();
        }

        // 遍历行带内每张相片
        while (++i != lineList.constEnd())
        {
            // 检查相邻两张相片的重叠度
            nextPhoto = *i;
            int percentage = myOlp.calculateOverlap(currentPhoto, nextPhoto);
            myOlp.setNextPhotoOl(currentPhoto, nextPhoto, percentage);

            // 匹配特征点
            int keys = 0;
            if (isEnable)
            {
                QString currentPhotoPath = mPp->getPhotoPath(currentPhoto);
                QString nextPhotoPath = mPp->getPhotoPath(nextPhoto);
                if (currentPhotoPath.isEmpty() || nextPhotoPath.isEmpty())
                {
                    myOlp.setNextPhotoKeys(currentPhoto, nextPhoto, keys);
                    QgsMessageLog::logMessage(QString("重叠度检查 : \t获取相片路径失败,%1与%2特征点匹配失败。")
                                              .arg(currentPhoto).arg(nextPhoto));
                }
                else
                {
                    // 连接点数量
                    keys = myOlp.calculateOverlapSiftGPU(currentPhotoPath, nextPhotoPath);
                    myOlp.setNextPhotoKeys(currentPhoto, nextPhoto, keys);

                    QgsMessageLog::logMessage(QString("重叠度检查 : \t%1与%2特征点匹配：%3个。")
                                              .arg(currentPhoto).arg(nextPhoto).arg(keys));
                }
            }

            currentPhoto = nextPhoto;

            prDialog.setValue(++prCount);
            QApplication::processEvents();
        }

        //当按下取消按钮则中断
        if (prDialog.wasCanceled()) break;
    }

    // 检查错误图层
    if ( !mLayer_OverlapIn )
    {
        QgsMessageLog::logMessage(QString("创建检查图层 : \t创建图层失败, 检查已终止, 注意检查plugins文件夹。"));
        return;
    }
    else if (mLayer_OverlapIn->isValid())
    {
        // 重置符号
        eqiSymbol *mySymbol = new eqiSymbol(nullptr, mLayer_OverlapIn, mField);
        mySymbol->delAllSymbolItem();
        delete mySymbol;

        if ( !( mLayer_OverlapIn->dataProvider()->capabilities() & QgsVectorDataProvider::DeleteFeatures ) )
        {
          MainWindow::instance()->messageBar()->pushMessage( tr( "Provider does not support deletion" ),
                                     tr( "Data provider does not support deleting features" ),
                                     QgsMessageBar::INFO, MainWindow::instance()->messageTimeout() );
          return;
        }

        mLayer_OverlapIn->selectAll();
        QgsFeatureIds ids = mLayer_OverlapIn->selectedFeaturesIds();
        if ( ids.size() != 0 )
        {
            mLayer_OverlapIn->dataProvider()->deleteFeatures(ids);
        }
    }

    // 初始化渲染类
    eqiSymbol *mySymbol = new eqiSymbol(this, mLayer_OverlapIn, mField);

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

    // 错误分类
    foreach (QString number, list)
    {
        QMap<QString, int> mapOverlap = myOlp.getNextPhotoOverlapping(number);
        QMap<QString, int>::const_iterator itOverlap = mapOverlap.constBegin();
        int overlap = itOverlap.value();

        QMap<QString, int> mapKeys = myOlp.getNextPhotoKeys(number);
        QMap<QString, int>::const_iterator itKeys = mapKeys.constBegin();
        int keys = itKeys.value();

        // 启用同名点检查
        if (isEnable)
        {
            // 检查是否与下张相片进行特征匹配，并且匹配有特征点
            if (!mapKeys.isEmpty() && keys !=0)
            {
                // 进行后续检查

                // 获得相交区域矢量面
                QgsGeometry *errGeometry = myOlp.getFeature(number)->geometry()
                        ->intersection(myOlp.getFeature(itOverlap.key())->geometry());

                if (overlap < heading_Min)   // 与航带内下张相片重叠度低于最低标准
                {
                    addOverLappingErrFeature(featureList, errGeometry, number
                                             , ++index, QString("%1%").arg(overlap)
                                             , QString::number(keys)
                                             , errTpye_nextPhotoMinOverlap
                                             , mySymbol, eqiSymbol::SeriousSrror);
                }
                else if (overlap < heading_Gen)   // 与航带内下张相片重叠度低于建议标准
                {
                    if (keys < keyThreshold)
                    {
                        addOverLappingErrFeature(featureList, errGeometry, number
                                                 , ++index, QString("%1%").arg(overlap)
                                                 , QString::number(keys)
                                                 , errTpye_nextPhotoKeysThreshold
                                                 , mySymbol, eqiSymbol::SeriousSrror);
                    }
                    else
                    {
                        addOverLappingErrFeature(featureList, errGeometry, number
                                                 , ++index, QString("%1%").arg(overlap)
                                                 , QString::number(keys)
                                                 , errTpye_nextPhotoGeneralOverlap
                                                 , mySymbol, eqiSymbol::warning);
                    }

                }
                else if (overlap > heading_Max)   // 与航带内下张相片重叠度大于最大建议值
                {
                    if (keys < keyThreshold)
                    {
                        addOverLappingErrFeature(featureList, errGeometry, number
                                                 , ++index, QString("%1%").arg(overlap)
                                                 , QString::number(keys)
                                                 , errTpye_nextPhotoKeysThreshold
                                                 , mySymbol, eqiSymbol::SeriousSrror);
                    }
                    else
                    {
                        addOverLappingErrFeature(featureList, errGeometry, number
                                                 , ++index, QString("%1%").arg(overlap)
                                                 , QString::number(keys)
                                                 , errTpye_nextPhotoMaxOverlap
                                                 , mySymbol, eqiSymbol::warning);
                    }

                }
                else if (keys < keyThreshold)  // 正常情况
                {
                    addOverLappingErrFeature(featureList, errGeometry, number
                                             , ++index, QString("%1%").arg(overlap)
                                             , QString::number(keys)
                                             , errTpye_nextPhotoKeysThreshold
                                             , mySymbol, eqiSymbol::SeriousSrror);
                }
            }
            else
            {
                // 检查是否最后一张
                if (!isCheckLineLast(number))
                {
                    QgsGeometry errGeometry = QgsGeometry(*myOlp.getFeature(number)->geometry());
                    addOverLappingErrFeature(featureList, errGeometry, number
                                             , ++index, "0%", "0"
                                             , errTpye_NoKeys
                                             , mySymbol, eqiSymbol::SeriousSrror);
                }
            }
        }
        else    // 不启用同名点检查
        {
            if (!mapOverlap.isEmpty() && overlap != 0)
            {
                // 进行后续检查

                // 获得相交区域矢量面
                QgsGeometry *errGeometry = myOlp.getFeature(number)->geometry()
                        ->intersection(myOlp.getFeature(itOverlap.key())->geometry());

                if (overlap < heading_Min)   // 与航带内下张相片重叠度低于最低标准
                {
                    addOverLappingErrFeature(featureList, errGeometry, number
                                             , ++index, QString("%1%").arg(overlap)
                                             , QString::number(keys)
                                             , errTpye_nextPhotoMinOverlap
                                             , mySymbol, eqiSymbol::SeriousSrror);
                }
                else if (overlap < heading_Gen)   // 与航带内下张相片重叠度低于建议标准
                {
                    addOverLappingErrFeature(featureList, errGeometry, number
                                             , ++index, QString("%1%").arg(overlap)
                                             , QString::number(keys)
                                             , errTpye_nextPhotoGeneralOverlap
                                             , mySymbol, eqiSymbol::warning);

                }
                else if (overlap > heading_Max)   // 与航带内下张相片重叠度大于最大建议值
                {
                    addOverLappingErrFeature(featureList, errGeometry, number
                                             , ++index, QString("%1%").arg(overlap)
                                             , QString::number(keys)
                                             , errTpye_nextPhotoMaxOverlap
                                             , mySymbol, eqiSymbol::warning);
                }
            }
            else
            {
                // 检查是否最后一张
                if (!isCheckLineLast(number))
                {
                    QgsGeometry errGeometry = QgsGeometry(*myOlp.getFeature(number)->geometry());
                    addOverLappingErrFeature(featureList, errGeometry, number
                                             , ++index, "0%", "0"
                                             , errTpye_nextPhotoNoOverlap
                                             , mySymbol, eqiSymbol::SeriousSrror);
                }
            }
        }

        prSymDialog.setValue(++prCount);
        QApplication::processEvents();
        //当按下取消按钮则中断
        if (prDialog.wasCanceled()) return;
    }

    // 将要素添加到图层中
    mLayer_OverlapIn->dataProvider()->addFeatures(featureList);
    MainWindow::instance()->refreshMapCanvas();
    mySymbol->updata();
}

void eqiAnalysisAerialphoto::checkoverlappingBetween(QgsVectorLayer* mLayer_OverlapBetween)
{
    if (!isGroup)
    {
        QgsMessageLog::logMessage(QString("重叠度检查 : \t创建航带失败，无法进行旋片角检查。"));
    }

    int lineSize = myOlp.getLineSize();
    if (!lineSize || (lineSize<2)) return;
    int lineNumber = 0;

    // 检查是否需要特征匹配以及能否达到匹配条件
    bool isEnable = false;
    isEnable = mSetting.value("/eqi/options/imagePreprocessing/match", true).toBool();
    if (isEnable)
    {
        if (!(mPp && mPp->islinked()))
        {
            isEnable = false;
            QgsMessageLog::logMessage("重叠度检查 : \tPOS未与相片创建联动关系,将忽略特征点匹配。");
        }
    }

    //进度条
    QProgressDialog prDialog("检查重叠度...", "取消", 0, myOlp.getAllPhotoNo().size(), nullptr);
    prDialog.setWindowTitle("处理进度");              //设置窗口标题
    prDialog.setWindowModality(Qt::WindowModal);      //将对话框设置为模态
    prDialog.show();
    int prCount = 0;

    while (lineNumber < lineSize-1)
    {
        QStringList lineList;
        QStringList nextlineList;

        // 获得当前航带相片
        myOlp.getLinePhoto(lineNumber, lineList);
        if (lineList.isEmpty()) return;

        // 获得下条航带相片
        myOlp.getLinePhoto(++lineNumber, nextlineList);
        if (nextlineList.isEmpty()) return;

        foreach (QString clPhoto, lineList)
        {
            // 保存重叠度最大的相片
            foreach (QString nlPhoto, nextlineList)
            {
                int percentage = myOlp.calculateOverlap(clPhoto, nlPhoto);
                myOlp.setNextLineOl(clPhoto, nlPhoto, percentage);
                if (percentage > sideways_Max) break;
            }

            // 将重叠度最大的相片提取出来进行匹配
            int keys = 0;
            if (isEnable)
            {
                QMap<QString, int> nlOverlap = myOlp.getNextLineOverlapping(clPhoto);
                if (!nlOverlap.isEmpty())
                {
                    QMap<QString, int>::const_iterator it = nlOverlap.constBegin();
                    QString nlPhoto = it.key();

                    QString currentPhotoPath = mPp->getPhotoPath(clPhoto);
                    QString nextPhotoPath = mPp->getPhotoPath(nlPhoto);
                    if (currentPhotoPath.isEmpty() || nextPhotoPath.isEmpty())
                    {
                        myOlp.setNextLineKeys(clPhoto, nlPhoto, keys);
                        QgsMessageLog::logMessage(QString("重叠度检查 : \t获取相片路径失败,%1与%2特征点匹配失败。")
                                                  .arg(clPhoto).arg(nlPhoto));
                    }
                    else
                    {
                        // 连接点数量
                        keys = myOlp.calculateOverlapSiftGPU(currentPhotoPath, nextPhotoPath);
                        myOlp.setNextLineKeys(clPhoto, nlPhoto, keys);
                        QgsMessageLog::logMessage(QString("重叠度检查 : \t%1与%2特征点匹配：%3个。")
                                                  .arg(clPhoto).arg(nlPhoto).arg(keys));
                    }
                }
            }
            prDialog.setValue(++prCount);
            QApplication::processEvents();
        }

        //当按下取消按钮则中断
        if (prDialog.wasCanceled()) return;
    }

    // 创建错误图层
    if ( !mLayer_OverlapBetween || !mLayer_OverlapBetween->isValid() )
    {
        QgsMessageLog::logMessage(QString("创建检查图层 : \t创建图层失败, 程序已终止, 注意检查plugins文件夹。"));
        return;
    }
    else if (mLayer_OverlapBetween->isValid())
    {
        // 重置符号
        eqiSymbol *mySymbol = new eqiSymbol(nullptr, mLayer_OverlapBetween, mField);
        mySymbol->delAllSymbolItem();
        delete mySymbol;

        if ( !( mLayer_OverlapBetween->dataProvider()->capabilities() & QgsVectorDataProvider::DeleteFeatures ) )
        {
          MainWindow::instance()->messageBar()->pushMessage( tr( "Provider does not support deletion" ),
                                     tr( "Data provider does not support deleting features" ),
                                     QgsMessageBar::INFO, MainWindow::instance()->messageTimeout() );
          return;
        }

        mLayer_OverlapBetween->selectAll();
        QgsFeatureIds ids = mLayer_OverlapBetween->selectedFeaturesIds();
        if ( ids.size() != 0 )
        {
            mLayer_OverlapBetween->dataProvider()->deleteFeatures(ids);
        }
    }

    // 初始化渲染类
    eqiSymbol *mySymbol = new eqiSymbol(this, mLayer_OverlapBetween, mField);

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
        QMap<QString, int> mapOverlap = myOlp.getNextLineOverlapping(number);
        QMap<QString, int>::const_iterator itOverlap = mapOverlap.constBegin();
        int overlap = itOverlap.value();

        QMap<QString, int> mapKeys = myOlp.getNextLineKeys(number);
        QMap<QString, int>::const_iterator itKeys = mapKeys.constBegin();
        int keys = itKeys.value();

        // 检查是否与下张相片进行特征匹配，并且匹配有特征点
        if (!mapKeys.isEmpty() && keys !=0)
        {
            // 获得相交区域矢量面
            QgsGeometry *currentGeometry = myOlp.getFeature(number)->geometry();
            QgsGeometry *maxGeometry = myOlp.getFeature(itOverlap.key())->geometry();
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

            if (overlap < sideways_Min)  // 低于最低标准
            {
                addOverLappingErrFeature(featureList, errGeometry, number
                                         , ++index, QString("%1%").arg(overlap)
                                         , QString::number(keys)
                                         , errTpye_nextLineMinOverlap
                                         , mySymbol, eqiSymbol::SeriousSrror);
                prSymDialog.setValue(++prCount);
                QApplication::processEvents();
                continue;
            }

            // 启用同名点检查
            if (isEnable)
            {
                if (overlap < sideways_Min) // 低于建议标准
                {
                    if (keys < keyThreshold)
                    {
                        addOverLappingErrFeature(featureList, errGeometry, number
                                                 , ++index, QString("%1%").arg(overlap)
                                                 , QString::number(keys)
                                                 , errTpye_nextLineGeneralOverlap
                                                 , mySymbol, eqiSymbol::warning);
                    }
                }
                else if (overlap > sideways_Max) // 大于最大建议
                {
                    if (keys < keyThreshold)
                    {
                        addOverLappingErrFeature(featureList, errGeometry, number
                                                 , ++index, QString("%1%").arg(overlap)
                                                 , QString::number(keys)
                                                 , errTpye_nextLineMaxOverlap
                                                 , mySymbol, eqiSymbol::warning);
                    }
                }
            }
            else
            {
                if (overlap < sideways_Min) // 低于建议标准
                {
                    addOverLappingErrFeature(featureList, errGeometry, number
                                             , ++index, QString("%1%").arg(overlap)
                                             , QString::number(keys)
                                             , errTpye_nextLineGeneralOverlap
                                             , mySymbol, eqiSymbol::warning);
                }
                else if (overlap > sideways_Max) // 大于最大建议
                {
                    addOverLappingErrFeature(featureList, errGeometry, number
                                             , ++index, QString("%1%").arg(overlap)
                                             , QString::number(keys)
                                             , errTpye_nextLineMaxOverlap
                                             , mySymbol, eqiSymbol::warning);
                }
            }
        }
        else
        {
            QgsGeometry errGeometry = QgsGeometry(*myOlp.getFeature(number)->geometry());
            addOverLappingErrFeature(featureList, errGeometry, number
                                     , ++index, "0%", "0"
                                     , errTpye_NoKeys
                                     , mySymbol, eqiSymbol::SeriousSrror);
        }

        prSymDialog.setValue(++prCount);
        QApplication::processEvents();
        //当按下取消按钮则中断
        if (prDialog.wasCanceled()) return;
    }

    // 将要素添加到图层中
    mLayer_OverlapBetween->dataProvider()->addFeatures(featureList);
    MainWindow::instance()->refreshMapCanvas();
    mySymbol->updata();
}

QStringList eqiAnalysisAerialphoto::delOverlapIn(QgsVectorLayer* mLayer_OverlapIn)
{
    if ( !(mLayer_OverlapIn && mLayer_OverlapIn->isValid()) )
    {
        MainWindow::instance()->messageBar()->pushMessage( "删除重叠度超限相片"
            ,"请先检查后再进行自动删除。", QgsMessageBar::INFO
            , MainWindow::instance()->messageTimeout() );
        return QStringList();
    }

    // 检查是否需要特征匹配以及能否达到匹配条件
    bool isEnable = false;
    isEnable = mSetting.value("/eqi/options/imagePreprocessing/match", true).toBool();
    if (isEnable)
    {
        if (!(mPp && mPp->islinked()))
        {
            isEnable = false;
            QgsMessageLog::logMessage("重叠度检查 : \tPOS未与相片创建联动关系,将忽略特征点匹配。");
        }
    }

    QStringList overrunHead;
    QStringList willDel;

    //进度条
    QProgressDialog prDialog("统计错误相片...", "取消", 0, mLayer_OverlapIn->featureCount(), nullptr);
    prDialog.setWindowTitle("处理进度");              //设置窗口标题
    prDialog.setWindowModality(Qt::WindowModal);      //将对话框设置为模态
    prDialog.show();
    int prCount = 0;

    // 从错误图层中提取所有超过最大限差的相片集合
    QgsFeature f;
    QgsFeatureIterator it = mLayer_OverlapIn->getFeatures();
    while (it.nextFeature(f))
    {
        if (f.isValid())
        {
            QString currentNo = f.attribute("相片编号").toString();
            QString strType = f.attribute("错误类型").toString();

            if (strType == errTpye_nextPhotoMaxOverlap)
                overrunHead << currentNo;
        }
        prDialog.setValue(++prCount);
        QApplication::processEvents();
    }

    prCount = 0;
    prDialog.reset();
    prDialog.setMaximum(overrunHead.size());
    prDialog.setLabelText("判断航带内相片是否可删除...");

    // 遍历每张相片  航带内
    for (int i=0; i<overrunHead.size(); ++i )
    {
        QString currentNo = overrunHead.at(i);
        QStringList list;
        int keys = 0;
        int percentage = 0;
        QString lastNo;
        QString nextNo;
        int lineNo = myOlp.getLineNumber(currentNo);
        myOlp.getLinePhoto(lineNo, list);
        if (currentNo!=list.first() && currentNo!=list.last())
        {
            bool isDeleted = false;
            int index = list.indexOf(currentNo);
            lastNo = list.at(index-1);
            nextNo = list.at(index+1);

            percentage = myOlp.calculateOverlap(lastNo, nextNo);
            if (isEnable)
            {
                QString lastPhotoPath = mPp->getPhotoPath(lastNo);
                QString nextPhotoPath = mPp->getPhotoPath(nextNo);
                keys = myOlp.calculateOverlapSiftGPU(lastPhotoPath, nextPhotoPath);
                if (keys > keyThreshold)
                {
                    if (percentage > heading_Gen) isDeleted = true;
                }
            }
            else
            {
                if (percentage > heading_Gen) isDeleted = true;
            }
            if (isDeleted)
            {
                QgsMessageLog::logMessage(QString("%1相邻两张相片重叠度为：%2，同名点为：%3，可删除").arg(currentNo).arg(percentage).arg(keys));
                myOlp.delItem(currentNo);
                willDel << currentNo;
                overrunHead.removeOne(currentNo);
            }
        }

        // debug
        QgsMessageLog::logMessage(QString("调试信息，航带内相片删除：当前相片：%1，上一张相片：%2，下一张相片：%3，重叠度为：%4，同名点为：%5")
                                  .arg(currentNo).arg(lastNo).arg(nextNo).arg(percentage).arg(keys));

        prDialog.setValue(++prCount);
        QApplication::processEvents();
        //当按下取消按钮则中断
        if (prDialog.wasCanceled())
            return QStringList();
    }

    myOlp.updataPhotoNumber();
    return willDel;
}

QStringList eqiAnalysisAerialphoto::delOverlapBetween(QgsVectorLayer *mLayer_OverlapBetween)
{
    // 检查图层有效性
    if ( !(mLayer_OverlapBetween && mLayer_OverlapBetween->isValid()) )
    {
        MainWindow::instance()->messageBar()->pushMessage( "删除重叠度超限相片"
            ,"请先检查后再进行自动删除。", QgsMessageBar::INFO
            , MainWindow::instance()->messageTimeout() );
        return QStringList();
    }

    // 检查是否需要特征匹配以及能否达到匹配条件
    bool isEnable = false;
    isEnable = mSetting.value("/eqi/options/imagePreprocessing/match", true).toBool();
    if (isEnable)
    {
        if (!(mPp && mPp->islinked()))
        {
            isEnable = false;
            QgsMessageLog::logMessage("重叠度检查 : \tPOS未与相片创建联动关系,将忽略特征点匹配。");
        }
    }

    //进度条
    QProgressDialog prDialog("统计错误相片...", "取消", 0, mLayer_OverlapBetween->featureCount(), nullptr);
    prDialog.setWindowTitle("处理进度");              //设置窗口标题
    prDialog.setWindowModality(Qt::WindowModal);      //将对话框设置为模态
    prDialog.show();
    int prCount = 0;

    // 从错误图层中提取所有超过最大限差的相片集合
    QStringList overrunSide;
    QStringList willDel;
    QgsFeature f;
    QgsFeatureIterator it = mLayer_OverlapBetween->getFeatures();
    while (it.nextFeature(f))
    {
        if (f.isValid())
        {
            QString currentNo = f.attribute("相片编号").toString();
            QString strType = f.attribute("错误类型").toString();

            if (strType == errTpye_nextLineMaxOverlap)
                overrunSide << currentNo;
        }
        prDialog.setValue(++prCount);
        QApplication::processEvents();
    }

    prCount = 0;
    prDialog.reset();
    prDialog.setMaximum(overrunSide.size());
    prDialog.setLabelText("判断航带间相片是否可删除...");

    // 遍历每张相片
    foreach (QString currentNo, overrunSide)
    {
        int lineNo = myOlp.getLineNumber(currentNo);

        // 跳过第一条航线
        if ( lineNo != 0 )
        {
            // 获得下条航带中的相片号
            QMap<QString, int> mapNext = myOlp.getNextLineOverlapping(currentNo);
            QMap<QString, int>::const_iterator itNext = mapNext.constBegin();
            QString maxNumber = itNext.key();

            // 遍历上一条航带中的每张相片，比较重叠度
            int keys = 0;
            int percentage = 0;
            bool isDeleted = false;
            QStringList lastList;
            QString lastPhoto;
            myOlp.getLinePhoto(lineNo-1, lastList);

            foreach (lastPhoto, lastList)
            {
                percentage = myOlp.calculateOverlap(lastPhoto, maxNumber);
                if (percentage > sideways_Max) break;
            }

            if (isEnable)
            {
                QString lastPhotoPath = mPp->getPhotoPath(lastPhoto);
                QString nextPhotoPath = mPp->getPhotoPath(maxNumber);
                keys = myOlp.calculateOverlapSiftGPU(lastPhotoPath, nextPhotoPath);
                if (keys > keyThreshold)
                {
                    isDeleted = true;
                }
            }
            else
            {
                isDeleted = true;
            }

            if (isDeleted)
            {
                QgsMessageLog::logMessage(QString("%1两张相片重叠度为：%2，同名点为：%3，可删除").arg(currentNo).arg(percentage).arg(keys));
                myOlp.setNextLineOl(lastPhoto, maxNumber, percentage);
                myOlp.setNextLineKeys(lastPhoto, maxNumber, keys);
                myOlp.delItem(currentNo);
                willDel << currentNo;
                overrunSide.removeOne(currentNo);
            }

            // debug
            QgsMessageLog::logMessage(QString("调试信息，航带间相片删除：当前相片：%1，上一航带相片：%2，下一航带相片：%3，重叠度为：%4，同名点为：%5")
                                      .arg(currentNo).arg(lastPhoto).arg(maxNumber).arg(percentage).arg(keys));

        }

        prDialog.setValue(++prCount);
        QApplication::processEvents();
        //当按下取消按钮则中断
        if (prDialog.wasCanceled())
            return QStringList();
    }

    myOlp.updataLineNumber();
    return willDel;
}

void eqiAnalysisAerialphoto::checkOmega(QgsVectorLayer* mLayer_Omega)
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
        QgsMessageLog::logMessage(QString("创建检查图层 : \t创建图层失败, 程序已终止, 注意检查plugins文件夹。"));
        return;
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

void eqiAnalysisAerialphoto::checkKappa(QgsVectorLayer *mLayer_Kappa)
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
        QgsMessageLog::logMessage(QString("创建检查图层 : \t创建图层失败, 程序已终止, 注意检查plugins文件夹。"));
        return;
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

QStringList eqiAnalysisAerialphoto::delOmegaAndKappa(QgsVectorLayer *layer, const QString& type, const bool isEdge)
{
    QStringList overrun;
    QStringList willDel;

    if (!(layer && layer->isValid()))
    {
        MainWindow::instance()->messageBar()->pushMessage( "删除角度超限相片"
            ,"请先检查后再进行自动删除。", QgsMessageBar::INFO
            , MainWindow::instance()->messageTimeout() );
        return willDel;
    }

    //进度条
    QProgressDialog prDialog("统计错误相片...", "取消", 0, layer->featureCount(), nullptr);
    prDialog.setWindowTitle("处理进度");              //设置窗口标题
    prDialog.setWindowModality(Qt::WindowModal);      //将对话框设置为模态
    prDialog.show();
    int prCount = 0;

    // 从错误图层中提取所有超过最大限差的相片集合
    QgsFeature f;
    QgsFeatureIterator it;
    it = layer->getFeatures();

    while (it.nextFeature(f))
    {
        if (f.isValid())
        {
            QString currentNo = f.attribute("相片编号").toString();
            if (type=="Omega")
            {
                double omegaValue = f.attribute("Omega").toString().toDouble();
                if (abs(omegaValue) > omega_angle_Max)
                {
                    overrun << currentNo;
                    // qdebug
                    QgsMessageLog::logMessage(QString("%1相片超限 ").arg(currentNo));
                }
            }
            else
            {
                double kappaValue = f.attribute("Kappa").toString().toDouble();
                if (abs(kappaValue) > kappa_angle_Max)
                {
                    overrun << currentNo;
                    // qdebug
                    QgsMessageLog::logMessage(QString("%1相片超限 ").arg(currentNo));
                }
            }

        }
        prDialog.setValue(++prCount);
        QApplication::processEvents();
    }

    prCount = 0;
    prDialog.setMaximum(overrun.size());
    prDialog.setLabelText("判断相片是否可删除...");

    // 检查是否需要特征匹配以及能否达到匹配条件
    bool isEnable = false;
    isEnable = mSetting.value("/eqi/options/imagePreprocessing/match", true).toBool();
    if (isEnable)
    {
        if (!(mPp && mPp->islinked()))
        {
            isEnable = false;
            QgsMessageLog::logMessage("重叠度检查 : \tPOS未与相片创建联动关系,将忽略特征点匹配。");
        }
    }

    // 遍历每张相片
    foreach (QString currentNo, overrun)
    {
        QStringList list;
        int lineNo = myOlp.getLineNumber(currentNo);
        myOlp.getLinePhoto(lineNo, list);
        if (currentNo==list.first() || currentNo==list.last())
        {
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
            int keys = 0;
            bool isDeleted = false;
            int index = list.indexOf(currentNo);
            QString lastNo = list.at(index-1);
            QString nextNo = list.at(index+1);
            int percentage = myOlp.calculateOverlap(lastNo, nextNo);

            if (isEnable)
            {
                QString lastPhotoPath = mPp->getPhotoPath(lastNo);
                QString nextPhotoPath = mPp->getPhotoPath(nextNo);
                keys = myOlp.calculateOverlapSiftGPU(lastPhotoPath, nextPhotoPath);
                if (keys > keyThreshold)
                {
                    if (percentage > heading_Gen) isDeleted = true;
                }
            }
            else
            {
                if (percentage > heading_Gen) isDeleted = true;
            }
            if (isDeleted)
            {
                QgsMessageLog::logMessage(QString("%1相邻两张相片重叠度为：%2，同名点为：%3，可删除").arg(currentNo).arg(percentage).arg(keys));
                myOlp.delItem(currentNo);
                willDel << currentNo;
                overrun.removeOne(currentNo);
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
    errTpye_nextPhotoMinOverlap = QString("航带内重叠度低于最低阈值%1%").arg(heading_Min);
    errTpye_nextPhotoGeneralOverlap = QString("航带内重叠度低于建议阈值%1%").arg(heading_Gen);
    errTpye_nextPhotoMaxOverlap = QString("航带内重叠度超过建议阈值%1%").arg(heading_Max);
    errTpye_NoKeys = QString("未匹配到同名点");
    errTpye_nextPhotoKeysThreshold = QString("航带内相邻相片同名点低于阈值：1%").arg(keyThreshold);;
    errTpye_nextLineNoOverlap = "与下航带相片没有重叠度";
    errTpye_nextLineMinOverlap = QString("航带间重叠度低于最低阈值%1%").arg(sideways_Min);
    errTpye_nextLineGeneralOverlap = QString("航带间重叠度低于建议阈值%1%").arg(sideways_Gen);
    errTpye_nextLineMaxOverlap = QString("航带间重叠度超过建议阈值%1%").arg(sideways_Max);
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
                                                      , const QString &overlappingValue
                                                      , const QString &keysValue
                                                      , const QString &errType
                                                      , eqiSymbol *mySymbol
                                                      , eqiSymbol::linkedType type)
{
    QgsFeature MyFeature;
    MyFeature.setGeometry(errGeometry);
    MyFeature.setAttributes(QgsAttributes() << QVariant(index)
                            << QVariant(number)
                            << QVariant(overlappingValue)
                            << QVariant(keysValue)
                            << QVariant(errType));
    featureList.append(MyFeature);
    mySymbol->addChangedItem(errType, type);
}

void eqiAnalysisAerialphoto::addOverLappingErrFeature(QgsFeatureList &featureList
                                                      , QgsGeometry errGeometry
                                                      , const QString &number
                                                      , const int index
                                                      , const QString &overlappingValue
                                                      , const QString &keysValue
                                                      , const QString &errType
                                                      , eqiSymbol *mySymbol
                                                      , eqiSymbol::linkedType type)
{
    QgsFeature MyFeature;
    MyFeature.setGeometry(errGeometry);
    MyFeature.setAttributes(QgsAttributes() << QVariant(index)
                            << QVariant(number)
                            << QVariant(overlappingValue)
                            << QVariant(keysValue)
                            << QVariant(errType));
    featureList.append(MyFeature);
    mySymbol->addChangedItem(errType, type);
}

bool eqiAnalysisAerialphoto::isCheckLineLast(const QString &name)
{
    QStringList list;
    int lineNo = myOlp.getLineNumber(name);
    myOlp.getLinePhoto(lineNo, list);

    if (name != list.last()) return false;
    return true;
}
