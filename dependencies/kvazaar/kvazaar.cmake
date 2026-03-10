include(ExternalProject)

function(uvgvpcc_add_kvazaar_external target_name stdio_header output_header output_library)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/dependencies/kvazaar/multilib.patch")
        message(FATAL_ERROR "multilib.patch not found at ${CMAKE_SOURCE_DIR}/dependencies/kvazaar/multilib.patch")
    endif()

    get_filename_component(output_header_dir "${output_header}" DIRECTORY)
    get_filename_component(output_library_dir "${output_library}" DIRECTORY)

    ExternalProject_Add(
        ${target_name}
        PREFIX ${CMAKE_BINARY_DIR}/external/${target_name}
        GIT_REPOSITORY ${KVAZAAR_REPO_URL}
        GIT_TAG ${KVAZAAR_REPO_TAGS}
        PATCH_COMMAND ${CMAKE_COMMAND}
            -Dpatch_file=${CMAKE_SOURCE_DIR}/dependencies/kvazaar/multilib.patch
            -Dsource_dir=<SOURCE_DIR>
            -P ${CMAKE_SOURCE_DIR}/dependencies/kvazaar/kvazaar_apply_multilib_patch.cmake
        BUILD_COMMAND ${CMAKE_COMMAND} -E copy "${stdio_header}" "<SOURCE_DIR>/src/stdio.h"
                      COMMAND ${CMAKE_COMMAND} --build . --target kvazaar
        INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "${output_header_dir}"
                        COMMAND ${CMAKE_COMMAND} -E make_directory "${output_library_dir}"
                        COMMAND ${CMAKE_COMMAND} -E copy "<SOURCE_DIR>/src/kvazaar.h" "${output_header}"
                        COMMAND ${CMAKE_COMMAND} -E copy "<BINARY_DIR>/libkvazaar.a" "${output_library}"
        BUILD_BYPRODUCTS "${output_library}"
        UPDATE_DISCONNECTED ${KVAZAAR_EP_UPDATE_DISCONNECTED}
        CMAKE_ARGS
            -DBUILD_SHARED_LIBS=OFF
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON  # Force kvazaar to be built with -fPIC
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCOMPILE_10BIT=ON
            -DBUILD_TESTS=OFF
            -DCMAKE_C_FLAGS=-fPIC
            -DCMAKE_CXX_FLAGS=-fPIC
        LOG_DOWNLOAD ON
        LOG_UPDATE ON
        LOG_PATCH ON
        LOG_CONFIGURE ON
        LOG_BUILD ON
        LOG_INSTALL ON
        LOG_TEST ON
        LOG_MERGED_STDOUTERR ON
        LOG_OUTPUT_ON_FAILURE ON
    )
endfunction()
