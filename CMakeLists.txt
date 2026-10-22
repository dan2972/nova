cmake_minimum_required(VERSION 3.22)
project(Nova)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(slang CONFIG REQUIRED)
find_package(spdlog REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    volk::volk
    glm::glm
    slang::slang
    spdlog::spdlog
    KTX::ktx
    tinyobjloader::tinyobjloader
    SDL3::SDL3
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VK_ENABLE_BETA_EXTENSIONS
    SPDLOG_NO_EXCEPTIONS
)

function(copy_target_resources TARGET_NAME SOURCE_DIR DEST_DIR)
    add_custom_target(${TARGET_NAME}_copy_folder ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${SOURCE_DIR} ${DEST_DIR}
        COMMENT "Copying resources for ${TARGET_NAME} to ${DEST_DIR}"
        VERBATIM
    )
    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_copy_folder)
endfunction()

copy_target_resources(${PROJECT_NAME} 
    ${CMAKE_CURRENT_SOURCE_DIR}/res 
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/res
)
