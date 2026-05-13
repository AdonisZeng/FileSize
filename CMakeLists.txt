# CMakeList.txt: FileSize 的 CMake 项目
cmake_minimum_required (VERSION 3.10)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("FileSize")

set(SOURCES
    main.cpp
    UI.cpp
    FileScanner.cpp
    FileSorter.cpp
    SizeFormatter.cpp
)

set(HEADERS
    UI.h
    FileScanner.h
    FileSorter.h
    SizeFormatter.h
)

add_executable (FileSize WIN32 ${SOURCES} ${HEADERS} app.manifest)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET FileSize PROPERTY CXX_STANDARD 20)
endif()

target_compile_definitions(FileSize PRIVATE UNICODE _UNICODE)

if(MINGW)
    target_link_options(FileSize PRIVATE -mwindows -municode)
    target_link_libraries(FileSize PRIVATE shell32 comctl32)
else()
    target_link_libraries(FileSize PRIVATE shell32.lib comctl32.lib)
endif()
