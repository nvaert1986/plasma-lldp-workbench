# SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
# SPDX-License-Identifier: GPL-3.0-or-later

ecm_add_test(neighborstest.cpp ../src/neighbors.cpp ../src/interfacemodel.cpp
    TEST_NAME neighborstest
    LINK_LIBRARIES Qt6::Test Qt6::Qml KF6::I18n
)
target_include_directories(neighborstest PRIVATE ../src)
target_compile_definitions(neighborstest PRIVATE TRANSLATION_DOMAIN="plasma-lldp-workbench")

ecm_add_test(settingstest.cpp ../src/lldpsettings.cpp ../src/daemonoptions.cpp
    TEST_NAME settingstest
    LINK_LIBRARIES Qt6::Test KF6::I18n PkgConfig::LLDPCTL
)
target_include_directories(settingstest PRIVATE ../src)
target_compile_definitions(settingstest PRIVATE TRANSLATION_DOMAIN="plasma-lldp-workbench")
