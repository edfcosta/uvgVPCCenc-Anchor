message(DEBUG "./src/lib/mapEncoding/cmake")

add_library(catchLibLogLibrary STATIC
    catchLibLog.cpp
)

target_link_libraries(catchLibLogLibrary
    PUBLIC
        uvgutils
)

set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_DEFAULT}")

# Set the default build libraries
set(LIB_SOURCES "mapEncoding.cpp" "catchLibLog.cpp" "encoderKvazaar.cpp")

if(ENABLE_FFMPEG)
    # check if ffmpeg is available if available include encoderFFmpeg.cpp
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
            libavcodec
            libavformat
            libavutil
        )
        message(STATUS "FFmpeg found: ${FFMPEG_VERSION}")
        set(LIB_SOURCES ${LIB_SOURCES} encoderFFmpeg.cpp)
        set(FFMPEG_LIBS PkgConfig::FFMPEG)
    else()
        message(WARNING "FFmpeg not found, skipping encoderFFmpeg")
        set(FFMPEG_LIBS "")
    endif()
endif()

# Define the library
add_library(mapEncodingLibrary STATIC
    ${LIB_SOURCES}
)

target_link_libraries(mapEncodingLibrary
    PUBLIC
        uvgutils
)

if(FFMPEG_LIBS)
    target_link_libraries(mapEncodingLibrary PUBLIC ${FFMPEG_LIBS})
endif()

set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_DEFAULT}")

# Include directories for headers
target_include_directories(mapEncodingLibrary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${KVAZAAR_INCLUDE_DIR})

# Optionally, add additional configurations, dependencies, or options
add_dependencies(mapEncodingLibrary kvazaar)
target_compile_options(mapEncodingLibrary PRIVATE -fPIC)
target_link_libraries(mapEncodingLibrary PUBLIC ${KVAZAAR_LIBRARY} ${KVAZAAR_STDIO_LIB})

# TODO(lf): following line might be useless (#gg)
target_compile_options(catchLibLogLibrary PRIVATE -fPIC)
