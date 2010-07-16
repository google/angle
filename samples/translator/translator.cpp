//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "GLSLANG/ShaderLang.h"

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
    EFailLink,
    EFailCompilerCreate,
    EFailLinkerCreate
};

static EShLanguage FindLanguage(char *lang);
bool CompileFile(char *fileName, ShHandle, int, const TBuiltInResource*);
void usage();
void FreeFileData(char **data);
char** ReadFileData(char *fileName);
void LogMsg(char* msg, const char* name, const int num, const char* logName);
int ShOutputMultipleStrings(char ** );
//Added to accomodate the multiple strings.
int OutputMultipleStrings = 1;

//
// Set up the per compile resources
//
void GenerateResources(TBuiltInResource& resources)
{
    resources.maxVertexAttribs = 8;
    resources.maxVertexUniformVectors = 128;
    resources.maxVaryingVectors = 8;
    resources.maxVertexTextureImageUnits = 0;
    resources.maxCombinedTextureImageUnits = 8;
    resources.maxTextureImageUnits = 8;
    resources.maxFragmentUniformVectors = 16;
    resources.maxDrawBuffers = 1;
}

int C_DECL main(int argc, char* argv[])
{
    int numCompilers = 0;
    bool compileFailed = false;
    int debugOptions = 0;
    int i;

    ShHandle    linker = 0;
    ShHandle    uniformMap = 0;
    ShHandle    compilers[EShLangCount];

    ShInitialize();

    argc--;
    argv++;    
    for (; argc >= 1; argc--, argv++) {
        if (argv[0][0] == '-' || argv[0][0] == '/') {
            switch (argv[0][1]) {
                case 'i': debugOptions |= EDebugOpIntermediate; break;
                case 'o': debugOptions |= EDebugOpObjectCode; break;
                default:  usage(); return EFailUsage;
            }
        } else {
            compilers[numCompilers] = ShConstructCompiler(FindLanguage(argv[0]), EShSpecGLES2);
            if (compilers[numCompilers] == 0)
                return EFailCompilerCreate;
            ++numCompilers;

            TBuiltInResource resources;
            GenerateResources(resources);
            if (!CompileFile(argv[0], compilers[numCompilers-1], debugOptions, &resources))
                compileFailed = true;
        }
    }

    if (!numCompilers) {
        usage();
        return EFailUsage;
    }

    for (i = 0; i < numCompilers; ++i) {
        LogMsg("BEGIN", "COMPILER", i, "INFO LOG");
        puts(ShGetInfoLog(compilers[i]));
        LogMsg("END", "COMPILER", i, "INFO LOG");
    }
    if ((debugOptions & EDebugOpObjectCode) != 0) {
        for (i = 0; i < numCompilers; ++i) {
            LogMsg("BEGIN", "COMPILER", i, "OBJ CODE");
            puts(ShGetObjectCode(compilers[i]));
            LogMsg("END", "COMPILER", i, "OBJ CODE");
        }
    }

    for (i = 0; i < numCompilers; ++i)
        ShDestruct(compilers[i]);

    return compileFailed ? EFailCompile : ESuccess;
}

//
//   Deduce the language from the filename.  Files must end in one of the
//   following extensions:
//
//   .frag*    = fragment programs
//   .vert*    = vertex programs
//
static EShLanguage FindLanguage(char *name)
{
    if (!name)
        return EShLangFragment;

    char *ext = strrchr(name, '.');

    if (ext && strcmp(ext, ".sl") == 0)
        for (; ext > name && ext[0] != '.'; ext--);

    if (ext = strrchr(name, '.')) {
        if (strncmp(ext, ".frag", 4) == 0) return EShLangFragment;
        if (strncmp(ext, ".vert", 4) == 0) return EShLangVertex;
    }

    return EShLangFragment;
}

//
//   Read a file's data into a string, and compile it using ShCompile
//
bool CompileFile(char *fileName, ShHandle compiler, int debugOptions, const TBuiltInResource *resources)
{
    int ret;
    char **data = ReadFileData(fileName);

    if (!data)
        return false;

    ret = ShCompile(compiler, data, OutputMultipleStrings, EShOptNone, resources, debugOptions);

    FreeFileData(data);

    return ret ? true : false;
}

//
//   print usage to stdout
//
void usage()
{
    printf("Usage: translate [-i -o] file1 file2 ...\n"
        "Where: filename = filename ending in .frag or .vert\n"
        "       -i = print intermediate tree\n"
        "       -o = print translated code\n");
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
    printf(num >= 0 ? "#### %s %s %d %s ####\n" :
        "#### %s %s INFO LOG ####\n", msg, name, num, logName);
}

int ShOutputMultipleStrings(char** argv)
{
    if(!(abs(OutputMultipleStrings = atoi(*argv)))||((OutputMultipleStrings >5 || OutputMultipleStrings < 1)? 1:0)){
        printf("Invalid Command Line Argument after -c option.\n"
            "Usage: -c <integer> where integer =[1,5]\n"
            "This option must be specified before the input file path\n");
        return 0;
    }
    return 1;
}
