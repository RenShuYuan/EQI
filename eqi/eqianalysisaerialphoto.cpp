#include "eqianalysisaerialphoto.h"
#include "eqi/pos/posdataprocessing.h"
#include "eqi/eqippinteractive.h"
#include "mainwindow.h"
#include "siftGPU.h"

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
    , mPp(nullptr)
    , mLayer_Omega(nullptr)
    , mLayer_Kappa(nullptr)
    , mLayer_OverlapIn(nullptr)
    , isGroup(false)
{
    mField = "错误类型";
    updataChackValue();
}

eqiAnalysisAerialphoto::eqiAnalysisAerialphoto(QObject *parent, QgsVectorLayer *layer, posDataProcessing *posdp, eqiPPInteractive *pp)
    : QObject(parent)
    , mLayerMap(layer)
    , mPosdp(posdp)
    , mPp(pp)
    , mLayer_Omega(nullptr)
    , mLayer_Kappa(nullptr)
    , mLayer_OverlapIn(nullptr)
    , isGroup(false)
{
    airLineGroup();
    mField = "错误类型";

    updataChackValue();
}

void eqiAnalysisAerialphoto::checkoverlappingIn()
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

    // 创建错误图层
    if ( !mLayer_OverlapIn )
    {
        mLayer_OverlapIn = MainWindow::instance()->createrMemoryMap("航带间重叠度检查",
                                                            "Polygon",
                                                            QStringList()
                                                            << "field=id:integer"
                                                            << "field=相片编号:string(50)"
                                                            << "field=与相邻相片重叠度:string(10)"
                                                            << "field=与相邻相片同名点:string(10)"
                                                            << "field=错误类型:string(100)");

        if (!mLayer_OverlapIn && !mLayer_OverlapIn->isValid())
        {
            MainWindow::instance()->messageBar()->pushMessage( "创建检查图层",
                "创建图层失败, 运行已终止, 注意检查plugins文件夹!",
                QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
            QgsMessageLog::logMessage(QString("创建检查图层 : \t创建图层失败, 程序已终止, 注意检查plugins文件夹。"));
            return;
        }
        mLayer_OverlapIn->setCrs(mLayerMap->crs());
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

void eqiAnalysisAerialphoto::checkoverlappingBetween()
{
    if (!isGroup)
    {
        QgsMessageLog::logMessage(QString("重叠度检查 : \t创建航带失败，无法进行旋片角检查。"));
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

    while (lineNumber < lineSize-1)
    {
        QString currentPhoto;
        QString nextPhoto;
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
        mLayer_OverlapBetween = MainWindow::instance()->createrMemoryMap("航带内重叠度检查",
                                                            "Polygon",
                                                            QStringList()
                                                            << "field=id:integer"
                                                            << "field=相片编号:string(50)"
                                                            << "field=与相邻相片重叠度:string(10)"
                                                            << "field=航带间相片同名点:string(10)"
                                                            << "field=错误类型:string(100)");

        if (!mLayer_OverlapBetween && !mLayer_OverlapBetween->isValid())
        {
            MainWindow::instance()->messageBar()->pushMessage( "创建检查图层",
                "创建图层失败, 运行已终止, 注意检查plugins文件夹!",
                QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
            QgsMessageLog::logMessage(QString("创建检查图层 : \t创建图层失败, 程序已终止, 注意检查plugins文件夹。"));
            return;
        }
        mLayer_OverlapBetween->setCrs(mLayerMap->crs());
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
            // 启用同名点检查
            if (isEnable)
            {

            }
            else
            {

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

//        if (mapNext.isEmpty())  // 没有重叠度
//        {
            // 检查当前相片是否为最后一条航线
//            int lineNo = myOlp.getLineNumber(number);
//            int lineSize = myOlp.getLineSize();

//            if (lineNo != lineSize)
//            {
//                QgsGeometry *errGeometry = myOlp.getFeature(number)->geometry();
//                addOverLappingErrFeature(featureList, errGeometry, number
//                                         , ++index, "0%"
//                                         , errTpye_nextLineNoOverlap
//                                         , mySymbol, eqiSymbol::SeriousSrror);
//            }
//        }
//        else
//        {
//            QMap<QString, int>::const_iterator itNext = mapNext.constBegin();
//            QString maxNumber = itNext.key();
//            int maxPercentage = itNext.value();

//            // 获得相交区域矢量面
//            QgsGeometry *currentGeometry = myOlp.getFeature(number)->geometry();
//            QgsGeometry *maxGeometry = myOlp.getFeature(maxNumber)->geometry();
//            QgsGeometry *errGeometry = nullptr;

//            if (currentGeometry && maxGeometry)
//            {
//                errGeometry = currentGeometry->intersection(maxGeometry);
//            }
//            else
//            {
//                prSymDialog.setValue(++prCount);
//                QApplication::processEvents();
//                continue;
//            }

//            if (maxPercentage < sideways_Min)  // 低于最低标准
//            {
//                addOverLappingErrFeature(featureList, errGeometry, number
//                                         , ++index, QString("%1%").arg(maxPercentage)
//                                         , errTpye_nextLineMinOverlap
//                                         , mySymbol, eqiSymbol::SeriousSrror);
//            }
//            else if (maxPercentage < sideways_Min) // 低于建议标准
//            {
//                addOverLappingErrFeature(featureList, errGeometry, number
//                                         , ++index, QString("%1%").arg(maxPercentage)
//                                         , errTpye_nextLineGeneralOverlap
//                                         , mySymbol, eqiSymbol::warning);
//            }
//            else if (maxPercentage > sideways_Max) // 大于最大建议
//            {
//                addOverLappingErrFeature(featureList, errGeometry, number
//                                         , ++index, QString("%1%").arg(maxPercentage)
//                                         , errTpye_nextLineMaxOverlap
//                                         , mySymbol, eqiSymbol::warning);
//            }
//        }

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

QStringList eqiAnalysisAerialphoto::delOverlapping()
{
    if ( !(mLayer_OverlapIn && mLayer_OverlapIn->isValid()) )
    {
        MainWindow::instance()->messageBar()->pushMessage( "删除重叠度超限相片"
            ,"请先检查后再进行自动删除。", QgsMessageBar::INFO
            , MainWindow::instance()->messageTimeout() );
        return QStringList();
    }

    QStringList overrunHead;
//    QStringList overrunSide;
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
//            else if (strType == errTpye_nextLineMaxOverlap)  // 航带间
//                overrunSide << currentNo;
        }
        prDialog.setValue(++prCount);
        QApplication::processEvents();
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

    prCount = 0;
//    prDialog.setMaximum(overrunHead.size() + overrunSide.size());
    prDialog.reset();
    prDialog.setMaximum(overrunHead.size());
    prDialog.setLabelText("判断航带内相片是否可删除...");

    // 遍历每张相片  航带内
    for (int i=0; i<overrunHead.size(); ++i )
    {
        QString currentNo = overrunHead.at(i);
        QStringList list;
        int lineNo = myOlp.getLineNumber(currentNo);
        myOlp.getLinePhoto(lineNo, list);
        if (currentNo!=list.first() && currentNo!=list.last())
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
                overrunHead.removeOne(currentNo);
            }
        }
        prDialog.setValue(++prCount);
        QApplication::processEvents();
        //当按下取消按钮则中断
        if (prDialog.wasCanceled())
            return QStringList();
    }

//    prDialog.setLabelText("判断航带间相片是否可删除...");

//    // 遍历每张相片  航带间
//    foreach (QString currentNo, overrunSide)
//    {
//        QStringList list;
//        int lineNo = myOlp.getLineNumber(currentNo);
//        myOlp.getLinePhoto(lineNo, list);
//        int lineSize = myOlp.getLineSize();

//        if ( (lineNo-1)>0 && (lineNo+1)<=lineSize ) // 保证上下还有航线
//        {
//            // 获得下条航带中的相片号
//            QMap<QString, int> mapNext = myOlp.getNextLineOverlapping(currentNo);
//            QMap<QString, int>::const_iterator itNext = mapNext.constBegin();
//            QString maxNumber = itNext.key();

//            // 遍历上一条航带中的每张相片，比较重叠度
//            QStringList lastList;
//            myOlp.getLinePhoto(lineNo-1, lastList);

//            QStringList::const_iterator itLast = lastList.constBegin();
//            while (itLast != lastList.constEnd())
//            {
//                QString lastPhoto = *itLast++;
//                int percentage = myOlp.calculateOverlap(lastPhoto, maxNumber);

//                if (percentage > sideways_Gen)
//                {
//                    // 删除本张相片
//                    myOlp.delItem(currentNo);
//                    willDel << currentNo;
//                    // qdebug
//                    qDebug() << (QString("%1相片航带间重叠度%2 删除").arg(currentNo).arg(percentage));
//                    break;
//                }
//            }
//        }

//        prDialog.setValue(++prCount);
//        QApplication::processEvents();
//        //当按下取消按钮则中断
//        if (prDialog.wasCanceled())
//            return QStringList();
//    }

//    myOlp.updataLineNumber();
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

QStringList eqiAnalysisAerialphoto::delOmegaAndKappa(const QString& type, const bool isEdge)
{
    // 重新检查mLayer_Kappa与mLayer_Omega图层有效性
//    if ( !mLayer_Kappa || !mLayer_Kappa->isValid() )
//    {
//        MainWindow::instance()->messageBar()->pushMessage( "删除错误相片"
//            ,"请先检查后再进行自动删除。", QgsMessageBar::INFO
//            , MainWindow::instance()->messageTimeout() );
//        return QStringList();
//    }

    QStringList overrun;
    QStringList willDel;

    //进度条
    QProgressDialog prDialog("统计错误相片...", "取消", 0, mLayer_Kappa->featureCount(), nullptr);
    prDialog.setWindowTitle("处理进度");              //设置窗口标题
    prDialog.setWindowModality(Qt::WindowModal);      //将对话框设置为模态
    prDialog.show();
    int prCount = 0;

    // 从错误图层中提取所有超过最大限差的相片集合
    QgsFeature f;
    QgsFeatureIterator it;
    if (type=="Omega")
        it = mLayer_Omega->getFeatures();
    else
        it = mLayer_Kappa->getFeatures();

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

//    char * argv[] = {"-fo", "-1",  "-v", "1", "-tc", "2000"};
    char * argv[] = {"-fo", "-1",  "-v", "1"};
    int argc = sizeof(argv)/sizeof(char*);
    sift->ParseParam(argc, argv);

    // 检查硬件是否支持SiftGPU
    int support = sift->CreateContextGL();
    if ( support != SiftGPU::SIFTGPU_FULL_SUPPORTED )
    {
        QgsMessageLog::logMessage("当前硬件不支持GPU计算功能!");
        return 0;
    }

    // 检查是否已匹配过
    if (mapKeyDescriptors.contains(currentNo))
    {
        descriptors1 = mapKeyDescriptors.value(currentNo);
    }
    else
    {
        // 使用GDAL读取影像
        float *image1;
        int xSize1 = 0;
        int ySize1 = 0;
        bool isReadImage1 = gdalTools.readRasterIO(currentNo, &image1, xSize1, ySize1);
        if (!isReadImage1) return 0;

        if(sift->RunSIFT(xSize1, ySize1, image1, GL_LUMINANCE, GL_FLOAT))
        {
            int num1 = sift->GetFeatureNum();
            vector<SiftGPU::SiftKeypoint> keys1(num1);
            descriptors1.resize(128*num1);
            sift->GetFeatureVector(&keys1[0], &descriptors1[0]);
            mapKeyDescriptors[currentNo] = descriptors1;
        }

        delete[] image1;
    }

    if (mapKeyDescriptors.contains(nextNo))
    {
        descriptors2 = mapKeyDescriptors.value(nextNo);
    }
    else
    {
        // 使用GDAL读取影像
        float *image2;
        int xSize2 = 0;
        int ySize2 = 0;
        bool isReadImage2 = gdalTools.readRasterIO(nextNo, &image2, xSize2, ySize2);
        if (!isReadImage2) return 0;

        if(sift->RunSIFT(xSize2, ySize2, image2, GL_LUMINANCE, GL_FLOAT))
        {
            int num2 = sift->GetFeatureNum();
            vector<SiftGPU::SiftKeypoint> keys2(num2);
            descriptors2.resize(128*num2);
            sift->GetFeatureVector(&keys2[0], &descriptors2[0]);
        }

        delete[] image2;
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
