#ifndef EXUVULNERABILITIESPROGRESSWIDGET_H
#define EXUVULNERABILITIESPROGRESSWIDGET_H

#include <QWidget>

namespace Ui {
class ExUVulnerabilitiesProgressWidget;
}

class ExUVulnerabilitiesProgressWidget : public QWidget{
    Q_OBJECT

public:
    explicit ExUVulnerabilitiesProgressWidget(QWidget *parent = nullptr);
    ~ExUVulnerabilitiesProgressWidget();
    void setMaxValue(double value);
    void setLabel(const QString& label);
    void updateProgress(double value);
private:
    Ui::ExUVulnerabilitiesProgressWidget *ui;
};

#endif // EXUVULNERABILITIESPROGRESSWIDGET_H
