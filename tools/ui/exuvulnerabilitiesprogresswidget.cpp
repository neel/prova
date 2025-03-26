#include "exuvulnerabilitiesprogresswidget.h"
#include "ui_exuvulnerabilitiesprogresswidget.h"

ExUVulnerabilitiesProgressWidget::ExUVulnerabilitiesProgressWidget(QWidget *parent): QWidget(parent), ui(new Ui::ExUVulnerabilitiesProgressWidget){
    ui->setupUi(this);
}

ExUVulnerabilitiesProgressWidget::~ExUVulnerabilitiesProgressWidget(){
    delete ui;
}

void ExUVulnerabilitiesProgressWidget::setMaxValue(double value){
    ui->progressBar->setMaximum(value);
}

void ExUVulnerabilitiesProgressWidget::setLabel(const QString &label){
    ui->label->setText(label);
}

void ExUVulnerabilitiesProgressWidget::updateProgress(double value){
    ui->progressBar->setValue(value);
}
