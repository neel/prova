#include "mainwindow.h"
#include "prova/store.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <QMessageBox>
#include <format>
#include "prova/execution_unit.h"
#include "prova/action.h"
#include <QSvgWidget>
#include <QTextEdit>
#include <QMdiSubWindow>
#include <QJsonModel.hpp>
#include <QScrollArea>
#include <QSvgRenderer>
#include <fstream>
#include <QContextMenuEvent>
#include <QMessageBox>
#include "settingsdialog.h"
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QStandardPaths>
#include "exuvulnerabilitiesviewer.h"
#include "exuresourcechartviewer.h"
#include "exuwidget.h"
#include "exufilterproxymodel.h"
#include <QSettings>
#include <QResource>
#include <QDir>
#include <QTemporaryFile>
#include <QMenuBar>
#include <QStyleFactory>
#include <QActionGroup>
#include <QQuickStyle>
#include <QQuickWidget>   // or QQuickView
#include <QQmlEngine>
#include <QUrl>
#include <QStyleFactory>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    unpackPlantUmlJar();

    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::showSettingsDialog);

    _network = new QNetworkAccessManager(this);
    _cache = new QNetworkDiskCache{this};
    QString directory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1StringView("/cacheDir/");
    _cache->setCacheDirectory(directory);
    _cache->setMaximumCacheSize(100 * 1024 * 1024);
    _network->setCache(_cache);

    _exuModel = new ExUModel{this};
    _exuFilterProxyModel = new ExUFilterProxyModel{this};
    _exuFilterProxyModel->setSourceModel(_exuModel);

    ui->exuTreeView->setExpandsOnDoubleClick(true);
    ui->exuTreeView->setItemsExpandable(true);
    ui->exuTreeView->setRootIsDecorated(true);
    ui->exuTreeView->setModel(_exuFilterProxyModel);
    ui->exuTreeView->setSortingEnabled(true);
    ui->exuTreeView->sortByColumn(1, Qt::DescendingOrder);
    ui->exuTreeView->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui->exuTreeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->exuTreeView->viewport()->installEventFilter(this);
    populate();
    std::cout << "Populated" << std::endl;
    connect(ui->exuTreeView, &QTreeView::doubleClicked, this, &MainWindow::exuSelected);
    connect(ui->exuTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this ,[this](const QItemSelection& sel, const QItemSelection&)->void{
        if (sel.indexes().isEmpty())
            return;

        QModelIndex proxyIdx = sel.indexes().first();
        QModelIndex srcIdx = _exuFilterProxyModel->mapToSource(proxyIdx);
        if (!srcIdx.isValid())
            return;
        std::size_t level = 0;
        for (QModelIndex p = srcIdx.parent(); p.isValid(); p = p.parent())
            ++level;

        _exuModel->updateHeaderLevel(level);
    });

    connect(this, &MainWindow::sequenceClicked, this, &MainWindow::showSequenceDiagram);
    connect(this, &MainWindow::resourcesClicked, this, &MainWindow::showResources);

    connect(ui->processNameLineEdit, &QLineEdit::textChanged, this, &MainWindow::filterProcess);

    buildStyleMenus();
}

void MainWindow::buildStyleMenus() {
    auto stylesMenu = ui->menuFile->addMenu(tr("&Styles"));

    /* ----- QWidget styles ------------------------------------ */
    auto widgetMenu = stylesMenu->addMenu(tr("Widget &Style"));
    m_widgetStyleGroup = new QActionGroup(this);
    m_widgetStyleGroup->setExclusive(true);

    const QString currentStyle = qApp->style()->objectName();

    if(QStyleFactory::keys().contains("Fusion")){
        QApplication::setStyle("Fusion");
    }

    for (const QString &styleKey : QStyleFactory::keys()) {
        QAction *a = widgetMenu->addAction(styleKey);
        a->setCheckable(true);
        a->setData(styleKey);
        if (!currentStyle.compare(styleKey, Qt::CaseInsensitive))
            a->setChecked(true);
        m_widgetStyleGroup->addAction(a);
    }
    connect(m_widgetStyleGroup, &QActionGroup::triggered, this, &MainWindow::applyWidgetStyle);

    /* ----- Qt Quick Controls 2 styles ------------------------ */
    auto quickMenu = stylesMenu->addMenu(tr("Quick Controls &Style"));
    m_quickStyleGroup = new QActionGroup(this);
    m_quickStyleGroup->setExclusive(true);

    QQmlEngine engine;
    QStringList quickStyles;
    qDebug() << engine.importPathList();
    for (const auto &path : engine.importPathList()) {
        QDir dir(path + "/QtQuick/Controls");
        if (dir.exists()) {
            auto list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            quickStyles.append(list);
        }
    }

    if(quickStyles.contains("Fusion")){
        QQuickStyle::setStyle("Fusion");
    }

    const QString currentQuick = QQuickStyle::name().isEmpty() ? QStringLiteral("Default") : QQuickStyle::name();
    for (const QString &style : quickStyles) {
        QAction *a = quickMenu->addAction(style);
        a->setCheckable(true);
        a->setData(style);
        if (currentQuick.compare(style, Qt::CaseInsensitive) == 0)
            a->setChecked(true);
        m_quickStyleGroup->addAction(a);
    }
    connect(m_quickStyleGroup, &QActionGroup::triggered, this, &MainWindow::applyQuickStyle);
}

