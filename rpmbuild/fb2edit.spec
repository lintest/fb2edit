#
# spec file for package fb2edit
#
# Copyright (c) 2012 Denis Kandrashin, Kyrill Detinov
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#

Name:           fb2edit
Version:        0.0.1
Release:        0
License:        GPL-3.0
Summary:        FB2 Files Editor
Url:            http://fb2edit.lintest.ru
Group:          Productivity/Text/Editors
Source0:        fb2edit-0.0.1.tar.bz2
BuildRequires:  cmake
BuildRequires:  desktop-file-utils
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig(QtWebKit) >= 4.7.0
BuildRequires:  pkgconfig(libxml-2.0)
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%if 0%{?fedora_version}
BuildRequires:  qscintilla-devel
%endif

%if 0%{?mandriva_version}
BuildRequires:  qscintilla-qt4-devel
%endif

%if 0%{?suse_version}
BuildRequires:  libqscintilla-devel
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

%postun
/usr/bin/update-desktop-database &> /dev/null || :
%endif

%if 0%{?mandriva_version}
%post
%update_desktop_database
%update_menus

%postun
%clean_desktop_database
%clean_menus
%endif

%if 0%{?suse_version}
%post
%desktop_database_post

%postun
%desktop_database_postun
%endif

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.png

%changelog
