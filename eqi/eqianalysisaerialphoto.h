#ifndef EQIANALYSISAERIALPHOTO_H
#define EQIANALYSISAERIALPHOTO_H

#include <QObject>
#include <QMap>
#include <QSettings>
#include <QList>
#include <QStringList>

#include "eqi/eqisymbol.h"
#include "eqi/gdal/eqigdalprogresstools.h"
#include "qgsvectorlayer.h"

#include <vector>
using std::vector;

class posDataProcessing;
class eqiPPInteractive;

// 重叠度处理
class OverlappingProcessing
{
public:
    OverlappingProcessing();
    virtual ~OverlappingProcessing();

    void addLineNumber(QString &photoNumber, int lineNumber);
    void setQgsFeature(const QString& photoNumber, QgsFeature* f);

    // 设置当前相片与下张相片重叠度比例
    void setNextPhotoOl(const QString& currentNumber, QString nextNumber, const int n);

    // 设置当前相片与下张相片连接点数量
    void setNextPhotoKeys(const QString& currentNumber, QString nextNumber, const int n);

    // 设置当前相片与下一航带相片重叠度比例
    void setNextLineOl(const QString& currentNumber, QString photoNumber, const int n);

    // 设置当前相片与下一航带相片连接点数量
    void setNextLineKeys(const QString& currentNumber, QString nextNumber, const int n);

    // 返回指定相片所在的航线号（从1开始)
    int getLineNumber(const QString &photoNumber);

    // 返回总相片数量
    int getPhotoSize(){ return map.size(); }

    // 返回航线数量
    int getLineSize();

    // 返回指定航线上所有相片
    void getLinePhoto(const int number, QStringList &list);

    // 返回指定相片的几何要素
    QgsFeature *getFeature(const QString &photoNumber);

    // 返回所有相片号
    QList< QString > getAllPhotoNo();

    // 计算2张相片的重叠度，同时保存到map中。指定是否是旁向重叠比较
    int calculateOverlap(const QString &currentNo, const QString &nextNo);

    // 基于SiftGPU计算两张相片连接点数量
    int calculateOverlapSiftGPU(const QString &currentNo, const QString &nextNo);

    // 返回指定相片与下张相片的重叠统计
    QMap<QString, int> getNextPhotoOverlapping(const QString &number);

    // 返回指定相片与下张相片的连接点统计
    QMap<QString, int> getNextPhotoKeys(const QString &number);

    // 返回指定相片与下一航带相片的重叠统计
    QMap<QString, int> getNextLineOverlapping(const QString &number);

    // 返回指定相片与下张相片的连接点统计
    QMap<QString, int> getNextLineKeys(const QString &number);

    // 删除指定相片
    void delItem(const QString &photoNumber);


    // 更新航线编号，如删除航带间相片后
    void updataLineNumber();

    // 更新保存的相片编号，如删除相片后
    void updataPhotoNumber();
private:
    struct Ol
    {
        int mLineNumber;                        // 航线编号
        QgsFeature* mF;                         // 图形要素
        QMap<QString, int> mMapNextPhoto;       // 航带内相邻相片
        QMap<QString, int> mMapNextLine;        // 航带间有重叠度关系相片
        QMap<QString, int> mMapNextPhotoKeys;   // 航带内有重叠度关系特征点匹配数
        QMap<QString, int> mMapNextLineKeys;   // 航带间有重叠度关系特征点匹配数

        Ol(){mLineNumber = -1; mF = nullptr;}
        Ol(int lineNumber){mLineNumber = lineNumber; mF = nullptr;}
        ~Ol()
        {
            if (mF)
            {
                delete mF;
                mF = nullptr;
            }
        }
    };

    QMap<QString, Ol*> map;
    eqiGdalProgressTools gdalTools;
    QSettings mSetting;

    // 用于保存每张相片匹配到的点，避免重复匹配
    QMap< QString, vector<float> > mapKeyDescriptors;
};


class eqiAnalysisAerialphoto : public QObject
{
    Q_OBJECT
public:
    explicit eqiAnalysisAerialphoto(QObject *parent);
    explicit eqiAnalysisAerialphoto(QObject *parent, QgsVectorLayer* layer,
                                    posDataProcessing *posdp,
                                    eqiPPInteractive *pp);

    //! 重叠度检查
    void checkoverlappingIn();
    void checkoverlappingBetween();

    //! 删除重叠度过大的相片
    QStringList delOverlapping();

    //! 倾角检查、删除
    void checkOmega();

    //! 旋片角检查
    void checkKappa();

    //! 删除超限倾角、旋偏角，当isEdge为true时位于边缘处的
    //! type="Omega" OR "Kappa"
    QStringList delOmegaAndKappa(const QString& type, const bool isEdge = true);
public slots:
    void updataChackValue();

private:
    //! 航带分组
    void airLineGroup();

    //! 根据筛选结果提取要素，重新赋属性并设置符号
    void extractFeature(QgsFeatureList& featureList
                        , QString &strExpression
                        , const int index
                        , const QString& field
                        , const QString& errType
                        , eqiSymbol *mySymbol
                        , eqiSymbol::linkedType type);

    //! 根据重叠度比较结果，添加错误面并设置符号
    void addOverLappingErrFeature(QgsFeatureList& featureList
                                  , QgsGeometry *errGeometry
                                  , const QString& number
                                  , const int index
                                  , const QString& overlappingValue
                                  , const QString &keysValue, const QString& errType
                                  , eqiSymbol *mySymbol
                                  , eqiSymbol::linkedType type);
    void addOverLappingErrFeature(QgsFeatureList& featureList
                                  , QgsGeometry errGeometry
                                  , const QString& number
                                  , const int index
                                  , const QString& overlappingValue
                                  , const QString &keysValue, const QString& errType
                                  , eqiSymbol *mySymbol
                                  , eqiSymbol::linkedType type);

    bool isCheckLineLast(const QString& name);
private:
    QSettings mSetting;
    QgsVectorLayer* mLayerMap;
    eqiPPInteractive* mPp;
    QgsVectorLayer* mLayer_Omega;
    QgsVectorLayer* mLayer_Kappa;
    QgsVectorLayer* mLayer_OverlapIn;
    QgsVectorLayer* mLayer_OverlapBetween;
    posDataProcessing* mPosdp;

    // 重叠度限差
    int heading_Max;
    int heading_Gen;
    int heading_Min;
    int sideways_Max;
    int sideways_Gen;
    int sideways_Min;

    // 倾斜角度限差
    double omega_angle_General;
    double omega_angle_Max;
    double omega_angle_Average;

    // Kappa角度限差
    double kappa_angle_General;
    double kappa_angle_Max;
    double kappa_angle_Line;

    // 错误类型定义
    QString errTpye_nextPhotoNoOverlap;
    QString errTpye_nextPhotoMinOverlap;
    QString errTpye_nextPhotoGeneralOverlap;
    QString errTpye_nextPhotoMaxOverlap;
    QString errTpye_NoKeys;
    QString errTpye_nextPhotoKeysThreshold;
    QString errTpye_nextLineNoOverlap;
    QString errTpye_nextLineMinOverlap;
    QString errTpye_nextLineGeneralOverlap;
    QString errTpye_nextLineMaxOverlap;

    bool isGroup;
    OverlappingProcessing myOlp;

    // 用于分类的字段名称
    QString mField;

    // 相邻相片最小连接点阈值
    static const int keyThreshold = 50;
};

#endif // EQIANALYSISAERIALPHOTO_H
