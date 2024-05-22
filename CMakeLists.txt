cmake_minimum_required(VERSION 3.17)
project(wzlibcpp)

set(CMAKE_CXX_STANDARD 20)

set(EXTERNAL_DIR ${PROJECT_SOURCE_DIR}/external)
set(WZLIB_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)

add_subdirectory(${EXTERNAL_DIR}/zlib)

set(ZLIB_INCLUDE_DIR ${EXTERNAL_DIR}/zlib)
set(MIO_INCLUDE_DIR ${EXTERNAL_DIR}/mio/include)
set(WZLIB_INCLUDE_DIR ${WZLIB_SOURCE_DIR})

include_directories(
	${EXTERNAL_DIR}
	${WZLIB_INCLUDE_DIR}
	${MIO_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
)

file(GLOB SOURCE_FILES ${WZLIB_SOURCE_DIR}/**/*.cpp)
file(GLOB AES_SOURCE_FILES ${EXTERNAL_DIR}/AES/AES.cpp)

add_library(wzlib ${SOURCE_FILES} ${AES_SOURCE_FILES})
target_link_libraries(wzlib zlibstatic)

if(WIN32)
	project(wzlibtest)

	add_executable(wzlibtest ${PROJECT_SOURCE_DIR}/test/main.cpp ${SOURCE_FILES})
	target_link_libraries(
			wzlibtest
			PRIVATE
			wzlib
			zlibstatic 
	)
endif()