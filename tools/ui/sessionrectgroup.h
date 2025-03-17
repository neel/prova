#ifndef SESSIONRECTGROUP_H
#define SESSIONRECTGROUP_H

#include <QGraphicsItemGroup>
#include <vector>
#include "prova/session.h"

class SessionRect;

class SessionRectGroup : public QGraphicsItemGroup{
public:
    static constexpr qreal margin_hz = 20.0f;
public:
    SessionRectGroup(prova::session::ptr session, QGraphicsItem* parent = nullptr);
private:
    void layoutChildren();
private:
    prova::session::ptr             _session;
    SessionRect*            _root;
    std::vector<SessionRectGroup*>  _items;
    qreal   _twidth;
};

#endif // SESSIONRECTGROUP_H
