cmake_minimum_required(VERSION 3.1)
project(GL_doodle)

add_subdirectory(${CMAKE_SOURCE_DIR}/SimpleGame)
add_subdirectory(${CMAKE_SOURCE_DIR}/GL_Invaders)
add_subdirectory(${CMAKE_SOURCE_DIR}/GL_SkeletalAnimationTest)
add_subdirectory(${CMAKE_SOURCE_DIR}/GL_Test)
add_subdirectory(${CMAKE_SOURCE_DIR}/RayTracer)
add_subdirectory(${CMAKE_SOURCE_DIR}/ModelViewer)
add_subdirectory(${CMAKE_SOURCE_DIR}/GL_RendererDoodle)

INCLUDE_EXTERNAL_MSPROJECT(Vulkan_Spirv_Compile ${CMAKE_SOURCE_DIR}/../VisualStudioProjects/Vulkan_Spirv_Compile)