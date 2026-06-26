cmake_minimum_required(VERSION 3.21)
project(TechnoRunner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Test)
find_package(LibArchive REQUIRED)

qt_standard_project_setup()

# All library sources (everything under src/ except main.cpp) are added by later
# tasks via target_sources(). Start empty so the lib exists from Task 1.
add_library(tprunner_lib STATIC)
target_include_directories(tprunner_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tprunner_lib PUBLIC Qt6::Widgets Qt6::Network LibArchive::LibArchive)

qt_add_executable(TechnoRunner src/main.cpp)
target_link_libraries(TechnoRunner PRIVATE tprunner_lib)
set_target_properties(TechnoRunner PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON)

# ---- Tests ----
enable_testing()
function(tprunner_add_test name)
    qt_add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE tprunner_lib Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

tprunner_add_test(SmokeTest tests/SmokeTest.cpp)

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
