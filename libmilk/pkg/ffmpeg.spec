Name:		ffmpeg
Version:	3.0.git
Release:	1%{?dist}
Summary:	ffmpeg
URL:		https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey
Group:		Development/Libraries
License:	GPL

BuildRequires:	gcc-c++ >= 4.4.6 yasm
Prefix:		/usr

%description

%build
	mkdir -p %{?_sourcedir}/%{?name}
	cd %{?_sourcedir}/%{?name}
	./configure --disable-debug --disable-static --enable-shared --disable-doc --prefix=%{?_builddir}/%{?name}
	make
	make install
	cd %{?_builddir}/%{?name}/lib
	strip *.so
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
	%{?_includedir}/*
	%{?_libdir}/*.so.*
	%{?_datarootdir}/*
	%{_libdir}/pkgconfig/*.pc
