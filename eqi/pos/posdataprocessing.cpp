#include "posdataprocessing.h"
#include "mainwindow.h"
#include "eqi/eqiapplication.h"
#include "eqi/eqiinquiredemvalue.h"

#include "qgis/app/delimitedtext/qgsdelimitedtextfile.h"
#include "qgssinglesymbolrendererv2.h"
#include "qgscategorizedsymbolrendererv2.h"
#include "qgsvectordataprovider.h"
#include "qgscoordinatetransform.h"
#include "qgsmessagelog.h"
#include "qgsmaplayerregistry.h"
#include "qgsvectorlayer.h"
#include "qgsmapcanvas.h"
#include "qgsfeature.h"
#include "qgscrscache.h"
#include "sqlite3.h"
#include <QUuid>
#include <QList>
#include <QMap>
#include <QVariant>
#include <QString>
#include <QColor>
#include <QStringList>

//#include "proj_api.h"

QRegExp posDataProcessing::WktPrefixRegexp( "^\\s*(?:\\d+\\s+|SRID\\=\\d+\\;)", Qt::CaseInsensitive );
QRegExp posDataProcessing::CrdDmsRegexp( "^\\s*(?:([-+nsew])\\s*)?(\\d{1,3})(?:[^0-9.]"
                                         "+([0-5]?\\d))?[^0-9.]+([0-5]?\\d(?:\\.\\d+)?)"
                                         "[^0-9.]*([-+nsew])?\\s*$", Qt::CaseInsensitive );

posDataProcessing::posDataProcessing(QObject *parent) : QObject(parent)
  , mFile( nullptr )
  , mXyDms( false )
  , mMaxInvalidLines(50)
  , mNExtraInvalidLines(0)
{
    this->parent = parent;
}

posDataProcessing::~posDataProcessing()
{
    if ( mFile )
    {
        delete mFile;
        mFile = nullptr;
    }
}

