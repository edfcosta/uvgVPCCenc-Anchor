message(DEBUG "./dependencies/cmake")

# ---------------------------------------------------------------------------
# miniply  (https://github.com/vilya/miniply) — no upstream CMakeLists.txt
# ---------------------------------------------------------------------------
message(STATUS "miniply git repository: ${MINIPLY_REPO_URL}")
message(STATUS "miniply tag: ${MINIPLY_REPO_TAG}")

FetchContent_Declare(
    miniply
    GIT_REPOSITORY ${MINIPLY_REPO_URL}
    GIT_TAG        ${MINIPLY_REPO_TAG}
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(miniply)
if(NOT miniply_POPULATED)
    FetchContent_Populate(miniply)
endif()

add_library(miniplyLibrary STATIC ${miniply_SOURCE_DIR}/miniply.cpp)
target_include_directories(miniplyLibrary PUBLIC ${miniply_SOURCE_DIR})
target_compile_options(miniplyLibrary PRIVATE -fPIC)

# ---------------------------------------------------------------------------
# nanoflann  (https://github.com/jlblancoc/nanoflann)
# ---------------------------------------------------------------------------
message(STATUS "nanoflann git repository: ${NANOFLANN_REPO_URL}")
message(STATUS "nanoflann tag: ${NANOFLANN_REPO_TAG}")

set(NANOFLANN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NANOFLANN_BUILD_TESTS    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    nanoflann
    GIT_REPOSITORY ${NANOFLANN_REPO_URL}
    GIT_TAG        ${NANOFLANN_REPO_TAG}
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nanoflann)

# KDTreeVectorOfVectorsAdaptor.h lives in examples/ — expose it as a named target
add_library(nanoflann_examples INTERFACE)
target_include_directories(nanoflann_examples INTERFACE ${nanoflann_SOURCE_DIR}/examples)

# ---------------------------------------------------------------------------
# robin-hood-hashing  (https://github.com/martinus/robin-hood-hashing)
# ---------------------------------------------------------------------------
message(STATUS "robin-hood-hashing git repository: ${ROBINHOOD_REPO_URL}")
message(STATUS "robin-hood-hashing tag: ${ROBINHOOD_REPO_TAG}")

FetchContent_Declare(
    robin_hood
    GIT_REPOSITORY ${ROBINHOOD_REPO_URL}
    GIT_TAG        ${ROBINHOOD_REPO_TAG}
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(robin_hood)


# Environment variable overrides for Kvazaar
if(DEFINED ENV{UVGVPCC_KVAZAAR_REPO_URL} AND NOT "$ENV{UVGVPCC_KVAZAAR_REPO_URL}" STREQUAL "")
    message(WARNING "Kvazaar repo overwritten by environment variable UVGVPCC_KVAZAAR_REPO_URL")
    set(KVAZAAR_REPO_URL $ENV{UVGVPCC_KVAZAAR_REPO_URL})
    if(DEFINED ENV{UVGVPCC_KVAZAAR_REPO_TAG} AND NOT "$ENV{UVGVPCC_KVAZAAR_REPO_TAG}" STREQUAL "")
        message(WARNING "Kvazaar tag overwritten by environment variable UVGVPCC_KVAZAAR_REPO_TAGS")
        set(KVAZAAR_REPO_TAGS $ENV{UVGVPCC_KVAZAAR_REPO_TAG})
    endif()
endif()

message(STATUS "Kvazaar git repository: ${KVAZAAR_REPO_URL}")
message(STATUS "Kvazaar tags: ${KVAZAAR_REPO_TAGS}")
message(STATUS "Kvazaar EP_UPDATE_DISCONNECTED: ${KVAZAAR_EP_UPDATE_DISCONNECTED}")

# Find system stdio.h (needed to build the kvazaar stdio wrapper)
find_file(STDIO_INCLUDE_PATH stdio.h PATHS /usr/include /usr/local/include)
if(NOT STDIO_INCLUDE_PATH)
    message(FATAL_ERROR "Could not find stdio.h")
endif()
message(STATUS "Found stdio.h: ${STDIO_INCLUDE_PATH}")

# Generate the kvazaar stdio wrapper library
set(STDIO_PREFIX "kvazaar")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kvazaar/uvg_stdio.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/${STDIO_PREFIX}_stdio.h
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/kvazaar/uvg_stdio.c.in
    ${CMAKE_CURRENT_BINARY_DIR}/${STDIO_PREFIX}_stdio.c
    @ONLY
)

add_library(${STDIO_PREFIX}_stdio STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/${STDIO_PREFIX}_stdio.c
)
target_include_directories(${STDIO_PREFIX}_stdio PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/include
)
target_compile_options(${STDIO_PREFIX}_stdio PRIVATE -fPIC)

# Build Kvazaar as an external project
include(${CMAKE_CURRENT_SOURCE_DIR}/kvazaar/kvazaar.cmake)

set(KVAZAAR_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include PARENT_SCOPE)
set(KVAZAAR_LIBRARY     ${CMAKE_CURRENT_BINARY_DIR}/libkvazaar.a PARENT_SCOPE)
set(KVAZAAR_STDIO_LIB   ${STDIO_PREFIX}_stdio PARENT_SCOPE)

uvgvpcc_add_kvazaar_external(
    kvazaar
    ${CMAKE_CURRENT_BINARY_DIR}/include/${STDIO_PREFIX}_stdio.h
    ${CMAKE_CURRENT_BINARY_DIR}/include/kvazaar.h
    ${CMAKE_CURRENT_BINARY_DIR}/libkvazaar.a
)

add_dependencies(kvazaar ${STDIO_PREFIX}_stdio)
