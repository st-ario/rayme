cmake_minimum_required(VERSION 3.20.1)

project(rayme)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME}
  bvh.cpp
  camera.cpp
  image_creation.cpp
  images.cpp
  materials.cpp
  math.cpp
  meshes.cpp
  my_vectors.cpp
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
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
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

#add_subdirectory(extern/stb)
#add_subdirectory(extern/simdjson)

target_link_directories(${PROJECT_NAME}
    PRIVATE extern/stb
    PRIVATE extern/simdjson
)

add_library(stb extern/stb/stb_image_write.h)
set_target_properties(stb PROPERTIES LINKER_LANGUAGE CXX)
add_library(simdjson extern/simdjson/singleheader/simdjson.h)
set_target_properties(simdjson PROPERTIES LINKER_LANGUAGE CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(rayme PRIVATE Threads::Threads)