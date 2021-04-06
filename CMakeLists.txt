
cmake_minimum_required(VERSION 3.20)

set(NOCONN_VERSION 1.0.0)
project(noconn VERSION ${NOCONN_VERSION} LANGUAGES CXX)

# CPack settings
set(CPACK_PACKAGE_VENDOR "noconnection")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "network adapter monitoring tool for windows")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/noconn")
set(CPACK_PACKAGE_CONTACT "smashmeta")

# adding qt dependencies
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(Qt5_FOLDER "D:/libraries/qt_5_12_10/5.12.10/msvc2017_64")
set(Qt5_ROOT_DIR "${Qt5_FOLDER}/lib/cmake/Qt5")
find_package(Qt5 COMPONENTS Core Widgets qml Quick quickcontrols2 REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS})

message( "Qt5 found? ${Qt5_FOUND}")
message( "Qt5 include directories: ${Qt5Core_INCLUDE_DIRS}" )
message( "Qt5 folder: ${Qt5_dir}")
# end qt dependencies

# adding third party dependencies
set(whatlog_DIR "d:/projects/whatlog")
find_package(whatlog REQUIRED whatlog)
include_directories(${whatlog_INCLUDE_DIRS})
# end third party dependencies

# adding boost dependencies
set(Boost_USE_STATIC_LIBS ON)
set(BOOST_ROOT "D:/libraries/boost_1_75_0")
find_package(Boost REQUIRED filesystem log log_setup)

add_definitions("-DBOOST_ALL_NO_LIB")
include_directories(${Boost_INCLUDE_DIRS})

message( "Boost found? ${Boost_FOUND}")
message( "Boost include dir is ${Boost_INCLUDE_DIRS}" )
message( "Boost library dir is ${Boost_LIBRARIES}" )
# end boost dependency

add_executable(noconn src/main.cpp src/network_adapter.cpp src/wbem_consumer.cpp src/noconn.qrc 
    include/noconn/network_adapter.hpp include/noconn/wbem_consumer.hpp )
target_include_directories(noconn PUBLIC "include")

target_link_libraries(noconn Qt5::Core Qt5::Widgets Qt5::Qml Qt5::Quick Qt5::QuickControls2)
target_link_libraries(noconn wsock32 ws2_32 IPHLPAPI wbemuuid)
target_link_libraries(noconn ${whatlog_LIBRARIES})
target_link_libraries(noconn ${Boost_LIBRARIES})

# post-build steps
# copy all the Qt support files to the debug/release directories
# You really only need the proper qt redistibution dll's/files, but this copies everything for easier development. Trim files out as needed.
# message("Copying required Qt libraries and binaries to output directory....")
# add_custom_command(TARGET noconn POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${Qt5_FOLDER}/bin $<TARGET_FILE_DIR:noconn>)
# add_custom_command(TARGET noconn POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${Qt5_FOLDER}/plugins $<TARGET_FILE_DIR:noconn>)
# add_custom_command(TARGET noconn POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${Qt5_FOLDER}/qml $<TARGET_FILE_DIR:noconn>)

add_custom_target(copy_files)
add_custom_command(TARGET copy_files COMMAND ${CMAKE_COMMAND} -E copy_directory ${Qt5_FOLDER}/bin $<TARGET_FILE_DIR:noconn>)
add_custom_command(TARGET copy_files COMMAND ${CMAKE_COMMAND} -E copy_directory ${Qt5_FOLDER}/plugins $<TARGET_FILE_DIR:noconn>)
add_custom_command(TARGET copy_files COMMAND ${CMAKE_COMMAND} -E copy_directory ${Qt5_FOLDER}/qml $<TARGET_FILE_DIR:noconn>)