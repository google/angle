//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libGLESv2/renderer/d3d/HLSLCompiler.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/main.h"

#include "common/utilities.h"

#include "third_party/trace_event/trace_event.h"

namespace rx
{

CompileConfig::CompileConfig()
    : flags(0),
      name()
{
}

CompileConfig::CompileConfig(UINT flags, const std::string &name)
    : flags(flags),
      name(name)
{
}

HLSLCompiler::HLSLCompiler()
    : mD3DCompilerModule(NULL),
      mD3DCompileFunc(NULL),
      mD3DDisassembleFunc(NULL)
{
}

HLSLCompiler::~HLSLCompiler()
{
    release();
}

bool HLSLCompiler::initialize()
{
    TRACE_EVENT0("gpu", "initializeCompiler");
#if defined(ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES)
    // Find a D3DCompiler module that had already been loaded based on a predefined list of versions.
    static const char *d3dCompilerNames[] = ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES;

    for (size_t i = 0; i < ArraySize(d3dCompilerNames); ++i)
    {
        if (GetModuleHandleExA(0, d3dCompilerNames[i], &mD3DCompilerModule))
        {
            break;
        }
    }
#endif  // ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES

    if (!mD3DCompilerModule)
    {
        // Load the version of the D3DCompiler DLL associated with the Direct3D version ANGLE was built with.
        mD3DCompilerModule = LoadLibrary(D3DCOMPILER_DLL);
    }

    if (!mD3DCompilerModule)
    {
        ERR("No D3D compiler module found - aborting!\n");
        return false;
    }

    mD3DCompileFunc = reinterpret_cast<pD3DCompile>(GetProcAddress(mD3DCompilerModule, "D3DCompile"));
    ASSERT(mD3DCompileFunc);

    mD3DDisassembleFunc = reinterpret_cast<pD3DDisassemble>(GetProcAddress(mD3DCompilerModule, "D3DDisassemble"));
    ASSERT(mD3DDisassembleFunc);

    return mD3DCompileFunc != NULL;
}

void HLSLCompiler::release()
{
    if (mD3DCompilerModule)
    {
        FreeLibrary(mD3DCompilerModule);
        mD3DCompilerModule = NULL;
        mD3DCompileFunc = NULL;
        mD3DDisassembleFunc = NULL;
    }
}

gl::Error HLSLCompiler::compileToBinary(gl::InfoLog &infoLog, const std::string &hlsl, const std::string &profile,
                                        const std::vector<CompileConfig> &configs, const D3D_SHADER_MACRO *overrideMacros,
                                        ID3DBlob **outCompiledBlob, std::string *outDebugInfo) const
{
    ASSERT(mD3DCompilerModule && mD3DCompileFunc);

    if (gl::perfActive())
    {
        std::string sourcePath = getTempPath();
        std::string sourceText = FormatString("#line 2 \"%s\"\n\n%s", sourcePath.c_str(), hlsl.c_str());
        writeFile(sourcePath.c_str(), sourceText.c_str(), sourceText.size());
    }

    const D3D_SHADER_MACRO *macros = overrideMacros ? overrideMacros : NULL;

    for (size_t i = 0; i < configs.size(); ++i)
    {
        ID3DBlob *errorMessage = NULL;
        ID3DBlob *binary = NULL;

        HRESULT result = mD3DCompileFunc(hlsl.c_str(), hlsl.length(), gl::g_fakepath, macros, NULL, "main", profile.c_str(),
                                         configs[i].flags, 0, &binary, &errorMessage);

        if (errorMessage)
        {
            std::string message = reinterpret_cast<const char*>(errorMessage->GetBufferPointer());
            SafeRelease(errorMessage);

            infoLog.appendSanitized(message.c_str());
            TRACE("\n%s", hlsl.c_str());
            TRACE("\n%s", message.c_str());

            if (message.find("error X3531:") != std::string::npos)   // "can't unroll loops marked with loop attribute"
            {
                macros = NULL;   // Disable [loop] and [flatten]

                // Retry without changing compiler flags
                i--;
                continue;
            }
        }

        if (SUCCEEDED(result))
        {
            *outCompiledBlob = binary;

#ifdef ANGLE_GENERATE_SHADER_DEBUG_INFO
            (*outDebugInfo) += "// COMPILER INPUT HLSL BEGIN\n\n" + hlsl + "\n// COMPILER INPUT HLSL END\n";
            (*outDebugInfo) += "\n\n// ASSEMBLY BEGIN\n\n";
            (*outDebugInfo) += "// Compiler configuration: " + configs[i].name + "\n// Flags:\n";
            (*outDebugInfo) += "\n" + disassembleBinary(binary) + "\n// ASSEMBLY END\n";
#endif

            return gl::Error(GL_NO_ERROR);
        }
        else
        {
            if (result == E_OUTOFMEMORY)
            {
                *outCompiledBlob = NULL;
                return gl::Error(GL_OUT_OF_MEMORY, "HLSL compiler had an unexpected failure, result: 0x%X.", result);
            }

            infoLog.append("Warning: D3D shader compilation failed with %s flags.", configs[i].name.c_str());

            if (i + 1 < configs.size())
            {
                infoLog.append(" Retrying with %s.\n", configs[i + 1].name.c_str());
            }
        }
    }

    // None of the configurations succeeded in compiling this shader but the compiler is still intact
    *outCompiledBlob = NULL;
    return gl::Error(GL_NO_ERROR);
}

std::string HLSLCompiler::disassembleBinary(ID3DBlob *shaderBinary) const
{
    // Retrieve disassembly
    UINT flags = D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS | D3D_DISASM_ENABLE_INSTRUCTION_NUMBERING;
    ID3DBlob *disassembly = NULL;
    pD3DDisassemble disassembleFunc = reinterpret_cast<pD3DDisassemble>(mD3DDisassembleFunc);
    LPCVOID buffer = shaderBinary->GetBufferPointer();
    SIZE_T bufSize = shaderBinary->GetBufferSize();
    HRESULT result = disassembleFunc(buffer, bufSize, flags, "", &disassembly);

    std::string asmSrc;
    if (SUCCEEDED(result))
    {
        asmSrc = reinterpret_cast<const char*>(disassembly->GetBufferPointer());
    }

    SafeRelease(disassembly);

    return asmSrc;
}

}
