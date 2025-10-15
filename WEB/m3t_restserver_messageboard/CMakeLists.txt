cmake_minimum_required(VERSION 3.12)
include(GNUInstallDirs)

get_filename_component(APP_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
get_filename_component(PARENT_DIRFULL ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(PARENT_DIR ${PARENT_DIRFULL} NAME)

project(${APP_NAME})
project(${PROJECT_NAME} VERSION ${SVERSION} DESCRIPTION "REST Web Application Server Dicussion Forum")

configure_file(src/config.h.in ${${APP_NAME}_SOURCE_DIR}/src/config.h)

file(GLOB_RECURSE EDV_INCLUDE_FILES "src/*.h*")
file(GLOB_RECURSE EDV_SOURCE_FILES "src/*.c*")

file(GLOB_RECURSE EDV_INCLUDE_FILES2 "src/*/*.h*")
file(GLOB_RECURSE EDV_SOURCE_FILES2 "src/*/*.c*")

add_executable( ${APP_NAME} ${EDV_INCLUDE_FILES} ${EDV_SOURCE_FILES} ${EDV_INCLUDE_FILES2} ${EDV_SOURCE_FILES2})

install( TARGETS ${APP_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

if (EXTRAPREFIX)
    target_include_directories(${APP_NAME} PUBLIC ${EXTRAPREFIX}/include)
    target_link_libraries(${APP_NAME} "-L${EXTRAPREFIX}/lib")
    target_link_libraries(${APP_NAME} "-L${EXTRAPREFIX}/lib64")
endif()

if (CMAKE_INSTALL_PREFIX)
    target_include_directories(${APP_NAME} PUBLIC ${CMAKE_INSTALL_PREFIX}/include)
    target_include_directories(${APP_NAME} PUBLIC ${CMAKE_INSTALL_PREFIX}/usr/include)
    target_link_libraries(${APP_NAME} "-L${CMAKE_INSTALL_PREFIX}/lib")
    target_link_libraries(${APP_NAME} "-L${CMAKE_INSTALL_PREFIX}/lib64")
endif()

################################################################################
# PKG-CONFIG INIT:
find_package(PkgConfig REQUIRED)
################################################################################

################################################################################
# Find jsoncpp
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
include_directories(${JSONCPP_INCLUDE_DIRS})
link_directories(${JSONCPP_LIBRARY_DIRS})
target_link_libraries(${APP_NAME} ${JSONCPP_LIBRARIES})

################################################################################
# Boost Packages:
find_package(Boost REQUIRED COMPONENTS regex thread)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${APP_NAME} ${Boost_LIBRARIES})

################################################################################
# openssl package:
option(SSLRHEL7 "OpenSSL 1.1 For Red Hat 7.x provided by EPEL" OFF)
if (SSLRHEL7)
    pkg_check_modules(OPENSSL REQUIRED libssl11)
else()
    pkg_check_modules(OPENSSL REQUIRED libssl)
endif()
target_include_directories(${APP_NAME} PUBLIC ${OPENSSL_INCLUDE_DIRS})
target_compile_options(${APP_NAME} PUBLIC ${OPENSSL_CFLAGS_OTHER})
target_link_libraries(${APP_NAME} ${OPENSSL_STATIC_LDFLAGS})

################################################################################
# List of Mantids30 libraries to link against
set(Mantids30_LIBRARIES
    Mantids30_Protocol_APISync
    Mantids30_Server_WebCore
    Mantids30_Server_RESTfulWebAPI
    Mantids30_API_RESTful
    Mantids30_Sessions
    Mantids30_Protocol_HTTP
    Mantids30_Protocol_MIME
    Mantids30_Net_Sockets
    Mantids30_Threads
    Mantids30_DB
    Mantids30_DB_SQLite3
    Mantids30_Memory
    Mantids30_Program_Service
    Mantids30_Program_Logs
    Mantids30_Config_Builder
    Mantids30_Helpers
    Mantids30_DataFormat_JWT
)

set(ENV{PKG_CONFIG_PATH} "/home/coding/Devel/ROOT/share/pkgconfig:$ENV{PKG_CONFIG_PATH}")

# Iterate over the list of Mantids30 libraries and add include directories and linker flags to the target
foreach(LIBRARY ${Mantids30_LIBRARIES})
    # Find the library using pkg-config
    pkg_check_modules(${LIBRARY} REQUIRED ${LIBRARY})
    # Add include directory to target
    target_include_directories(${APP_NAME} PUBLIC ${${LIBRARY}_INCLUDEDIR})
    # Link target against the library
    target_link_libraries(${APP_NAME} ${${LIBRARY}_LDFLAGS})
endforeach()

# Add src directory to the include path globally
include_directories(${CMAKE_SOURCE_DIR}/src)

# This will ensure that any file including headers like #include "IdentityManager/identitymanager.h"
# will find them relative to the src/ directory, no matter where the cpp file is located.
