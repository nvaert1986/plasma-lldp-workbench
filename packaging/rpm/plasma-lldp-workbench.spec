# SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
# SPDX-License-Identifier: GPL-3.0-or-later
# Build: rpmbuild -ba --define "_sourcedir $PWD" plasma-lldp-workbench.spec

Name:           plasma-lldp-workbench
Version:        0.1.0
Release:        1%{?dist}
Summary:        Show and control LLDP on KDE Plasma
License:        GPL-3.0-or-later
URL:            https://github.com/nvaert1986/plasma-lldp-workbench
Source0:        %{url}/archive/refs/tags/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  extra-cmake-modules
BuildRequires:  gcc-c++
BuildRequires:  kf6-kauth-devel
BuildRequires:  kf6-kconfig-devel
BuildRequires:  kf6-kcoreaddons-devel
BuildRequires:  kf6-kdbusaddons-devel
BuildRequires:  kf6-ki18n-devel
BuildRequires:  kf6-kirigami-addons-devel
BuildRequires:  kf6-knotifications-devel
BuildRequires:  kf6-kstatusnotifieritem-devel
BuildRequires:  kf6-kwindowsystem-devel
BuildRequires:  lldpd-devel
BuildRequires:  pkgconfig
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel

Requires:       lldpd
# QML modules are loaded at run time, so rpm cannot find them by itself
Requires:       kf6-kirigami
Requires:       kf6-kirigami-addons
Requires:       kf6-qqc2-desktop-style
Requires:       qt6-qtdeclarative
# Asks for the password when settings are saved or lldpd restarts
Recommends:     polkit

%description
Shows the switch, port and VLAN this computer is connected to, from what
neighbors announce over LLDP and LLDP-MED, with a tray icon and notifications.
It also controls what this computer sends: identity, interfaces, timing,
LLDP-MED device class, voice and video policies, location, Power over Ethernet
and custom TLVs, with profiles such as an IP phone or an access point to test
switch and VLAN policies. Experimental. lldpd does the sending and receiving.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake -DBUILD_TESTING=OFF
%cmake_build

%install
%cmake_install

%files
%license LICENSES/GPL-3.0-or-later.txt
%doc README.md
%{_bindir}/plasma-lldp-workbench
%{_libexecdir}/kf6/kauth/plasma-lldp-workbench-helper
%{_datadir}/applications/io.github.nvaert1986.lldpworkbench.desktop
%{_datadir}/dbus-1/system.d/io.github.nvaert1986.lldpworkbench.conf
%{_datadir}/dbus-1/system-services/io.github.nvaert1986.lldpworkbench.service
%{_datadir}/knotifications6/plasma-lldp-workbench.notifyrc
%{_datadir}/polkit-1/actions/io.github.nvaert1986.lldpworkbench.policy

%changelog
* Fri Nov 13 2026 nvaert1986 <25772781+nvaert1986@users.noreply.github.com> - 0.1.0-1
- First release: LLDP neighbors with tray icon and notifications, and control
  over everything lldpd sends, with profiles.
