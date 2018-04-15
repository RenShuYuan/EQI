#include "eqippinteractive.h"
#include "mainwindow.h"
#include "eqi/eqiapplication.h"
#include "eqi/eqisymbol.h"

#include "qgsmapcanvas.h"
#include "qgsmessagebar.h"

#include "qgsvectorfilewriter.h"
#include "qgslayertreeview.h"
#include "qgssymbolv2.h"
#include "qgssinglesymbolrendererv2.h"
#include "qgscategorizedsymbolrendererv2.h"
#include "qgsfillsymbollayerv2.h"

#include <QFile>
#include <QProgressDialog>

eqiPPInteractive::eqiPPInteractive(QObject *parent, QgsVectorLayer* layer, posDataProcessing *posdp)
    : QObject(parent),
    mLayer(layer),
    mPosdp(posdp)
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
    if (mPosdp->noList()->isEmpty())
    {
        isLinked = false;
        return;
    }

    QString photoPath = mSetting.value("/eqi/pos/lePosFile", "").toString();
    photoPath = QFileInfo(photoPath).path();
    photoPath += "/" + mSetting.value("/eqi/options/imagePreprocessing/lePhotoFolder", "").toString();

    if (!QFileInfo(photoPath).exists())
    {
        MainWindow::instance()->messageBar()->pushMessage( "动态联动",
                                                           QString("%1 未指定正确的相片路径, 创建联动关系失败...")
                                                           .arg(QDir::toNativeSeparators(photoPath)),
                                                           QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("动态联动 : \t%1 未指定正确的相片路径, 创建联动关系失败...")
                                  .arg(QDir::toNativeSeparators(photoPath)));
        isLinked = false;
        return;
    }

    emit startProcess();
    photosList.clear();
    photosList = eqiApplication::searchFiles(photoPath, QStringList() << "*.tif" << "*.jpg");
    emit stopProcess();

    if (photosList.isEmpty())
    {
        MainWindow::instance()->messageBar()->pushMessage( "动态联动",
                                                           QString("%1 路径下未搜索到tif、jpg格式的航飞相片, 创建联动关系失败...")
                                                           .arg(QDir::toNativeSeparators(photoPath)),
                                                           QgsMessageBar::CRITICAL,
                                                           MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("动态联动 : \t%1 路径下未搜索到tif、jpg格式的航飞相片, 创建联动关系失败...")
                                  .arg(QDir::toNativeSeparators(photoPath)));
        isLinked = false;
        return;
    }

    QgsMessageLog::logMessage(QString("动态联动 : \t%1 路径下搜索到%2张相片.")
                              .arg(QDir::toNativeSeparators(photoPath))
                              .arg(photosList.size()));

    //matchPosName(list, basePos);

    // 填充mPhotoMap, mPhotoMap[001] = "D:\测试数据\平台处理\photo\001.tif"
    foreach (QString str, photosList)
        mPhotoMap[QFileInfo(str).baseName()] = str;

    // 检查相片与POS的对应关系
    const QStringList *posList = mPosdp->noList();
    QMap<QString, QString>::iterator it = mPhotoMap.begin();
    while (it != mPhotoMap.end())
    {
        QString key = it.key();
        if (!posList->contains(key))
        {
            QgsMessageLog::logMessage(QString("\t\t||--> 相片:%1在曝光点文件中未找到对应记录.").arg(key));
        }
        else
        {
            mySymbol->addChangedItem(key, eqiSymbol::linked);
        }
        ++it;
    }

    // 检查POS与相片的对应关系
    for (int i = 0; i < posList->size(); ++i)
    {
        if (!mPhotoMap.contains(posList->at(i)))
            QgsMessageLog::logMessage(QString("\t\t||--> POS:%1在相片文件中未找到对应数据.").arg(posList->at(i)));
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

void eqiPPInteractive::saveSelect(const QString &savePath)
{
    if (!islinked() || !isAlllinked())
    {
        MainWindow::instance()->messageBar()->pushMessage("修改航摄数据",
                                                          "在修改航摄数据前，相片需要与曝光点完全对应。",
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

    // 保存略图
    saveMap(savePath);

    // 保存POS
    savePos(savePath, photoNames);

    // 保存相片
    savePhoto(savePath, photoNames);
}

int eqiPPInteractive::delDirectMap(const QStringList& delList)
{
    int deletedCount = 0;

    mLayer->startEditing();
    mLayer->deleteSelectedFeatures(&deletedCount);
    mLayer->commitChanges();

    // 更新略图符号系统
    mySymbol->delSymbolItem(delList);

    return deletedCount;
}

int eqiPPInteractive::delMap(const QStringList &delList)
{
    QgsFeature f;
    QString strExpression;
    QgsFeatureRequest reQuest;
    QgsFeatureIds fids;

    foreach (QString no, delList)
    {
        strExpression += QString("相片编号='%1' OR ").arg(no);
    }
    strExpression = strExpression.left(strExpression.size() - 3);
    reQuest.setFilterExpression(strExpression);
    QgsFeatureIterator it = mLayer->getFeatures(reQuest);
    while (it.nextFeature(f))
    {
        fids << f.id();
    }

    mLayer->startEditing();
    mLayer->deleteFeatures(fids);
    mLayer->commitChanges();

    // 更新略图符号系统
    mySymbol->delSymbolItem(delList);

    return fids.size();
}

void eqiPPInteractive::delPhoto(const QStringList &photoList, const QString &tempFolder)
{
    //进度条
    QProgressDialog prDialog("删除错误相片...", "取消", 0, photoList.size(), nullptr);
    prDialog.setWindowTitle("处理进度");              //设置窗口标题
    prDialog.setWindowModality(Qt::WindowModal);      //将对话框设置为模态
    prDialog.show();
    int prCount = 0;

    foreach (QString name, photoList)
    {
        if (mPhotoMap.contains(name))
        {
            // 删除相片
            if (QFile::exists(mPhotoMap.value(name)))
            {
                if (tempFolder.isEmpty()) // 删除相片
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
                else // 移动相片
                {
                    QString oldName = mPhotoMap.value(name);
                    QString newName = tempFolder + "/" + QFileInfo(oldName).fileName();
                    if (QFile::copy(oldName, newName))
                    {
                        if (QFile::remove(oldName))
                        {
                            mPhotoMap.remove(name);
                        }
                        else
                        {
                            QgsMessageLog::logMessage(QString("\t\t||--> 删除相片 : 删除%1相片失败。").arg(name));
                        }
                    }
                    else
                    {
                        QgsMessageLog::logMessage(QString("\t\t||--> 删除相片 : 移动%1相片失败。").arg(name));
                    }
                }
            }
            else
            {
                QgsMessageLog::logMessage(QString("\t\t||--> 删除相片 : 删除%1相片失败，未在文件夹下找到。").arg(name));
            }
        }
        else
        {
//            QgsMessageLog::logMessage(QString("\t\t||--> 删除相片 : mPhotoMap err。").arg(name));
        }
        prDialog.setValue(++prCount);
        QApplication::processEvents();
        if (prDialog.wasCanceled()) return;
    }
}

QStringList eqiPPInteractive::modifyPos()
{
    QStringList willDel;

    if (!isLinked) return willDel;

    const QStringList *posList = mPosdp->noList();
    for (int i = 0; i != posList->size(); ++i)
    {
        if (!mPhotoMap.contains(posList->at(i)))
        {
            willDel << posList->at(i);
        }
    }
    return willDel;
}

QStringList eqiPPInteractive::modifyPhoto()
{
    QStringList willDel;

    if (!isLinked) return willDel;

    const QStringList *posList = mPosdp->noList();
    QMap<QString, QString>::iterator it = mPhotoMap.begin();
    while (it != mPhotoMap.end())
    {
        QString key = it.key();
        if (!posList->contains(key))
        {
            willDel << key;
        }
        ++it;
    }

    return willDel;
}

QString eqiPPInteractive::getPhotoPath(const QString &name)
{
    if (mPhotoMap.contains(name))
    {
        return mPhotoMap.value(name);
    }
    return QString();
}

void eqiPPInteractive::saveMap(const QString &savePath)
{
    // 定义略图名称
    QString sketchMapName = mSetting.value("/eqi/pos/lePosFile", "").toString();
    sketchMapName = QFileInfo(sketchMapName).baseName();
    if (sketchMapName.isEmpty())
        sketchMapName = "航摄略图";
    sketchMapName += QString("(%1)").arg(mLayer->selectedFeatureCount());

    QString newSavePath = savePath + QString("/航飞略图(%1)").arg(mLayer->selectedFeatureCount());
    if (!QDir(newSavePath).exists())
    {
        QDir dir;
        if ( !dir.mkpath(newSavePath) )
        {
            QgsMessageLog::logMessage(QString("保存航摄数据 : \t创建文件夹失败，%1略图保存失败。")
                                      .arg(QDir::toNativeSeparators(sketchMapName)));
            return;
        }
    }

    QString vectorFilename = newSavePath + "/" + sketchMapName + ".shp";
    QgsCoordinateTransform* ct = nullptr;
    QgsVectorFileWriter::WriterError error;
    QString errorMessage;
    error = QgsVectorFileWriter::writeAsVectorFormat(
                mLayer, vectorFilename, "system", ct,
                "ESRI Shapefile", true, &errorMessage );
    if ( error == QgsVectorFileWriter::NoError )
    {
        QgsMessageLog::logMessage(QString("保存航摄数据 : \t导出%1航摄略图成功。")
                                  .arg(QDir::toNativeSeparators(sketchMapName)));
    }
    else
    {
        QgsMessageLog::logMessage(QString("保存航摄数据 : \t导出%1航摄略图失败。")
                                  .arg(QDir::toNativeSeparators(sketchMapName)));
    }
}

void eqiPPInteractive::savePos(const QString &savePath, const QStringList &photoList)
{
    QString path = QString("%1/pos(%2).txt").arg(savePath).arg(photoList.size());

    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate))   //只写、文本、重写
    {
        MainWindow::instance()->messageBar()->pushMessage( "保存航摄数据",
            QString("创建%1曝光点文件失败。").arg(QDir::toNativeSeparators(path)),
            QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("保存航摄数据 : \t创建%1曝光点文件失败。")
                                  .arg(QDir::toNativeSeparators(path)));
        return;
    }

    QTextStream out(&file);

    foreach (QString noName, photoList)
    {
        QString str_xField = mPosdp->getPosRecord(noName, "xField");
        QString str_yField = mPosdp->getPosRecord(noName, "yField");
        QString str_zField = mPosdp->getPosRecord(noName, "zField");
        QString str_omegaField = mPosdp->getPosRecord(noName, "omegaField");
        QString str_phiField = mPosdp->getPosRecord(noName, "phiField");
        QString str_kappaField = mPosdp->getPosRecord(noName, "kappaField");

        if (str_xField.isEmpty() || str_yField.isEmpty() || str_zField.isEmpty() ||
            str_omegaField.isEmpty() || str_phiField.isEmpty() || str_kappaField.isEmpty())
        {
            QgsMessageLog::logMessage(QString("保存航摄数据 : \t%1POS记录提取失败，该条记录未保存。").arg(noName));
        }
        else
        {
            out << noName + '\t' + str_xField + '\t' + str_yField + '\t' + str_zField + '\t' +
                   str_omegaField + '\t' + str_phiField + '\t' + str_kappaField + '\n';
        }
    }
    file.close();
    QgsMessageLog::logMessage(QString("保存航摄数据 : \t导出%1曝光点文件成功。").arg(QDir::toNativeSeparators(path)));
}

void eqiPPInteractive::savePhoto(const QString &savePath, const QStringList &photoList)
{
    QString newPhotoPath = savePath + "/"
            + mSetting.value("/eqi/options/imagePreprocessing/lePhotoFolder", "photo").toString()
            + QString("(%1)").arg(photoList.size());

    if (!QDir(newPhotoPath).exists())
    {
        QDir dir;
        if ( !dir.mkpath(newPhotoPath) )
        {
            QgsMessageLog::logMessage(QString("保存航摄数据 : \t创建文件夹失败，相片保存失败。"));
            return;
        }
    }

    //进度条
    QProgressDialog prDialog("保存相片...", "取消", 0, photoList.size(), nullptr);
    prDialog.setWindowTitle("处理进度");              //设置窗口标题
    prDialog.setWindowModality(Qt::WindowModal);      //将对话框设置为模态
    prDialog.show();
    int prCount = 0;

    foreach (QString name, photoList)
    {
        if (mPhotoMap.contains(name))
        {
            QString oldName = mPhotoMap.value(name);
            QString newName = newPhotoPath + "/" + QFileInfo(oldName).fileName();
            bool isbl = QFile::copy(oldName, newName);
            if (!isbl)
            {
                QgsMessageLog::logMessage(QString("保存航摄数据 : \t保存相片到%1失败。").arg(newName));
            }
        }
        else
        {
            QgsMessageLog::logMessage(QString("保存航摄数据 : \t%1相片未找到原始对应记录，保存失败。").arg(name));
        }
        prDialog.setValue(++prCount);
        QApplication::processEvents();
        if (prDialog.wasCanceled()) return;
    }
    QgsMessageLog::logMessage("保存航摄数据 : \t导出相片成功。");
}

bool eqiPPInteractive::isValid()
{
    if (!mLayer || !mPosdp->noList())
    {
        return false;
    }
    if (!mLayer->isValid() || mPosdp->noList()->isEmpty())
    {
        return false;
    }
    return true;
}

bool eqiPPInteractive::isAlllinked()
{
    const QStringList *posList = mPosdp->noList();
    for (int i = 0; i != posList->size(); ++i)
    {
        if (!mPhotoMap.contains(posList->at(i)))
        {
            return false;
        }
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
