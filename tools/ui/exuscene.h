#ifndef EXUSCENE_H
#define EXUSCENE_H

#include <QGraphicsScene>
#include "prova/session.h"

class ExUScene : public QGraphicsScene{
    Q_OBJECT
public:
    explicit ExUScene(QObject *parent = nullptr);
signals:
    void exuSessionSelected(prova::session::ptr, bool);
};

#endif // EXUSCENE_H
