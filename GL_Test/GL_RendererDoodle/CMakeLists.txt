cmake_minimum_required(VERSION 3.1)

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})


file(GLOB_RECURSE SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
		
file(GLOB_RECURSE HEADER_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/*.h)
	
file(GLOB_RECURSE SHADER_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/*.glsl
	${CMAKE_CURRENT_SOURCE_DIR}/*.fs
	${CMAKE_CURRENT_SOURCE_DIR}/*.vs)
	
file(GLOB_RECURSE MODEL_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/*.obj)
	
	
# Add dear-imgui files as source files
set(DEAR_IMGUI_DIR ${CMAKE_SOURCE_DIR}/../ThirdParty/dear-imgui/)
file(GLOB DEAR_IMGUI_FILES
	${DEAR_IMGUI_DIR}/*.cpp
	${DEAR_IMGUI_DIR}/*.h)

	
# Add stb-image files as source files
set(STB_DIR ${CMAKE_SOURCE_DIR}/../ThirdParty/stb/)
file(GLOB STB_FILES
	${STB_DIR}/*.cpp
	${STB_DIR}/*.h)

add_executable(${ProjectId} ${STB_FILES} ${DEAR_IMGUI_FILES} ${SHADER_FILES} ${HEADER_FILES} ${SOURCE_FILES})
	
configure_file(${CMAKE_SOURCE_DIR}/PROJECT_OPTIONS.h.in ${CMAKE_CURRENT_BINARY_DIR}/includes/PROJECT_OPTIONS.h)
target_include_directories(${ProjectId} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/includes)
target_include_directories(${ProjectId} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${ProjectId} PRIVATE ${DEAR_IMGUI_DIR})
target_include_directories(${ProjectId} PRIVATE ${STB_DIR})

source_group("Shader Files" ${SHADER_FILES}) 
source_group("3rdParty\\Dear IMGUI" FILES ${DEAR_IMGUI_FILES})
source_group("3rdParty\\STB Image" FILES ${STB_FILES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES  ${HEADER_FILES} ${SOURCE_FILES} ${SHADER_FILES})

target_compile_definitions(${ProjectId} PRIVATE PROJECT_NAME="${ProjectId}")
target_compile_definitions(${ProjectId} PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLEW)

#Get Dependencies 
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)
find_package(glew CONFIG REQUIRED)
find_package(Python COMPONENTS Interpreter Development REQUIRED)

target_link_libraries(${ProjectId} ${OPENGL_LIBRARIES})
target_link_libraries(${ProjectId} glm)
target_link_libraries(${ProjectId} GLEW::glew_s)
target_link_libraries(${ProjectId} SDL2::SDL2 SDL2::SDL2main)
target_link_libraries(${ProjectId} Python::Python)

# Copy DLLs to output folder on Windows
get_target_property(LINKED_LIBRARIES ${ProjectId} INTERFACE_LINK_LIBRARIES)
if(WIN32)
    foreach(CUR_LIB ${LINKED_LIBRARIES})
		if(TARGET ${CUR_LIB})
			get_target_property(CUR_TYPE ${CUR_LIB} TYPE)
			if( CUR_TYPE MATCHES "SHARED_LIBRARY")
				add_custom_command(TARGET ${ProjectId} POST_BUILD COMMAND
					${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${CUR_LIB}> $<TARGET_FILE_DIR:${ProjectId}>)
			endif() 
		endif()
    endforeach()
endif()