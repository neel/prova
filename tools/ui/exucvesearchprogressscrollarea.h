#ifndef EXUCVESEARCHPROGRESSSCROLLAREA_H
#define EXUCVESEARCHPROGRESSSCROLLAREA_H

#include <QScrollArea>

namespace Ui {
class ExUCVESearchProgressScrollArea;
}

class QVBoxLayout;
class ExUVulnerabilitiesProgressWidget;

class ExUCVESearchProgressScrollArea : public QScrollArea
{
    Q_OBJECT

public:
    explicit ExUCVESearchProgressScrollArea(QWidget *parent = nullptr);
    ~ExUCVESearchProgressScrollArea();
public:
    void add(ExUVulnerabilitiesProgressWidget* progressWidget);
    void remove(ExUVulnerabilitiesProgressWidget* progressWidget);
private:
    Ui::ExUCVESearchProgressScrollArea* ui;
    QWidget*                            _container;
    QVBoxLayout*                        _centralLayout;
    QVBoxLayout*                        _layout;
};

#endif // EXUCVESEARCHPROGRESSSCROLLAREA_H
