cmake_minimum_required(VERSION 3.21)
# Set CMP0091 NEW *before* project() so CMAKE_MSVC_RUNTIME_LIBRARY is honored —
# the Windows static build passes -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded to
# link the static CRT (/MT) and match vcpkg's x64-windows-static libraries.
cmake_policy(SET CMP0091 NEW)
project(TechnoRunner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Test)

# Homebrew's libarchive is keg-only (macOS ships its own), so it isn't on CMake's
# default search path. Auto-detect the keg prefix so find_package works without a
# manually-passed -DCMAKE_PREFIX_PATH.
if(APPLE)
    execute_process(COMMAND brew --prefix libarchive
        OUTPUT_VARIABLE LIBARCHIVE_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(LIBARCHIVE_PREFIX)
        list(APPEND CMAKE_PREFIX_PATH "${LIBARCHIVE_PREFIX}")
    endif()
endif()

find_package(LibArchive REQUIRED)

qt_standard_project_setup()

# All library sources (everything under src/ except main.cpp) are added by later
# tasks via target_sources(). Start empty so the lib exists from Task 1.
add_library(tprunner_lib STATIC)
target_include_directories(tprunner_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tprunner_lib PUBLIC Qt6::Widgets Qt6::Network LibArchive::LibArchive)

qt_add_executable(TechnoRunner src/main.cpp resources/resources.qrc)
target_link_libraries(TechnoRunner PRIVATE tprunner_lib)
set_target_properties(TechnoRunner PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON)

# A static Qt build (Windows single-exe) links plugins into the binary instead of
# loading them from disk, so the ones we use must be imported explicitly. Guarded
# by WIN32 + static so shared Qt (Linux/macOS) is untouched and the Windows-only
# plugin targets are never referenced elsewhere.
if(WIN32 AND NOT QT6_IS_SHARED_LIBS_BUILD)
    qt_import_plugins(TechnoRunner INCLUDE
        Qt6::QWindowsIntegrationPlugin   # GUI platform plugin
        Qt6::QJpegPlugin)                # background.jpeg (PNG is built into QtGui)
endif()

if(APPLE)
    # Bundle dir must be "Minecraft.app" (matches the current release) so it replaces
    # an existing install in place. CMake names the .app and the inner executable after
    # OUTPUT_NAME, and auto-fills MACOSX_BUNDLE_EXECUTABLE_NAME in the plist accordingly.
    set_target_properties(TechnoRunner PROPERTIES
        OUTPUT_NAME "Minecraft"
        MACOSX_BUNDLE_BUNDLE_NAME "Minecraft"
        MACOSX_BUNDLE_GUI_IDENTIFIER "ru.glitchless.games"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/macos/Info.plist.in")
    target_sources(TechnoRunner PRIVATE resources/macos/icon.icns)
    set_source_files_properties(resources/macos/icon.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
endif()

# ---- Tests ----
enable_testing()
function(tprunner_add_test name)
    qt_add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE tprunner_lib Qt6::Test)
    # A static Qt links QPA platform plugins into the binary instead of loading them
    # from disk, so they must be imported explicitly. vcpkg's static qtbase only
    # builds the native "windows" platform plugin (offscreen/minimal are not in its
    # QT_QPA_PLATFORMS), so the Windows tests use the windows platform like the app
    # does -- run with QT_QPA_PLATFORM=windows (see the Test step in build.yml).
    # QJpegPlugin is needed by SplashScreenTest, which loads background.jpeg.
    if(WIN32 AND NOT QT6_IS_SHARED_LIBS_BUILD)
        qt_import_plugins(${name} INCLUDE
            Qt6::QWindowsIntegrationPlugin
            Qt6::QJpegPlugin)
    endif()
    add_test(NAME ${name} COMMAND ${name})
endfunction()

tprunner_add_test(SmokeTest tests/SmokeTest.cpp)

# ---- Optional dev utility (HasherMain port) ----
option(TPRUNNER_BUILD_HASHER "Build the SHA-256 hasher utility" OFF)
if(TPRUNNER_BUILD_HASHER)
    qt_add_executable(technorunner-hasher src/tools/hasher_main.cpp)
    target_link_libraries(technorunner-hasher PRIVATE tprunner_lib)
endif()

target_sources(tprunner_lib PRIVATE src/util/Hash.cpp)
tprunner_add_test(HashTest tests/HashTest.cpp)

target_sources(tprunner_lib PRIVATE src/util/Platform.cpp)
tprunner_add_test(PlatformTest tests/PlatformTest.cpp)

target_sources(tprunner_lib PRIVATE src/util/Paths.cpp)
tprunner_add_test(PathsTest tests/PathsTest.cpp)

target_sources(tprunner_lib PRIVATE src/models/Models.cpp)
tprunner_add_test(ModelsTest tests/ModelsTest.cpp)

target_sources(tprunner_lib PRIVATE src/util/Archive.cpp)
tprunner_add_test(ArchiveTest tests/ArchiveTest.cpp)
target_compile_definitions(ArchiveTest PRIVATE SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/tests")

target_sources(tprunner_lib PRIVATE src/net/Downloader.cpp)
tprunner_add_test(DownloaderTest tests/DownloaderTest.cpp)

target_sources(tprunner_lib PRIVATE src/net/JavaDownloader.cpp)
tprunner_add_test(JavaDownloaderTest tests/JavaDownloaderTest.cpp)

target_sources(tprunner_lib PRIVATE src/net/LauncherDownloader.cpp)
tprunner_add_test(LauncherDownloaderTest tests/LauncherDownloaderTest.cpp)

target_sources(tprunner_lib PRIVATE src/run/Launcher.cpp)
tprunner_add_test(LauncherTest tests/LauncherTest.cpp)

target_sources(tprunner_lib PRIVATE src/app/Bootstrapper.cpp)
tprunner_add_test(BootstrapperTest tests/BootstrapperTest.cpp)

target_sources(tprunner_lib PRIVATE src/ui/GProgressBar.cpp)
tprunner_add_test(GProgressBarTest tests/GProgressBarTest.cpp)

target_sources(tprunner_lib PRIVATE src/ui/SplashScreen.cpp)
# .qrc is attached to the executable and the test (NOT the static lib) so the
# resource initializer is not dropped by the linker.
tprunner_add_test(SplashScreenTest tests/SplashScreenTest.cpp resources/resources.qrc)
