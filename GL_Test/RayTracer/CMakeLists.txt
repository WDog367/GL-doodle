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
	
add_executable(${ProjectId} ${SHADER_FILES} ${HEADER_FILES} ${SOURCE_FILES})
	
configure_file(../PROJECT_OPTIONS.h.in includes/PROJECT_OPTIONS.h)
target_include_directories(${ProjectId} PRIVATE ${CMAKE_BINARY_DIR}/${ProjectId}/includes)
target_include_directories(${ProjectId} PRIVATE ${CMAKE_SOURCE_DIR}/${ProjectId})

source_group("Shader Files" FILES ${SHADER_FILES})

target_compile_definitions(${ProjectId} PRIVATE PROJECT_NAME="${ProjectId}")

#Get Dependencies 
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)
find_package(glew CONFIG REQUIRED)

target_link_libraries(${ProjectId} ${OPENGL_LIBRARIES})
target_link_libraries(${ProjectId} glm)
target_link_libraries(${ProjectId} GLEW::glew_s)
target_link_libraries(${ProjectId} SDL2::SDL2 SDL2::SDL2main)

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