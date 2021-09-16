cmake_minimum_required(VERSION 3.20.1)

project(rayme)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
## set(GCC_WARNING_FLAGS "-Wall -Wextra")
set(GCC_WARNING_FLAGS "")

add_executable(${PROJECT_NAME}
  bdf.cpp
  bvh.cpp
  camera.cpp
  gltf_parser.cpp
  image_creation.cpp
  images.cpp
  integrator.cpp
  math.cpp
  meshes.cpp
  render.cpp
  extern/simdjson/singleheader/simdjson.cpp
  )

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


target_link_directories(${PROJECT_NAME}
    PRIVATE extern/stb
    PRIVATE extern/simdjson
    PRIVATE extern/glm
)

add_library(stb extern/stb/stb_image_write.h)
set_target_properties(stb PROPERTIES LINKER_LANGUAGE CXX)
add_library(simdjson extern/simdjson/singleheader/simdjson.h)
set_target_properties(simdjson PROPERTIES LINKER_LANGUAGE CXX)
add_library(glm extern/glm/glm/glm.hpp)
set_target_properties(glm PROPERTIES LINKER_LANGUAGE CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(rayme PRIVATE Threads::Threads)

add_definitions(${GCC_WARNING_FLAGS})
