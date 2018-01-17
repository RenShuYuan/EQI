#ifndef EQISYMBOL_H
#define EQISYMBOL_H

#include <QObject>
#include <QColor>
#include <QMap>

class QgsVectorLayer;
class QgsSymbolV2;

class eqiSymbol : public QObject
{
    Q_OBJECT
public:
    enum linkedType { linked, unlinked, error, warning, SeriousSrror };

    explicit eqiSymbol(QObject *parent = nullptr);
    explicit eqiSymbol(QObject *parent, QgsVectorLayer *layer, const QString &field);

    /**
    * @brief			addChangedItem
    * @author			YuanLong
    * @Access			public
    * @Param item
    * @Param QgsSymbolV2*
    * @warning			将对应的item添加到更新列表中，并同时指定符号。
    * @Returns			void
    */
    void addChangedItem(const QString& item, QgsSymbolV2*v2);
    void addChangedItem(const QString& item, eqiSymbol::linkedType type);

    /**
    * @brief			clearAllChangedItem
    * @author			YuanLong
    * @Access			public
    * @warning			清除更新列表中所有项
    * @Returns			void
    */
    void clearAllChangedItem();

    /**
    * @brief			updata
    * @author			YuanLong
    * @Access			public
    * @warning			使用更新列表中的项和符号更新略图，
    *					全部更新完后清空列表。
    * @Returns			void
    */
    void updata();

    /**
    * @brief			delSymbolItem
    * @author			YuanLong
    * @Access			public
    * @Param items     删除的label
    * @warning			根据输入的字段属性，删除对应分类符号。
    * @Returns			void
    */
    void delSymbolItem(const QStringList& items);

private:
    // 基于指定字段初始化图层的渲染方式为分类渲染
    void initLayerCategorizedSymbolRendererV2(const QString &field);

    // 返回符号样式拷贝，定义了颜色与透明度
    QgsSymbolV2* customizeSymbolV2(eqiSymbol::linkedType type);

private:
    // 关联图层指针
    QgsVectorLayer* mLayer;

    // 用于分类的字段名称
    QString mField;

    // 保存需要更新的符号项
    QMap< QString, QgsSymbolV2* > mChangeList;

    QColor cLinked;			// 已关联的符号颜色
    QColor cUnlinked;		// 未关联符号颜色
    QColor cWarning;		// 警告符号颜色
    QColor cError;			// 错误符号颜色
    QColor cSeriousError;	// 严重错误符号颜色
};

#endif // EQISYMBOL_H
