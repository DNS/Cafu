cmake_minimum_required(VERSION 3.9)

project("Cafu Libraries")

find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
# find_package(ZLIB REQUIRED)

add_library(cfsLib STATIC
    Bitmap/Bitmap.cpp Bitmap/jdatasrc.cpp
    ConsoleCommands/ConVar.cpp # ConsoleCommands/ConFunc.cpp
    #ConsoleCommands/ConsoleInterpreterImpl.cpp ConsoleCommands/ConsoleInterpreter_LuaBinding.cpp
    ConsoleCommands/Console.cpp # ConsoleCommands/Console_Lua.cpp ConsoleCommands/ConsoleComposite.cpp
        ConsoleCommands/ConsoleStdout.cpp ConsoleCommands/ConsoleStringBuffer.cpp ConsoleCommands/ConsoleWarningsOnly.cpp ConsoleCommands/ConsoleFile.cpp
    Fonts/Font.cpp Fonts/FontTT.cpp
    FileSys/FileManImpl.cpp FileSys/FileSys_LocalPath.cpp FileSys/FileSys_ZipArchive_GV.cpp FileSys/File_local.cpp FileSys/File_memory.cpp FileSys/Password.cpp
)

target_include_directories(cfsLib PUBLIC . ../ExtLibs)    # ExtLibs ist wegen einem #include "minizip/unzip.h"
target_link_libraries(cfsLib ${JPEG_LIBRARIES} ${PNG_LIBRARIES})
