#include "exucvesearchprogressscrollarea.h"
#include "ui_exucvesearchprogressscrollarea.h"
#include <QVBoxLayout>
#include "exuvulnerabilitiesprogresswidget.h"

ExUCVESearchProgressScrollArea::ExUCVESearchProgressScrollArea(QWidget *parent): QScrollArea(parent), ui(new Ui::ExUCVESearchProgressScrollArea){
    ui->setupUi(this);

    _container = new QWidget{this};
    _centralLayout = new QVBoxLayout{_container};
    _container->setLayout(_centralLayout);

    _container->setMinimumHeight(0);
    _container->setMinimumSize(0, 0);
    _container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _centralLayout->setContentsMargins(0, 0, 0, 0);
    // _centralLayout->setStretch(0, 0);
    _centralLayout->setSpacing(0);
    // _centralLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    _centralLayout->setSizeConstraint(QLayout::SetMaximumSize);

    setWidget(_container);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setWidgetResizable(true);

    _layout = new QVBoxLayout{this};
    _layout->setContentsMargins(0, 0, 0, 0);
    // _layout->setStretch(0, 0);
    _layout->setSpacing(0);
    _layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    _layout->addWidget(_container);

    // setStyleSheet("background-color: red;");
    // _container->setStyleSheet("background-color: green;");
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
}

ExUCVESearchProgressScrollArea::~ExUCVESearchProgressScrollArea(){
    delete ui;
}

void ExUCVESearchProgressScrollArea::add(ExUVulnerabilitiesProgressWidget* progressWidget){
    _centralLayout->addWidget(progressWidget);
    // progressWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    _container->update();
    _container->adjustSize();
    viewport()->update();
}

void ExUCVESearchProgressScrollArea::remove(ExUVulnerabilitiesProgressWidget* progressWidget){
    _centralLayout->removeWidget(progressWidget);
    _container->adjustSize();
}
