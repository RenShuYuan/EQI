﻿#include "eqippinteractive.h"
#include "mainwindow.h"
#include "eqi/eqiapplication.h"
#include "eqi/eqisymbol.h"

#include "qgsmapcanvas.h"
#include "qgsmessagelog.h"
#include "qgsmessagebar.h"
#include "qgsvectorlayer.h"
#include "qgslayertreeview.h"
#include "qgssymbolv2.h"
#include "qgssinglesymbolrendererv2.h"
#include "qgscategorizedsymbolrendererv2.h"
#include "qgsfillsymbollayerv2.h"

#include <QFile>

eqiPPInteractive::eqiPPInteractive(QObject *parent, QgsVectorLayer* layer, const QStringList *noFields)
    : QObject(parent),
    mLayer(layer),
    mNoFields(noFields)
{
    isLinked = false;
    fieldName = "相片编号";
    mySymbol = new eqiSymbol(parent, layer, fieldName);
}

eqiPPInteractive::~eqiPPInteractive()
{

}

void eqiPPInteractive::upDataLinkedSymbol()
{
    if (mNoFields->isEmpty())
    {
        isLinked = false;
        return;
    }

    QString phtotPath = mSetting.value("/eqi/pos/lePosFile", "").toString();
    phtotPath = QFileInfo(phtotPath).path();
    phtotPath += "/" + mSetting.value("/eqi/options/imagePreprocessing/lePhotoFolder", "").toString();

    if (!QFileInfo(phtotPath).exists())
    {
        MainWindow::instance()->messageBar()->pushMessage( "动态联动",
                                                           QString("%1 未指定正确的相片路径, 创建联动关系失败...")
                                                           .arg(QDir::toNativeSeparators(phtotPath)),
                                                           QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("动态联动 : \t%1 未指定正确的相片路径, 创建联动关系失败...")
                                  .arg(QDir::toNativeSeparators(phtotPath)));
        isLinked = false;
        return;
    }

    emit startProcess();
    photosList.clear();
    photosList = eqiApplication::searchFiles(phtotPath, QStringList() << "*.tif" << "*.jpg");
    emit stopProcess();

    if (photosList.isEmpty())
    {
        MainWindow::instance()->messageBar()->pushMessage( "动态联动",
                                                           QString("%1 路径下未搜索到tif、jpg格式的航飞相片, 创建联动关系失败...")
                                                           .arg(QDir::toNativeSeparators(phtotPath)),
                                                           QgsMessageBar::CRITICAL,
                                                           MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("动态联动 : \t%1 路径下未搜索到tif、jpg格式的航飞相片, 创建联动关系失败...")
                                  .arg(QDir::toNativeSeparators(phtotPath)));
        isLinked = false;
        return;
    }
    else
    {
        QgsMessageLog::logMessage(QString("动态联动 : \t%1 路径下搜索到%2张相片.")
                                  .arg(QDir::toNativeSeparators(phtotPath))
                                  .arg(photosList.size()));
    }

    //matchPosName(list, basePos);

    // 填充mPhotoMap, mPhotoMap[001] = "D:\测试数据\平台处理\photo\001.tif"
    foreach (QString str, photosList)
        mPhotoMap[QFileInfo(str).baseName()] = str;

    QMap<QString, QString>::iterator it = mPhotoMap.begin();
    while (it != mPhotoMap.end())
    {
        QString key = it.key();
        if (!mNoFields->contains(key))
        {
            QgsMessageLog::logMessage(QString("\t\t||--> 相片:%1在曝光点文件中未找到对应记录.").arg(key));
            it = mPhotoMap.erase(it);
        }
        else
        {
            mySymbol->addChangedItem(key, eqiSymbol::linked);
            ++it;
        }
    }

    isLinked = true;

    // 更新符号
    mySymbol->updata();
}

void eqiPPInteractive::upDataUnLinkedSymbol()
{
    QMap<QString, QString>::iterator it = mPhotoMap.begin();
    while (it != mPhotoMap.end())
    {
        mySymbol->addChangedItem(it.key(), eqiSymbol::unlinked);
        ++it;
    }

    mPhotoMap.clear();
    isLinked = false;

    // 更新符号
    mySymbol->updata();
}

void eqiPPInteractive::delSelect()
{
//    QgsMessageLog::logMessage(QString("\t\t||--> mNoFields.size() = %1").arg(mNoFields->size()));
    if (!islinked() || !isAlllinked())
    {
        MainWindow::instance()->messageBar()->pushMessage("修改航飞数据",
                                                          "在修改航飞数据前，相片需要与曝光点完全对应。",
                                                          QgsMessageBar::CRITICAL,
                                                          MainWindow::instance()->messageTimeout());
        return;
    }

    // 获得选择的相片编号
    QStringList photoNames;
    QgsFeatureList fList =	mLayer->selectedFeatures();
    foreach (QgsFeature f, fList)
    {
        photoNames.append(f.attribute(fieldName).toString());
    }

    // 删除POS数据
    emit delPos(photoNames);

    // 删除航飞略图
    delMap();

    // 更新略图
    mySymbol->delSymbolItem(photoNames);

    // 删除相片
    delPhoto(photoNames);

}

