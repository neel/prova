#include "sessionrectgroup.h"
#include "sessionrect.h"

SessionRectGroup::SessionRectGroup(prova::session::ptr session, QGraphicsItem* parent): QGraphicsRectItem(parent), _session(session) {
    setHandlesChildEvents(false);

    _root = new SessionRect(_session, this);

    for(auto it = _session->children_begin(); it != _session->children_end(); ++it){
        auto* childItem = new SessionRectGroup(*it, _root);
        _items.push_back(childItem);
    }

    layoutChildren();
}

// void SessionRectGroup::sessionSelected(prova::session::ptr session, bool selected){
//     if(parentItem() != nullptr){
//         SessionRect* p = dynamic_cast<SessionRect*>(parentItem());
//         if(p){
//             p->sessionSelected(session, selected);
//         }
//     }
// }

void SessionRectGroup::layoutChildren(){
    setPos(0, 0);
    qreal xoffset = margin_hz/2;
    qreal children_width = 0.0f;
    for (SessionRectGroup* c: _items){
        c->layoutChildren();
        c->setPos(xoffset+children_width, SessionRect::max_height+5);
        children_width += c->_twidth;
    }

    _twidth = std::max(children_width, SessionRect::min_width)+margin_hz;
    _root->setRect(0, 0, _twidth-margin_hz/2, SessionRect::max_height);
}
