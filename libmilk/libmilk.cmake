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

if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
	add_compile_options(-Wno-deprecated)
	add_compile_options(-Wno-switch)
endif()




macro(libmilk_cmake_def_shared libname projname src_cpps src_hs alone)
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

	set(CMAKE_LIBS_DEPENDS ${LIBS_DEPENDS})

	foreach(v ${CONFIG_MACROS})
		set(CMAKE_CONFIG_MACROS "${CMAKE_CONFIG_MACROS}#define ${v}\n")
	endforeach()

	#修改配置文件
	file(COPY ${${src_hs}} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/${projname}/ FOLLOW_SYMLINK_CHAIN)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/${projname}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/${projname}/config.h)

	#定义目标
	add_library(${libname} SHARED ${${src_cpps}} ${${src_hs}})

	set_target_properties(${libname} PROPERTIES OUTPUT_NAME "${libname}")
	set_target_properties(${libname} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${soversion})
	if(alone)
		target_link_libraries(${libname} -static-libstdc++ -static-libgcc)
	endif()

	#定义安装目标
	install(TARGETS ${libname} DESTINATION lib64)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pkg/${projname}.pc DESTINATION lib64/pkgconfig)
	install(FILES ${${src_hs}} DESTINATION include/${projname})
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/${projname}/config.h DESTINATION include/${projname})
endmacro()





macro(libmilk_cmake_def_static libname projname src_cpps src_hs alone)
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

	set(CMAKE_LIBS_DEPENDS ${LIBS_DEPENDS})

	foreach(v ${CONFIG_MACROS})
		set(CMAKE_CONFIG_MACROS "${CMAKE_CONFIG_MACROS}#define ${v}\n")
	endforeach()

	#修改配置文件
	file(COPY ${${src_hs}} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/${projname}/ FOLLOW_SYMLINK_CHAIN)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/${projname}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/${projname}/config.h)

	#定义目标
	add_compile_options(-fPIC) 
	add_library(${libname} STATIC ${${src_cpps}} ${${src_hs}})
	if(alone)
		target_link_libraries(${libname} -static)
	endif()


	#定义安装目标
	install(TARGETS ${libname} DESTINATION lib64)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pkg/${projname}.pc DESTINATION lib64/pkgconfig)
	install(FILES ${${src_hs}} DESTINATION include/${projname})
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/${projname}/config.h DESTINATION include/${projname})
endmacro()
