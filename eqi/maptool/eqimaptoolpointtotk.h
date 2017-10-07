#ifndef EQIMAPTOOLPOINTTOTK_H
#define EQIMAPTOOLPOINTTOTK_H

#include "qgsmaptool.h"
#include "qgspoint.h"

class QgsMapCanvas;
class QgsVectorLayer;
class QgsVectorDataProvider;

class eqiMapToolPointToTk : public QgsMapTool
{
    Q_OBJECT

public:
    eqiMapToolPointToTk(QgsMapCanvas* canvas);

    //! Overridden mouse release event
    virtual void canvasReleaseEvent( QgsMapMouseEvent* e ) override;

private:
    int count;
    bool fisrtMap;
    QgsPoint lastPoint;
    QgsPoint nextPoint;
    QgsVectorLayer* newLayer;
    QgsVectorDataProvider* dateProvider;
};

#endif // EQIMAPTOOLPOINTTOTK_H
