#include "eqisymbol.h"
#include "mainwindow.h"

#include "qgsvectorlayer.h"
#include "qgssymbolv2.h"
#include "qgslayertreeview.h"
#include "qgslayertreenode.h"
#include "qgssinglesymbolrendererv2.h"
#include "qgscategorizedsymbolrendererv2.h"
#include "qgsfillsymbollayerv2.h"

#include "qgsmessagelog.h"  //

eqiSymbol::eqiSymbol(QObject *parent) :
    QObject(parent),
    mLayer(nullptr)
{
    cLinked.setRgb(186, 221, 105);
    cUnlinked = Qt::gray;
    cWarning = Qt::yellow;
    cError = Qt::darkMagenta;
    cSeriousError = Qt::red;
}

eqiSymbol::eqiSymbol(QObject *parent, QgsVectorLayer *layer, const QString &field) :
    QObject(parent),
    mLayer(layer)
{
    mField = field;

    cLinked.setRgb(186, 221, 105);
    cUnlinked = Qt::gray;
    cWarning = Qt::yellow;
    cError = Qt::darkMagenta;
    cSeriousError = Qt::red;
}

void eqiSymbol::addChangedItem(const QString &item, QgsSymbolV2 *v2)
{
    mChangeList[item] = v2;
}

void eqiSymbol::addChangedItem(const QString &item, eqiSymbol::linkedType type)
{
    addChangedItem(item, customizeSymbolV2(type));
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

void eqiSymbol::delAllSymbolItem()
{
    // 获得缺省的符号
    QgsSymbolV2* mSymbolV2 = nullptr;
    mSymbolV2 = QgsSymbolV2::defaultSymbol(mLayer->geometryType());
    QgsSingleSymbolRendererV2* rv2 = new QgsSingleSymbolRendererV2( mSymbolV2 );
    mLayer->setRendererV2(rv2);

    QgsLayerTreeNode* currentNode =	MainWindow::instance()->layerTreeView()->currentNode();
    bool isExpanded = currentNode->isExpanded();
    MainWindow::instance()->layerTreeView()->refreshLayerSymbology(mLayer->id());
    currentNode->setExpanded(isExpanded);
}

void eqiSymbol::initLayerCategorizedSymbolRendererV2(const QString &field)
{
    QStringList list;
    QgsCategoryList cats;
    QgsFeature f;
    QgsFeatureIterator it = mLayer->getFeatures();

    while (it.nextFeature(f))
    {
        if (!list.contains(f.attribute(field).toString()))
        {
            list << f.attribute(field).toString();
        }
    }

    foreach (QString value, list)
    {
        cats << QgsRendererCategoryV2(QVariant(value), customizeSymbolV2(eqiSymbol::unlinked), value);
    }

    mLayer->setRendererV2( new QgsCategorizedSymbolRendererV2(field, cats) );
    MainWindow::instance()->layerTreeView()->refreshLayerSymbology(mLayer->id());
}

QgsSymbolV2 *eqiSymbol::customizeSymbolV2(eqiSymbol::linkedType type)
{
    // 获得缺省的符号
    QgsSymbolV2* mSymbolV2 = nullptr;
    mSymbolV2 = QgsSymbolV2::defaultSymbol(mLayer->geometryType());

    // 设置透明度与颜色
    mSymbolV2->setAlpha(0.5);

    if (type == eqiSymbol::linked)
        mSymbolV2->setColor(cLinked);
    else if (type == eqiSymbol::unlinked)
        mSymbolV2->setColor(cUnlinked);
    else if (type == eqiSymbol::warning)
        mSymbolV2->setColor(cWarning);
    else if (type == eqiSymbol::error)
        mSymbolV2->setColor(cError);
    else if (type == eqiSymbol::SeriousSrror)
        mSymbolV2->setColor(cSeriousError);

    return mSymbolV2->clone();
}