/* ------------------------------------------------------------- *
 *  QWidget styles – immediate
 * ------------------------------------------------------------- */
void MainWindow::applyWidgetStyle(QAction *act)
{
    const QString styleName = act->data().toString();
    if (!styleName.compare(qApp->style()->objectName(), Qt::CaseInsensitive))
        return;                              // already active

    QApplication::setStyle(styleName);
}

/* ------------------------------------------------------------- *
 *  Qt Quick Controls 2 styles – requires reload
 * ------------------------------------------------------------- */
void MainWindow::applyQuickStyle(QAction *act)
{
    const QString styleName = act->data().toString();
    if (!styleName.compare(QQuickStyle::name(), Qt::CaseInsensitive))
        return;                              // already active

    QQuickStyle::setStyle(styleName);
}


MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::populate(){
    QSettings settings("Simula", "SimVul");
    std::string host = settings.value("host", "localhost").toString().toStdString();
    unsigned port = settings.value("port", 8529).toUInt();
    std::string username = settings.value("username", "root").toString().toStdString();
    std::string password = settings.value("password", "").toString().toStdString();
    try{
        _store.fetch(host, port, username, password);
    } catch(std::exception& ex){
        QMessageBox::critical(this,
            QString::fromStdString("Failed to Fetch"),
            QString::fromStdString("Filed to fetch auto log information from the graph database with error %1").arg(QString::fromStdString(ex.what())),
            QMessageBox::Ok
        );
    }
    std::vector<std::shared_ptr<prova::execution_unit>> exu_units;
    _store.extract(exu_units);
    for(const auto& unit: exu_units){
        _exuModel->add_unit(unit);
    }
    std::cout << "Fetched " << exu_units.size() << std::endl;

    // std::size_t i = 0;
    // for(const auto& unit: exu_units){
    //     nlohmann::json dataset = nlohmann::json::array();
    //     unit->root()->flatten(dataset);
    //     std::string json_str = dataset.dump(4);
    //     std::string out_path = std::format("{}.syscalls.json", i++);
    //     std::ofstream outfile(out_path.c_str());
    //     if (outfile.is_open()) {
    //         outfile << json_str;
    //         outfile.close();
    //         std::cout << "Data successfully written to " << out_path << std::endl;
    //     } else {
    //         std::cerr << "Unable to open file for writing." << std::endl;
    //     }
    // }

    std::cout << "ready " << std::endl;
    qDebug() << "Ready!";
}

void MainWindow::exuSelected(const QModelIndex& index){
    QModelIndex srcIdx = _exuFilterProxyModel->mapToSource(index);
    if (!srcIdx.isValid())
        return;

    if (!srcIdx.parent().isValid()) {
        std::shared_ptr<prova::execution_unit> unit = _exuModel->unit(srcIdx.row());

        QSettings settings("Simula", "SimVul");
        QString nvd_api_key = settings.value("apiKeyNVD", "").toString();

        auto* exuWidget  = new ExUWidget{ unit, _network };
        exuWidget->setNVDApiKey(nvd_api_key);
        auto* subWindow  = ui->mdiArea->addSubWindow(exuWidget);
        subWindow->setWindowTitle(QString::fromStdString(std::format("ExU {}", srcIdx.row())));
        subWindow->show();
    }
}

