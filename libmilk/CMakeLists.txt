#定义工程名及版本
project(milk)
set(version 1.1.129)
set(soversion 1.1)

cmake_minimum_required (VERSION 3.0) 
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)
set(CMAKE_USE_RELATIVE_PATHS ON)

#导入一此需要用到的工具包
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(FindPkgConfig)

#定义安装目录
set(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local)

#定制编译选项
set(CMAKE_BUILD_TYPE Release)
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb -D_DEBUG -fstack-protector")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

add_compile_options(-Wall)

if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
	add_compile_options(-Wno-deprecated)
	add_compile_options(-Wno-switch)
endif()


option(shared "build dynamic library" ON)
option(static "build static library" OFF)
option(minimal "without third party" OFF)
option(alone "build without other dynamic library" OFF)

message("optional:CMAKE_BUILD_TYPE=" ${CMAKE_BUILD_TYPE})
message("optional:shared=" ${shared})
message("optional:static=" ${static})
message("optional:minimal=" ${minimal})
message("optional:alone=" ${alone})

#搜索源代码
aux_source_directory(src src_cpps)

#搜索头文件
file(GLOB src_hs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.h)
include_directories(src)

aux_source_directory(src/linux src_cpps)
include_directories(src/linux)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

if(NOT minimal)
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
	list(REMOVE_ITEM src_cpps src/linux/script_lua.cpp)
	list(REMOVE_ITEM src_hs src/script_lua.h)
endif()

if(YAML_FOUND)
	list(APPEND LIBS_DEPENDS ${YAML_LDFLAGS})
	list(APPEND INCS_DEPENDS ${YAML_CFLAGS})
	list(APPEND CONFIG_MACROS YAML_FOUND)
else()
	list(REMOVE_ITEM src_cpps src/linux/yaml.cpp)
	list(REMOVE_ITEM src_hs src/yam1.h)
endif()

if(JS17_FOUND)
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

if(TINYXML2_FOUND)
	list(APPEND LIBS_DEPENDS -ltinyxml2)
	list(APPEND CONFIG_MACROS TINYXML2_FOUND)
	list(REMOVE_ITEM src_cpps src/linux/xml.cpp)
elseif(TINYXML_FOUND)
	list(APPEND LIBS_DEPENDS -ltinyxml)
	list(APPEND CONFIG_MACROS TINYXML_FOUND)
	list(REMOVE_ITEM src_cpps src/linux/xml2.cpp)
else()
	list(REMOVE_ITEM src_hs src/xml.h)
	list(REMOVE_ITEM src_cpps src/linux/xml2.cpp)
	list(REMOVE_ITEM src_cpps src/linux/xml.cpp)
endif()








# 根据外部依赖的存在与定义一些宏

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



if(shared)
	add_library(${PROJECT_NAME} SHARED ${src_cpps} ${src_hs})
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
	set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${version} SOVERSION ${soversion})
	install(TARGETS ${PROJECT_NAME} DESTINATION lib64)
	if(alone)
		target_link_libraries(${PROJECT_NAME} -static-libstdc++ -static-libgcc)
	endif()
endif()

if(static)
	add_compile_options(-fPIC) 
	add_library(${PROJECT_NAME}_static STATIC ${src_cpps} ${src_hs})
	install(TARGETS ${PROJECT_NAME}_static DESTINATION lib64)
	if(alone)
		target_link_libraries(${PROJECT_NAME}_static -static)
	endif()
endif()


#target_link_libraries(${PROJECT_NAME} -lpthread)
#target_link_libraries(${PROJECT_NAME} -lrt)
#target_link_libraries(${PROJECT_NAME} -ldl)
#target_link_libraries(${PROJECT_NAME} ${CMAKE_LIBS_DEPENDS})

#定义安装目标
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pkg/lib${PROJECT_NAME}.pc DESTINATION lib64/pkgconfig)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pkg/lib${PROJECT_NAME}_gdb.py DESTINATION lib64/pkgconfig)
install(FILES ${src_hs} DESTINATION include/libmilk)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/config.h DESTINATION include/libmilk)