void posDataProcessing::readFieldsList( QString& strUrl )
{
    QUrl url = QUrl::fromEncoded( strUrl.toAscii() );
    mFile = new QgsDelimitedTextFile();
    mFile->setFromUrl( url );

    if ( ! mFile->isValid() )
    {
        MainWindow::instance()->messageBar()->pushMessage( "读取曝光点文件内容",
                                                           "读取字段失败, 运行已终止!",
                                                           QgsMessageBar::CRITICAL,
                                                           MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage("读取曝光点文件内容 : \t读取字段失败, 运行已终止...");
        return;
    }

    mFieldsList.clear();
    QString mNoFieldName, mXFieldName, mYFieldName,
            mZFieldName,mOmegaFieldName, mPhiFieldName,
            mKappaFieldName, mPphotoMarkFieldName;
    if ( url.hasQueryItem( "noField" ) )
    {
        mFieldsList["noField"] = QStringList();
        mNoFieldName = url.queryItemValue( "noField" );
    }
    if ( url.hasQueryItem( "xField" ) )
    {
        mFieldsList["xField"] = QStringList();
        mXFieldName = url.queryItemValue( "xField" );
    }
    if ( url.hasQueryItem( "yField" ) )
    {
        mFieldsList["yField"] = QStringList();
        mYFieldName = url.queryItemValue( "yField" );
    }
    if ( url.hasQueryItem( "zField" ) )
    {
        mFieldsList["zField"] = QStringList();
        mZFieldName = url.queryItemValue( "zField" );
    }
    if ( url.hasQueryItem( "omegaField" ) )
    {
        mFieldsList["omegaField"] = QStringList();
        mOmegaFieldName = url.queryItemValue( "omegaField" );
    }
    if ( url.hasQueryItem( "phiField" ) )
    {
        mFieldsList["phiField"] = QStringList();
        mPhiFieldName = url.queryItemValue( "phiField" );
    }
    if ( url.hasQueryItem( "kappaField" ) )
    {
        mFieldsList["kappaField"] = QStringList();
        mKappaFieldName = url.queryItemValue( "kappaField" );
    }
    if ( url.hasQueryItem( "photoMarkField" ) )
    {
        mFieldsList["photoMarkField"] = QStringList();
        mPphotoMarkFieldName = url.queryItemValue( "photoMarkField" );
    }

    if (mFieldsList.isEmpty())
    {
        MainWindow::instance()->messageBar()->pushMessage( "读取曝光点文件内容",
                                                           "读取字段失败, 曝光点文件字段解析失败, 运行已终止!",
                                                           QgsMessageBar::CRITICAL,
                                                           MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage("读取曝光点文件内容 : \t读取字段失败, 曝光点文件字段解析失败, 运行已终止...");
        return;
    }

    if ( url.hasQueryItem( "xyDms" ) )
    {
        mXyDms = ! url.queryItemValue( "xyDms" ).toLower().startsWith( 'n' );
    }

    QStringList parts;

    int maxField = 0;
    QgsDelimitedTextFile::Status status = mFile->nextRecord( parts );
    if ( status == QgsDelimitedTextFile::RecordOk )
    {
        maxField = parts.size();
    }
    mFile->reset();

    while ( true )
    {
        QgsDelimitedTextFile::Status status = mFile->nextRecord( parts );
        if ( status == QgsDelimitedTextFile::RecordEOF ) break;
        if ( status != QgsDelimitedTextFile::RecordOk )
        {
            recordInvalidLine( "\t**-->第%1行记录格式无效" );
            continue;
        }
        if (parts.size() != maxField)
        {
            recordInvalidLine( "\t**-->第%1行记录缺少字段" );
            continue;
        }
        // 跳过空记录
        if ( recordIsEmpty( parts ) )
        {
            continue;
        }

        // 获得字段内容
        bool isbl = true;
        bool isXField = false;
        bool isYField = false;
        QMap<QString, QString> tmp;
        QMap< QString, QStringList >::iterator it = mFieldsList.begin();
        while (it != mFieldsList.end())
        {
            int mFieldIndex = 0;
            if (it.key() == "noField") mFieldIndex = mFile->fieldIndex( mNoFieldName );
            else if (it.key() == "xField")
            {
                isXField = true;
                mFieldIndex = mFile->fieldIndex( mXFieldName );
            }
            else if (it.key() == "yField")
            {
                isYField = true;
                mFieldIndex = mFile->fieldIndex( mYFieldName );
            }
            else if (it.key() == "zField") mFieldIndex = mFile->fieldIndex( mZFieldName );
            else if (it.key() == "omegaField") mFieldIndex = mFile->fieldIndex( mOmegaFieldName );
            else if (it.key() == "phiField") mFieldIndex = mFile->fieldIndex( mPhiFieldName );
            else if (it.key() == "kappaField") mFieldIndex = mFile->fieldIndex( mKappaFieldName );
            else if (it.key() == "photoMarkField") mFieldIndex = mFile->fieldIndex( mPphotoMarkFieldName );

            QString field = mFieldIndex < parts.size() ? parts[mFieldIndex] : QString();

            if ( (it.key() == "xField") || (it.key() == "yField") )
            {
                bool ok = dFromDms( field, mXyDms );
                if (!ok)
                {
                    isbl = false;
                    recordInvalidLine( "\t**-->第%1行记录的x或y字段是无效的地理坐标格式" );
                }
            }

            tmp[it.key()] = field;
            ++it;
        }
        if (!isXField || !isYField)
        {
            isbl = false;
            recordInvalidLine( "\t**-->第%1行记录缺少x或y坐标" );
        }

        it = mFieldsList.begin();
        while (it != mFieldsList.end())
        {
            QMap<QString, QString>::iterator itSub = tmp.find(it.key());
            if ( itSub == tmp.end() )
            {
                isbl = false;
                recordInvalidLine( "\t**-->第%1行记录缺少" + it.key() + "字段" );
            }
            ++it;
        }

        if (isbl)
        {
            QMap<QString, QString>::iterator it = tmp.begin();
            while (it != tmp.end())
            {
                QMap< QString, QStringList >::iterator itSub = mFieldsList.find(it.key());
                if (itSub != mFieldsList.end())
                {
                    QStringList *list = &(itSub.value());
                    list->append(it.value());
                }
                ++it;
            }
        }
    }
    // 计算相片地面分辨率
    resolutionList = getPosElev();

    reportErrors(QStringList()<<"曝光点文件处理");
}

bool posDataProcessing::autoPosTransform()
{
    /**
     * 检查定义的如果为投影坐标系，
     * 则忽略，否则根据定义的地理
     * 坐标系创建投影坐标系。
    */
    // 获得源坐标系
    QString myDefaultCrs = mSettings.value( "/eqi/prjTransform/projectDefaultCrs", GEO_EPSG_CRS_AUTHID ).toString();
    mSourceCrs.createFromOgcWmsCrs( myDefaultCrs );
    eqiPrj.setSourceCrs(mSourceCrs);

    // 验证正确性
    if (!eqiPrj.sourceCrs().isValid())
    {
        MainWindow::instance()->messageBar()->pushMessage( "曝光点坐标转换",
                                                           "源参照坐标系定义错误，请重新设置，否则相关结果不正确!",
                                                           QgsMessageBar::CRITICAL,
                                                           MainWindow::instance()->messageTimeout() );
    }

    QgsMessageLog::logMessage(QString("曝光点坐标转换 : \t定义参照坐标系为：%1").arg(eqiPrj.sourceCrs().description()));

    // 创建目标坐标系，如果不是投影坐标系，则创建
    if (eqiPrj.isValidPCS(eqiPrj.sourceCrs().postgisSrid()))
    {
        // 为投影坐标系则直接返回
        QgsMessageLog::logMessage("曝光点坐标转换 : \t参照坐标系已经是投影坐标系，未进行坐标转换。");
        return true;
    }
    else
    {
        int cm = getCentralMeridian();
        QgsMessageLog::logMessage(QString("曝光点坐标转换 : \t曝光点文件中央经线计算为%1°。").arg(cm));
        if (!eqiPrj.createTargetCrs( cm ))
        {
            return false;
        }
    }

    // 开始转换
    QMap< QString, QStringList >::iterator it_x = mFieldsList.find("xField");
    QMap< QString, QStringList >::iterator it_y = mFieldsList.find("yField");
    QStringList xList = it_x.value();
    QStringList yList = it_y.value();
    if (xList.size() != yList.size())
    {
        MainWindow::instance()->messageBar()->pushMessage( "曝光点坐标转换",
            "横坐标与纵坐标数量不一致，无法进行下一步转换, 运行已终止!",
            QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage("曝光点坐标转换 : \t横坐标与纵坐标数量不一致，无法进行下一步转换, 运行已终止。");
        return false;
    }

    for (int i=0; i<xList.size(); ++i)
    {
        QgsPoint p = eqiPrj.prjTransform( QgsPoint(xList.at(i).toDouble(), yList.at(i).toDouble()) );
        xList[i] = QString::number(p.x(), 'f');
        yList[i] = QString::number(p.y(), 'f');
    }
    mFieldsList["xField"] = xList;
    mFieldsList["yField"] = yList;

    QgsMessageLog::logMessage("曝光点坐标转换 : \t曝光点文件坐标已自动转换完成。");

    return true;
}

int posDataProcessing::getCentralMeridian()
{
    QMap< QString, int > cmMap;

    QMap< QString, QStringList >::iterator it_x = mFieldsList.find("xField");
    QStringList* xList = &(it_x.value());

    for (int i=0; i<xList->size(); ++i)
    {
        bool isok = false;
        QString str_x = xList->at(i);
        double x = str_x.toDouble(&isok);
        if (!isok)
        {
            QgsMessageLog::logMessage(QString("曝光点中央经度计算 : \t||--> 第%1行横坐标不可识别, 请注意检查该行内容。").arg(i));
        }

        // 计算中央经线
        int cm = eqiProjectionTransformation::getCentralmeridian3(x);

        // 统计相同中央经线出现次数
        if (cmMap.contains(QString::number(cm)))
        {
            int count = cmMap[QString::number(cm)];
            cmMap[QString::number(cm)] = count + 1;
        }
        else
        {
            cmMap[QString::number(cm)] = 1;
        }
    }

    // 返回出现次数最多的中央经线
    QMap< QString, int >::iterator it = cmMap.begin();
    QString strCm;
    int count=0;
    while (it!=cmMap.end())
    {
        if (it.value() > count)
        {
            strCm = it.key();
        }
        ++it;
    }

    return strCm.toInt();
}

QStringList posDataProcessing::getPosElev()
{
    eqiInquireDemValue dem(this);
    QList< QgsPoint > pointFirst;
    QList< qreal > elevations;
    QStringList resolutionList;
    bool isbl = false;

    QMap< QString, QStringList >::iterator it_x = mFieldsList.find("xField");
    QMap< QString, QStringList >::iterator it_y = mFieldsList.find("yField");
    QMap< QString, QStringList >::iterator it_z = mFieldsList.find("zField");
    QStringList* xList = &(it_x.value());
    QStringList* yList = &(it_y.value());
    QStringList* zList = &(it_z.value());

    for(int i = 0; i < xList->size(); ++i)
    {
        // 取出x，y坐标字段内容
        QgsPoint point;
        point.setX(xList->at(i).toDouble());
        point.setY(yList->at(i).toDouble());
        pointFirst.append(point);
    }

    // 计算曝光点坐标对应的DEM高程
    if ( eqiInquireDemValue::eOK == dem.inquireElevations(pointFirst, elevations, &eqiPrj.sourceCrs()) )
    {
        isbl = true;
        if (xList->size() == elevations.size())
            isbl = true;
        else
            isbl = false;
    }

    // 利用DEM高程计算地面分辨率
    for (int i = 0; i < zList->size(); ++i)
    {
        qreal elevation = 0.0;
        if (isbl)
            elevation = elevations.at(i);
        else
            elevation = -9999;
        double resolution = calculateResolution(zList->at(i).toDouble(), elevation);
        resolutionList.append(QString::number(resolution, 'f', 2));
    }
    return resolutionList;
}

QgsVectorLayer* posDataProcessing::autoSketchMap()
{
    emit startProcess();

    QString sketchMapName;
    sketchMapName = mSettings.value("/eqi/pos/lePosFile", "").toString();
    sketchMapName = QFileInfo(sketchMapName).baseName();
    if (sketchMapName.isEmpty())
        sketchMapName = "航摄略图";

    QgsVectorLayer* newLayer = MainWindow::instance()->createrMemoryMap(sketchMapName,
                                                        "Polygon",
                                                        QStringList()
                                                        << "field=id:integer"
                                                        << "field=相片编号:string(50)"
                                                        << "field=曝光点坐标:string(30)"
                                                        << "field=Omega:string(10)"
                                                        << "field=Phi:string(10)"
                                                        << "field=Kappa:string(10)"
                                                        << "field=地面分辨率:string(10)");

    if (!newLayer && !newLayer->isValid())
    {
        MainWindow::instance()->messageBar()->pushMessage( "创建航摄略图",
            "创建略图失败, 运行已终止, 注意检查plugins文件夹!",
            QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("创建航摄略图 : \t创建略图失败, 程序已终止, 注意检查plugins文件夹。"));
        return nullptr;
    }

    // 设置图层参照坐标系
    newLayer->setCrs(eqiPrj.destCRS());

    // 将地图画布设置为与图层同样的参照坐标系
    MainWindow::instance()->mapCanvas()->setDestinationCrs(eqiPrj.destCRS());

    QMap< QString, QStringList >::iterator it_n = mFieldsList.find("noField");
    QMap< QString, QStringList >::iterator it_x = mFieldsList.find("xField");
    QMap< QString, QStringList >::iterator it_y = mFieldsList.find("yField");
    QMap< QString, QStringList >::iterator it_o = mFieldsList.find("omegaField");
    QMap< QString, QStringList >::iterator it_p = mFieldsList.find("phiField");
    QMap< QString, QStringList >::iterator it_k = mFieldsList.find("kappaField");
    QStringList* nList = &(it_n.value());
    QStringList* xList = &(it_x.value());
    QStringList* yList = &(it_y.value());
    QStringList* oList = &(it_o.value());
    QStringList* pList = &(it_p.value());
    QStringList* kList = &(it_k.value());

    QList< QgsPoint > pointFirst;
    for(int i = 0; i < xList->size(); ++i)
    {
        // 取出x，y坐标字段内容
        QgsPoint point;
        point.setX(xList->at(i).toDouble());
        point.setY(yList->at(i).toDouble());
        pointFirst.append(point);
    }

    // 创建面要素
    int icount = 0;
    QgsFeatureList featureList;
    for (int i = 0; i < xList->size(); ++i)
    {
        // 取出字段内容
        double resolution = resolutionList.at(i).toDouble();
        double mRotate = kList->at(i).toDouble();

        if (resolution == 0.0)
        {
            QgsMessageLog::logMessage(QString("\t\t||-->相片:%1 高程异常，地面分辨率计算为0，已跳过该张相片.").arg(nList->at(i)));
            deletePosRecord(nList->at(i--));
            continue;
        }

        // 创建面要素, 并根据Omega选择角度
        QgsPolygon polygon = rectangle( pointFirst.at(i), resolution );
        QgsGeometry* mGeometry = QgsGeometry::fromPolygon(polygon);
        mGeometry->rotate( mRotate, pointFirst.at(i) );

        // 设置几何要素与属性
        QgsFeature MyFeature;
        MyFeature.setGeometry( mGeometry );
        MyFeature.setAttributes(QgsAttributes() << QVariant(++icount)
                                                << QVariant(nList->at(i))
                                                << QVariant(QString(xList->at(i)+","+yList->at(i)))
                                                << QVariant(oList->at(i))
                                                << QVariant(pList->at(i))
                                                << QVariant(kList->at(i))
                                                << QVariant(resolutionList.at(i)));
        featureList.append(MyFeature);
    }

    MainWindow::instance()->mapCanvas()->freeze();

    // 添加要素集到图层中
    newLayer->dataProvider()->addFeatures(featureList);

    // 更新范围
    newLayer->updateExtents();

    // 初始化符号
    // 获得缺省的符号
    QgsSymbolV2* newSymbolV2 = QgsSymbolV2::defaultSymbol(newLayer->geometryType());

    // 设置透明度与颜色
    newSymbolV2->setAlpha(0.5);
    newSymbolV2->setColor(Qt::gray);
    QgsSingleSymbolRendererV2* singleRenderer = new QgsSingleSymbolRendererV2(newSymbolV2);
    newLayer->setRendererV2(singleRenderer);

    emit stopProcess();

    // 添加到地图
    MainWindow::instance()->mapCanvas()->freeze( false );
    MainWindow::instance()->refreshMapCanvas();

    QgsMessageLog::logMessage(QString("创建航飞略图 : \t成功创建%1张相片略图。").arg(newLayer->featureCount()));
    return newLayer;
}

QgsPolygon posDataProcessing::rectangle( const QgsPoint& point, const double& resolution )
{
    int weight = mSettings.value("/eqi/options/imagePreprocessing/leWidth", 0).toInt();
    int height = mSettings.value("/eqi/options/imagePreprocessing/leHeight", 0).toInt();
    double midx = (weight*resolution) / 2;
    double midy = (height*resolution) / 2;

    QgsPolyline polyline;
    QgsPolygon polyon;
    polyline << QgsPoint( point.x()-midx, point.y()+midy )
             << QgsPoint( point.x()+midx, point.y()+midy )
             << QgsPoint( point.x()+midx, point.y()-midy )
             << QgsPoint( point.x()-midx, point.y()-midy );
    polyon << polyline;

    return polyon;
}

bool posDataProcessing::isValid()
{
    return !mFieldsList.isEmpty();
}

const QStringList *posDataProcessing::noList()
{
    QMap< QString, QStringList >::iterator it_no = mFieldsList.find("noField");
    if (it_no != mFieldsList.end())
        return &(it_no.value());
    else
        return nullptr;
}

const QString posDataProcessing::getPosRecord( const QString& noValue, const QString& valueField )
{
    QString returnStr;
    QStringList* noList;
    QStringList* valueList;
    QMap< QString, QStringList >::iterator it_no = mFieldsList.find("noField");
    QMap< QString, QStringList >::iterator it_value = mFieldsList.find(valueField);

    if (it_no == mFieldsList.end() || it_value == mFieldsList.end())
        return returnStr;

    noList = &(it_no.value());
    valueList = &(it_value.value());
    int noIndex = noList->indexOf(noValue);
    if (noIndex != -1)
    {
        returnStr = valueList->at(noIndex);
    }

    return returnStr;
}

//void posDataProcessing::setPosRecord( const QString& keyField, const QString& valueField, const QString& value )
//{

//}

void posDataProcessing::deletePosRecord(const QString &No )
{
    if (No.isEmpty())
    {
        return;
    }

    if (mFieldsList.isEmpty())
    {
        MainWindow::instance()->messageBar()->pushMessage( "曝光点文件",
            "内存结构被破坏，未正确处理曝光点文件，请重新尝试.",
            QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("曝光点文件 : \t内存结构被破坏，未正确处理曝光点文件，请重新尝试."));
        return;
    }

    QMap< QString, QStringList >::iterator it_no = mFieldsList.find("noField");
    QMap< QString, QStringList >::iterator it_x = mFieldsList.find("xField");
    QMap< QString, QStringList >::iterator it_y = mFieldsList.find("yField");
    QMap< QString, QStringList >::iterator it_z = mFieldsList.find("zField");
    QMap< QString, QStringList >::iterator it_omega = mFieldsList.find("omegaField");
    QMap< QString, QStringList >::iterator it_phi = mFieldsList.find("phiField");
    QMap< QString, QStringList >::iterator it_kappa = mFieldsList.find("kappaField");
    QMap< QString, QStringList >::iterator it_photoMark = mFieldsList.find("photoMarkField");

    int index = it_no->indexOf(No);
    if (index == -1)
    {
        return;
    }

    it_no->removeAt(index);
    it_x->removeAt(index);
    it_y->removeAt(index);
    it_z->removeAt(index);
    it_omega->removeAt(index);
    it_phi->removeAt(index);
    it_kappa->removeAt(index);
    if (it_photoMark != mFieldsList.end())
        it_photoMark->removeAt(index);
}

void posDataProcessing::deletePosRecords(const QStringList &NoList)
{
    if (NoList.isEmpty())
    {
        return;
    }

    foreach (QString no, NoList)
    {
        deletePosRecord(no);
    }
}

double posDataProcessing::calculateResolution( const double &absoluteHeight, const double &groundHeight )
{
    double resolution = 0.0;
    double elevation = 0.0;
    double pixelSize = 0.0;
    double focal = 0.0;

    if (groundHeight == -9999)
        elevation = mSettings.value("/eqi/options/imagePreprocessing/leAverageEle", 0.0).toDouble();
    else
        elevation = groundHeight;

    pixelSize = mSettings.value("/eqi/options/imagePreprocessing/lePixelSize", 0.0).toDouble();
    focal = mSettings.value("/eqi/options/imagePreprocessing/leFocal", 0.0).toDouble();
    resolution = (absoluteHeight-elevation)*pixelSize/1000/focal;

    return resolution;
}

bool posDataProcessing::posExport()
{
    if (mFieldsList.isEmpty())
    {
        MainWindow::instance()->messageBar()->pushMessage( "导出曝光点文件",
            "内存结构被破坏，未正确导出，请联系开发人员解决.",
            QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("导出曝光点文件 : \t内存结构被破坏，未正确导出，请联系开发人员解决."));
        return false;
    }

    QString path = mSettings.value("/eqi/pos/lePosFile", "").toString();
    path.insert(path.size()-4, "out");
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate))   //只写、文本、重写
    {
        MainWindow::instance()->messageBar()->pushMessage( "导出曝光点文件",
            QString("创建%1曝光点文件失败.").arg(QDir::toNativeSeparators(path)),
            QgsMessageBar::CRITICAL, MainWindow::instance()->messageTimeout() );
        QgsMessageLog::logMessage(QString("导出曝光点文件 : \t创建%1曝光点文件失败.").arg(QDir::toNativeSeparators(path)));
        return false;
    }

    QTextStream out(&file);

    QMap< QString, QStringList >::iterator it_no = mFieldsList.find("noField");
    QMap< QString, QStringList >::iterator it_x = mFieldsList.find("xField");
    QMap< QString, QStringList >::iterator it_y = mFieldsList.find("yField");
    QMap< QString, QStringList >::iterator it_z = mFieldsList.find("zField");
    QMap< QString, QStringList >::iterator it_omega = mFieldsList.find("omegaField");
    QMap< QString, QStringList >::iterator it_phi = mFieldsList.find("phiField");
    QMap< QString, QStringList >::iterator it_kappa = mFieldsList.find("kappaField");
    QMap< QString, QStringList >::iterator it_photoMark = mFieldsList.find("photoMarkField");

    QList< QStringList* > outList;
    if (it_no != mFieldsList.end()) outList << &(it_no.value());
    if (it_x != mFieldsList.end()) outList << &(it_x.value());
    if (it_y != mFieldsList.end()) outList << &(it_y.value());
    if (it_z != mFieldsList.end()) outList << &(it_z.value());
    if (it_omega != mFieldsList.end()) outList << &(it_omega.value());
    if (it_phi != mFieldsList.end()) outList << &(it_phi.value());
    if (it_kappa != mFieldsList.end()) outList << &(it_kappa.value());
    if (it_photoMark != mFieldsList.end()) outList << &(it_photoMark.value());

    int maxCount = 0;
    foreach (QStringList* list, outList)
    {
        maxCount = list->size() > maxCount ? list->size() : maxCount;
    }

    for (int i=0; i<maxCount; ++i)
    {
        QString strLine;
        foreach (QStringList* list, outList)
        {
            QString str = list->at(i);
            strLine.append(str + '\t');
        }
        out << strLine + '\n';
    }

    file.close();
    MainWindow::instance()->messageBar()->pushMessage( "导出曝光点文件",
        QString("导出%1曝光点文件成功.").arg(QDir::toNativeSeparators(path)),
        QgsMessageBar::SUCCESS, MainWindow::instance()->messageTimeout() );
    QgsMessageLog::logMessage(QString("导出曝光点文件 : \t导出%1曝光点文件成功.").arg(QDir::toNativeSeparators(path)));
    return true;
}

const QStringList posDataProcessing::checkPosSettings()
{
    QStringList errList;
    double tmpDouble = 0.0;
    int tmpInt = 0;

    tmpDouble = mSettings.value("/eqi/options/imagePreprocessing/leFocal", 0.0).toDouble();
    if (!tmpDouble)
        errList.append("相机焦距");
    tmpDouble = mSettings.value("/eqi/options/imagePreprocessing/lePixelSize", 0.0).toDouble();
    if (!tmpDouble)
        errList.append("相机大小");
    tmpInt = mSettings.value("/eqi/options/imagePreprocessing/leHeight", 0.0).toInt();
    if (!tmpInt)
        errList.append("相幅大小(长)");
    tmpInt = mSettings.value("/eqi/options/imagePreprocessing/leWidth", 0.0).toInt();
    if (!tmpInt)
        errList.append("相幅大小(宽)");

    return errList;
}

void posDataProcessing::recordInvalidLine( const QString& message )
{
    if ( mInvalidLines.size() < mMaxInvalidLines )
    {
        mInvalidLines.append( message.arg( mFile->recordId() ) );
    }
    else
    {
        mNExtraInvalidLines++;
    }
}

bool posDataProcessing::recordIsEmpty( QStringList &record )
{
    foreach ( const QString& s, record )
    {
        if ( ! s.isEmpty() ) return false;
    }
    return true;
}

bool posDataProcessing::dFromDms( QString &sDms, bool xyDms )
{
    bool Ok;
    double xy = 0;

    if ( xyDms )
    {
        xy = this->dmsStringToDouble( sDms, &Ok );

        if (Ok)
        {
            sDms = QString::number(xy, 'f', 9);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        xy = sDms.toDouble(&Ok);
        if (Ok)
            return true;
    }

    return false;
}

double posDataProcessing::dmsStringToDouble( const QString &sX, bool *xOk )
{
    static QString negative( "swSW-" );
    QRegExp re( CrdDmsRegexp );
    double x = 0.0;

    *xOk = re.indexIn( sX ) == 0;
    if ( ! *xOk ) return 0.0;
    QString dms1 = re.capturedTexts().at( 2 );
    QString dms2 = re.capturedTexts().at( 3 );
    QString dms3 = re.capturedTexts().at( 4 );
    x = dms3.toDouble( xOk );
    // Allow for Degrees/minutes format as well as DMS
    if ( ! dms2.isEmpty() )
    {
        x = dms2.toInt( xOk ) + x / 60.0;
    }
    x = dms1.toInt( xOk ) + x / 60.0;
    QString sign1 = re.capturedTexts().at( 1 );
    QString sign2 = re.capturedTexts().at( 5 );

    if ( sign1.isEmpty() )
    {
        if ( ! sign2.isEmpty() && negative.contains( sign2 ) ) x = -x;
    }
    else if ( sign2.isEmpty() )
    {
        if ( ! sign1.isEmpty() && negative.contains( sign1 ) ) x = -x;
    }
    else
    {
        *xOk = false;
    }
    return x;
}

int posDataProcessing::getInvalidLineSize()
{
    return mInvalidLines.size() + mNExtraInvalidLines;
}

void posDataProcessing::clearInvalidLines()
{
    mInvalidLines.clear();
    mNExtraInvalidLines = 0;
}

void posDataProcessing::reportErrors( const QStringList& messages /*= QStringList()*/, bool showDialog /*= false */ )
{
    if ( !mInvalidLines.isEmpty() && !messages.isEmpty() )
    {
        QString tag( "曝光点处理" );
        QgsMessageLog::logMessage( QString("错误文件 %1").arg( mFile->fileName() ), tag );
        foreach ( const QString& message, messages )
        {
            QgsMessageLog::logMessage( message, tag );
        }
        if ( !mInvalidLines.isEmpty() )
        {
            QgsMessageLog::logMessage( "由于错误，以下行未加载到略图中:", tag );
            for ( int i = 0; i < mInvalidLines.size(); ++i )
                QgsMessageLog::logMessage( mInvalidLines.at( i ), tag );
            if ( mNExtraInvalidLines > 0 )
                QgsMessageLog::logMessage( QString( "文件中还有%1个其他错误" ).arg( mNExtraInvalidLines ), tag );
        }

        // We no longer need these lines.
        clearInvalidLines();
    }
}
