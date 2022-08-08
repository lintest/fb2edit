#
# spec file for package fb2edit
#
# Copyright (c) 2012 Denis Kandrashin, Kyrill Detinov
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#

Name:           fb2edit
Version:        0.0.8
Release:        0
License:        GPL-3.0
Summary:        FB2 Files Editor
URL:            http://fb2edit.lintest.ru
Group:          Productivity/Text/Editors
Source0:        http://www.lintest.ru/pub/%{name}_%{version}.orig.tar.bz2
Source90:       %{name}_%{version}-squeeze1.debian.tar.gz
Source91:       %{name}_%{version}-squeeze1.dsc
Source92:       %{name}_%{version}-squeeze1_source.changes
BuildRequires:  cmake
BuildRequires:  desktop-file-utils
BuildRequires:  gcc-c++
BuildRequires:  hicolor-icon-theme
BuildRequires:  pkgconfig(QtCore) >= 4.6.0
BuildRequires:  pkgconfig(QtGui) >= 4.6.0
BuildRequires:  pkgconfig(QtNetwork) >= 4.6.0
BuildRequires:  pkgconfig(QtWebKit) >= 4.6.0
BuildRequires:  pkgconfig(QtXml) >= 4.6.0
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%if 0%{?suse_version}
BuildRequires:  update-desktop-files
%endif

%description
fb2edit is an application for editing FB2 e-books files.

Authors:
--------
    Denis Kandrashin <mail@lintest.ru>

%prep
%setup -q

%build
mkdir build
cd build
export CFLAGS="%{optflags}"
export CXXFLAGS="%{optflags}"
cmake .. -DCMAKE_INSTALL_PREFIX=%{_prefix}
make %{?_smp_mflags} VERBOSE=1

%install
pushd build
%make_install
popd

%if 0%{?fedora_version}
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop
%endif

%if 0%{?suse_version}
%suse_update_desktop_file %{name}
%endif

%if 0%{?fedora_version}
%post
/usr/bin/update-desktop-database &> /dev/null || :
/bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :

%postun
/usr/bin/update-desktop-database &> /dev/null || :
if [ $1 -eq 0 ] ; then
    /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null
    /usr/bin/gtk-update-icon-cache -f %{_datadir}/icons/hicolor &>/dev/null || :
fi

%posttrans
/usr/bin/gtk-update-icon-cache -f %{_datadir}/icons/hicolor &>/dev/null || :
%endif

%if 0%{?mandriva_version}
%post
%update_desktop_database
%update_icon_cache hicolor
%update_menus

%postun
%clean_desktop_database
%update_icon_cache hicolor
%clean_menus
%endif

%if 0%{?suse_version}
%post
%desktop_database_post
%icon_theme_cache_post

%postun
%desktop_database_postun
%icon_theme_cache_postun
%endif

%files
%defattr(-,root,root,-)
%doc AUTHORS LICENSE
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/*/%{name}.png
%{_datadir}/pixmaps/%{name}.png

%changelog
