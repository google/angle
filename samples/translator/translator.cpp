//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "GLSLANG/ShaderLang.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Return codes from main.
//
enum TFailCode {
    ESuccess = 0,
    EFailUsage,
    EFailCompile,
    EFailCompilerCreate,
};

static ShShaderType FindShaderType(char *lang);
static bool CompileFile(char *fileName, ShHandle, int);
static void usage();
static void FreeFileData(char **data);
static char** ReadFileData(char *fileName);
static void LogMsg(char* msg, const char* name, const int num, const char* logName);
static void PrintActiveVariables(ShHandle compiler, ShShaderInfo varType);

//Added to accomodate the multiple strings.
int OutputMultipleStrings = 1;

//
// Set up the per compile resources
//
void GenerateResources(ShBuiltInResources* resources)
{
    ShInitBuiltInResources(resources);

    resources->MaxVertexAttribs = 8;
    resources->MaxVertexUniformVectors = 128;
    resources->MaxVaryingVectors = 8;
    resources->MaxVertexTextureImageUnits = 0;
    resources->MaxCombinedTextureImageUnits = 8;
    resources->MaxTextureImageUnits = 8;
    resources->MaxFragmentUniformVectors = 16;
    resources->MaxDrawBuffers = 1;

    resources->OES_standard_derivatives = 0;
}

int main(int argc, char* argv[])
{
    TFailCode failCode = ESuccess;

    int compileOptions = 0;
    int numCompiles = 0;
    ShHandle vertexCompiler = 0;
    ShHandle fragmentCompiler = 0;
    char* buffer = 0;
    int bufferLen = 0;
    int numAttribs = 0, numUniforms = 0;

    ShInitialize();

    ShBuiltInResources resources;
    GenerateResources(&resources);

    argc--;
    argv++;
    for (; (argc >= 1) && (failCode == ESuccess); argc--, argv++) {
        if (argv[0][0] == '-' || argv[0][0] == '/') {
            switch (argv[0][1]) {
            case 'i': compileOptions |= SH_INTERMEDIATE_TREE; break;
            case 'o': compileOptions |= SH_OBJECT_CODE; break;
            case 'u': compileOptions |= SH_ATTRIBUTES_UNIFORMS; break;
            default: failCode = EFailUsage;
            }
        } else {
            ShHandle compiler = 0;
            switch (FindShaderType(argv[0])) {
            case SH_VERTEX_SHADER:
                if (vertexCompiler == 0)
                    vertexCompiler = ShConstructCompiler(SH_VERTEX_SHADER, SH_GLES2_SPEC, &resources);
                compiler = vertexCompiler;
                break;
            case SH_FRAGMENT_SHADER:
                if (fragmentCompiler == 0)
                    fragmentCompiler = ShConstructCompiler(SH_FRAGMENT_SHADER, SH_GLES2_SPEC, &resources);
                compiler = fragmentCompiler;
                break;
            default: break;
            }
            if (compiler) {
              bool compiled = CompileFile(argv[0], compiler, compileOptions);

              LogMsg("BEGIN", "COMPILER", numCompiles, "INFO LOG");
              ShGetInfo(compiler, SH_INFO_LOG_LENGTH, &bufferLen);
              buffer = (char*) realloc(buffer, bufferLen * sizeof(char));
              ShGetInfoLog(compiler, buffer);
              puts(buffer);
              LogMsg("END", "COMPILER", numCompiles, "INFO LOG");
              printf("\n\n");

              if (compiled && (compileOptions & SH_OBJECT_CODE)) {
                  LogMsg("BEGIN", "COMPILER", numCompiles, "OBJ CODE");
                  ShGetInfo(compiler, SH_OBJECT_CODE_LENGTH, &bufferLen);
                  buffer = (char*) realloc(buffer, bufferLen * sizeof(char));
                  ShGetObjectCode(compiler, buffer);
                  puts(buffer);
                  LogMsg("END", "COMPILER", numCompiles, "OBJ CODE");
                  printf("\n\n");
              }
              if (compiled && (compileOptions & SH_ATTRIBUTES_UNIFORMS)) {
                  LogMsg("BEGIN", "COMPILER", numCompiles, "ACTIVE ATTRIBS");
                  PrintActiveVariables(compiler, SH_ACTIVE_ATTRIBUTES);
                  LogMsg("END", "COMPILER", numCompiles, "ACTIVE ATTRIBS");
                  printf("\n\n");

                  LogMsg("BEGIN", "COMPILER", numCompiles, "ACTIVE UNIFORMS");
                  PrintActiveVariables(compiler, SH_ACTIVE_UNIFORMS);
                  LogMsg("END", "COMPILER", numCompiles, "ACTIVE UNIFORMS");
                  printf("\n\n");
              }
              if (!compiled)
                  failCode = EFailCompile;
              ++numCompiles;
            } else {
                failCode = EFailCompilerCreate;
            }
        }
    }

    if ((vertexCompiler == 0) && (fragmentCompiler == 0))
        failCode = EFailUsage;
    if (failCode == EFailUsage)
        usage();

    if (vertexCompiler)
        ShDestruct(vertexCompiler);
    if (fragmentCompiler)
        ShDestruct(fragmentCompiler);
    if (buffer)
        free(buffer);
    ShFinalize();

    return failCode;
}

//
//   Deduce the shader type from the filename.  Files must end in one of the
//   following extensions:
//
//   .frag*    = fragment shader
//   .vert*    = vertex shader
//
ShShaderType FindShaderType(char *name)
{
    if (!name)
        return SH_FRAGMENT_SHADER;

    char *ext = strrchr(name, '.');

    if (ext && strcmp(ext, ".sl") == 0)
        for (; ext > name && ext[0] != '.'; ext--);

    if (ext = strrchr(name, '.')) {
        if (strncmp(ext, ".frag", 4) == 0) return SH_FRAGMENT_SHADER;
        if (strncmp(ext, ".vert", 4) == 0) return SH_VERTEX_SHADER;
    }

    return SH_FRAGMENT_SHADER;
}

