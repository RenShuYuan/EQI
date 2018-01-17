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
//class OverlappingProcessing;


// 重叠度处理
class OverlappingProcessing
{
public:
    OverlappingProcessing();
    virtual ~OverlappingProcessing();

    void addLineNumber(const QString &photoNumber, int lineNumber);
    void setQgsFeature(const QString& photoNumber, QgsFeature* f);
    void setNextPhotoOl(const QString& photoNumber, const int n);
    void setNextLineOl(const QString& photoNumber, const int n);

    int getLineNumber(const QString &photoNumber);

    // 返回相片数量
    int getPhotoSize(){ map.size(); }

    // 返回行带数
    int getLineSize();

    // 返回指定航线相片数量
    int getLinePhotoSize(const int number);
private:
    struct Ol
    {
        int mLineNumber; // 航线编号
        QgsFeature* mF;  // 图形要素
        QMap<QString*, int> mMapNextPhoto; // 航带内相邻相片
        QMap<QString*, int> mMapNextLine;  // 航带间有重叠度关系相片

        Ol(){mLineNumber = -1; mF = nullptr;}
        Ol(int lineNumber){mLineNumber = lineNumber; mF = nullptr;}
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

private:
    QSettings mSetting;
    QgsVectorLayer* mLayerMap;
    QgsVectorLayer* mLayer_Omega;
    QgsVectorLayer* mLayer_Kappa;
    posDataProcessing* mPosdp;

    bool isGroup;
    QList< QStringList > mAirLineGroup;	// 航带分组

    OverlappingProcessing myOlp;
};

#endif // EQIANALYSISAERIALPHOTO_H
