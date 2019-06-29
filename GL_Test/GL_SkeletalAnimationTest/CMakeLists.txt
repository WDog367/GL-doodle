cmake_minimum_required(VERSION 3.1)

file(GLOB_RECURSE SOURCE_FILES
		${CMAKE_SOURCE_DIR}/GL_SkeletalAnimationTest/*.cpp)
		
file(GLOB_RECURSE HEADER_FILES
	${CMAKE_SOURCE_DIR}/GL_SkeletalAnimationTest/*.h)
	
file(GLOB_RECURSE SHADER_FILES
	${CMAKE_SOURCE_DIR}/GL_SkeletalAnimationTest/*.glsl)
	
file(GLOB_RECURSE MODEL_FILES
	${CMAKE_SOURCE_DIR}/GL_SkeletalAnimationTest/*.obj)
	
add_executable(GL_SkeletalAnimationTest ${SHADER_FILES} ${HEADER_FILES} ${SOURCE_FILES})
	
configure_file(../PROJECT_OPTIONS.h.in includes/PROJECT_OPTIONS.h)
target_include_directories(GL_SkeletalAnimationTest PRIVATE ${CMAKE_BINARY_DIR}/GL_SkeletalAnimationTest/includes)
target_include_directories(GL_SkeletalAnimationTest PRIVATE ${CMAKE_SOURCE_DIR}/GL_SkeletalAnimationTest)

source_group("Shader Files" FILES ${SHADER_FILES})

target_compile_definitions(GL_SkeletalAnimationTest PRIVATE PROJECT_NAME="GL_SkeletalAnimationTest")

#Get Dependencies
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)
find_package(glew CONFIG REQUIRED)
find_package(tinyxml2 REQUIRED)

target_link_libraries(GL_SkeletalAnimationTest ${OPENGL_LIBRARIES})
target_link_libraries(GL_SkeletalAnimationTest glm)
target_link_libraries(GL_SkeletalAnimationTest GLEW::glew_s)
target_link_libraries(GL_SkeletalAnimationTest SDL2::SDL2 SDL2::SDL2main)
target_link_libraries(GL_SkeletalAnimationTest tinyxml2::tinyxml2)

# Copy DLLs to output folder on Windows
get_target_property(LINKED_LIBRARIES GL_SkeletalAnimationTest INTERFACE_LINK_LIBRARIES)
if(WIN32)
    foreach(CUR_LIB ${LINKED_LIBRARIES})
		if(TARGET ${CUR_LIB})
			get_target_property(CUR_TYPE ${CUR_LIB} TYPE)
			if( CUR_TYPE MATCHES "SHARED_LIBRARY")
				add_custom_command(TARGET GL_SkeletalAnimationTest POST_BUILD COMMAND
					${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${CUR_LIB}> $<TARGET_FILE_DIR:GL_SkeletalAnimationTest>)
			endif() 
		endif()
    endforeach()
endif()