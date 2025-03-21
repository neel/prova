#include "exuvulnerabilitiesprogresswidget.h"
#include "ui_exuvulnerabilitiesprogresswidget.h"

ExUVulnerabilitiesProgressWidget::ExUVulnerabilitiesProgressWidget(const QString& keyword, QWidget *parent): QWidget(parent), ui(new Ui::ExUVulnerabilitiesProgressWidget){
    ui->setupUi(this);
    ui->label->setText(keyword);
}

ExUVulnerabilitiesProgressWidget::~ExUVulnerabilitiesProgressWidget(){
    delete ui;
}
