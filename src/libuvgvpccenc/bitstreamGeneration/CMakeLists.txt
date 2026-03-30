message(DEBUG "./src/lib/bitstreamGeneration/cmake")

add_library(bitstreamGeneration STATIC
    vps.cpp
    video_sub_bitstream.cpp
    atlas_context.cpp
    bitstreamGeneration.cpp
)

target_link_libraries(bitstreamGeneration
    PUBLIC
        libuvgv3cbitstream
        uvgutils
)

target_compile_options(bitstreamGeneration PRIVATE -fPIC)
