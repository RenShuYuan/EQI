#ifndef EQIANALYSISAERIALPHOTO_H
#define EQIANALYSISAERIALPHOTO_H

#include "head.h"
#include <QObject>
#include <QMap>
#include <QList>

#include "eqi/eqisymbol.h"
#include "eqi/overlappingprocessing.h"
#include "eqi/pos/posdataprocessing.h"
#include "eqi/eqippinteractive.h"

class eqiAnalysisAerialphoto : public QObject
{
    Q_OBJECT
public:
    eqiAnalysisAerialphoto();
    eqiAnalysisAerialphoto(QObject *parent, QgsVectorLayer* layer,
                                    posDataProcessing *posdp,
                                    eqiPPInteractive *pp);

    //! 重叠度检查
    void checkoverlappingIn(QgsVectorLayer* mLayer_OverlapIn);
    void checkoverlappingBetween(QgsVectorLayer* mLayer_OverlapBetween);

    //! 删除航带内重叠度过大的相片
    QStringList delOverlapIn(QgsVectorLayer* mLayer_OverlapIn);

    //! 删除航带间重叠度过大的相片
    QStringList delOverlapBetween(QgsVectorLayer* mLayer_OverlapBetween);

    //! 倾角检查、删除
    void checkOmega(QgsVectorLayer* mLayer_Omega);

    //! 旋片角检查
    void checkKappa(QgsVectorLayer* mLayer_Kappa);

    //! 删除超限倾角、旋偏角，当isEdge为true时位于边缘处的
    //! type="Omega" OR "Kappa"
    QStringList delOmegaAndKappa(QgsVectorLayer* layer, const QString& type, const bool isEdge = true);
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
};

#endif // EQIANALYSISAERIALPHOTO_H
