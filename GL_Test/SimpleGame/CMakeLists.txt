cmake_minimum_required(VERSION 3.1)

file(GLOB_RECURSE SOURCE_FILES
		${CMAKE_SOURCE_DIR}/SimpleGame/*.cpp)
		
file(GLOB_RECURSE HEADER_FILES
	${CMAKE_SOURCE_DIR}/SimpleGame/*.h)
	
file(GLOB_RECURSE SHADER_FILES
	${CMAKE_SOURCE_DIR}/SimpleGame/*.glsl
	${CMAKE_SOURCE_DIR}/RayTracer/*.vs
	${CMAKE_SOURCE_DIR}/RayTracer/*.fs)
	
file(GLOB_RECURSE MODEL_FILES
	${CMAKE_SOURCE_DIR}/SimpleGame/*.obj)
	
add_executable(SimpleGame ${SHADER_FILES} ${HEADER_FILES} ${SOURCE_FILES})
source_group("Shader Files" FILES ${SHADER_FILES})

configure_file(../PROJECT_OPTIONS.h.in includes/PROJECT_OPTIONS.h)

target_include_directories(SimpleGame PRIVATE ${CMAKE_BINARY_DIR}/SimpleGame/includes)
target_include_directories(SimpleGame PRIVATE ${CMAKE_SOURCE_DIR}/SimpleGame)

target_compile_definitions(SimpleGame PRIVATE PROJECT_NAME="SimpleGame")

# Get Dependencies
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)
find_package(glew CONFIG REQUIRED)
find_package(eigen3 REQUIRED)
find_package(freetype CONFIG REQUIRED)

target_link_libraries(SimpleGame ${OPENGL_LIBRARIES})
target_link_libraries(SimpleGame glm)
target_link_libraries(SimpleGame GLEW::glew_s)
target_link_libraries(SimpleGame SDL2::SDL2 SDL2::SDL2main)
target_link_libraries(SimpleGame Eigen3::Eigen)
target_link_libraries(SimpleGame freetype)

# Copy DLLs to output folder on Windows
get_target_property(LINKED_LIBRARIES SimpleGame INTERFACE_LINK_LIBRARIES)
if(WIN32)
    foreach(CUR_LIB ${LINKED_LIBRARIES})
		if(TARGET ${CUR_LIB})
			get_target_property(CUR_TYPE ${CUR_LIB} TYPE)
			if( CUR_TYPE MATCHES "SHARED_LIBRARY")
				add_custom_command(TARGET SimpleGame POST_BUILD COMMAND
					${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${CUR_LIB}> $<TARGET_FILE_DIR:SimpleGame>)
			endif() 
		endif()
    endforeach()
endif()