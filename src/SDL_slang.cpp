#include "SDL_slang.h"

#include <cstdio>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <slang.h>
#include <slang-com-ptr.h>

Slang::ComPtr<slang::IGlobalSession> globalSession;

SDL_SLANG_DECLSPEC bool SDL_SLANG_CALL SDL_Slang_Init()
{
    slang::createGlobalSession(globalSession.writeRef());
    return true;
}

SDL_SLANG_DECLSPEC void SDL_SLANG_CALL SDL_Slang_Quit()
{
    globalSession = nullptr;
}

static void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob)
{
    if (diagnosticsBlob != nullptr)
        fprintf(stderr, "%s\n", static_cast<const char *>(diagnosticsBlob->getBufferPointer()));
}

static Slang::ComPtr<slang::IComponentType> compileShader(const std::vector<slang::TargetDesc>& targets, const char* shader, const char* entryPoint, const char** searchPaths, int numSearchPaths, SDL_Slang_Define* defines, int numDefines)
{
    slang::SessionDesc sessionDesc{};
    sessionDesc.targets = targets.data();
    sessionDesc.targetCount = static_cast<SlangInt>(targets.size());
    sessionDesc.searchPaths = searchPaths;
    sessionDesc.searchPathCount = numSearchPaths;

    std::vector<slang::PreprocessorMacroDesc> macros;
    for (int i = 0; i < numDefines; i++)
    {
        slang::PreprocessorMacroDesc macroDesc = { defines[i].name, defines[i].value };
        macros.push_back(macroDesc);
    }

    sessionDesc.preprocessorMacros = macros.data();
    sessionDesc.preprocessorMacroCount = static_cast<SlangInt>(macros.size());

    Slang::ComPtr<slang::ISession> session;
    SlangResult result = globalSession->createSession(sessionDesc, session.writeRef());
    if (result != SLANG_OK)
    {
        int facility = SLANG_GET_RESULT_FACILITY(result);
        int code = SLANG_GET_RESULT_CODE(result);
        SDL_SetError("Failed to create session: %x, %x", facility, code);
        return nullptr;
    }

    Slang::ComPtr<slang::IBlob> diagnostics;
    slang::IModule* module = session->loadModule(shader, diagnostics.writeRef());

    diagnoseIfNeeded(diagnostics);
    diagnostics = nullptr;

    if (module == nullptr)
    {
        SDL_SetError("Failed to open shader");
        return nullptr;
    }

    for (int i = 0; i < module->getDefinedEntryPointCount(); i++)
    {
        Slang::ComPtr<slang::IEntryPoint> ep;
        module->getDefinedEntryPoint(i, ep.writeRef());

        const char* name = ep->getFunctionReflection()->getName();
        SDL_Log("Name: %s", name);
    }

    Slang::ComPtr<slang::IEntryPoint> entryPointPtr;
    result = module->findEntryPointByName(entryPoint, entryPointPtr.writeRef());

    if (entryPointPtr == nullptr)
    {
        int facility = SLANG_GET_RESULT_FACILITY(result);
        int code = SLANG_GET_RESULT_CODE(result);

        //if (result == SLANG_E_INVALID_ARG)
        //    SDL_SetError("Failed to find entry point: %s", entryPoint);
        //else
        SDL_SetError("Failed to find entry point: %x, %x", facility, code);
        return nullptr;
    }

    slang::IComponentType* componentTypes[] = { module, entryPointPtr };
    Slang::ComPtr<slang::IComponentType> program;
    session->createCompositeComponentType(componentTypes, 2, program.writeRef(), diagnostics.writeRef());

    diagnoseIfNeeded(diagnostics);
    diagnostics = nullptr;

    /*slang::ProgramLayout* programLayout = program->getLayout(0, diagnostics.writeRef());

    diagnoseIfNeeded(diagnostics);
    diagnostics = nullptr;*/

    Slang::ComPtr<slang::IComponentType> linkedProgram;
    program->link(linkedProgram.writeRef(), diagnostics.writeRef());

    diagnoseIfNeeded(diagnostics);
    diagnostics = nullptr;

    return linkedProgram;
}