void eqiPPInteractive::saveSelect(const QString &savePath)
{

}

int eqiPPInteractive::delMap()
{
    int deletedCount = 0;

    mLayer->startEditing();
    mLayer->deleteSelectedFeatures(&deletedCount);
    mLayer->commitChanges();

    return deletedCount;
}

void eqiPPInteractive::delPhoto(QStringList &photoList)
{
    foreach (QString name, photoList)
    {
        if (mPhotoMap.contains(name))
        {
            // 删除相片
            if (QFile::exists(mPhotoMap.value(name)))
            {
                if (QFile::remove(mPhotoMap.value(name)))
                {
                    mPhotoMap.remove(name);
                }
                else
                {
                    QgsMessageLog::logMessage(QString("\t\t||--> 删除相片 : 删除%1相片失败，remove()。").arg(name));
                }
            }
            else
            {
                QgsMessageLog::logMessage(QString("\t\t||--> 删除相片 : 删除%1相片失败，未在文件夹下找到。").arg(name));
            }
        }
        else
        {
            QgsMessageLog::logMessage(QString("\t\t||--> 删除相片 : mPhotoMap err。").arg(name));
        }
    }
}

bool eqiPPInteractive::isValid()
{
    if (!mLayer || !mNoFields)
    {
        return false;
    }
    if (!mLayer->isValid() || mNoFields->isEmpty())
    {
        return false;
    }
    return true;
}

bool eqiPPInteractive::isAlllinked()
{
    QMap<QString, QString>::iterator it = mPhotoMap.begin();
    if (mPhotoMap.size() != mNoFields->size())
    {
        return false;
    }
    while (it != mPhotoMap.end())
    {
        QString key = it.key();
        if (!mNoFields->contains(key))
        {
            return false;
            break;
        }
        ++it;
    }
    return true;
}

void eqiPPInteractive::setVectorLayer( QgsVectorLayer* layer )
{
    if (!layer || !layer->isValid())
        return;
    mLayer = layer;
}

//void eqiPPInteractive::setNoFieldsList(QStringList* fieldsList)
//{
//	if (!fieldsList)
//		return;
//	mNoFields = fieldsList;
//}

//void eqiPPInteractive::matchPosName()
//{
//    QString photoName;
//    QStringList* tmpPosList;
//    QStringList photoNameList;
//    foreach (QString photoPath, photosList)
//        photoNameList.append(QFileInfo(photoPath).baseName());
//    if (photoNameList.isEmpty())
//        return;

//    tmpPosList = mNoFields;

//    QgsMessageLog::logMessage(QString("自动匹配名称 : \t开始匹配曝光点与相片名称..."));

//    for (int i=0; i<tmpPosList->size(); ++i)
//    {
//        int count = 0;
//        QString posName = tmpPosList->at(i);
//        foreach (const QString tmpPhotoName, photoNameList)
//        {
//            if (tmpPhotoName.contains(posName, Qt::CaseInsensitive))
//            {
//                photoName = tmpPhotoName;
//                ++count;
//            }
//        }
//        if (count==0)	// 没匹配到
//        {
//            int i = 0;
//            while (i<posName.size())
//            {
//                if (posName.at(i).isNumber())
//                {
//                    if (posName.at(i)==QChar('0'))
//                    {
//                        posName.remove(i, 1);
//                    }
//                    else
//                        break;
//                }
//                ++i;
//            }
//            (*tmpPosList)[i] = posName;
//            --i;
//        }
//        else if (count==1) // 匹配到一个
//        {
//            QgsMessageLog::logMessage(QString("\t\t匹配到曝光点: %1与相片名称: %2符合规则, 已自动更新曝光点名称.")
//                                      .arg(mNoFields->at(i)).arg(photoName));
//            (*mNoFields)[i] = photoName;
//        }
//        else				// 匹配到多个
//        {
//            if (posName.size() < photoName.size())
//            {
//                (*tmpPosList)[i] = posName.insert(0, '0');
//                --i;
//            }
//            else
//                QgsMessageLog::logMessage(QString("\t\t未匹配到曝光点: %1.").arg(mNoFields->at(i)));
//        }
//    }

//    //
//}

//void eqiPPInteractive::pTosSwitch()
//{
//    QgsMessageLog::logMessage("1111");

//    QgsSymbolV2* mSymbolV2 = nullptr;
//    mSymbolV2 = QgsSymbolV2::defaultSymbol(mLayer->geometryType());
//    mSymbolV2->changeSymbolLayer(0,QgsCentroidFillSymbolLayerV2::create());

//    QgsCategorizedSymbolRendererV2* cRenderer;
//    cRenderer = dynamic_cast< QgsCategorizedSymbolRendererV2* >( mLayer->rendererV2() );
//    cRenderer->updateSymbols(mSymbolV2->clone());
//    MainWindow::instance()->layerTreeView()->refreshLayerSymbology(mLayer->id());
//    MainWindow::instance()->refreshMapCanvas();
//}
