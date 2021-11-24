Name:		mozjs17
Version:	17.0.0
Release:	1%{?dist}
Summary:	Spider Monkey
URL:		https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey
Group:		Development/Libraries
License:	MPL

BuildRequires:	gcc-c++ >= 4.4.6 nspr-devel autoconf213
Requires:	nspr
Prefix:		/usr

%description

%build
	mkdir -p %{?_sourcedir}/%{?name}
	cd %{?_sourcedir}/%{?name}/js/src/
	autoconf-2.13
	mkdir -p build_OPT.OBJ
	cd build_OPT.OBJ/
	export MOZJS_MAJOR_VERSION=17
	export MOZJS_MINOR_VERSION=0
	export MOZILLA_VERSION=17.0
	#../configure --enable-jemalloc --enable-strip --enable-release --enable-optimize=-O2 --disable-tests --enable-threadsafe --with-system-nspr --prefix=%{?_builddir}/%{?name}
	../configure --enable-strip --enable-release --enable-optimize=-O2 --disable-tests --enable-threadsafe --with-system-nspr --prefix=%{?_builddir}/%{?name}
	make
	make install
	cd %{?_builddir}/%{?name}/lib

%install
	rm %{?buildroot} -rf
	mkdir -p %{?buildroot}
	mv %{?_builddir}/%{?name} %{?buildroot}%{?_usr}
	mv %{?buildroot}%{?_usr}/lib %{?buildroot}%{?_libdir}
	sed -ri "s,prefix=.*,prefix=%{?_usr}," %{?buildroot}%{?_libdir}/pkgconfig/*.pc
	sed -ri "s,libdir=.*,libdir=%{?_libdir},"  %{?buildroot}%{?_libdir}/pkgconfig/*.pc
	sed -ri "s,includedir=.*,includedir=%{?_includedir},"  %{?buildroot}%{?_libdir}/pkgconfig/*.pc

#清理
%clean

#安装之前执行脚本
%pre

#安装之后执行脚本
%post

#卸载之前执行脚本
%preun

#卸载之前执行脚本
%postun

#需要文件
%files
	%defattr(-,root,root,-)
	%exclude %{?_bindir}
	%{?_libdir}/*.so

#libmilk-devel包
%package devel 
Summary:	Development files for %{?name} 
Group:		Development/Libraries
Requires:       %{name} = %{?version}
Prefix:		/usr
%description devel

%files devel
	%defattr(-,root,root,-)
	%{?_libdir}/*.a
	%{?_includedir}/js-17.0/*
	%{_libdir}/pkgconfig/*.pc
