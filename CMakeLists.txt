cmake_minimum_required (VERSION 3.5)
project(httptiny)

IF(WIN32)
	set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin)
ELSEIF(UNIX)
	set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin/gcc_x64)
ENDIF()

include_directories(${PROJECT_SOURCE_DIR}/src)

SET(SRC_LISTS ${PROJECT_SOURCE_DIR}/src/httpd.cpp ${PROJECT_SOURCE_DIR}/src/json11.cpp ${PROJECT_SOURCE_DIR}/src/main.cpp)

ADD_EXECUTABLE(httptiny ${SRC_LISTS})

