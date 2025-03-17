#ifndef SESSIONRECT_H
#define SESSIONRECT_H

#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include "prova/session.h"
#include "nlohmann/json.hpp"

class SessionRect : public QGraphicsRectItem{
public:
    static constexpr qreal max_height = 20.0f;
    static constexpr qreal radius     = 10.0f;
    static constexpr qreal min_width  = 50.0f;
public:
    SessionRect(prova::session::ptr s, QGraphicsItem* parent = nullptr);
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
private:
    prova::session::ptr   _session;
    nlohmann::json        _properties;
    QGraphicsTextItem*    _text;
    QGraphicsEllipseItem* _circle;

};

#endif // SESSIONRECT_H
