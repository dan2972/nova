cmake_minimum_required(VERSION 3.22)
project(Nova)

file(GLOB_RECURSE NOVA_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(slang CONFIG REQUIRED)
find_package(spdlog REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

add_library(Nova STATIC ${NOVA_SOURCES})
add_library(Nova::Nova ALIAS Nova)

target_compile_features(Nova PUBLIC cxx_std_20)

target_include_directories(Nova PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Nova PUBLIC
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    volk::volk
    glm::glm
    slang::slang
    spdlog::spdlog
    KTX::ktx
    SDL3::SDL3
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    VK_ENABLE_BETA_EXTENSIONS
    SPDLOG_NO_EXCEPTIONS
)
