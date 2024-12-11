project(BasicExample)

add_executable(BasicExample main.cpp)
target_link_libraries(BasicExample PRIVATE SDL3::SDL3 SDL_slang)
