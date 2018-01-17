#ifndef EQIMAPTOOLPOINTTOTK_H
#define EQIMAPTOOLPOINTTOTK_H

#include <QSettings>
#include <QList>
#include "qgsmaptool.h"
#include "qgspoint.h"

class QgsMapCanvas;
class QgsRubberBand;
class QgsVectorLayer;

class eqiMapToolPointToTk : public QgsMapTool
{
    Q_OBJECT

public:
    eqiMapToolPointToTk(QgsMapCanvas* canvas);
    ~eqiMapToolPointToTk();

    //! Overridden mouse release event
    virtual void canvasReleaseEvent( QgsMapMouseEvent* e ) override;

private:
    // 获得当前矢量图层，并且必须包含“TH”字段
    QgsVectorLayer* getCurrentVectorLayer( QgsMapCanvas* canvas );

    //! Overridden mouse move event
    virtual void canvasMoveEvent( QgsMapMouseEvent* e ) override;

    // 计算矩形四个角点坐标
    QList<QgsPoint> getRect(const QgsPoint& lastPoint, const QgsPoint& nextPoint);

private:
    int count;
    QgsPoint lastPoint;
    QgsPoint nextPoint;
    QgsVectorLayer* newLayer;

    //! used for storing all of the maps point for the polygon
    QgsRubberBand* mRubberBand;
    QColor mFillColor;
    QColor mBorderColour;

    QSettings mSettings;

    bool isAvailable;
};

#endif // EQIMAPTOOLPOINTTOTK_H
