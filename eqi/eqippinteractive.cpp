#include "eqippinteractive.h"
#include "mainwindow.h"
#include "eqi/eqiapplication.h"

#include "qgsmapcanvas.h"
#include "qgsmessagelog.h"
#include "qgslayertreeview.h"
#include "qgsvectorlayer.h"
#include "qgssymbolv2.h"
#include "qgssinglesymbolrendererv2.h"
#include "qgscategorizedsymbolrendererv2.h"
#include "qgsfillsymbollayerv2.h"

eqiPPInteractive::eqiPPInteractive(QObject *parent)
    : QObject(parent),
    mLayer(nullptr),
    mNoFields(nullptr),
    mLinkedSymbolV2(nullptr),
    mUnlinkedSymbolV2(nullptr),
    mWarningSymbolV2(nullptr),
    mErrorSymbolV2(nullptr)
{
    this->parent = parent;
    isLinked = false;

    cLinked.setRgb(186, 221, 105);
    cUnlinked = Qt::gray;
    cError = Qt::red;
    cWarning = Qt::yellow;
}

eqiPPInteractive::eqiPPInteractive(QObject *parent, QgsVectorLayer* layer, QStringList* noFields)
    : QObject(parent),
    mLayer(layer),
    mNoFields(noFields),
    mLinkedSymbolV2(nullptr),
    mUnlinkedSymbolV2(nullptr),
    mWarningSymbolV2(nullptr),
    mErrorSymbolV2(nullptr)
{
    this->parent = parent;
    isLinked = false;

    cLinked.setRgb(186, 221, 105);
    cUnlinked = Qt::gray;
    cError = Qt::red;
    cWarning = Qt::yellow;
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
            addChangedItem(key, linkedSymbolV2());
            ++it;
        }
    }

    isLinked = true;
    updata();
}

void eqiPPInteractive::initLayerCategorizedSymbolRendererV2()
{
    QgsCategoryList cats;
    QgsFeature f;
    QgsFeatureIterator it = mLayer->getFeatures();
    while (it.nextFeature(f))
    {
        cats << QgsRendererCategoryV2(f.attribute("相片编号"),
                                      unlinkedSymbolV2(),
                                      f.attribute("相片编号").toString());
    }

    mLayer->setRendererV2( new QgsCategorizedSymbolRendererV2("相片编号", cats) );
    MainWindow::instance()->layerTreeView()->refreshLayerSymbology(mLayer->id());
}

void eqiPPInteractive::upDataUnLinkedSymbol()
{
    QMap<QString, QString>::iterator it = mPhotoMap.begin();
    while (it != mPhotoMap.end())
    {
        addChangedItem(it.key(), unlinkedSymbolV2());
        ++it;
    }

    mPhotoMap.clear();
    isLinked = false;

    updata();
}

QgsSymbolV2* eqiPPInteractive::linkedSymbolV2()
{
    // 获得缺省的符号
    QgsSymbolV2* mSymbolV2 = nullptr;
    mSymbolV2 = QgsSymbolV2::defaultSymbol(mLayer->geometryType());

    // 设置透明度与颜色
    mSymbolV2->setAlpha(0.5);
    mSymbolV2->setColor(cLinked);

    return mSymbolV2->clone();
}

QgsSymbolV2* eqiPPInteractive::unlinkedSymbolV2()
{
    // 获得缺省的符号
    QgsSymbolV2* mSymbolV2 = nullptr;
    mSymbolV2 = QgsSymbolV2::defaultSymbol(mLayer->geometryType());

    // 设置透明度与颜色
    mSymbolV2->setAlpha(0.5);
    mSymbolV2->setColor(cUnlinked);

    return mSymbolV2->clone();
}

QgsSymbolV2* eqiPPInteractive::warningSymbolV2()
{
    // 获得缺省的符号
    QgsSymbolV2* mSymbolV2 = nullptr;
    mSymbolV2 = QgsSymbolV2::defaultSymbol(mLayer->geometryType());

    // 设置透明度与颜色
    mSymbolV2->setAlpha(0.5);
    mSymbolV2->setColor(cWarning);

    return mSymbolV2->clone();
}

