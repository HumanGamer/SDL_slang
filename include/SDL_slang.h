#ifndef __SDL_SLANG_H__
#define __SDL_SLANG_H__
#include <SDL3/SDL_gpu.h>

#ifndef SDL_SLANG_DECLSPEC
# if defined(SDL_PLATFORM_WINDOWS)
#  ifdef SDL_SLANG_EXPORTS
#   define SDL_SLANG_DECLSPEC __declspec(dllexport)
#  else
#   define SDL_SLANG_DECLSPEC
#  endif
# else
#  if defined(__GNUC__) && __GNUC__ >= 4
#   define SDL_SLANG_DECLSPEC __attribute__ ((visibility("default")))
#  else
#   define SDL_SLANG_DECLSPEC
#  endif
# endif
#endif

#define SDL_SLANG_CALL __cdecl

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct SDL_Slang_Define
    {
        const char* name;
        const char* value;
    } SDL_Slang_Define;

    SDL_SLANG_DECLSPEC bool SDL_SLANG_CALL SDL_Slang_Init();
    SDL_SLANG_DECLSPEC void SDL_SLANG_CALL SDL_Slang_Quit();

    SDL_SLANG_DECLSPEC SDL_GPUShader* SDL_SLANG_CALL SDL_Slang_CompileGraphicsShader(SDL_GPUDevice* device, SDL_GPUShaderStage shaderStage, const char* shader, const char* entryPoint, const char** searchPaths, int numSearchPaths, SDL_Slang_Define* defines, int numDefines);
    SDL_SLANG_DECLSPEC SDL_GPUComputePipeline* SDL_SLANG_CALL SDL_Slang_CompileComputeShader(SDL_GPUDevice* device, const char* shader, const char* entryPoint, const char** searchPaths, int numSearchPaths, SDL_Slang_Define* defines, int numDefines);

#ifdef __cplusplus
}
#endif

#endif
