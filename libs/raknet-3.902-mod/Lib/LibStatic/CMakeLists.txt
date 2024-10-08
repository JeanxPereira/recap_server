cmake_minimum_required(VERSION 2.6)
project(LibStatic)

FILE(GLOB ALL_HEADER_SRCS ${RakNet_SOURCE_DIR}/Source/*.h)
FILE(GLOB ALL_CPP_SRCS ${RakNet_SOURCE_DIR}/Source/*.cpp)

include_directories(${RakNet_SOURCE_DIR}/Source) 
IF(WIN32 AND NOT UNIX)
	add_library(LibStatic STATIC ${ALL_CPP_SRCS} ${ALL_HEADER_SRCS} readme.txt)
ELSE(WIN32 AND NOT UNIX)
	add_library(RakNetStatic STATIC ${ALL_CPP_SRCS} ${ALL_HEADER_SRCS} readme.txt)
ENDIF(WIN32 AND NOT UNIX)

IF(WIN32 AND NOT UNIX)
	SET( CMAKE_CXX_FLAGS "/D WIN32 /D _RAKNET_LIB /D _CRT_NONSTDC_NO_DEPRECATE /D _CRT_SECURE_NO_DEPRECATE ")
ENDIF(WIN32 AND NOT UNIX)

IF(WIN32 AND NOT UNIX)
	target_link_libraries (LibStatic ${RAKNET_LIBRARY_LIBS})
	set_target_properties(LibStatic PROPERTIES STATIC_LIBRARY_FLAGS "/NODEFAULTLIB:\"LIBCD.lib LIBCMTD.lib MSVCRT.lib\"" )
ELSE(WIN32 AND NOT UNIX)
	target_link_libraries (RakNetStatic ${RAKNET_LIBRARY_LIBS})
	INSTALL(TARGETS RakNetStatic DESTINATION ${RakNet_SOURCE_DIR}/Lib/LibStatic)
	INSTALL(FILES ${ALL_HEADER_SRCS} DESTINATION ${RakNet_SOURCE_DIR}/include/raknet)
ENDIF(WIN32 AND NOT UNIX)


