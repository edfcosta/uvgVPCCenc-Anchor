if(NOT DEFINED source_dir)
    message(FATAL_ERROR "source_dir is required to apply multilib patch")
endif()
if(NOT DEFINED patch_file)
    message(FATAL_ERROR "patch_file is required to apply multilib patch")
endif()

if(NOT EXISTS "${patch_file}")
    message(FATAL_ERROR "multilib.patch not found at ${patch_file}")
endif()

find_program(GIT_EXECUTABLE git)
if(NOT GIT_EXECUTABLE)
    message(FATAL_ERROR "git not found; required to apply multilib.patch")
endif()

execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${source_dir}" apply --check "${patch_file}"
    RESULT_VARIABLE patch_check_result
)

if(patch_check_result EQUAL 0)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${source_dir}" apply --whitespace=nowarn "${patch_file}"
        RESULT_VARIABLE patch_apply_result
    )
    if(NOT patch_apply_result EQUAL 0)
        message(FATAL_ERROR "Failed to apply multilib.patch")
    endif()
else()
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${source_dir}" apply --reverse --check "${patch_file}"
        RESULT_VARIABLE patch_reverse_check_result
    )
    if(patch_reverse_check_result EQUAL 0)
        message(STATUS "multilib.patch already applied; skipping")
    else()
        message(FATAL_ERROR "multilib.patch cannot be applied or reversed cleanly")
    endif()
endif()
