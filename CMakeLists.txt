cmake_minimum_required(VERSION 3.15)
project(imj C)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if (MINGW)
    set(CMAKE_C_FLAGS_DEBUG "-g -Wall -DIMJ_EXPORTS")
    set(CMAKE_C_FLAGS_RELEASE "-O2 -Wall -DIMJ_EXPORTS -s -DNDEBUG")
else()
    set(CMAKE_C_FLAGS_DEBUG "-g -Wall -DIMJ_EXPORTS")
    set(CMAKE_C_FLAGS_RELEASE "-O2 -Wall -DIMJ_EXPORTS -DNDEBUG")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB SRC_FILES src/*.c)

add_library(imj SHARED ${SRC_FILES})

target_link_libraries(imj m)

install(TARGETS imj
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)

# --- example
add_executable(imj_example example/main.c)
target_include_directories(imj_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(imj_example imj)