bool MainWindow::eventFilter(QObject* target, QEvent *event){
    if (target == ui->exuTreeView->viewport()) {
        if (event->type() == QEvent::ContextMenu) {
            auto *ce = static_cast<QContextMenuEvent*>(event);
            QPoint  epos  = ce->pos();                                     // viewport-local
            QPoint  gpos  = ui->exuTreeView->viewport()->mapToGlobal(epos);
            QModelIndex proxyIdx = ui->exuTreeView->indexAt(epos);
            if (!proxyIdx.isValid())
                return true;

            QModelIndex srcIdx = _exuFilterProxyModel->mapToSource(proxyIdx);

            if (srcIdx.parent().isValid())
                return true;

            QMenu context_menu;
            QAction *seq_action = context_menu.addAction("Sequence");
            QAction *res_action = context_menu.addAction("Resources");

            connect(seq_action, &QAction::triggered, [srcIdx, this](){
                emit sequenceClicked(srcIdx.row());
            });

            connect(res_action, &QAction::triggered, [srcIdx, this](){
                emit resourcesClicked(srcIdx.row());
            });

            context_menu.exec(gpos);
            return true;
        }
    }
    return QMainWindow::eventFilter(target, event);
}

void MainWindow::showSequenceDiagram(int row){
    const std::shared_ptr<prova::execution_unit>& unit = _exuModel->unit(row);
    std::filesystem::path image_path{std::format("{}.svg", row)};
    try{
        unit->render_svg(_plantumlJarPath.toStdString(), image_path);
    } catch (const std::exception& ex) {
        QMessageBox::critical(this,
            QString::fromStdString("Failed to run plantuml"),
            QString::fromStdString("Filed to execute plantuml with error %1").arg(QString::fromStdString(ex.what())),
            QMessageBox::Ok
        );
        return;
    }
    std::cout << "Rendered " << image_path << std::endl;

    // Create a new SVG Widget
    QSvgWidget* svgWidget = new QSvgWidget(QString::fromStdString(image_path.string()));

    // Get the intrinsic size of the SVG document
    QSize svgSize = svgWidget->renderer()->defaultSize();

    // Set the SVG Widget to its intrinsic size
    svgWidget->setFixedSize(svgSize);

    // Create a scroll area to contain the SVG widget
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(svgWidget);
    scrollArea->setWidgetResizable(false); // Important to avoid resizing the SVG widget
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Create a subwindow and set its widget to the scroll area
    QMdiSubWindow* subWindow = ui->mdiArea->addSubWindow(scrollArea);
    QSize mdiAreaSize = ui->mdiArea->size();
    QSize windowSize = svgSize + QSize(20, 20); // Add some margins or adjust as needed
    if (windowSize.width() > mdiAreaSize.width() || windowSize.height() > mdiAreaSize.height()) {
        windowSize = QSize(qMin(windowSize.width(), mdiAreaSize.width()), qMin(windowSize.height(), mdiAreaSize.height()) ); // If the image is larger, fit the window to the MDI area
    }
    subWindow->setWindowTitle(QString::fromStdString(std::format("ExU {}", row)));
    subWindow->resize(windowSize);

    subWindow->show();
}

void MainWindow::showResources(int row){
    const std::shared_ptr<prova::execution_unit>& unit = _exuModel->unit(row);

    ExUResourceChartViewer* viewer = new ExUResourceChartViewer{unit};
    viewer->show();
}

void MainWindow::showSettingsDialog(){
    SettingsDialog* dialog = new SettingsDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose); // to prevent memory leak
    dialog->show();
}

void MainWindow::filterProcess(const QString& processName){
    _exuFilterProxyModel->setExeFilter(processName);
}

void MainWindow::unpackPlantUmlJar(){
    QString jarResourcePath(":/jar/plantuml.jar");
    QTemporaryFile tempJar(QDir::tempPath() + "/plantuml.jar");
    tempJar.setAutoRemove(false);  // Optional: remove manually after process exits

    if (tempJar.open()) {
        QFile jarInResource(jarResourcePath);
        if (jarInResource.open(QIODevice::ReadOnly)) {
            tempJar.write(jarInResource.readAll());
            tempJar.flush();
            jarInResource.close();
        } else {
            qWarning() << "Failed to open resource JAR";
            return;
        }
        tempJar.close();
    } else {
        qWarning() << "Failed to create temp JAR";
        return;
    }

    _plantumlJarPath = tempJar.fileName();
}
