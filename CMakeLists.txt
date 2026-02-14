cmake_minimum_required(VERSION 3.10)
project(mate-paint C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
include_directories(src)
link_directories(${GTK3_LIBRARY_DIRS})

add_definitions(${GTK3_CFLAGS_OTHER})

# Add -lm for math library
set(LIBS ${GTK3_LIBRARIES} m)

file(GLOB SOURCES "src/*.c")

add_executable(mate-paint ${SOURCES})
target_link_libraries(mate-paint ${LIBS})

# Set PACKAGE_DATA_DIR and PACKAGE_LOCALE_DIR
# For development, we might want to set this to source dir or handle it differently
add_definitions(-DPACKAGE_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share")
add_definitions(-DPACKAGE_LOCALE_DIR="${CMAKE_INSTALL_PREFIX}/share/locale")
add_definitions(-DPACKAGE_VERSION="0.4.0")
add_definitions(-DGETTEXT_PACKAGE="mate-paint")

install(TARGETS mate-paint DESTINATION bin)
