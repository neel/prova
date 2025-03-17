#ifndef EXURESOURCECHARTVIEWER_H
#define EXURESOURCECHARTVIEWER_H

#include <QWidget>

class QGraphicsScene;

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
    explicit ExUResourceChartViewer(std::shared_ptr<prova::execution_unit> unit, QWidget *parent = nullptr);
    ~ExUResourceChartViewer();

private:
    Ui::ExUResourceChartViewer *ui;
    QGraphicsScene*            _scene;
};

#endif // EXURESOURCECHARTVIEWER_H