QgsSymbolV2* eqiPPInteractive::errorSymbolV2()
{
    // 获得缺省的符号
    QgsSymbolV2* mSymbolV2 = nullptr;
    mSymbolV2 = QgsSymbolV2::defaultSymbol(mLayer->geometryType());

    // 设置透明度与颜色
    mSymbolV2->setAlpha(0.5);
    mSymbolV2->setColor(cError);

    return mSymbolV2->clone();
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

void eqiPPInteractive::matchPosName()
{
    QString photoName;
    QStringList* tmpPosList;
    QStringList photoNameList;
    foreach (QString photoPath, photosList)
        photoNameList.append(QFileInfo(photoPath).baseName());
    if (photoNameList.isEmpty())
        return;

    tmpPosList = mNoFields;

    QgsMessageLog::logMessage(QString("自动匹配名称 : \t开始匹配曝光点与相片名称..."));

    for (int i=0; i<tmpPosList->size(); ++i)
    {
        int count = 0;
        QString posName = tmpPosList->at(i);
        foreach (const QString tmpPhotoName, photoNameList)
        {
            if (tmpPhotoName.contains(posName, Qt::CaseInsensitive))
            {
                photoName = tmpPhotoName;
                ++count;
            }
        }
        if (count==0)	// 没匹配到
        {
            int i = 0;
            while (i<posName.size())
            {
                if (posName.at(i).isNumber())
                {
                    if (posName.at(i)==QChar('0'))
                    {
                        posName.remove(i, 1);
                    }
                    else
                        break;
                }
                ++i;
            }
            (*tmpPosList)[i] = posName;
            --i;
        }
        else if (count==1) // 匹配到一个
        {
            QgsMessageLog::logMessage(QString("\t\t匹配到曝光点: %1与相片名称: %2符合规则, 已自动更新曝光点名称.")
                                      .arg(mNoFields->at(i)).arg(photoName));
            (*mNoFields)[i] = photoName;
        }
        else				// 匹配到多个
        {
            if (posName.size() < photoName.size())
            {
                (*tmpPosList)[i] = posName.insert(0, '0');
                --i;
            }
            else
                QgsMessageLog::logMessage(QString("\t\t未匹配到曝光点: %1.").arg(mNoFields->at(i)));
        }
    }

    //
}

void eqiPPInteractive::addChangedItem( const QString& item, QgsSymbolV2* v2 )
{
    mChangeList[item] = v2;
}

void eqiPPInteractive::addChangedItem(const QString& item, eqiPPInteractive::linkedType type)
{
    if (type == eqiPPInteractive::linked)
    {
        addChangedItem(item, linkedSymbolV2());
    }
    else if (type == eqiPPInteractive::unlinked)
    {
        addChangedItem(item, unlinkedSymbolV2());
    }
    else if (type == eqiPPInteractive::warning)
    {
        addChangedItem(item, warningSymbolV2());
    }
    else if (type == eqiPPInteractive::error)
    {
        addChangedItem(item, errorSymbolV2());
    }
}

void eqiPPInteractive::clearAllChangedItem()
{
    mChangeList.clear();
}

void eqiPPInteractive::updata()
{
    // 获得目前图层的分类样式符号渲染器
    QgsCategorizedSymbolRendererV2* cRenderer;
    cRenderer = dynamic_cast< QgsCategorizedSymbolRendererV2* >( mLayer->rendererV2() );
    if (!cRenderer)
    {
        initLayerCategorizedSymbolRendererV2();
        cRenderer = dynamic_cast< QgsCategorizedSymbolRendererV2* >( mLayer->rendererV2() );
    }

    QMap< QString, QgsSymbolV2* >::iterator it = mChangeList.begin();
    while (it != mChangeList.end())
    {
        int index = cRenderer->categoryIndexForValue(QVariant(it.key()));
        if (index != -1)
        {
            cRenderer->updateCategorySymbol(index, it.value());
        }
        ++it;
    }

    MainWindow::instance()->layerTreeView()->refreshLayerSymbology(mLayer->id());
    MainWindow::instance()->refreshMapCanvas();
    clearAllChangedItem();
}

void eqiPPInteractive::testSwitch()
{
    QgsMessageLog::logMessage("1111");

    QgsSymbolV2* mSymbolV2 = nullptr;
    mSymbolV2 = QgsSymbolV2::defaultSymbol(mLayer->geometryType());
    mSymbolV2->changeSymbolLayer(0,QgsCentroidFillSymbolLayerV2::create());

    QgsCategorizedSymbolRendererV2* cRenderer;
    cRenderer = dynamic_cast< QgsCategorizedSymbolRendererV2* >( mLayer->rendererV2() );
    cRenderer->updateSymbols(mSymbolV2->clone());
    MainWindow::instance()->layerTreeView()->refreshLayerSymbology(mLayer->id());
    MainWindow::instance()->refreshMapCanvas();
}
