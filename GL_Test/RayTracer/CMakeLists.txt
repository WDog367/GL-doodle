cmake_minimum_required(VERSION 3.1)

file(GLOB_RECURSE SOURCE_FILES
		${CMAKE_SOURCE_DIR}/RayTracer/*.cpp)
		
file(GLOB_RECURSE HEADER_FILES
	${CMAKE_SOURCE_DIR}/RayTracer/*.h)
	
file(GLOB_RECURSE SHADER_FILES
	${CMAKE_SOURCE_DIR}/RayTracer/*.glsl
	${CMAKE_SOURCE_DIR}/RayTracer/*.vs
	${CMAKE_SOURCE_DIR}/RayTracer/*.fs)
	
file(GLOB_RECURSE MODEL_FILES
	${CMAKE_SOURCE_DIR}/RayTracer/*.obj)
	
add_executable(RayTracer ${SHADER_FILES} ${HEADER_FILES} ${SOURCE_FILES})
	
configure_file(../PROJECT_OPTIONS.h.in includes/PROJECT_OPTIONS.h)
target_include_directories(RayTracer PRIVATE ${CMAKE_BINARY_DIR}/RayTracer/includes)
target_include_directories(RayTracer PRIVATE ${CMAKE_SOURCE_DIR}/RayTracer)

source_group("Shader Files" FILES ${SHADER_FILES})

target_compile_definitions(RayTracer PRIVATE PROJECT_NAME="RayTracer")

#Get Dependencies 
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)
find_package(glew CONFIG REQUIRED)

target_link_libraries(RayTracer ${OPENGL_LIBRARIES})
target_link_libraries(RayTracer glm)
target_link_libraries(RayTracer GLEW::glew_s)
target_link_libraries(RayTracer SDL2::SDL2 SDL2::SDL2main)

# Copy DLLs to output folder on Windows
get_target_property(LINKED_LIBRARIES RayTracer INTERFACE_LINK_LIBRARIES)
if(WIN32)
    foreach(CUR_LIB ${LINKED_LIBRARIES})
		if(TARGET ${CUR_LIB})
			get_target_property(CUR_TYPE ${CUR_LIB} TYPE)
			if( CUR_TYPE MATCHES "SHARED_LIBRARY")
				add_custom_command(TARGET RayTracer POST_BUILD COMMAND
					${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${CUR_LIB}> $<TARGET_FILE_DIR:RayTracer>)
			endif() 
		endif()
    endforeach()
endif()