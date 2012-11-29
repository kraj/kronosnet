###############################################################################
###############################################################################
##
##  Copyright (C) 2010-2012 Red Hat, Inc.  All rights reserved.
##
##  This copyrighted material is made available to anyone wishing to use,
##  modify, copy, or redistribute it subject to the terms and conditions
##  of the GNU General Public License v.2 or higher
##
###############################################################################
###############################################################################

# keep around ready for later user
## global alphatag rc4

Name: kronosnetd
Summary: Multipoint-to-multipoint VPN daemon
Version: 0.1
Release: 1%{?alphatag:.%{alphatag}}%{?dist}
License: GPLv2+ and LGPLv2+
Group: System Environment/Base
URL: https://github.com/fabbione/kronosnet/
Source0: https://github.com/fabbione/kronosnet/archive/%{name}-%{version}.tar.xz
Requires(post): chkconfig, shadow-utils
Requires(preun): chkconfig, shadow-utils, initscripts

## Setup/build bits

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

# Build dependencies
BuildRequires: libqb-devel nss-devel

%prep
%setup -q -n %{name}-%{version}

%build
%{configure} --with-initddir=%{_sysconfdir}/rc.d/init.d/

make %{_smp_mflags}

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

# tree cleanup
# remove static libraries
find %{buildroot} -name "*.a" -exec rm {} \;
# remove libtools leftovers
find %{buildroot} -name "*.la" -exec rm {} \;
# remove systemd specific bits
find %{buildroot} -name "*.service" -exec rm {} \;
# remove docs
rm -rf %{buildroot}/usr/share/doc/kronosnetd

%clean
rm -rf %{buildroot}

## Runtime and subpackages section
%description -n kronosnetd
 The kronosnet daemon is a bridge between kronosnet switching engine
 and kernel network tap devices, to create and administer a
 distributed LAN over multipoint-to-multipoint VPNs.
 The daemon does a poor attempt to provide a configure UI similar
 to other known network devices/tools (Cisco, quagga).
 Beside looking horrific, it allows runtime changes and
 reconfiguration of the kronosnet(s) without daemon reload
 or service disruption.

%post -n kronosnetd
/sbin/chkconfig --add kronosnetd
/usr/sbin/groupadd --force --system kronosnetadm

%preun -n kronosnetd
if [ "$1" = 0 ]; then
	/sbin/service kronosnetd stop >/dev/null 2>&1
	/sbin/chkconfig --del kronosnetd
	groupdel kronosnetadm
fi

%files -n kronosnetd
%defattr(-,root,root,-)
%doc COPYING.* COPYRIGHT
%dir %{_sysconfdir}/kronosnet
%dir %{_sysconfdir}/kronosnet/*
%{_sysconfdir}/rc.d/init.d/kronosnetd
%{_sbindir}/*
%{_mandir}/man8/*

%package -n libtap1
Group: System Environment/Libraries
Summary: Simple userland wrapper around kernel tap devices

%description -n libtap1
 This is an over-engineered commodity library to manage a pool
 of tap devices and provides the basic
 pre-up.d/up.d/down.d/post-down.d infrastructure.

%files -n libtap1
%defattr(-,root,root,-)
%doc COPYING.* COPYRIGHT
%{_libdir}/libtap.so.*

%post -n libtap1 -p /sbin/ldconfig

%postun -n libtap1 -p /sbin/ldconfig

%package -n libtap1-devel
Group: Development/Libraries
Summary: Simple userland wrapper around kernel tap devices (developer files)
Requires: libtap1 = %{version}-%{release}
Requires: pkgconfig

%description -n libtap1-devel
 This is an over-engineered commodity library to manage a pool
 of tap devices and provides the basic
 pre-up.d/up.d/down.d/post-down.d infrastructure.

%files -n libtap1-devel
%defattr(-,root,root,-)
%doc COPYING.* COPYRIGHT
%{_libdir}/libtap.so
%{_includedir}/libtap.h
%{_libdir}/pkgconfig/libtap.pc

%package -n libknet0
Group: System Environment/Libraries
Summary: Kronosnet core switching implementation

%description -n libknet0
 The whole kronosnet core is implemented in this library.
 Please refer to the not-yet-existing documentation for further
 information.

%files -n libknet0
%defattr(-,root,root,-)
%doc COPYING.* COPYRIGHT
%{_libdir}/libknet.so.*

%post -n libknet0 -p /sbin/ldconfig

%postun -n libknet0 -p /sbin/ldconfig

%package -n libknet0-devel
Group: Development/Libraries
Summary: Kronosnet core switching implementation (developer files)
Requires: libknet0 = %{version}-%{release}
Requires: pkgconfig

%description -n libknet0-devel
 The whole kronosnet core is implemented in this library.
 Please refer to the not-yet-existing documentation for further
 information. 

%files -n libknet0-devel
%defattr(-,root,root,-)
%doc COPYING.* COPYRIGHT
%{_libdir}/libknet.so
%{_includedir}/libknet.h
%{_libdir}/pkgconfig/libknet.pc

%changelog
* Thu Nov 29 2012 Fabio M. Di Nitto <fabbione@kronosnet.org> - 0.1-1
- commodity package

