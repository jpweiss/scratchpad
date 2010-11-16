# -*- mode: sh; eval: (sh-set-shell "rpm") -*-

Name: msnek4k-driverd
Summary: A driver for the MS Natural Ergo Keyboard 4000
Version: 0.7.2
Release: 1
Source: http://sourceforge.net/projects/msnek4kdriverd/files/v%{version}/%{name}-%{version}.tgz
URL: http://sourceforge.net/projects/msnek4kdriverd
License: Artistic
Group: X11
Requires: boost >= 1.33.1
Requires: libXtst.so.6

BuildRoot: %{_tmppath}/%{name}-buildroot
BuildArchitectures: i386 i586 i686 x86_64 amd64


# Distro-Specific Requirements:
%if %{defined fedora}
BuildRequires: boost-devel >= 1.34.1
%if 0%{fedora} >= 13
BuildRequires: libXtst-devel
%else
BuildRequires: xorg-x11-proto-devel
%endif
%{define requirements_are_set 1}
%endif

%if %{defined mdkversion} || %{defined mandriva_version}
BuildRequires: devel(libboost_program_options) >= 1.35.0
BuildRequires: x11-proto-devel
%{define requirements_are_set 1}
%endif

%if %{defined suse_version}
BuildRequires: boost-devel >= 1.36.0
BuildRequires: xorg-x11-proto-devel
%{define requirements_are_set 1}
%endif

%if %{defined centos_version} || %{defined rhel_version}
BuildRequires: boost-devel >= 1.33.1
BuildRequires: xorg-x11-proto-devel
%{define requirements_are_set 1}
%endif

%if %{defined arklinux_version}
BuildRequires: boost-devel >= 1.35.0
BuildRequires: xorg-proto-devel
%{define requirements_are_set 1}
%endif

%if %{defined altlinux_version}
BuildRequires: boost-program_options-devel >= 1.33.1
BuildRequires: xorg-x11-proto-devel
%{define requirements_are_set 1}
%endif

%if %{defined pclinuxos_version}
BuildRequires: boost-program_options-devel >= 1.40.0
BuildRequires: x11-proto-devel
%{define requirements_are_set 1}
%endif

# The Defaults:
%if %{undefined requirements_are_set}
BuildRequires: boost-devel >= 1.36.0
BuildRequires: xorg-x11-proto-devel
%endif


%description
 There are 3 keys on the Microsoft Natural© Ergonomic Keyboard 4000 which
 the linux kernel cannot handle (yet):
    The "Spell" function key
    Zoom-Up
    Zoom-Down
 .
 This is a daemon which listens on a "/dev/input/event*" device for scancodes
 from those 3 keys and generates X11 keycodes for them using the XTest
 extension.  You can also map the Zoom keys to mouse-button events (say, to
 turn the Zoom-jog into a scroll-wheel).


%prep
rm -rf %{buildroot}/*

%setup
%define xconfdir %{_sysconfdir}/X11
%define xsessiondir %{xconfdir}/Xsession.d

%build
echo BuildRoot=%{buildroot}

%configure

%{__make}

%postun

%post

%install
%{__make} install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}/*

%files
    %defattr(-,root,root)
    %doc %{_docdir}/msnek4k-driverd/*
    %config %{_sysconfdir}/msnek4k_driverd.conf
    %config(noreplace) %{xconfdir}/msnek4k_driverd.xmodmap
    %{_bindir}/*
    %{xsessiondir}/90x11-msnek4k_driverd
#    %{_mandir}/*

%changelog
* Tue Nov 16 2010 John Weiss <jpwcandide@sourceforge.net> %{version}-%{release}
- Initial Version
