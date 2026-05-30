cmake_minimum_required(VERSION 3.16)

project(Aegis)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Aegis
    src/main.cpp
    src/command_router.cpp
    src/password_manager.cpp
    src/entry.cpp
    src/storage.cpp
    src/auth_manager.cpp
    src/crypto_service.cpp
    src/ui.cpp
    src/clipboard.cpp
    src/version.rc
)

target_include_directories(Aegis PRIVATE src)

target_link_libraries(Aegis PRIVATE bcrypt shell32)

enable_testing()

add_executable(PasswordManagerTests
    tests/test_main.cpp
    src/entry.cpp
    src/auth_manager.cpp
    src/crypto_service.cpp
    src/ui.cpp
)

target_include_directories(PasswordManagerTests PRIVATE src)
target_link_libraries(PasswordManagerTests PRIVATE bcrypt shell32)

add_test(NAME PasswordManagerTests COMMAND PasswordManagerTests)