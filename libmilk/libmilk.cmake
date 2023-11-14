include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(FindPkgConfig)


#定制编译选项
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb -D_DEBUG -fstack-protector")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

unset(BUILD_DEPENDS)
unset(RUN_DEPENDS)
unset(LIBS_DEPEND)
unset(INCS_DEPENDS)
unset(CONFIG_MACROS)

add_compile_options(-Wall)
add_compile_options(-fPIC) 

if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
	add_compile_options(-Wno-deprecated)
	add_compile_options(-Wno-switch)
endif()

if(NOT SUBMODULE_BIN)
	set(SUBMODULE_BIN ${PROJECT_BINARY_DIR}/submodule)
	set(SUBMODULE_INCLUDE ${PROJECT_BINARY_DIR}/include)
	include_directories(${SUBMODULE_INCLUDE})
endif()



macro(libmilk_cmake_init_env)
	set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)
	set(CMAKE_USE_RELATIVE_PATHS ON)
	
	option(shared "build shared library" ON)
	option(minimal "without third party" OFF)
	option(alone "build without other shared library" OFF)

	if(NOT DEFINED ${PROJECT_NAME}-shared)
		set(${PROJECT_NAME}-shared ${shared})
		set(shared ${shared})
	endif()

	if(NOT DEFINED ${PROJECT_NAME}-minimal)
		set(${PROJECT_NAME}-minimal ${minimal})
	endif()
	if(NOT DEFINED ${PROJECT_NAME}-alone)
		set(${PROJECT_NAME}-alone ${alone})
	endif()

	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE Release)
	endif()

	if(NOT DEFINED prefix)
		set(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local)
	else()
		set(CMAKE_INSTALL_PREFIX ${prefix})
	endif()

	message("optional:PROJECT_BINARY_DIR=" ${PROJECT_BINARY_DIR})
	message("optional:PROJECT_NAME=" ${PROJECT_NAME})
	message("optional:CMAKE_BUILD_TYPE=" ${CMAKE_BUILD_TYPE})
	message("optional:CMAKE_INSTALL_PREFIX=" ${CMAKE_INSTALL_PREFIX})
	message("optional:SUBMODULE=" ${SUBMODULE})
	message("optional:SUBMODULE_BIN=" ${SUBMODULE_BIN})
	message("optional:${PROJECT_NAME}-shared=" ${${PROJECT_NAME}-shared})
	message("optional:${PROJECT_NAME}-minimal=" ${${PROJECT_NAME}-minimal})
	message("optional:${PROJECT_NAME}-alone=" ${${PROJECT_NAME}-alone})
endmacro()








macro(libmilk_cmake_def libname isdynamic projname src_cpps src_hs alone)
	unset(CMAKE_BUILD_DEPENDS)
	unset(CMAKE_RUN_DEPENDS)
	unset(CMAKE_CXX_FLAGS)
	unset(CMAKE_LIBS_DEPENDS)
	unset(CMAKE_CONFIG_MACROS)
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

	#set(CMAKE_LIBS_DEPENDS ${LIBS_DEPENDS})

	foreach(v ${CONFIG_MACROS})
		set(CMAKE_CONFIG_MACROS "${CMAKE_CONFIG_MACROS}#define ${v}\n")
	endforeach()

	#修改配置文件
	file(COPY ${${src_hs}} DESTINATION ${SUBMODULE_INCLUDE}/${projname}/ FOLLOW_SYMLINK_CHAIN)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/${projname}/config.h.in ${SUBMODULE_INCLUDE}/${projname}/config_generated.h)

	#定义目标
	if(${${isdynamic}})
		message(定义shared目标${libname})
		add_library(${libname} SHARED ${${src_cpps}} ${${src_hs}})
		set_target_properties(${libname} PROPERTIES OUTPUT_NAME "${libname}")
		set_target_properties(${libname} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${soversion})
	else()
		message(定义static目标${libname})
		add_library(${libname} STATIC ${${src_cpps}} ${${src_hs}})
	endif()
	target_link_libraries(${libname} PRIVATE ${LIBS_DEPENDS})
	target_compile_options(${libname} PRIVATE -include ${SUBMODULE_INCLUDE}/${projname}/config_generated.h)
	

	if(alone)
		if(${${isdynamic}})
			target_compile_options(${libname} -static-libstdc++ -static-libgcc)
		else()
			target_compile_options(${libname} -static)		
		endif()

	endif()

	#定义安装目标
	install(TARGETS ${libname} DESTINATION lib64)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pkg/${projname}.pc DESTINATION lib64/pkgconfig)
	install(FILES ${${src_hs}} DESTINATION include/${projname})
	install(FILES ${SUBMODULE_INCLUDE}/${projname}/config.h DESTINATION include/${projname})
endmacro()
