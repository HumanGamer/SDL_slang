project(SDL_slang)

set(CMAKE_CXX_STANDARD 17)

add_library(${PROJECT_NAME} SHARED "")
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3 slang::slang)

target_sources(${PROJECT_NAME}
    PRIVATE
        SDL_slang.cpp

    PUBLIC
        ../include/SDL_slang.h
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ../include
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        SDL_SLANG_EXPORTS
)

add_dependencies(${PROJECT_NAME} prjcopy_assets)
