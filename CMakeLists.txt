cmake_minimum_required(VERSION 3.15)

project(imj C)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

file(GLOB SRC_FILES src/*.c)

add_library(imj SHARED ${SRC_FILES})

set_target_properties(imj PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(imj PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(NOT MSVC)
    target_link_libraries(imj PRIVATE m)
endif()
target_compile_definitions(imj PRIVATE IMJ_EXPORTS)
if (MSVC)
    target_compile_options(imj PRIVATE
        $<$<CONFIG:Debug>:/Zi>
        $<$<CONFIG:Release>:/O2>
    )
else()
    target_compile_options(imj PRIVATE
        -Wall
        -Wextra
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Release>:-O3>
    )
endif()
set_target_properties(imj PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

install(TARGETS imj
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)

# --- example
message(STATUS "Configuring example ...")
add_executable(imj_example example/main.c)
target_include_directories(imj_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(imj_example PRIVATE imj)
set_target_properties(imj_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")