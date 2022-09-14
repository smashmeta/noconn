
cmake_minimum_required(VERSION 3.20)

# adding windows version setting
add_definitions(-D_WIN32_WINNT=0x0600)

# CPack settings
set(CPACK_PACKAGE_VENDOR "noconnection")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "network adapter monitoring tool for windows")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/noconn")
set(CPACK_PACKAGE_CONTACT "smashmeta")

set(CMAKE_TOOLCHAIN_FILE "C:/Users/fredrhe/Desktop/void/programs/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(whatlog_DIR "C:/Users/fredrhe/Desktop/void/projects/whatlog")

set(NOCONN_VERSION 1.0.0)
project(noconn VERSION ${NOCONN_VERSION} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

# adding fmt dependencies
find_package(fmt CONFIG REQUIRED)

# adding whatlog dependencies
find_package(whatlog REQUIRED whatlog)
include_directories(${whatlog_INCLUDE_DIRS})

# adding boost dependencies
find_package(Boost REQUIRED filesystem json thread log)

# setting up source files and header files groups
file(GLOB HEADERS_DEFAULT_GRP "${CMAKE_CURRENT_SOURCE_DIR}/include/noconn/*.hpp")
file(GLOB HEADERS_REST_GRP "${CMAKE_CURRENT_SOURCE_DIR}/include/noconn/rest/*.hpp")
file(GLOB HEADERS_NET_GRP "${CMAKE_CURRENT_SOURCE_DIR}/include/noconn/net/*.hpp")
file(GLOB SOURCE_DEFAULT_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB SOURCE_REST_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/rest/*.cpp")
file(GLOB SOURCE_NET_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/net/*.cpp")

add_executable(noconn 
    ${HEADERS_DEFAULT_GRP}
	${SOURCE_DEFAULT_GRP}
	${HEADERS_REST_GRP}
	${SOURCE_REST_GRP}
	${HEADERS_NET_GRP}
	${SOURCE_NET_GRP}
)

target_include_directories(noconn PUBLIC "include")

target_link_libraries(noconn wsock32 ws2_32 IPHLPAPI wbemuuid)
target_link_libraries(noconn ${whatlog_LIBRARIES})
target_link_libraries(noconn ${Boost_LIBRARIES})
target_link_libraries(noconn fmt::fmt)

# adding file filters to visual studio project
source_group("header/" FILES ${HEADERS_DEFAULT_GRP})
source_group("header/rest/" FILES ${HEADERS_REST_GRP})
source_group("header/net/" FILES ${HEADERS_NET_GRP})
source_group("src/" FILES ${SOURCE_DEFAULT_GRP})
source_group("src/rest/" FILES ${SOURCE_REST_GRP})
source_group("src/net/" FILES ${SOURCE_NET_GRP})

# post-build steps
# message("example of how to copy files in a directory to our build directory")
# add_custom_command(TARGET noconn POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${Qt5_FOLDER}/bin $<TARGET_FILE_DIR:noconn>)