cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(nurl)

set(CMAKE_CXX_STADARD 11)

set(BOOST_ROOT "C:/DISTRO")

set(BOOST_COMPONENTS
        thread
        filesystem
        system
        log
        program_options
        )

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

#workaround to link libcurl (if cmake unable)
#save libcurl.a & libcurldll.a files and rename libcurldll.a to libcurl.a
set (CURL_ROOT "C:/Curl")
find_package(CURL REQUIRED)

message (" ${CURL_INCLUDE_DIRS}")
message (" ${CURL_LIBRARY}")

include_directories(include ${Boost_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
add_definitions(-DBOOST_THREAD_VERSION=4)
SET (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -static)
SET (CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -static)

add_executable(nurl
        include/contents_proc.h
        include/file_enum.h
        include/host_proc.h
        include/init_logs.h
        include/log_wrapper.h
        include/nurl_interact.h
        include/nurl_transport.h
        include/program_options.h
        src/init_logs.cpp
        src/main.cpp
        src/program_options.cpp)

target_link_libraries(nurl ${Boost_LIBRARIES} ${CURL_LIBRARY} -lws2_32 )
