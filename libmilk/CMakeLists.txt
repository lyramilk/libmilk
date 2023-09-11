#定义工程名及版本
project(milk)
set(version 1.1.127)
set(soversion 1.1)

cmake_minimum_required (VERSION 3.0) 
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)
set(CMAKE_USE_RELATIVE_PATHS ON)
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)

#是否开启内存泄露测试
set(MEMLEAKTEST 0)

#导入一此需要用到的工具包
include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckIncludeFileCXX)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckCSourceCompiles)
include(FindPkgConfig)
include(UsePkgConfig)
enable_testing()

#定制编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -Wall -fPIC -Bsymbolic")

if(release)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
	set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -O2 ")
	message("Build as release")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -D_DEBUG -fstack-protector")
	set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g -fstack-protector ")
	message("Build as debug")
endif()

if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-switch")
endif()

#定义安装目录
set(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local)

#定义输出目标的类型
if(NOT share_type)
	set(share_type SHARED)
endif()

#搜索源代码
aux_source_directory(src src_cpps)

#搜索头文件
file(GLOB src_hs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.h)

include_directories(src)

if(UNIX)
	aux_source_directory(src/linux src_cpps)
	include_directories(src/linux)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

## 搜索可选的外部依赖
if(NOT minimal)
	pkg_check_modules(OPENSSL openssl)
	pkg_check_modules(LZ4 liblz4)
	pkg_check_modules(LUAJIT luajit)
	pkg_check_modules(YAML yaml-0.1)
	pkg_check_modules(JS52 mozjs-52)
	if(NOT JS52_FOUND)
		pkg_check_modules(JS17 mozjs-17.0)
	endif()
	#check_include_file_cxx(tinyxml.h		Z_HAVE_TINYXML)
	check_include_file_cxx(tinyxml2.h		Z_HAVE_TINYXML2)
	check_include_file_cxx(yaml.h		Z_HAVE_YAML)
endif()

#check_include_file_cxx(unordered_map		Z_HAVE_UNORDEREDMAP)
check_include_file_cxx(tr1/unordered_map	Z_HAVE_TR1_UNORDEREDMAP)

if(OPENSSL_FOUND AND NOT minimal)
	list(APPEND BUILD_DEPENDS openssl-devel)
	list(APPEND RUN_DEPENDS openssl)
	list(APPEND LIBS_DEPENDS ${OPENSSL_LDFLAGS})
	list(APPEND INCS_DEPENDS ${OPENSSL_CFLAGS})
	list(APPEND CONFIG_MACROS OPENSSL_FOUND)
endif()

if(LZ4_FOUND AND NOT minimal)
	list(APPEND BUILD_DEPENDS lz4-devel)
	list(APPEND RUN_DEPENDS lz4)
	list(APPEND LIBS_DEPENDS ${LZ4_LDFLAGS})
	list(APPEND INCS_DEPENDS ${LZ4_CFLAGS})
	list(APPEND CONFIG_MACROS LZ4_FOUND)
endif()

if(LUAJIT_FOUND AND NOT minimal)
	list(APPEND BUILD_DEPENDS luajit-devel)
	list(APPEND RUN_DEPENDS luajit)
	list(APPEND LIBS_DEPENDS ${LUAJIT_LDFLAGS})
	list(APPEND INCS_DEPENDS ${LUAJIT_CFLAGS})
	list(APPEND CONFIG_MACROS LUAJIT_FOUND)
else()
	list(REMOVE_ITEM src_cpps src/linux/script_lua.cpp)
	list(REMOVE_ITEM src_hs src/script_lua.h)
endif()

if(YAML_FOUND AND NOT minimal)
	list(APPEND BUILD_DEPENDS libyaml-devel)
	list(APPEND RUN_DEPENDS libyaml)
	list(APPEND LIBS_DEPENDS ${YAML_LDFLAGS})
	list(APPEND INCS_DEPENDS ${YAML_CFLAGS})
	list(APPEND CONFIG_MACROS YAML_FOUND)
elseif(Z_HAVE_YAML AND NOT minimal)
	list(APPEND BUILD_DEPENDS libyaml-devel)
	list(APPEND RUN_DEPENDS libyaml)
	list(APPEND LIBS_DEPENDS -lyaml)
	list(APPEND CONFIG_MACROS Z_HAVE_YAML)
else()
	list(REMOVE_ITEM src_cpps src/linux/yaml.cpp)
	list(REMOVE_ITEM src_hs src/yam1.h)
endif()

if(JS52_FOUND AND NOT minimal)
	list(APPEND BUILD_DEPENDS mozjs52-devel)
	list(APPEND RUN_DEPENDS mozjs52)
	list(APPEND LIBS_DEPENDS ${JS52_LDFLAGS})
	list(APPEND INCS_DEPENDS ${JS52_CFLAGS})
	list(APPEND CONFIG_MACROS JS52_FOUND)
	list(REMOVE_ITEM src_cpps src/linux/script_js17.cpp)
