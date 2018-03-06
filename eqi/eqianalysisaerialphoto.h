#ifndef EQIANALYSISAERIALPHOTO_H
#define EQIANALYSISAERIALPHOTO_H

#include <QObject>
#include <QMap>
#include <QSettings>
#include <QList>
#include <QStringList>

#include "eqi/eqisymbol.h"
#include "qgsvectorlayer.h"

class posDataProcessing;

// 重叠度处理
class OverlappingProcessing
{
public:
    OverlappingProcessing();
    virtual ~OverlappingProcessing();

    void addLineNumber(QString &photoNumber, int lineNumber);
    void setQgsFeature(const QString& photoNumber, QgsFeature* f);
    void setNextPhotoOl(const QString& currentNumber, QString nextNumber, const int n);
    void setNextLineOl(const QString& currentNumber, QString photoNumber, const int n);

    // 返回指定相片所在的航线号
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
    int calculateOverlap(const QString &currentNo, const QString &nextNo, const bool isSideways);

    // 返回指定相片与下张相片的重叠统计
    QMap<QString, int> getNextPhotoOverlapping(const QString &number);

    // 返回指定相片与下一航带相片的重叠统计
    QMap<QString, int> getNextLineOverlapping(const QString &number);
private:
    struct Ol
    {
        int mLineNumber; // 航线编号
        QgsFeature* mF;  // 图形要素
        QMap<QString, int> mMapNextPhoto; // 航带内相邻相片
        QMap<QString, int> mMapNextLine;  // 航带间有重叠度关系相片

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
};


class eqiAnalysisAerialphoto : public QObject
{
    Q_OBJECT
public:
    explicit eqiAnalysisAerialphoto(QObject *parent, QgsVectorLayer* layer, posDataProcessing *posdp);

    //! 重叠度检查
    void checkOverlapping();

    //! 倾角检查
    void checkOmega();

    //! 旋片角检查
    void checkKappa();

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
                                  , const QString& overlappingVelue
                                  , const QString& errType
                                  , eqiSymbol *mySymbol
                                  , eqiSymbol::linkedType type);

private:
    QSettings mSetting;
    QgsVectorLayer* mLayerMap;
    QgsVectorLayer* mLayer_Omega;
    QgsVectorLayer* mLayer_Kappa;
    QgsVectorLayer* mLayer_Overlapping;
    posDataProcessing* mPosdp;

    bool isGroup;
    OverlappingProcessing myOlp;
};

#endif // EQIANALYSISAERIALPHOTO_H
