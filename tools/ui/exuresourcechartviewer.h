#ifndef EXURESOURCECHARTVIEWER_H
#define EXURESOURCECHARTVIEWER_H

#include <QWidget>
#include "prova/session.h"

class QGraphicsScene;
class ExUScene;

namespace prova{
struct execution_unit;
}

namespace Ui {
class ExUResourceChartViewer;
}

class ExUResourceChartViewer : public QWidget
{
    Q_OBJECT

public:
    explicit ExUResourceChartViewer(QWidget *parent = nullptr);
    explicit ExUResourceChartViewer(std::shared_ptr<prova::execution_unit> unit, QWidget *parent = nullptr);
    ~ExUResourceChartViewer();
    void setUnit(std::shared_ptr<prova::execution_unit> unit);
private:
    Ui::ExUResourceChartViewer *ui;
    ExUScene*                  _scene;
    std::shared_ptr<prova::execution_unit> _unit;
signals:
    void exuSessionSelected(prova::session::ptr, bool);
};

#endif // EXURESOURCECHARTVIEWER_H
