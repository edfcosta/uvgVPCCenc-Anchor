add_library(libuvgv3cbitstream STATIC
    v3cbitstream_bitstream.cpp
    v3cbitstream_writer.cpp
)

target_include_directories(libuvgv3cbitstream
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uvgv3cbitstream
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_options(libuvgv3cbitstream PRIVATE -fPIC)
