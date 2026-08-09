cmake_minimum_required(VERSION 3.19)

# Qt 6.5.3 msvc2019_64 (ABI 兼容 VS2022 MSVC)
set(CMAKE_PREFIX_PATH
    "D:/app/qt/6.5.3/msvc2019_64"
    ${CMAKE_PREFIX_PATH}
)

project(QueryReader VERSION 0.1.0 LANGUAGES C CXX)

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

# 引导下载器：分发用，从 GitHub Release 下载 zip 安装包
find_package(Qt6 6.5 REQUIRED COMPONENTS Network)

add_executable(QueryReaderSetup
    tools/downloader/main.cpp
    tools/downloader/Downloader.cpp
    tools/downloader/Downloader.h
)

target_link_libraries(QueryReaderSetup PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)