#include "eqisymbol.h"
#include "mainwindow.h"

#include "qgsvectorlayer.h"
#include "qgssymbolv2.h"
#include "qgslayertreeview.h"
#include "qgslayertreenode.h"
#include "qgssinglesymbolrendererv2.h"
#include "qgscategorizedsymbolrendererv2.h"
#include "qgsfillsymbollayerv2.h"

eqiSymbol::eqiSymbol(QObject *parent) :
    QObject(parent),
    mLayer(nullptr),
    mLinkedSymbolV2(nullptr),
    mUnlinkedSymbolV2(nullptr),
    mWarningSymbolV2(nullptr),
    mErrorSymbolV2(nullptr)
{
    cLinked.setRgb(186, 221, 105);
    cUnlinked = Qt::gray;
    cError = Qt::red;
    cWarning = Qt::yellow;
}

eqiSymbol::eqiSymbol(QObject *parent, QgsVectorLayer *layer, const QString &field) :
    QObject(parent),
    mLayer(layer),
    mLinkedSymbolV2(nullptr),
    mUnlinkedSymbolV2(nullptr),
    mWarningSymbolV2(nullptr),
    mErrorSymbolV2(nullptr)
{
    mField = field;

    cLinked.setRgb(186, 221, 105);
    cUnlinked = Qt::gray;
    cError = Qt::red;
    cWarning = Qt::yellow;
}

void eqiSymbol::addChangedItem(const QString &item, QgsSymbolV2 *v2)
{
    mChangeList[item] = v2;
}

void eqiSymbol::addChangedItem(const QString &item, eqiSymbol::linkedType type)
{
    if (type == eqiSymbol::linked)
    {
        addChangedItem(item, linkedSymbolV2());
    }
    else if (type == eqiSymbol::unlinked)
    {
        addChangedItem(item, unlinkedSymbolV2());
    }
    else if (type == eqiSymbol::warning)
    {
        addChangedItem(item, warningSymbolV2());
    }
    else if (type == eqiSymbol::error)
    {
        addChangedItem(item, errorSymbolV2());
    }
}

void eqiSymbol::clearAllChangedItem()
{
    mChangeList.clear();
}

void eqiSymbol::updata()
{
    // 获得目前图层的分类样式符号渲染器
    QgsCategorizedSymbolRendererV2* cRenderer;
    cRenderer = dynamic_cast< QgsCategorizedSymbolRendererV2* >( mLayer->rendererV2() );
    if (!cRenderer)
    {
        initLayerCategorizedSymbolRendererV2(mField);
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

void eqiSymbol::delSymbolItem(const QStringList &items)
{
    QgsCategorizedSymbolRendererV2* cRenderer;
    cRenderer = dynamic_cast< QgsCategorizedSymbolRendererV2* >( mLayer->rendererV2() );
    if (!cRenderer)
    {
        return;
    }

    foreach (QString item, items)
    {
        int index = cRenderer->categoryIndexForValue(item);
        if (index != -1)
        {
            cRenderer->deleteCategory(index);
        }
    }

    QgsLayerTreeNode* currentNode =	MainWindow::instance()->layerTreeView()->currentNode();
    bool isExpanded = currentNode->isExpanded();

    MainWindow::instance()->layerTreeView()->refreshLayerSymbology(mLayer->id());

    currentNode->setExpanded(isExpanded);
}

void eqiSymbol::initLayerCategorizedSymbolRendererV2(const QString &field)
{
    QgsCategoryList cats;
    QgsFeature f;
    QgsFeatureIterator it = mLayer->getFeatures();
    while (it.nextFeature(f))
    {
        cats << QgsRendererCategoryV2(f.attribute(field),
                                      unlinkedSymbolV2(),
                                      f.attribute(field).toString());
    }

    mLayer->setRendererV2( new QgsCategorizedSymbolRendererV2(field, cats) );
    MainWindow::instance()->layerTreeView()->refreshLayerSymbology(mLayer->id());
}

QgsSymbolV2 *eqiSymbol::linkedSymbolV2()
{
    // 获得缺省的符号
    QgsSymbolV2* mSymbolV2 = nullptr;
    mSymbolV2 = QgsSymbolV2::defaultSymbol(mLayer->geometryType());

    // 设置透明度与颜色
    mSymbolV2->setAlpha(0.5);
    mSymbolV2->setColor(cLinked);

    return mSymbolV2->clone();
}

QgsSymbolV2 *eqiSymbol::unlinkedSymbolV2()
{
    // 获得缺省的符号
    QgsSymbolV2* mSymbolV2 = nullptr;
    mSymbolV2 = QgsSymbolV2::defaultSymbol(mLayer->geometryType());

    // 设置透明度与颜色
    mSymbolV2->setAlpha(0.5);
    mSymbolV2->setColor(cUnlinked);

    return mSymbolV2->clone();
}

QgsSymbolV2 *eqiSymbol::warningSymbolV2()
{
    // 获得缺省的符号
    QgsSymbolV2* mSymbolV2 = nullptr;
    mSymbolV2 = QgsSymbolV2::defaultSymbol(mLayer->geometryType());

    // 设置透明度与颜色
    mSymbolV2->setAlpha(0.5);
    mSymbolV2->setColor(cWarning);

    return mSymbolV2->clone();
}

QgsSymbolV2 *eqiSymbol::errorSymbolV2()
{
    // 获得缺省的符号
    QgsSymbolV2* mSymbolV2 = nullptr;
    mSymbolV2 = QgsSymbolV2::defaultSymbol(mLayer->geometryType());

    // 设置透明度与颜色
    mSymbolV2->setAlpha(0.5);
    mSymbolV2->setColor(cError);

    return mSymbolV2->clone();
}
