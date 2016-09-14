#定义工程名及版本
project(milk)
set(version 1.1.16)
set(soversion 1.1)

cmake_minimum_required (VERSION 2.8.11) 
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

if(release)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O4 -DNDEBUG -Wall --std=c++0x ")
	set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -O4 -Wall ")
	message("Build as release")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall --std=c++0x -fstack-protector ")
	set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g -Wall -fstack-protector ")
	message("Build as debug")
endif()

#定义安装目录
if(NOT install_dir)
	set(install_dir ${CMAKE_CURRENT_SOURCE_DIR}/output)
endif()

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

## 搜索可选的外部依赖
if(UNIX)
	pkg_check_modules(JSON_C json-c)
	pkg_search_module(OPENSSL openssl)
	pkg_search_module(LZ4 liblz4)
	pkg_search_module(LUA lua)
	pkg_check_modules(JS17 mozjs-17.0)
	pkg_search_module(LEVELDB leveldb)
	#set(CMAKE_REQUIRED_INCLUDES /usr/include/)
	#check_include_file_cxx(unordered_map	Z_HAVE_UNORDEREDMAP)
	check_include_file_cxx(tr1/unordered_map	Z_HAVE_TR1_UNORDEREDMAP)
endif()

if(JSON_C_FOUND)
	list(APPEND BUILD_DEPENDS json-c-devel)
	list(APPEND RUN_DEPENDS json-c)
	list(APPEND LIBS_DEPENDS ${JSON_C_LDFLAGS})
	list(APPEND INCS_DEPENDS ${JSON_C_CFLAGS})
	list(APPEND CONFIG_MACROS JSON_C_FOUND)
endif()

if(OPENSSL_FOUND)
	list(APPEND BUILD_DEPENDS openssl-devel)
	list(APPEND RUN_DEPENDS openssl)
	list(APPEND LIBS_DEPENDS ${OPENSSL_LDFLAGS})
	list(APPEND INCS_DEPENDS ${OPENSSL_CFLAGS})
	list(APPEND CONFIG_MACROS OPENSSL_FOUND)
endif()

if(LZ4_FOUND)
	list(APPEND BUILD_DEPENDS lz4-devel)
	list(APPEND RUN_DEPENDS lz4)
	list(APPEND LIBS_DEPENDS ${LZ4_LDFLAGS})
	list(APPEND INCS_DEPENDS ${LZ4_CFLAGS})
	list(APPEND CONFIG_MACROS LZ4_FOUND)
endif()

if(LUA_FOUND)
	list(APPEND BUILD_DEPENDS lua-devel)
	list(APPEND RUN_DEPENDS lua)
	list(APPEND LIBS_DEPENDS ${LUA_LDFLAGS})
	list(APPEND INCS_DEPENDS ${LUA_CFLAGS})
	list(APPEND CONFIG_MACROS LUA_FOUND)
else()
	list(REMOVE_ITEM src_cpps src/script_lua.cpp)
	list(REMOVE_ITEM src_hs src/script_lua.h)
endif()

if(LEVELDB_FOUND)
	list(APPEND BUILD_DEPENDS leveldb-devel)
	list(APPEND RUN_DEPENDS leveldb)
	list(APPEND LIBS_DEPENDS ${LEVELDB_LDFLAGS})
	list(APPEND INCS_DEPENDS ${LEVELDB_CFLAGS})
	list(APPEND CONFIG_MACROS LEVELDB_FOUND)
endif()


if(JS17_FOUND)
	list(APPEND BUILD_DEPENDS mozjs17-devel)
	list(APPEND RUN_DEPENDS mozjs17)
	list(APPEND LIBS_DEPENDS ${JS17_LDFLAGS})
	list(APPEND INCS_DEPENDS ${JS17_CFLAGS})
	list(APPEND CONFIG_MACROS JS17_FOUND)
else()
	list(REMOVE_ITEM src_hs src/script_js.h)
	list(REMOVE_ITEM src_cpps src/script_js17.cpp)
endif()

if(Z_HAVE_UNORDEREDMAP)
	list(APPEND CONFIG_MACROS Z_HAVE_UNORDEREDMAP)
elseif(Z_HAVE_TR1_UNORDEREDMAP)
	list(APPEND CONFIG_MACROS Z_HAVE_TR1_UNORDEREDMAP)
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
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pkg/libmilk.spec.in ${CMAKE_CURRENT_SOURCE_DIR}/pkg/libmilk.spec)

#目标动态库
string(COMPARE EQUAL ${share_type} "BOTH" PROJECT_LIBRARY_BOTH)
if(PROJECT_LIBRARY_BOTH)
	add_library(${PROJECT_NAME} SHARED ${src_cpps} ${src_hs})
	add_library(${PROJECT_NAME}_static STATIC ${src_cpps} ${src_hs})
else()
	add_library(${PROJECT_NAME} ${share_type} ${src_cpps} ${src_hs})
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${version} SOVERSION 1.1)
if(UNIX)
	 target_link_libraries(${PROJECT_NAME} -lpthread  -lrt -ldl ${CMAKE_LIBS_DEPENDS})
else()
	message(FATAL_ERROR "libmilk is works on Linux only!!!")
endif()

#定义安装目标
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pkg/lib${PROJECT_NAME}.pc DESTINATION ${install_dir})
install(FILES ${src_hs} DESTINATION ${install_dir})
install(TARGETS ${PROJECT_NAME} DESTINATION ${install_dir})
if(PROJECT_LIBRARY_BOTH)
	install(TARGETS ${PROJECT_NAME}_static DESTINATION ${install_dir})
	target_link_libraries(${PROJECT_NAME}_static -lpthread -lrt -ldl ${CMAKE_LIBS_DEPENDS})
endif()

#单元测试，是否开启内存泄露测试
if(MEMLEAKTEST)
	set(unittest valgrind --tool=memcheck --leak-check=full)
else()
	set(unittest )
endif()

file(GLOB src_test RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} testsuite/*.cc)
foreach(var ${src_test})
	string(REGEX REPLACE "testsuite/(.*).cc" "\\1" var2 ${var})
	add_executable(${var2} ${var})
	target_link_libraries(${var2} ${PROJECT_NAME} -lhiredis)
	add_test(NAME ${var2} COMMAND ${unittest} ./${var2})
endforeach()

