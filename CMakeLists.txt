cmake_minimum_required(VERSION 3.16)

project(rayme)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O3")
##set(CMAKE_CXX_FLAGS_DEBUG "-g -gdwarf-4 -fvar-tracking-assignments -O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
## set(GCC_WARNING_FLAGS "-Wall -Wextra -Wno-deprecated -fno-elide-type -fdiagnostics-show-template-tree -Wall -Wextra -Wpedantic -Wvla -Wextra-semi -Wnull-dereference -Wswitch-enum -Wduplicated-cond -Wduplicated-branches -Wsuggest-override  -Wfloat-conversion")
set(GCC_WARNING_FLAGS "")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost 1.60 COMPONENTS program_options REQUIRED)

#####    DOWNLOAD ALL THE SUBMODULES
find_package(Git QUIET)

if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

#####    CHECK ALL THE SUBMODULES
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/stb/stb_image_write.h")
    message(FATAL_ERROR "The stb submodule was not downloaded! GIT_SUBMODULE was turned off or failed; please update submodules and try again.")
endif()
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/simdjson/singleheader/simdjson.h")
    message(FATAL_ERROR "The simdjson submodule was not downloaded! GIT_SUBMODULE was turned off or failed; please update submodules and try again.")
endif()
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/simdjson/singleheader/simdjson.cpp")
    message(FATAL_ERROR "The simdjson submodule was not downloaded! GIT_SUBMODULE was turned off or failed; please update submodules and try again.")
endif()
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/glm/CMakeLists.txt")
    message(FATAL_ERROR "The glm submodule was not downloaded! GIT_SUBMODULE was turned off or failed; please update submodules and try again.")
endif()

add_library(stb extern/stb/stb_image_write.h)
set_target_properties(stb PROPERTIES LINKER_LANGUAGE CXX)
add_library(simdjson extern/simdjson/singleheader/simdjson.h)
set_target_properties(simdjson PROPERTIES LINKER_LANGUAGE CXX)
add_library(glm extern/glm/glm/glm.hpp)
set_target_properties(glm PROPERTIES LINKER_LANGUAGE CXX)

find_library(OID OpenImageDenoise)
if (NOT OID)
    ## compile without denoise
    message("WARNING: Intel(R) OpenImageDenoise library not found, the program will be compiled
        WITHOUT denosing options")
    add_compile_definitions(NO_DENOISE=1)

    add_executable(rayme
      bdf.cpp
      bvh.cpp
      camera.cpp
      gltf_parser.cpp
      images.cpp
      integrator.cpp
      main.cpp
      math.cpp
      meshes.cpp
      render.cpp
      rng.cpp
      extern/simdjson/singleheader/simdjson.cpp
      2d_samples/hardcoded_2d_rng.cpp
      )
else()
    ## compile with denoise
    add_executable(rayme
      bdf.cpp
      bvh.cpp
      camera.cpp
      denoise.cpp
      gltf_parser.cpp
      images.cpp
      integrator.cpp
      main.cpp
      math.cpp
      meshes.cpp
      render.cpp
      rng.cpp
      extern/simdjson/singleheader/simdjson.cpp
      2d_samples/hardcoded_2d_rng.cpp
      )
target_link_libraries(rayme PRIVATE "${OID}")
endif()

## add_compile_definitions(NO_DENOISE=1)

target_link_libraries(rayme PRIVATE Boost::program_options)
target_link_libraries(rayme PRIVATE Threads::Threads)

target_link_directories(${PROJECT_NAME}
    PRIVATE extern/stb
    PRIVATE extern/simdjson
    PRIVATE extern/glm
)

add_definitions(${GCC_WARNING_FLAGS})