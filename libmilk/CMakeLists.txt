cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0048 NEW)
#定义工程名及版本
project(milk VERSION 1.1.130)
set(soversion 1.1)

# 设置环境变量*
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)
set(CMAKE_USE_RELATIVE_PATHS ON)

option(shared "build dynamic library" ON)
option(static "build static library" OFF)
option(minimal "without third party" OFF)
option(alone "build without other dynamic library" OFF)

if(NOT DEFINED ${PROJECT_NAME}-shared)
	set(${PROJECT_NAME}-shared ${shared})
endif()
if(NOT DEFINED ${PROJECT_NAME}-static)
	set(${PROJECT_NAME}-static ${static})
endif()
if(NOT DEFINED ${PROJECT_NAME}-minimal)
	set(${PROJECT_NAME}-minimal ${minimal})
endif()
if(NOT DEFINED ${PROJECT_NAME}-alone)
	set(${PROJECT_NAME}-alone ${alone})
endif()

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local)

message("optional:CMAKE_BUILD_TYPE=" ${CMAKE_BUILD_TYPE})
message("optional:${PROJECT_NAME}-shared=" ${${PROJECT_NAME}-shared})
message("optional:${PROJECT_NAME}-static=" ${${PROJECT_NAME}-static})
message("optional:${PROJECT_NAME}-minimal=" ${${PROJECT_NAME}-minimal})
message("optional:${PROJECT_NAME}-alone=" ${${PROJECT_NAME}-alone})

# 搜索代码*
unset(codes)
unset(headers)

aux_source_directory(src codes)
file(GLOB headers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/libmilk/*.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include/libmilk)

# 包含公共代码*
include(libmilk.cmake)



#搜索依赖
if(NOT ${PROJECT_NAME}-minimal)
	pkg_check_modules(OPENSSL openssl)
	pkg_check_modules(OPENSSL openssl)
	pkg_check_modules(LZ4 liblz4)
	#pkg_check_modules(LUAJIT luajit)
	pkg_check_modules(YAML yaml-0.1)
	pkg_check_modules(JS17 mozjs-17.0)
	pkg_check_modules(TINYXML2 tinyxml2)
	pkg_check_modules(TINYXML tinyxml)
	# check_include_file_cxx(yaml.h    Z_HAVE_YAML)
endif()

check_include_file_cxx(unordered_map    Z_HAVE_UNORDEREDMAP)
check_include_file_cxx(tr1/unordered_map    Z_HAVE_TR1_UNORDEREDMAP)

if(OPENSSL_FOUND)
	list(APPEND LIBS_DEPENDS ${OPENSSL_LDFLAGS})
	list(APPEND INCS_DEPENDS ${OPENSSL_CFLAGS})
	list(APPEND CONFIG_MACROS OPENSSL_FOUND)
endif()

if(LZ4_FOUND)
	list(APPEND LIBS_DEPENDS ${LZ4_LDFLAGS})
	list(APPEND INCS_DEPENDS ${LZ4_CFLAGS})
	list(APPEND CONFIG_MACROS LZ4_FOUND)
endif()

if(LUAJIT_FOUND)
	list(APPEND LIBS_DEPENDS ${LUAJIT_LDFLAGS})
	list(APPEND INCS_DEPENDS ${LUAJIT_CFLAGS})
	list(APPEND CONFIG_MACROS LUAJIT_FOUND)
else()
	list(REMOVE_ITEM codes src/script_lua.cpp)
	list(REMOVE_ITEM headers include/libmilk/script_lua.h)
endif()

if(YAML_FOUND)
	list(APPEND LIBS_DEPENDS ${YAML_LDFLAGS})
	list(APPEND INCS_DEPENDS ${YAML_CFLAGS})
	list(APPEND CONFIG_MACROS YAML_FOUND)
else()
	list(REMOVE_ITEM codes src/yaml.cpp)
	list(REMOVE_ITEM headers include/libmilk/yam1.h)
endif()

if(JS17_FOUND)
	list(APPEND LIBS_DEPENDS ${JS17_LDFLAGS})
	list(APPEND INCS_DEPENDS ${JS17_CFLAGS})
	list(APPEND CONFIG_MACROS JS17_FOUND)
	list(REMOVE_ITEM codes src/script_js52.cpp)
else()
	list(REMOVE_ITEM headers include/libmilk/script_js.h)
	list(REMOVE_ITEM codes src/script_js17.cpp)
	list(REMOVE_ITEM codes src/script_js52.cpp)
endif()

if(Z_HAVE_UNORDEREDMAP)
	list(APPEND CONFIG_MACROS Z_HAVE_UNORDEREDMAP)
elseif(Z_HAVE_TR1_UNORDEREDMAP)
	list(APPEND CONFIG_MACROS Z_HAVE_TR1_UNORDEREDMAP)
endif()

if(TINYXML2_FOUND)
	list(APPEND LIBS_DEPENDS -ltinyxml2)
	list(APPEND CONFIG_MACROS TINYXML2_FOUND)
	list(REMOVE_ITEM codes src/xml.cpp)
elseif(TINYXML_FOUND)
	list(APPEND LIBS_DEPENDS -ltinyxml)
	list(APPEND CONFIG_MACROS TINYXML_FOUND)
	list(REMOVE_ITEM codes src/xml2.cpp)
else()
	list(REMOVE_ITEM headers include/libmilk/xml.h)
	list(REMOVE_ITEM codes src/xml2.cpp)
	list(REMOVE_ITEM codes src/xml.cpp)
endif()



#定义目标*
if(${PROJECT_NAME}-shared)
	libmilk_cmake_def_shared(${PROJECT_NAME} libmilk codes headers ${PROJECT_NAME}-alone)
endif()

if(${PROJECT_NAME}-static)
	libmilk_cmake_def_static(${PROJECT_NAME}_static libmilk codes headers ${PROJECT_NAME}-alone)
endif()