elseif(JS17_FOUND AND NOT minimal)
	list(APPEND BUILD_DEPENDS mozjs17-devel)
	list(APPEND RUN_DEPENDS mozjs17)
	list(APPEND LIBS_DEPENDS ${JS17_LDFLAGS})
	list(APPEND INCS_DEPENDS ${JS17_CFLAGS})
	list(APPEND CONFIG_MACROS JS17_FOUND)
	list(REMOVE_ITEM src_cpps src/linux/script_js52.cpp)
else()
	list(REMOVE_ITEM src_hs src/script_js.h)
	list(REMOVE_ITEM src_cpps src/linux/script_js17.cpp)
	list(REMOVE_ITEM src_cpps src/linux/script_js52.cpp)
endif()

if(Z_HAVE_UNORDEREDMAP)
	list(APPEND CONFIG_MACROS Z_HAVE_UNORDEREDMAP)
elseif(Z_HAVE_TR1_UNORDEREDMAP)
	list(APPEND CONFIG_MACROS Z_HAVE_TR1_UNORDEREDMAP)
endif()

if(Z_HAVE_TINYXML AND NOT minimal)
	list(APPEND BUILD_DEPENDS tinyxml-devel)
	list(APPEND RUN_DEPENDS tinyxml)
	list(APPEND LIBS_DEPENDS -ltinyxml)
	list(APPEND CONFIG_MACROS Z_HAVE_TINYXML)
	list(REMOVE_ITEM src_cpps src/linux/xml2.cpp)
elseif(Z_HAVE_TINYXML2 AND NOT minimal)
	list(APPEND BUILD_DEPENDS tinyxml2-devel)
	list(APPEND RUN_DEPENDS tinyxml2)
	list(APPEND LIBS_DEPENDS -ltinyxml2)
	list(APPEND CONFIG_MACROS Z_HAVE_TINYXML2)
	list(REMOVE_ITEM src_cpps src/linux/xml.cpp)
else()
	list(REMOVE_ITEM src_hs src/xml.h)
	list(REMOVE_ITEM src_cpps src/linux/xml2.cpp)
	list(REMOVE_ITEM src_cpps src/linux/xml.cpp)
endif()

if(UNIX)
	set(LIBRARIES -lpthread)
else()
	message(FATAL_ERROR "libmilk is works on Linux only!!!")
endif()

#根据外部依赖的存在与定义一些宏
foreach(v ${BUILD_DEPENDS})
	set(CMAKE_BUILD_DEPENDS "${CMAKE_BUILD_DEPENDS} ${v}")
endforeach()

foreach(v ${RUN_DEPENDS})
	set(CMAKE_RUN_DEPENDS "${CMAKE_RUN_DEPENDS} ${v}")
endforeach()

foreach(v ${INCS_DEPENDS})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${v}")
endforeach()

set(CMAKE_LIBS_DEPENDS ${LIBS_DEPENDS})

foreach(v ${CONFIG_MACROS})
	set(CMAKE_CONFIG_MACROS "${CMAKE_CONFIG_MACROS}#define ${v}\n")
endforeach()

#修改配置文件
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pkg/libmilk.spec.in ${CMAKE_CURRENT_SOURCE_DIR}/pkg/libmilk.spec)

#目标动态库
string(COMPARE EQUAL ${share_type} "BOTH" PROJECT_LIBRARY_BOTH)

#拷贝文件
file(COPY ${src_hs} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)

#target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++)
if(PROJECT_LIBRARY_BOTH)
	add_library(${PROJECT_NAME} SHARED ${src_cpps} ${src_hs})
	add_library(${PROJECT_NAME}_static STATIC ${src_cpps} ${src_hs})
else()
	add_library(${PROJECT_NAME} ${share_type} ${src_cpps} ${src_hs})
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${version} SOVERSION ${soversion})
if(UNIX)
	 target_link_libraries(${PROJECT_NAME} -lpthread  -lrt -ldl ${CMAKE_LIBS_DEPENDS})
else()
	message(FATAL_ERROR "libmilk is works on Linux only!!!")
endif()

#定义安装目标
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pkg/lib${PROJECT_NAME}.pc DESTINATION lib64/pkgconfig)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pkg/lib${PROJECT_NAME}_gdb.py DESTINATION lib64/pkgconfig)
install(FILES ${src_hs} DESTINATION include/libmilk)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/config.h DESTINATION include/libmilk)
install(TARGETS ${PROJECT_NAME} DESTINATION lib64)

if(PROJECT_LIBRARY_BOTH)
	install(TARGETS ${PROJECT_NAME}_static DESTINATION ${prefix})
	target_link_libraries(${PROJECT_NAME}_static -lpthread -lrt -ldl ${CMAKE_LIBS_DEPENDS})
endif()

enable_testing()
ADD_TEST(NAME test_add_2_3 COMMAND add 2 3)



file(GLOB src_test RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} testsuite/*.cc)
foreach(var ${src_test})
	string(REGEX REPLACE "testsuite/(.*).cc" "\\1" var2 ${var})
	add_executable(${var2} ${var})
	target_link_libraries(${var2} ${PROJECT_NAME})
	add_test(NAME ${var2} COMMAND ${unittest} ./${var2})
endforeach()

