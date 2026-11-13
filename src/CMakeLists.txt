# SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
# SPDX-License-Identifier: GPL-3.0-or-later

add_executable(plasma-lldp-workbench
    main.cpp
    app.cpp
    backend.cpp
    interfacemodel.cpp
    lldpclient.cpp
    lldpwatcher.cpp
    neighbors.cpp
    lldpsettings.cpp
    daemonoptions.cpp
    settingseditor.cpp
)
target_compile_definitions(plasma-lldp-workbench PRIVATE TRANSLATION_DOMAIN="plasma-lldp-workbench")
qt_policy(SET QTP0004 NEW)
qt_add_qml_module(plasma-lldp-workbench
    URI io.github.nvaert1986.lldpworkbench
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/Main.qml
        qml/NeighborsPage.qml
        qml/InterfaceCard.qml
        qml/NeighborView.qml
        qml/settings/Labels.js
        qml/settings/Paths.js
        qml/settings/PendingBar.qml
        qml/settings/SendingWarning.qml
        qml/settings/SettingChoice.qml
        qml/settings/SettingNumber.qml
        qml/settings/SettingSwitch.qml
        qml/settings/SettingText.qml
        qml/settings/ProfilesPage.qml
        qml/settings/IdentityPage.qml
        qml/settings/InterfacesPage.qml
        qml/settings/TimingPage.qml
        qml/settings/MedPage.qml
        qml/settings/LocationPage.qml
        qml/settings/PowerPage.qml
        qml/settings/CustomTlvPage.qml
        qml/settings/StartupPage.qml
        qml/settings/SavePage.qml
)
target_link_libraries(plasma-lldp-workbench PRIVATE
    PkgConfig::LLDPCTL
    KirigamiAddonsStatefulApp
    KF6::AuthCore
    KF6::ConfigCore
    KF6::ConfigGui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    KF6::CoreAddons
    KF6::DBusAddons
    KF6::I18n
    KF6::I18nQml
    KF6::Notifications
    KF6::StatusNotifierItem
    KF6::WindowSystem
)

install(TARGETS plasma-lldp-workbench ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(helper)
