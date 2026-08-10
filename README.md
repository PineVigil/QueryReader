# QueryReader 轻量阅读器

基于 C++ / Qt 6 / MuPDF 的多格式阅读器，支持：

| 格式 | 说明 |
|------|------|
| TXT / Markdown | 流式排版，A4 分页，支持中文 |
| PDF | MuPDF 渲染原始页面，目录/书签 |
| EPUB | 电子书，目录/书签 |
| DOCX / ODT / XLSX / PPTX | Office 文档 |
| MOBI / FB2 / XPS / CBZ / 图片 | MuPDF 可识别的其余格式 |

## 功能

- 打开文档：菜单「文件 → 打开」，或命令行 `QueryReader.exe 文件路径`
- 翻页：PgUp / PgDn / 方向键 / PageDown，或输入页码回车
- 缩放：工具栏按钮，或 **Ctrl + 鼠标滚轮**；双击切换原始大小/适应宽度
- 目录：打开带书签的文档（PDF/EPUB 等）后，左侧显示目录，点击跳页
- 搜索：工具栏搜索框输入文字回车，在全书逐页查找并跳转

## 构建

环境要求：

- Visual Studio 2022（含 C++ 工作负载与 CMake 工具）
- Qt 6.5.3 msvc2019_64（`D:\app\qt\6.5.3\msvc2019_64`）
- MuPDF 1.28 静态库已预编译（见 `third_party/mupdf`，含 `platform/win32` 的 VS 工程）

一键构建：

```
build.bat
```

> **路径配置**：Qt 与 CMake 的安装路径统一放在项目根 `paths.txt` 中
> （`QT_PREFIX`、`CMAKE_EXE`），构建脚本与 CMake 都会读取它。
> 换到别的机器只需编辑这一个文件，无需改动 `build.bat` 或 `CMakeLists.txt`。

产物：`build\Release\QueryReader.exe`。

> 说明：MuPDF 以静态库形式使用（Release，/MD），因此工程必须用 **Release** 配置链接。若需重编 MuPDF，用 `third_party/mupdf/platform/win32/mupdf.sln`（MSBuild 时加 `/p:PlatformToolset=v143`）。

## 运行

Qt 6 是动态库，运行前把 Qt bin 加入 PATH：

```
set PATH=D:\app\qt\6.5.3\msvc2019_64\bin;%PATH%
build\Release\QueryReader.exe
```

## 发布（打包给他人使用）

1. 复制 `QueryReader.exe` 到发布目录（如 `dist\`）。
2. 将下列 Qt 依赖 DLL 复制到同目录：
   - Core / Gui / Widgets：`Qt6Core.dll`、`Qt6Gui.dll`、`Qt6Widgets.dll`
   - 插件目录 `platforms\`（内含 `qwindows.dll`）、`styles\`、`imageformats\`
   - MSVC 运行库 `vcruntime140.dll`、`msvcp140.dll`（或安装 VC++ 运行库）
3. 也可用 Qt 官方部署工具自动收集依赖：

```
D:\app\qt\6.5.3\msvc2019_64\bin\windeployqt.exe build\Release\QueryReader.exe --dir dist
```

然后把 `dist\` 整个目录压缩即可分发。MuPDF 是静态链接，无需额外库。

## 测试文件

`test-data/` 下包含用于验证的 txt / md / pdf / epub / docx 示例，可用 `ReaderTest.exe`（构建时自动生成）做无界面验证：

```
set QT_QPA_PLATFORM=offscreen
build\Release\ReaderTest.exe test-data\sample_book.epub
```

输出 `report.txt` 与 `page_export/` PNG。

## 引导下载器（QueryReaderSetup）

面向分发：别人下载这个 exe 后，点击即可从你的网络地址下载完整的阅读器 zip，解压安装并自动运行。

- 产物：`build\Release\QueryReaderSetup.exe`（Qt 动态库，需连同 Qt DLL 一起分发）
- 使用：运行后在「下载源」填入 zip 的下载地址，选安装目录，点「开始安装」
- 下载源示例（GitHub Release 附件）：
  `https://github.com/<owner>/<repo>/releases/latest/download/QueryReader-win64.zip`

发布流程：

1. 用 `windeployqt` 收集 `QueryReader.exe` 依赖，得到完整运行时目录
2. 将该目录打成 zip（用 `tar -a -cf 包名.zip 目录内容` 或压缩软件）
3. 把 zip 传到 GitHub Release（或自己的服务器）
4. 将上面的 release 下载链接发给用户，配合 `QueryReaderSetup.exe` 使用

## 第三方组件

- MuPDF 1.28（AGPLv3）——文档解析与渲染，内置 cmark-gfm 提供 Markdown 支持
- Qt 6.5.3（LGPLv3）——GUI 框架

## 已知限制

- 加密 PDF 暂不弹密码框（MuPDF 可自动打开部分公开加密文件）
- Office/网页类文档按 MuPDF 的分页排版渲染，与原生软件排版可能有差异