void* SDL_CreateGPUShader_Slang_Internal(SDL_GPUDevice* device, SDL_GPUShaderStage shaderStage, bool compute, const char* shader, const char* entryPoint, const char** searchPaths, int numSearchPaths, SDL_Slang_Define* defines, int numDefines)
{
    SDL_GPUShaderFormat shader_formats = SDL_GetGPUShaderFormats(device);

    std::vector<slang::TargetDesc> targetDescriptions;
    if (shader_formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        slang::TargetDesc targetDesc;
        targetDesc.format = SLANG_SPIRV;
        targetDesc.profile = globalSession->findProfile("glsl_450");
        targetDescriptions.push_back(targetDesc);
    } else if (shader_formats & SDL_GPU_SHADERFORMAT_DXBC)
    {
        slang::TargetDesc targetDesc;
        targetDesc.format = SLANG_DXBC;
        targetDesc.profile = globalSession->findProfile("sm_5_0");
        targetDescriptions.push_back(targetDesc);
    } else if (shader_formats & SDL_GPU_SHADERFORMAT_DXIL)
    {
        slang::TargetDesc targetDesc;
        targetDesc.format = SLANG_DXIL;
        targetDesc.profile = globalSession->findProfile("sm_5_0");
        targetDescriptions.push_back(targetDesc);
    } else if (shader_formats & SDL_GPU_SHADERFORMAT_MSL)
    {
        slang::TargetDesc targetDesc;
        targetDesc.format = SLANG_METAL;
        targetDesc.profile = globalSession->findProfile("metal");
        targetDescriptions.push_back(targetDesc);
    }

    Slang::ComPtr<slang::IBlob> diagnostics;

    Slang::ComPtr<slang::IComponentType> linkedProgram = compileShader(targetDescriptions, shader, entryPoint, searchPaths, numSearchPaths, defines, numDefines);

    if (linkedProgram == nullptr)
    {
        return nullptr;
    }

    int targetIndex = 0;
    Slang::ComPtr<slang::IBlob> codeBlob;
    linkedProgram->getEntryPointCode(0, targetIndex, codeBlob.writeRef(), diagnostics.writeRef());

    diagnoseIfNeeded(diagnostics);
    diagnostics = nullptr;

    const Uint8 *code = static_cast<Uint8*>(const_cast<void*>(codeBlob->getBufferPointer()));
    size_t codeSize = codeBlob->getBufferSize();

    SDL_GPUShaderFormat format;
    switch (targetDescriptions[targetIndex].format)
    {
        case SLANG_SPIRV:
            format = SDL_GPU_SHADERFORMAT_SPIRV;
        break;
        case SLANG_DXBC:
            format = SDL_GPU_SHADERFORMAT_DXBC;
        break;
        case SLANG_DXIL:
            format = SDL_GPU_SHADERFORMAT_DXIL;
        break;
        case SLANG_METAL:
            format = SDL_GPU_SHADERFORMAT_MSL;
        break;
        default:
            SDL_SetError("Invalid Format");
        return nullptr;
    }

    if (compute)
    {
        SDL_GPUComputePipelineCreateInfo createInfo;
        createInfo.code = code;
        createInfo.code_size = codeSize;
        createInfo.entrypoint = entryPoint;
        createInfo.format = format;
        createInfo.props = 0;

        slang::ProgramLayout* programLayout = linkedProgram->getLayout(targetIndex, diagnostics.writeRef());

        diagnoseIfNeeded(diagnostics);
        diagnostics = nullptr;

        Uint32 readOnlyStorageBufferCount = 0;
        Uint32 readWriteStorageBufferCount = 0;
        Uint32 readOnlyStorageTextureCount = 0;
        Uint32 readWriteStorageTextureCount = 0;
        Uint32 samplerCount = 0;
        Uint32 uniformBufferCount = 0;

        Uint32 parameterCount = programLayout->getParameterCount();
        for (Uint32 i = 0; i < parameterCount; i++)
        {
            slang::VariableLayoutReflection* parameter = programLayout->getParameterByIndex(i);

            auto tl = parameter->getTypeLayout();
            slang::TypeReflection::Kind kind = tl->getKind();
            if (kind == slang::TypeReflection::Kind::SamplerState)
            {
                samplerCount++;
                continue;
            }

            if (kind == slang::TypeReflection::Kind::ShaderStorageBuffer)
            {
                if (tl->getResourceAccess() == SLANG_RESOURCE_ACCESS_READ_WRITE)
                {
                    readWriteStorageBufferCount++;
                } else
                {
                    readOnlyStorageBufferCount++;
                }
                continue;
            }

            if (kind == slang::TypeReflection::Kind::TextureBuffer)
            {
                if (tl->getResourceAccess() == SLANG_RESOURCE_ACCESS_READ_WRITE)
                {
                    readWriteStorageTextureCount++;
                } else
                {
                    readOnlyStorageTextureCount++;
                }
                continue;
            }


            if (tl->getParameterCategory() == slang::ParameterCategory::Uniform)
            {
                uniformBufferCount++;
                continue;
            }
        }

        createInfo.num_samplers = samplerCount;
        createInfo.num_readonly_storage_textures = readOnlyStorageTextureCount;
        createInfo.num_readonly_storage_buffers = readOnlyStorageBufferCount;
        createInfo.num_readwrite_storage_textures = readWriteStorageTextureCount;
        createInfo.num_readwrite_storage_buffers = readWriteStorageBufferCount;
        createInfo.num_uniform_buffers = uniformBufferCount;
        createInfo.threadcount_x = 0;
        createInfo.threadcount_y = 0;
        createInfo.threadcount_z = 0;

        return SDL_CreateGPUComputePipeline(device, &createInfo);
    }

    SDL_GPUShaderCreateInfo createInfo;
    createInfo.code = code;
    createInfo.code_size = codeSize;
    createInfo.entrypoint = entryPoint;
    createInfo.format = format;
    createInfo.stage = shaderStage;
    createInfo.props = 0;

    slang::ProgramLayout* programLayout = linkedProgram->getLayout(targetIndex, diagnostics.writeRef());

    diagnoseIfNeeded(diagnostics);
    diagnostics = nullptr;

    Uint32 samplerCount = 0;
    Uint32 storageBufferCount = 0;
    Uint32 storageTextureCount = 0;
    Uint32 uniformBufferCount = 0;

    Uint32 parameterCount = programLayout->getParameterCount();
    for (Uint32 i = 0; i < parameterCount; i++)
    {
        slang::VariableLayoutReflection* parameter = programLayout->getParameterByIndex(i);

        auto tl = parameter->getTypeLayout();
        slang::TypeReflection::Kind kind = tl->getKind();
        if (kind == slang::TypeReflection::Kind::SamplerState)
        {
            samplerCount++;
            continue;
        }

        if (kind == slang::TypeReflection::Kind::ShaderStorageBuffer)
        {
            storageBufferCount++;
            continue;
        }

        if (kind == slang::TypeReflection::Kind::TextureBuffer)
        {
            storageTextureCount++;
            continue;
        }


        if (tl->getParameterCategory() == slang::ParameterCategory::Uniform)
        {
            uniformBufferCount++;
            continue;
        }
    }

    createInfo.num_samplers = samplerCount;
    createInfo.num_storage_buffers = storageBufferCount;
    createInfo.num_storage_textures = storageTextureCount;
    createInfo.num_uniform_buffers = uniformBufferCount;

    return SDL_CreateGPUShader(device, &createInfo);
}

SDL_SLANG_DECLSPEC SDL_GPUShader* SDL_SLANG_CALL SDL_Slang_CompileGraphicsShader(SDL_GPUDevice* device, SDL_GPUShaderStage shaderStage, const char* shader, const char* entryPoint, const char** searchPaths, int numSearchPaths, SDL_Slang_Define* defines, int numDefines)
{
    return static_cast<SDL_GPUShader *>(SDL_CreateGPUShader_Slang_Internal(device, shaderStage, false, shader, entryPoint, searchPaths, numSearchPaths, defines, numDefines));
}

SDL_SLANG_DECLSPEC SDL_GPUComputePipeline* SDL_SLANG_CALL SDL_Slang_CompileComputeShader(SDL_GPUDevice* device, const char* shader, const char* entryPoint, const char** searchPaths, int numSearchPaths, SDL_Slang_Define* defines, int numDefines)
{
    return static_cast<SDL_GPUComputePipeline *>(SDL_CreateGPUShader_Slang_Internal(device, SDL_GPU_SHADERSTAGE_VERTEX, true, shader, entryPoint, searchPaths, numSearchPaths, defines, numDefines));
}