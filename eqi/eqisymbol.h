#ifndef EQISYMBOL_H
#define EQISYMBOL_H

#include <QObject>

class QgsVectorLayer;
class QgsSymbolV2;

class eqiSymbol : public QObject
{
    Q_OBJECT
public:
    explicit eqiSymbol(QObject *parent = nullptr);
    explicit eqiSymbol(QObject *parent = nullptr, QgsVectorLayer *layer);

signals:

public slots:
};

#endif // EQISYMBOL_H
