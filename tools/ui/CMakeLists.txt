cmake_minimum_required(VERSION 3.16)

project(provaui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Core Widgets Network Quick QuickWidgets Qml QuickControls2 Svg SvgWidgets REQUIRED)
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
    settingsdialog.h settingsdialog.cpp settingsdialog.ui
    exuvulnerabilitiesprogresswidget.h exuvulnerabilitiesprogresswidget.cpp exuvulnerabilitiesprogresswidget.ui
    colorhelper.h colorhelper.cpp
    cveproxymodel.h cveproxymodel.cpp
    directorytrie.h directorytrie.cpp
    keywordlistmodel.h keywordlistmodel.cpp
    exufilterproxymodel.h exufilterproxymodel.cpp
)

target_link_libraries(prova-ui PRIVATE Qt6::Core Qt6::Widgets Qt6::Quick Qt6::Qml Qt6::QuickWidgets Qt6::QuickControls2 Qt6::Svg Qt6::SvgWidgets Qt6::Network prova-tools QJsonModelStatic)
if (MSVC)
  target_compile_definitions(prova-ui PRIVATE NOMINMAX _WINSOCKAPI_ WIN32_LEAN_AND_MEAN)
  target_compile_options(prova-ui PRIVATE /FI${CMAKE_CURRENT_SOURCE_DIR}/win32_winsock_shim.hpp)
  target_compile_definitions(prova-ui PRIVATE $<$<PLATFORM_ID:Windows>:_WINSOCKAPI_> $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>)
endif()

if (WIN32)
  set_target_properties(prova-ui PROPERTIES WIN32_EXECUTABLE ON LINK_FLAGS "/SUBSYSTEM:CONSOLE")
elseif(APPLE)
  set_target_properties(prova-ui PROPERTIES MACOSX_BUNDLE ON)
endif()

include(GNUInstallDirs)

install(TARGETS prova-ui BUNDLE  DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

qt6_generate_deploy_app_script(
  TARGET         prova-ui
  OUTPUT_SCRIPT  deploy_prova_ui
  NO_UNSUPPORTED_PLATFORM_ERROR
  DEPLOY_TOOL_OPTIONS
    --qmldir=${CMAKE_CURRENT_SOURCE_DIR}
    --quickcontrols2
)

install(SCRIPT ${deploy_prova_ui})


if (WIN32)
  install(
      DIRECTORY      $<TARGET_RUNTIME_DLLS:prova-ui>
      DESTINATION    ${CMAKE_INSTALL_BINDIR}
      USE_SOURCE_PERMISSIONS
  )

  message(STATUS "Qt6_DIR: " ${Qt6_DIR})
  get_filename_component(QT_PREFIX "${Qt6_DIR}/../../.." ABSOLUTE)
  set(QT_STYLE_PLUGIN_DIR   "${QT_PREFIX}/plugins/styles")
  set(QT_QUICK_CONTROLS_DIR "${QT_PREFIX}/qml/QtQuick/Controls")

  install(
      DIRECTORY   "${QT_STYLE_PLUGIN_DIR}/"
      DESTINATION "plugins/styles"
      FILES_MATCHING PATTERN "qwindows*.dll"  # keep it small, copy only the style DLLs
  )

  install(
      DIRECTORY   "${QT_QUICK_CONTROLS_DIR}"
      DESTINATION "qml/QtQuick"          # <prefix>/qml/QtQuick/Controls/…
  )

  find_file(D3DCOMPILER d3dcompiler_47.dll
    PATHS "${QT_PREFIX}/bin"
    "$ENV{VCToolsRedistDir}/x64/Microsoft.VC143.CRT"
    NO_DEFAULT_PATH
  )

  if (D3DCOMPILER)
      install(FILES "${D3DCOMPILER}" DESTINATION "${CMAKE_INSTALL_BINDIR}")
  else()
      message(WARNING "d3dcompiler_47.dll not found — Qt Quick may fail at run-time")
  endif()

elseif(APPLE)

    set_target_properties(prova-ui PROPERTIES
        MACOSX_BUNDLE               TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.simula.provaui
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )

endif()

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "ProvaUI-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_DMG_CREATE_DEVICE false)
include(CPack)


