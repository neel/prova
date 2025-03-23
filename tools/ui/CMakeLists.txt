cmake_minimum_required(VERSION 3.16)

project(provaui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Core Widgets Network Quick QuickWidgets Svg SvgWidgets REQUIRED)
qt_standard_project_setup()

qt_add_resources(RESOURCES resources.qrc)

qt_add_executable(prova-ui
    main.cpp
    mainwindow.h mainwindow.cpp mainwindow.ui
    exumodel.h exumodel.cpp
    exuvulnerabilitiesviewer.h exuvulnerabilitiesviewer.cpp exuvulnerabilitiesviewer.ui
    ${RESOURCES}
    qmldir
    exuresourcechartviewer.h exuresourcechartviewer.cpp exuresourcechartviewer.ui
    sessionrect.h sessionrect.cpp
    sessionrectgroup.h sessionrectgroup.cpp
    exuwidget.h exuwidget.cpp
    exuscene.h exuscene.cpp
    cvelistmodel.h cvelistmodel.cpp
    exuvulnerabilitiesprogresswidget.h exuvulnerabilitiesprogresswidget.cpp exuvulnerabilitiesprogresswidget.ui
    exucvesearchprogressscrollarea.h exucvesearchprogressscrollarea.cpp exucvesearchprogressscrollarea.ui
)

target_link_libraries(prova-ui PRIVATE Qt6::Core Qt6::Widgets Qt6::Quick Qt6::QuickWidgets Qt6::Svg Qt6::SvgWidgets Qt6::Network prova-tools QJsonModelShared)

set_target_properties(prova-ui PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