//
//   Read a file's data into a string, and compile it using ShCompile
//
bool CompileFile(char *fileName, ShHandle compiler, int compileOptions)
{
    int ret;
    char **data = ReadFileData(fileName);

    if (!data)
        return false;

    ret = ShCompile(compiler, data, OutputMultipleStrings, compileOptions);

    FreeFileData(data);

    return ret ? true : false;
}

//
//   print usage to stdout
//
void usage()
{
    printf("Usage: translate [-i -o -u] file1 file2 ...\n"
        "Where: filename = filename ending in .frag or .vert\n"
        "       -i = print intermediate tree\n"
        "       -o = print translated code\n"
        "       -u = print active attribs and uniforms\n");
}

//
//   Malloc a string of sufficient size and read a string into it.
//
# define MAX_SOURCE_STRINGS 5
char** ReadFileData(char *fileName) 
{
    FILE* in = fopen(fileName, "r");
    char* fdata = 0;
    int count = 0;
    char** return_data = (char**)malloc(MAX_SOURCE_STRINGS+1);

    if (!in) {
        printf("Error: unable to open input file: %s\n", fileName);
        return 0;
    }

    while (fgetc(in) != EOF)
        count++;

    fseek(in, 0, SEEK_SET);

    if (!(fdata = (char*)malloc(count+2))) {
        printf("Error allocating memory\n");
        return 0;
    }
    if (fread(fdata, 1, count, in) != count) {
        printf("Error reading input file: %s\n", fileName);
        return 0;
    }
    fdata[count] = '\0';
    fclose(in);
    if (count == 0){
        return_data[0] = (char*)malloc(count+2);
        return_data[0][0] = '\0';
        OutputMultipleStrings = 0;
        return return_data;  
    }

    int len = (int)(ceil)((float)count / (float)OutputMultipleStrings);
    int ptr_len = 0, i = 0;
    while (count > 0) {
        return_data[i] = (char*)malloc(len+2);
        memcpy(return_data[i], fdata+ptr_len, len);
        return_data[i][len] = '\0';
        count -= (len);
        ptr_len += (len);
        if (count < len){
            if(count == 0){
                OutputMultipleStrings = (i+1);
                break;
            }
            len = count;
        }
        ++i;
    }
    return return_data;
}

void FreeFileData(char** data)
{
    for(int i=0;i<OutputMultipleStrings;i++)
        free(data[i]);
}

void LogMsg(char* msg, const char* name, const int num, const char* logName)
{
    printf("#### %s %s %d %s ####\n", msg, name, num, logName);
}

void PrintActiveVariables(ShHandle compiler, ShShaderInfo varType)
{
    int nameSize = 0;
    switch (varType) {
        case SH_ACTIVE_ATTRIBUTES:
            ShGetInfo(compiler, SH_ACTIVE_ATTRIBUTE_MAX_LENGTH, &nameSize);
            break;
        case SH_ACTIVE_UNIFORMS:
            ShGetInfo(compiler, SH_ACTIVE_UNIFORM_MAX_LENGTH, &nameSize);
            break;
        default: assert(0);
    }
    if (nameSize <= 1) return;
    char* name = (char*) malloc(nameSize * sizeof(char));

    int activeVars = 0, size = 0;
    ShDataType type = SH_NONE;
    char* typeName = NULL;
    ShGetInfo(compiler, varType, &activeVars);
    for (int i = 0; i < activeVars; ++i) {
        switch (varType) {
            case SH_ACTIVE_ATTRIBUTES:
                ShGetActiveAttrib(compiler, i, NULL, &size, &type, name);
                break;
            case SH_ACTIVE_UNIFORMS:
                ShGetActiveUniform(compiler, i, NULL, &size, &type, name);
                break;
            default: assert(0);
        }
        switch (type) {
            case SH_FLOAT: typeName = "GL_FLOAT"; break;
            case SH_FLOAT_VEC2: typeName = "GL_FLOAT_VEC2"; break;
            case SH_FLOAT_VEC3: typeName = "GL_FLOAT_VEC3"; break;
            case SH_FLOAT_VEC4: typeName = "GL_FLOAT_VEC4"; break;
            case SH_INT: typeName = "GL_INT"; break;
            case SH_INT_VEC2: typeName = "GL_INT_VEC2"; break;
            case SH_INT_VEC3: typeName = "GL_INT_VEC3"; break;
            case SH_INT_VEC4: typeName = "GL_INT_VEC4"; break;
            case SH_BOOL: typeName = "GL_BOOL"; break;
            case SH_BOOL_VEC2: typeName = "GL_BOOL_VEC2"; break;
            case SH_BOOL_VEC3: typeName = "GL_BOOL_VEC3"; break;
            case SH_BOOL_VEC4: typeName = "GL_BOOL_VEC4"; break;
            case SH_FLOAT_MAT2: typeName = "GL_FLOAT_MAT2"; break;
            case SH_FLOAT_MAT3: typeName = "GL_FLOAT_MAT3"; break;
            case SH_FLOAT_MAT4: typeName = "GL_FLOAT_MAT4"; break;
            case SH_SAMPLER_2D: typeName = "GL_SAMPLER_2D"; break;
            case SH_SAMPLER_CUBE: typeName = "GL_SAMPLER_CUBE"; break;
            default: assert(0);
        }
        printf("%d: name:%s type:%s size:%d\n", i, name, typeName, size);
    }
    free(name);
}

