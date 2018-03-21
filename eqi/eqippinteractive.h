#ifndef EQIPPINTERACTIVE_H
#define EQIPPINTERACTIVE_H

#include <QObject>
#include <QSettings>
#include <QColor>
#include <QMap>

class eqiSymbol;
class QgsVectorLayer;
class posDataProcessing;

/*
 *负责曝光点与相片的所有互交细节
 **/
class eqiPPInteractive : public QObject
{
    Q_OBJECT
public:
    enum linkedType { linked, unlinked, error, warning };

    explicit eqiPPInteractive(QObject *parent, QgsVectorLayer* layer, posDataProcessing *posdp);
    ~eqiPPInteractive();

    /**
    * @brief            检查是否满足联动条件
    * @author           YuanLong
    * @warning          检查航飞略图与曝光点是否都有效.
    * @return			有效则返回true
    */
    bool isValid();

    /**
    * @brief            检查是否已创建联动关系
    * @author           YuanLong
    * @return			有效则返回true
    */
    bool islinked(){ return isLinked; };

    /**
    * @brief            检查POS与photo是否已全部对应
    * @author           YuanLong
    * @return			有效则返回true
    */
    bool isAlllinked();

    /**
    * @brief            设置航飞略图
    * @author           YuanLong
    * @param layer 		指向有效航飞略图的QgsVectorLayer
    * @return
    */
    void setVectorLayer(QgsVectorLayer* layer);

    /**
    * @brief            赋值曝光点相片名称列表的指针
    * @author           YuanLong
    * @param fieldsList 指向有效的曝光点文件名称列表
    * @return
    */
    //void setNoFieldsList(QStringList* fieldsList);

    /**
    * @brief            更新曝光点与相片已关联的符号
    * @author           YuanLong
    * @warning			POS需要提前使用对应函数设置好，Photo文件
    *					路径将自动从设置中读取，创建成功后将会调用
    *					upDataLinkedSymbol()更新QgsVectorLayer符号
    *					，并更新isLinked。
    *					使用图层的分类样式符号渲染器更新已关联符号
    * @return
    */
    void upDataLinkedSymbol();

    /**
    * @brief            断开POS与Photo联动关系
    * @author           YuanLong
    * @warning			使用图层的分类样式符号渲染器更新未关联符号
    * @return
    */
    void upDataUnLinkedSymbol();

    /**
    * @brief            匹配曝光点名称
    * @author           YuanLong
    * @warning			检查当曝光点名称与相片名称不一致时，使用默认
    *					的匹配规则将曝光点名称与相片名称修改为一致
    * @return			当成功匹配则返回true
    */
//    void matchPosName();

    /**
    * @brief            保存所选航摄数据
    * @author           YuanLong
    * @param savePath  保存相片的文件夹路径
    * @warning          将选中的相片、曝光点数据、以及略图另存为。
    * @return
    */
    void saveSelect(const QString& savePath);

    int delMap(const QStringList& delList);

    /**
    * @brief                删除所选相片
    * @author               YuanLong
    * @param tempFolder    字符串为空则直接删除相片，
    *                       否则将相片移动到指定文件夹中。
    * @warning
    * @return
    */
    void delPhoto(const QStringList& photoList, const QString& tempFolder);
signals:
    /**
    * @brief            向主窗口发送信号更新繁忙进度条状态
    * @author           YuanLong
    * @return
    */
    void startProcess();
    void stopProcess();

    void delPos(QStringList& photoList);

private:
    int delDirectMap(const QStringList& delList);

    void saveMap(const QString& savePath);
    void savePos(const QString& savePath, const QStringList &photoList);
    void savePhoto(const QString& savePath, const QStringList &photoList);

private:
    QSettings mSetting;
    eqiSymbol *mySymbol;
    QgsVectorLayer* mLayer;
    QgsVectorLayer* mPosLayer;
    posDataProcessing* mPosdp;
    QString fieldName;

    // 是否联动成功
    bool isLinked;

    // 相片列表,保存文件baseName与完整路径
    QMap<QString, QString> mPhotoMap;

    QStringList photosList;		// 用于保存搜索到的相片路径
};

#endif // EQIPPINTERACTIVE_H
