#ifndef HEAD_H
#define HEAD_H

// Qt常用类
#include <QSettings>
#include <QDebug>
#include <QString>
#include <QStringList>

// QGis常用类
#include "qgsvectorlayer.h"
#include "qgsmessagelog.h"

// OpenGL
#define GL_RED                            0x1903
#define GL_RGB                            0x1907
#define GL_LUMINANCE                      0x1909
#define GL_STENCIL_INDEX                  0x1901
#define GL_DEPTH_COMPONENT                0x1902
#define GL_INTENSITY8                     0x804B
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_FLOAT                          0x1406

// GDAL
#include "cpl_string.h"
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "gdal/commonutils.h"
#include "gdal/gdal_utils_priv.h"

typedef unsigned char                     DT_8U;

//! EQI自定义常量

// 相邻相片最小连接点阈值
static const int keyThreshold = 50;

// 分幅图框字段名称
static const QString ThFieldName = "TH";

// 设置的参考坐标系
static const QString globalCrs = "/eqi/prjTransform/projectDefaultCrs";

#endif // HEAD_H
