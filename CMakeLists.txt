cmake_minimum_required(VERSION 3.19)

# Qt 6.5.3 msvc2019_64 等本地路径统一从项目根 paths.txt 读取
# （构建者只需编辑 paths.txt 一处即可，无需改动本文件）
set(_PATHS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/paths.txt)
set(QT_PREFIX "")
if(EXISTS ${_PATHS_FILE})
    file(STRINGS ${_PATHS_FILE} _PATHS_LINES)
    foreach(_line IN LISTS _PATHS_LINES)
        if(_line MATCHES "^[ \t]*QT_PREFIX[ \t]*=[ \t]*(.*)$")
            set(QT_PREFIX "${CMAKE_MATCH_1}")
        endif()
    endforeach()
endif()
if(NOT QT_PREFIX)
    message(FATAL_ERROR "QT_PREFIX not found in paths.txt. "
        "Create/edit paths.txt (see paths.txt comments) before building.")
endif()
set(CMAKE_PREFIX_PATH
    "${QT_PREFIX}"
    ${CMAKE_PREFIX_PATH}
)

project(QueryReader VERSION 1.1.0 LANGUAGES C CXX)

# 第三方库：cmark 已被 MuPDF 内置的 cmark-gfm 取代（移除独立的 cmark 链接，避免符号冲突）

# MuPDF 是预编译好的静态库，头文件在 third_party/mupdf/include
set(MUPDF_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mupdf)
set(MUPDF_LIB_DIR ${MUPDF_ROOT}/platform/win32/x64/Release)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)

qt_standard_project_setup()

add_executable(QueryReader WIN32
    src/main.cpp
    src/core/Document.h
    src/core/DocumentManager.cpp
    src/core/DocumentManager.h
    src/core/MuPDFDocument.cpp
    src/core/MuPDFDocument.h
    src/core/TextDocument.cpp
    src/core/TextDocument.h
    src/parsers/IParser.h
    src/parsers/MarkdownParser.cpp
    src/parsers/MarkdownParser.h
    src/parsers/MuPDFParser.cpp
    src/parsers/MuPDFParser.h
    src/parsers/TxtParser.cpp
    src/parsers/TxtParser.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/DocumentView.cpp
    src/ui/DocumentView.h
)

target_include_directories(QueryReader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MUPDF_ROOT}/include
    ${MUPDF_ROOT}/thirdparty/cmark-gfm/src
    ${MUPDF_ROOT}/scripts/cmark-gfm
) 

target_link_libraries(QueryReader PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${MUPDF_LIB_DIR}/libmupdf.lib
    ${MUPDF_LIB_DIR}/libresources.lib
    ${MUPDF_LIB_DIR}/libthirdparty.lib
    ${MUPDF_LIB_DIR}/libharfbuzz.lib
    ${MUPDF_LIB_DIR}/libextract.lib
    ${MUPDF_LIB_DIR}/libleptonica.lib
    ${MUPDF_LIB_DIR}/libmubarcode.lib
    ${MUPDF_LIB_DIR}/libpkcs7.lib
    ${MUPDF_LIB_DIR}/libtesseract.lib
    ${MUPDF_LIB_DIR}/libzxing.lib
)

# 临时验证工具
add_executable(ReaderTest
    tools/readertest/main.cpp
    src/core/DocumentManager.cpp
    src/core/DocumentManager.h
    src/core/MuPDFDocument.cpp
    src/core/MuPDFDocument.h
    src/core/TextDocument.cpp
    src/core/TextDocument.h
    src/parsers/IParser.h
    src/parsers/MarkdownParser.cpp
    src/parsers/MarkdownParser.h
    src/parsers/MuPDFParser.cpp
    src/parsers/MuPDFParser.h
    src/parsers/TxtParser.cpp
    src/parsers/TxtParser.h
)

target_include_directories(ReaderTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MUPDF_ROOT}/include
    ${MUPDF_ROOT}/thirdparty/cmark-gfm/src
    ${MUPDF_ROOT}/scripts/cmark-gfm
)

target_link_libraries(ReaderTest PRIVATE
    Qt6::Core
    Qt6::Gui
    ${MUPDF_LIB_DIR}/libmupdf.lib
    ${MUPDF_LIB_DIR}/libresources.lib
    ${MUPDF_LIB_DIR}/libthirdparty.lib
    ${MUPDF_LIB_DIR}/libharfbuzz.lib
    ${MUPDF_LIB_DIR}/libextract.lib
    ${MUPDF_LIB_DIR}/libleptonica.lib
    ${MUPDF_LIB_DIR}/libmubarcode.lib
    ${MUPDF_LIB_DIR}/libpkcs7.lib
    ${MUPDF_LIB_DIR}/libtesseract.lib
    ${MUPDF_LIB_DIR}/libzxing.lib
)

# 引导下载器：分发用，从 GitHub Release 下载 zip 安装包（纯 Win32，单文件零依赖）
add_executable(QueryReaderSetup WIN32
    tools/downloader/main.cpp
    tools/downloader/resource.rc
    tools/downloader/QueryReaderSetup.ico
)

# 静态链接 MSVC 运行库（/MT），让下载器成为真正零依赖的单文件
if(MSVC)
    target_compile_options(QueryReaderSetup PRIVATE /MT)
    target_link_options(QueryReaderSetup PRIVATE /NODEFAULTLIB:msvcrt)
endif()

target_link_libraries(QueryReaderSetup PRIVATE
    wininet
    comctl32
    shell32
    ole32
    user32
    gdi32
)