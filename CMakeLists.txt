cmake_minimum_required(VERSION 3.20)
project(fiscmd VERSION 1.975 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Link OpenSSL / libzip / curl optionally or use native OS APIs + lightweight fallbacks
find_package(OpenSSL QUIET)

add_executable(fiscmd
    src/main.cpp
    src/SetColor.cpp
    src/ConsoleUtils.cpp
    src/ProcessUtils.cpp
    src/FileCommands.cpp
    src/FileIdentify.cpp
    src/SystemCommands.cpp
    src/TouchEditor.cpp
    src/Snake.cpp
    src/FisScript.cpp
    src/FisDraw.cpp
    src/Program.cpp
)

if(MINGW)
    target_link_options(fiscmd PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()

target_include_directories(fiscmd PRIVATE include)

if(WIN32)
    target_link_libraries(fiscmd PRIVATE bcrypt ws2_32 iphlpapi winhttp advapi32 shell32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(fiscmd PRIVATE Threads::Threads)
    if(OpenSSL_FOUND)
        target_compile_definitions(fiscmd PRIVATE FIS_HAS_OPENSSL)
        target_link_libraries(fiscmd PRIVATE OpenSSL::Crypto)
    endif()
endif()
