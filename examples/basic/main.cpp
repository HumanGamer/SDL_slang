#include <cstdio>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL_slang.h>

int main(int argc, const char** argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    auto device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXBC | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, nullptr);
    if (!device)
    {
        printf("Failed to create GPU device: %s\n", SDL_GetError());
        return 1;
    }

    auto window = SDL_CreateWindow("Basic Example", 800, 600, 0);
    if (!window)
    {
        printf("Failed to create window: %s\n", SDL_GetError());
        return 1;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        printf("Failed to claim window for GPU device: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Slang_Init();

    SDL_GPUShader* shader = SDL_Slang_CompileGraphicsShader(device, SDL_GPU_SHADERSTAGE_VERTEX, "assets/shader.slang", "vertexMain", nullptr, 0, nullptr, 0);
    if (!shader)
    {
        printf("Failed to compile shader: %s\n", SDL_GetError());
        return 1;
    }

    SDL_ReleaseGPUShader(device, shader);

    SDL_Slang_Quit();

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyWindow(window);
    SDL_DestroyGPUDevice(device);
    SDL_Quit();

    return 0;
}
