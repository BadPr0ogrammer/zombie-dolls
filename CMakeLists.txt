cmake_minimum_required (VERSION 3.14)
project (zombie-dolls)

set (CMAKE_CXX_STANDARD 17)

set (Urho3D_DIR "" CACHE PATH "../rbfx/build/install/share/CMake")
message (STATUS "set Urho3D_DIR to ${Urho3D_DIR}")

set (Urho3D_Generated_DIR "" CACHE PATH "../rbfx/build/install/share/CMake")
message(STATUS "set Urho3D_Generated_DIR to ${Urho3D_Generated_DIR}")

add_executable (${PROJECT_NAME}
    ${PROJECT_SOURCE_DIR}/Source/SamplesManager.cpp)

find_package (Urho3D REQUIRED ${Urho3D_DIR})

file (GLOB sources
	${PROJECT_SOURCE_DIR}/Source/*.cpp
	${PROJECT_SOURCE_DIR}/Source/*.h
)

target_sources (${PROJECT_NAME} PRIVATE ${sources})

get_target_property (Urho3Dincl Urho3D INTERFACE_INCLUDE_DIRECTORIES)
get_target_property (Urho3Ddefs Urho3D INTERFACE_COMPILE_DEFINITIONS)
get_target_property (Urho3Dlink Urho3D INTERFACE_LINK_LIBRARIES)

target_include_directories (${PROJECT_NAME} PRIVATE
    ${Urho3Dincl}
    ${PROJECT_SOURCE_DIR}/Source
    )

target_compile_definitions (${PROJECT_NAME} PRIVATE ${Urho3Ddefs})

target_link_libraries (${PROJECT_NAME} PRIVATE ${Urho3Dlink})

# copy to the target "${Urho3D_Generated_DIR}/../../bin/Debug/Urho3D.dll"
# set (Urho3DDll "${Urho3D_Generated_DIR}/../../bin/Debug/Urho3D.dll")
# add_custom_command (TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND}
#    ARGS -E copy_if_different ${Urho3DDll} ${CMAKE_BINARY_DIR}/Debug)
