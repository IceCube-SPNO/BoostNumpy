# CMake find script to search for the sphinx Python package.

set(SPHINX_FOUND FALSE CACHE BOOL "Sphinx found." FORCE)

if(PYTHON_FOUND)

    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import sphinx; print sphinx.__version__"
        RESULT_VARIABLE SPHINX_PROCESS_
        OUTPUT_VARIABLE SPHINX_VERSION_
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(SPHINX_PROCESS_ EQUAL 0)
        find_program(SPHINX_BUILD_ sphinx-build)
        if(SPHINX_BUILD_)
            set(SPHINX_FOUND TRUE CACHE BOOL "Sphinx found." FORCE)
            set(SPHINX_BUILD SPHINX_BUILD_ CACHE FILEPATH "The sphinx-build executable." FORCE)
            set(SPHINX_VERSION SPHINX_VERSION_ CACHE STRING "The version of sphinx." FORCE)
            message(STATUS "+ Found Sphinx ${SPHINX_VERSION}: ${SPHINX_BUILD}")
        endif()
    endif()

endif()