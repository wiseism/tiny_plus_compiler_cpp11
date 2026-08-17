cmake_minimum_required(VERSION 3.14)
project(tiny_compiler_cpp11)

enable_testing()

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# GoogleTest is downloaded into the build tree, never into the source tree.
# Set GOOGLETEST_SOURCE_DIR to use a pre-downloaded checkout when working offline.
set(GOOGLETEST_SOURCE_DIR "" CACHE PATH "Path to a local GoogleTest source checkout")
if (GOOGLETEST_SOURCE_DIR)
    if (NOT EXISTS "${GOOGLETEST_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
                "GOOGLETEST_SOURCE_DIR must contain GoogleTest's CMakeLists.txt: "
                "${GOOGLETEST_SOURCE_DIR}")
    endif ()
    add_subdirectory("${GOOGLETEST_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/googletest")
else ()
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://gitee.com/mirrors/googletest.git
            # Gitee 镜像没有 v1.12.1 这个 tag，其对应版本名为 release-1.12.1（同一 commit 58d77fa）
            GIT_TAG release-1.12.1
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(googletest)
endif ()

add_subdirectory(src)
add_subdirectory(tests)

# Doxygen 构建(本人尚未使用,先空置)
option(BUILD_DOC "Build Documentation" ON)
if (BUILD_DOC)
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        set(BUILD_DOC_DIR ${CMAKE_SOURCE_DIR}/build/docs)
        if (NOT EXISTS ${BUILD_DOC_DIR})
            file(MAKE_DIRECTORY ${BUILD_DOC_DIR})
        endif ()

        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        message("Doxygen build started")
        add_custom_target(Doxygen ALL
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM)
    else ()
        message("Doxygen needs to be installed to generate the documentation.")
    endif ()
endif ()


# 附上个人经常使用的配置
## 加上boost编译
# find_package(Boost 1.58.0 COMPONENTS system filesystem REQUIRED)
# include_directories(${Boost_INCLUDE_DIRS})
# target_link_libraries(可执行文件名 ${Boost_LIBRARIES})
