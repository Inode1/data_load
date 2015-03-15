cmake_minimum_required (VERSION 2.8)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON) 
set(Boost_USE_STATIC_RUNTIME OFF)

project (data_load)
find_package(Boost COMPONENTS thread system regex iostreams filesystem REQUIRED)
if(NOT Boost_FOUND)
    return()    
else()
    include_directories(${Boost_INCLUDE_DIRS}) 
endif()
file (GLOB HEADER 
	../*.h)
file (GLOB SOURCE
	../*.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -lpthread")
add_executable(data_load ${SOURCE} ${HEADER})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
