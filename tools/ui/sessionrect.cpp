#include "sessionrect.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QPen>
#include <QPainter>
#include <QBrush>
#include <QGraphicsSceneHoverEvent>
#include "prova/artifact.h"
#include "sessionrectgroup.h"
#include "exuscene.h"
#include <QApplication>
#include <QStyle>

std::tuple<uint8_t, uint8_t, uint8_t> toLighterColor(uint16_t value) {
    // Use 5 bits each for R, G, and B. (Ignore one bit if needed.)
    uint8_t r5 = (value >> 11) & 0x1F; // bits 11..15
    uint8_t g5 = (value >> 6)  & 0x1F; // bits 6..10
    uint8_t b5 = (value >> 1)  & 0x1F; // bits 1..5

    // Map 0..31 to 0..127 (scale factor = 127/31).
    // Then add 128 to shift into the lighter range.
    auto scale = [](uint8_t v5) -> uint8_t {
        return static_cast<uint8_t>((v5 * 127) / 31 + 128);
    };

    uint8_t r = scale(r5);
    uint8_t g = scale(g5);
    uint8_t b = scale(b5);
    return std::make_tuple(r, g, b);
}

SessionRect::SessionRect(prova::session::ptr s, QGraphicsItem* parent): QGraphicsRectItem(parent), _session(s) {
    _color = Qt::lightGray;

    // _text = new QGraphicsTextItem{this};

    setPen(QPen(Qt::transparent));
    setFlags(ItemIsSelectable | ItemSendsGeometryChanges);
    _properties = s->artifact()->properties();

    // _text->setPos(5, 5);
    if(_properties.contains("path")){
        std::string path = _properties["path"].get<std::string>();
        setToolTip(QString::fromStdString(path));
        QByteArrayView path_bytes{path.c_str()};
        quint16 checksum = qChecksum(path_bytes);
        auto colors = toLighterColor(checksum);
        _color = QColor{std::get<0>(colors), std::get<1>(colors), std::get<2>(colors)};
    }

    setBrush(QBrush{_color});

    _circle = new QGraphicsEllipseItem{this};
    _circle->setPen(QPen(Qt::transparent));
    _circle->setRect(0, 0, max_height-radius, max_height-radius);
    _circle->setPos(radius/2, radius/2);
    QColor color;
    std::string subtype = s->artifact()->subtype();
    if (subtype == "file") {
        color = QColor::fromString("#5dc0fe");
    } else if (subtype == "directory") {
        color = QColor::fromString("#eff195");
    } else if (subtype == "network socket") {
        color = QColor::fromString("#fe49a0");
    }

    if (!color.isValid()) {
        color = QColor(Qt::gray);
    }
    _circle->setBrush(color);

    setAcceptedMouseButtons(Qt::LeftButton);
    _circle->setAcceptedMouseButtons(Qt::NoButton);
}

QVariant SessionRect::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == QGraphicsItem::ItemSelectedHasChanged) {
        qDebug() << "Item selected";
        bool selected = value.toBool();
        if (selected) {
            setPen(QPen(_color.darker(150), 2));
        } else{
            setPen(QPen(Qt::transparent));
        }
        if(scene() != nullptr){
            ExUScene* pscene = qobject_cast<ExUScene*>(scene());
            if(pscene){
                emit pscene->exuSessionSelected(_session, selected);
            }
            // SessionRectGroup* group = dynamic_cast<SessionRectGroup*>(parentItem());
            // if(group){
            //     group->sessionSelected(_session, selected);
            // }
        }
    }
    return QGraphicsRectItem::itemChange(change, value);
}

void SessionRect::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(pen());
    painter->setBrush(brush());
    painter->drawRoundedRect(rect(), radius, radius);
}
