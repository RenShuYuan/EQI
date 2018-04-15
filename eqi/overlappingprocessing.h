#ifndef OVERLAPPINGPROCESSING_H
#define OVERLAPPINGPROCESSING_H

#include "head.h"
#include "eqi/gdal/eqigdalprogresstools.h"

#include <vector>
using std::vector;

class OverlappingProcessing
{
public:
    OverlappingProcessing();
    ~OverlappingProcessing();

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

#endif // OVERLAPPINGPROCESSING_H
