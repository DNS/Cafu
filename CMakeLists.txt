cmake_minimum_required(VERSION 3.9)

project("Cafu Engine")

add_subdirectory(Libs)
add_subdirectory(ExtLibs/minizip)

find_package(Freetype REQUIRED)

add_executable(MakeFont "CaTools/MakeFont.cpp")
target_include_directories(MakeFont PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(MakeFont cfsLib minizip ${FREETYPE_LIBRARIES})
