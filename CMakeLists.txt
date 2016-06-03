cmake_minimum_required(VERSION 2.8.12)

set(PROJECT_NAME hstTOcsv)

project(${PROJECT_NAME})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	add_definitions("/Zi /W4")
	if (${UNICODE})
		add_definitions(-DUNICODE -D_UNICODE)
	endif ()
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -pthread -Wall -Wextra")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++1y -O3 -pthread -Wall -Wextra")
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++1y -O3 -Wall -Wextra")
endif ()

enable_testing()

set(INCLUDE_ROOT_DIR "${PROJECT_SOURCE_DIR}/include")

add_subdirectory(src)
