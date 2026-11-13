# SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
# SPDX-License-Identifier: GPL-3.0-or-later

# Root helper for what lldpd only accepts from root: its startup options, the permanent
# configuration in /etc/lldpd.d and restarting it
add_executable(plasma-lldp-workbench-helper helper.cpp ../daemonoptions.cpp)
target_link_libraries(plasma-lldp-workbench-helper PRIVATE Qt6::Core KF6::AuthCore)

install(TARGETS plasma-lldp-workbench-helper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})
kauth_install_helper_files(plasma-lldp-workbench-helper io.github.nvaert1986.lldpworkbench root)
kauth_install_actions(io.github.nvaert1986.lldpworkbench io.github.nvaert1986.lldpworkbench.actions)
