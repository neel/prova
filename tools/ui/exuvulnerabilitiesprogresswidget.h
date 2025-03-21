#ifndef EXUVULNERABILITIESPROGRESSWIDGET_H
#define EXUVULNERABILITIESPROGRESSWIDGET_H

#include <QWidget>

namespace Ui {
class ExUVulnerabilitiesProgressWidget;
}

class ExUVulnerabilitiesProgressWidget : public QWidget{
    Q_OBJECT

public:
    explicit ExUVulnerabilitiesProgressWidget(const QString& keyword, QWidget *parent = nullptr);
    ~ExUVulnerabilitiesProgressWidget();

private:
    Ui::ExUVulnerabilitiesProgressWidget *ui;
};

#endif // EXUVULNERABILITIESPROGRESSWIDGET_H
