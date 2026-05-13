message(DEBUG "./src/libuvgformat/cmake")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(UVG_LOG_LEVEL_PRESET)
  add_compile_definitions(UVG_LOG_LEVEL=LogLevel::${UVG_LOG_LEVEL_PRESET})
endif()

set(LIB_SOURCES
    uvgformat.cpp
    io/plyLoader.cpp
    utils/parameters.cpp
    adaptation/voxelization.cpp
)

if(BUILD_SHARED_LIBS)
  add_library(uvgformat SHARED ${LIB_SOURCES})
else()
  add_library(uvgformat STATIC ${LIB_SOURCES})
endif()

target_link_libraries(uvgformat PRIVATE miniplyLibrary uvgutils)

# Include directories for headers
target_include_directories(uvgformat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
