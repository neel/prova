cmake_minimum_required(VERSION 3.16)

project(provaui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Widgets Svg SvgWidgets REQUIRED)
qt_standard_project_setup()

qt_add_executable(prova-ui
    main.cpp
    mainwindow.h mainwindow.cpp mainwindow.ui
    exumodel.h exumodel.cpp
)

target_link_libraries(prova-ui PRIVATE Qt6::Widgets Qt6::Svg Qt6::SvgWidgets prova-tools)

set_target_properties(prova-ui PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
