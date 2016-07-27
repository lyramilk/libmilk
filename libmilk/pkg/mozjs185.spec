Name:		js
Version:	1.8.5
Release:	1%{?dist}
Summary:	JavaScript interpreter and libraries
URL:		https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey
Group:		Development/Languages
License:	MPL

BuildRequires:	gcc-c++ >= 4.4.6 nspr-devel autoconf213 zip
Requires:	nspr
Prefix:		/

%description

%build
	mkdir -p %{?_sourcedir}/%{?name}
	cd %{?_sourcedir}/%{?name}/js/src/
	autoconf-2.13
	mkdir -p build_OPT.OBJ
	cd build_OPT.OBJ/
	#../configure --enable-jemalloc --enable-release --enable-optimize=-O2 --disable-tests --enable-threadsafe --with-system-nspr --prefix=%{?_builddir}/%{?name}
	#../configure --enable-jemalloc --enable-debug --disable-tests --enable-threadsafe --with-system-nspr --prefix=%{?_builddir}/%{?name}
	../configure --enable-jemalloc --enable-gczeal --disable-tests --prefix=%{?_builddir}/%{?name}
	make
	make install
	cd %{?_builddir}/%{?name}/lib
	#strip *.a *.so
	tree %{?_builddir}/%{?name}
%install
	rm %{?buildroot} -rf
	mkdir -p %{?buildroot}
	mv %{?_builddir}/%{?name} %{?buildroot}%{?_usr}
	mv %{?buildroot}%{?_usr}/lib %{?buildroot}%{?_libdir}
	unlink %{?buildroot}%{?_libdir}/libmozjs185.so
	unlink %{?buildroot}%{?_libdir}/libmozjs185.so.1.0
	cd %{?buildroot}%{?_libdir}
	ln libmozjs185.so.1.0.0 libmozjs185.so.1.0 -s
	ln libmozjs185.so.1.0 libmozjs185.so -s
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
	%{?_libdir}/*.so.*

#更新日志
%changelog
	*Thu Sep 17 2015 打包
	-打包

#libmilk-devel包
%package devel 
Summary:	Development files for %{?name} 
Group:		Development/Libraries
Requires:       %{name} = %{?version}
Prefix:		/
%description devel

%files devel
	%defattr(-,root,root,-)
	%{?_libdir}/*.a
	%{?_libdir}/*.so
	%{?_includedir}/js/*
	%{_libdir}/pkgconfig/*.pc
