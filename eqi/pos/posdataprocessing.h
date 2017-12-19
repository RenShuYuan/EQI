#ifndef POSDATAPROCESSING_H
#define POSDATAPROCESSING_H

#include <QObject>
#include <QMap>
#include <QSettings>
#include <QRegExp>
#include "qgscoordinatereferencesystem.h"
#include "eqi/eqiProjectionTransformation.h"
#include "qgsgeometry.h"

class QgsVectorLayer;
class QgsDelimitedTextFile;

// 负责处理与曝光点数据相关的类
class posDataProcessing : public QObject
{
    Q_OBJECT
public:
    static QRegExp WktPrefixRegexp;
    static QRegExp CrdDmsRegexp;

    explicit posDataProcessing(QObject *parent = nullptr);
    ~posDataProcessing();

    /**
    * @brief                    返回此类是否有效
    * @author                   YuanLong
    * @warning					判断mFieldsList是否为空，该成员变量是
    *							后续处理的基础
    * @return					有效则返回true
    */
    bool isValid();

    /**
    * @brief                    检查POS相关参数是否正确
    * @author                   YuanLong
    * @return					列表为空表示全部正确，不正确的参数将会返回
    *							对应的名称
    */
    const QStringList checkPosSettings();

    /**
    * @brief                    返回Pos相片编号
    * @author                   YuanLong
    * @return					返回一个指向相片编号的指针
    */
    QStringList* noList();


    /**
    * @brief			getPosRecord		查询指定记录值
    * @author			YuanLong
    * @Access			public
    * @Param const QString & keyField		查询的相片编号
    * @Param const QString & valueField		值字段名称
    * @warning
    * @Returns			QT_NAMESPACE::QString	返回查询字段值
    */
    QString getPosRecord(const QString& noValue, const QString& valueField);


    /**
    * @brief			setPosRecord		修改指定记录值
    * @author			YuanLong
    * @Access			public
    * @Param const QString & keyField		修改的相片编号
    * @Param const QString & valueField		修改值字段名称
    * @Param const QString & value			修改的值
    * @warning
    * @Returns			void
    */
//    void setPosRecord(const QString& keyField, const QString& valueField, const QString& value);

    /**
    * @brief                    删除内存中曝光点记录
    * @author                   YuanLong
    * @param No 				要删除的相片编号
    * @return
    */
    void deletePosRecord( const QString& No );

    // POS坐标转换
    bool autoPosTransform();

    // 创建略图
    QgsVectorLayer* autoSketchMap();

    // 导出Pos文件
    bool posExport();

    // 返回无效记录数量
    int getInvalidLineSize();

signals:
    void startProcess();
    void stopProcess();

private slots:
    // 读取POS文件并设置字段列表
    void readFieldsList(QString &);

private:
    // 计算中央经度, 以多为准
    int getCentralMeridian();

    // 获得曝光点的相对行高
    QStringList getPosElev();

    // 利用曝光点中心坐标与绝对高程计算相片4个角点的坐标，并构成面要素
    QgsPolygon rectangle( const QgsPoint& point, const double& resolution );

    // 计算分辨率
    double calculateResolution(const double &absoluteHeight, const double &groundHeight);

    // 度分秒转度
    bool dFromDms( QString &sDms, bool xyDms );
    static double dmsStringToDouble( const QString &sX, bool *xOk );

    // 记录错误
    void recordInvalidLine( const QString& message );
    static bool recordIsEmpty( QStringList &record );
    void reportErrors( const QStringList& messages = QStringList(), bool showDialog = false );
    void clearInvalidLines();

private:
    QObject *parent;
    QSettings mSettings;
    QMap< QString, QStringList > mFieldsList;
    QStringList descriptionList;
    QStringList descriptionUserList;
    QStringList resolutionList;

    QgsDelimitedTextFile *mFile;

    QgsCoordinateReferenceSystem mSourceCrs;
    QgsCoordinateReferenceSystem mTargetCrs;
    eqiProjectionTransformation eqiPrj;

    bool mXyDms;

    //! 存储无法加载文件中的行
    int mMaxInvalidLines;
    int mNExtraInvalidLines;
    QStringList mInvalidLines;
};

#endif // POSDATAPROCESSING_H
