//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// trace_fixture.cpp:
//   Common code for the ANGLE trace replays.
//

#include "trace_fixture.h"

#include "angle_trace_gl.h"

namespace
{
void UpdateResourceMap(ResourceMap *resourceMap, GLuint id, GLsizei readBufferOffset)
{
    GLuint returnedID;
    memcpy(&returnedID, &gReadBuffer[readBufferOffset], sizeof(GLuint));
    (*resourceMap)[id] = returnedID;
}

DecompressCallback gDecompressCallback;
const char *gBinaryDataDir = ".";

void LoadBinaryData(const char *fileName)
{
    // TODO(b/179188489): Fix cross-module deallocation.
    if (gBinaryData != nullptr)
    {
        delete[] gBinaryData;
    }
    char pathBuffer[1000] = {};
    sprintf(pathBuffer, "%s/%s", gBinaryDataDir, fileName);
    FILE *fp = fopen(pathBuffer, "rb");
    if (fp == 0)
    {
        fprintf(stderr, "Error loading binary data file: %s\n", fileName);
        return;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (gDecompressCallback)
    {
        if (!strstr(fileName, ".gz"))
        {
            fprintf(stderr, "Filename does not end in .gz");
            exit(1);
        }
        std::vector<uint8_t> compressedData(size);
        (void)fread(compressedData.data(), 1, size, fp);
        gBinaryData = gDecompressCallback(compressedData);
    }
    else
    {
        if (!strstr(fileName, ".angledata"))
        {
            fprintf(stderr, "Filename does not end in .angledata");
            exit(1);
        }
        gBinaryData = new uint8_t[size];
        (void)fread(gBinaryData, 1, size, fp);
    }
    fclose(fp);
}
}  // namespace

LocationsMap gUniformLocations;
BlockIndexesMap gUniformBlockIndexes;
GLuint gCurrentProgram = 0;

void UpdateUniformLocation(GLuint program, const char *name, GLint location)
{
    gUniformLocations[program][location] = glGetUniformLocation(program, name);
}
void DeleteUniformLocations(GLuint program)
{
    gUniformLocations.erase(program);
}
void UpdateUniformBlockIndex(GLuint program, const char *name, GLuint index)
{
    gUniformBlockIndexes[program][index] = glGetUniformBlockIndex(program, name);
}
void UpdateCurrentProgram(GLuint program)
{
    gCurrentProgram = program;
}

uint8_t *gBinaryData;
uint8_t *gReadBuffer;
uint8_t *gClientArrays[kMaxClientArrays];
ResourceMap gBufferMap;
ResourceMap gFenceNVMap;
ResourceMap gFramebufferMap;
ResourceMap gMemoryObjectMap;
ResourceMap gProgramPipelineMap;
ResourceMap gQueryMap;
ResourceMap gRenderbufferMap;
ResourceMap gSamplerMap;
ResourceMap gSemaphoreMap;
ResourceMap gShaderProgramMap;
ResourceMap gTextureMap;
ResourceMap gTransformFeedbackMap;
ResourceMap gVertexArrayMap;
SyncResourceMap gSyncMap;

void SetBinaryDataDecompressCallback(DecompressCallback callback)
{
    gDecompressCallback = callback;
}

void SetBinaryDataDir(const char *dataDir)
{
    gBinaryDataDir = dataDir;
}

void InitializeReplay(const char *binaryDataFileName,
                      size_t maxClientArraySize,
                      size_t readBufferSize)
{
    LoadBinaryData(binaryDataFileName);

    for (uint8_t *&clientArray : gClientArrays)
    {
        clientArray = new uint8_t[maxClientArraySize];
    }

    gReadBuffer = new uint8_t[readBufferSize];
}

void FinishReplay()
{
    for (uint8_t *&clientArray : gClientArrays)
    {
        delete[] clientArray;
    }
    delete[] gReadBuffer;
}

void UpdateClientArrayPointer(int arrayIndex, const void *data, uint64_t size)
{
    memcpy(gClientArrays[arrayIndex], data, static_cast<size_t>(size));
}
BufferHandleMap gMappedBufferData;

void UpdateClientBufferData(GLuint bufferID, const void *source, GLsizei size)
{
    memcpy(gMappedBufferData[gBufferMap[bufferID]], source, size);
}

void UpdateBufferID(GLuint id, GLsizei readBufferOffset)
{
    UpdateResourceMap(&gBufferMap, id, readBufferOffset);
}

void UpdateFenceNVID(GLuint id, GLsizei readBufferOffset)
{
    UpdateResourceMap(&gFenceNVMap, id, readBufferOffset);
}

void UpdateFramebufferID(GLuint id, GLsizei readBufferOffset)
{
    UpdateResourceMap(&gFramebufferMap, id, readBufferOffset);
}

void UpdateMemoryObjectID(GLuint id, GLsizei readBufferOffset)
{
    UpdateResourceMap(&gMemoryObjectMap, id, readBufferOffset);
}

void UpdateProgramPipelineID(GLuint id, GLsizei readBufferOffset)
{
    UpdateResourceMap(&gProgramPipelineMap, id, readBufferOffset);
}

void UpdateQueryID(GLuint id, GLsizei readBufferOffset)
{
    UpdateResourceMap(&gQueryMap, id, readBufferOffset);
}

void UpdateRenderbufferID(GLuint id, GLsizei readBufferOffset)
{
    UpdateResourceMap(&gRenderbufferMap, id, readBufferOffset);
}

void UpdateSamplerID(GLuint id, GLsizei readBufferOffset)
{
    UpdateResourceMap(&gSamplerMap, id, readBufferOffset);
}

void UpdateSemaphoreID(GLuint id, GLsizei readBufferOffset)
{
    UpdateResourceMap(&gSemaphoreMap, id, readBufferOffset);
}

void UpdateShaderProgramID(GLuint id, GLsizei readBufferOffset)
{
    UpdateResourceMap(&gShaderProgramMap, id, readBufferOffset);
}

void UpdateTextureID(GLuint id, GLsizei readBufferOffset)
{
    UpdateResourceMap(&gTextureMap, id, readBufferOffset);
}

void UpdateTransformFeedbackID(GLuint id, GLsizei readBufferOffset)
{
    UpdateResourceMap(&gTransformFeedbackMap, id, readBufferOffset);
}

void UpdateVertexArrayID(GLuint id, GLsizei readBufferOffset)
{
    UpdateResourceMap(&gVertexArrayMap, id, readBufferOffset);
}
