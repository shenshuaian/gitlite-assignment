cmake_minimum_required(VERSION 3.10)
project(gitlite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(gitlite
    main.cpp
    src/Utils.cpp
    src/GitliteException.cpp
    src/SomeObj.cpp
    src/Repository.cpp
    src/Commit.cpp
    src/ObjectStore.cpp
    src/Index.cpp
    src/IgnoreMatcher.cpp
)

target_include_directories(gitlite PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# ==================== ACMOJ 配置（请勿删除）====================
add_custom_command(
    TARGET gitlite POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:gitlite>"
            "${CMAKE_SOURCE_DIR}/code"
    COMMENT "Copying gitlite executable to ACMOJ artifact: code"
    VERBATIM
)
# ================== ACMOJ 配置结束（请勿删除）==================