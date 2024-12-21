cmake_minimum_required(VERSION 3.16)

project(REMOTE_CONTROL_APP VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Bluetooth Core Gui Quick)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appREMOTE_CONTROL_APP
    main.cpp
)

qt_add_qml_module(appREMOTE_CONTROL_APP
    URI REMOTE_CONTROL_APP
    VERSION 1.0
    QML_FILES
        Main.qml
        RESOURCES android/AndroidManifest.xml android/build.gradle android/res/values/libs.xml android/res/xml/qtprovider_paths.xml
        QML_FILES JoyStick.qml
        QML_FILES
SOURCES characteristicinfo.cpp device.cpp deviceinfo.cpp main.cpp serviceinfo.cpp characteristicinfo.h device.h deviceinfo.h serviceinfo.h
QML_FILES Characteristics.qml Devices.qml Dialog.qml Header.qml Label.qml  Menu.qml Services.qml
RESOURCES assets/busy_dark.png
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appREMOTE_CONTROL_APP PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appREMOTE_CONTROL_APP
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appREMOTE_CONTROL_APP PUBLIC
    Qt6::Bluetooth
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS appREMOTE_CONTROL_APP
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
