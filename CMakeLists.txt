# SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.25)

project(plasma-lldp-workbench VERSION 0.1.0 LANGUAGES CXX)

set(QT_MIN_VERSION "6.8.0")
set(KF_MIN_VERSION "6.13.0")
set(KDE_COMPILERSETTINGS_LEVEL "6.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMAddTests)
include(FeatureSummary)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2 Test)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS Auth Config CoreAddons DBusAddons I18n Notifications StatusNotifierItem WindowSystem)
find_package(KF6Kirigami ${KF_MIN_VERSION})
# Menu bar actions, shortcut editor, command bar and the About page
find_package(KF6KirigamiAddons 1.4.0 REQUIRED)
set_package_properties(KF6Kirigami PROPERTIES TYPE RUNTIME DESCRIPTION "QML components of the window")

# lldpd's client library: the daemon does the receiving and sending, the app talks to it
find_package(PkgConfig REQUIRED)
pkg_check_modules(LLDPCTL REQUIRED IMPORTED_TARGET lldpctl>=1.0)

# Settings that newer lldpd versions added; older ones (1.0.18 in Debian 13 and Fedora 45)
# build without them and the app hides them
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${LLDPCTL_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES ${LLDPCTL_LINK_LIBRARIES})
check_cxx_source_compiles("#include <lldpctl.h>
int main() { return lldpctl_k_port_vlan_advertise_pattern; }" HAVE_LLDPCTL_VLAN_PATTERN)
check_cxx_source_compiles("#include <lldpctl.h>
int main() { return lldpctl_k_config_lldp_portdescr_type; }" HAVE_LLDPCTL_PORTDESCR_TYPE)
# Without it a waiting watch cannot be stopped; the app then polls instead
check_cxx_source_compiles("#include <lldpctl.h>
int main() { lldpctl_watch_sync_unblock(nullptr); return 0; }" HAVE_LLDPCTL_WATCH_UNBLOCK)
unset(CMAKE_REQUIRED_INCLUDES)
unset(CMAKE_REQUIRED_LIBRARIES)
if(HAVE_LLDPCTL_VLAN_PATTERN)
    add_compile_definitions(HAVE_LLDPCTL_VLAN_PATTERN)
endif()
if(HAVE_LLDPCTL_PORTDESCR_TYPE)
    add_compile_definitions(HAVE_LLDPCTL_PORTDESCR_TYPE)
endif()
if(HAVE_LLDPCTL_WATCH_UNBLOCK)
    add_compile_definitions(HAVE_LLDPCTL_WATCH_UNBLOCK)
endif()

add_compile_definitions(LLDPWORKBENCH_VERSION="${PROJECT_VERSION}")

add_subdirectory(src)
add_subdirectory(data)

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
