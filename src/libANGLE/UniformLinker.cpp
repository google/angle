//
// Copyright (c) 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// UniformLinker.cpp: implements link-time checks for default block uniforms, and generates uniform
// locations. Populates data structures related to uniforms so that they can be stored in program
// state.

#include "libANGLE/UniformLinker.h"

#include "common/utilities.h"
#include "libANGLE/Caps.h"
#include "libANGLE/Context.h"
#include "libANGLE/Shader.h"
#include "libANGLE/features.h"

namespace gl
{

namespace
{

LinkedUniform *FindUniform(std::vector<LinkedUniform> &list, const std::string &name)
{
    for (LinkedUniform &uniform : list)
    {
        if (uniform.name == name)
            return &uniform;
    }

    return nullptr;
}

}  // anonymouse namespace

UniformLinker::UniformLinker(const ProgramState &state) : mState(state)
{
}

void UniformLinker::getResults(std::vector<LinkedUniform> *uniforms,
                               std::vector<VariableLocation> *uniformLocations)
{
    uniforms->swap(mUniforms);
    uniformLocations->swap(mUniformLocations);
}

bool UniformLinker::link(const Context *context,
                         InfoLog &infoLog,
                         const Program::Bindings &uniformLocationBindings)
{
    if (mState.getAttachedVertexShader() && mState.getAttachedFragmentShader())
    {
        ASSERT(mState.getAttachedComputeShader() == nullptr);
        if (!validateVertexAndFragmentUniforms(context, infoLog))
        {
            return false;
        }
    }

    // Flatten the uniforms list (nested fields) into a simple list (no nesting).
    // Also check the maximum uniform vector and sampler counts.
    if (!flattenUniformsAndCheckCaps(context, infoLog))
    {
        return false;
    }

    if (!indexUniforms(infoLog, uniformLocationBindings))
    {
        return false;
    }

    return true;
}

bool UniformLinker::validateVertexAndFragmentUniforms(const Context *context,
                                                      InfoLog &infoLog) const
{
    // Check that uniforms defined in the vertex and fragment shaders are identical
    std::map<std::string, LinkedUniform> linkedUniforms;
    const std::vector<sh::Uniform> &vertexUniforms =
        mState.getAttachedVertexShader()->getUniforms(context);
    const std::vector<sh::Uniform> &fragmentUniforms =
        mState.getAttachedFragmentShader()->getUniforms(context);

    for (const sh::Uniform &vertexUniform : vertexUniforms)
    {
        linkedUniforms[vertexUniform.name] = LinkedUniform(vertexUniform);
    }

    for (const sh::Uniform &fragmentUniform : fragmentUniforms)
    {
        auto entry = linkedUniforms.find(fragmentUniform.name);
        if (entry != linkedUniforms.end())
        {
            LinkedUniform *linkedUniform   = &entry->second;
            const std::string &uniformName = "uniform '" + linkedUniform->name + "'";
            if (!linkValidateUniforms(infoLog, uniformName, *linkedUniform, fragmentUniform))
            {
                return false;
            }
        }
    }
    return true;
}

// GLSL ES Spec 3.00.3, section 4.3.5.
bool UniformLinker::linkValidateUniforms(InfoLog &infoLog,
                                         const std::string &uniformName,
                                         const sh::Uniform &vertexUniform,
                                         const sh::Uniform &fragmentUniform)
{
#if ANGLE_PROGRAM_LINK_VALIDATE_UNIFORM_PRECISION == ANGLE_ENABLED
    const bool validatePrecision = true;
#else
    const bool validatePrecision = false;
#endif

    if (!Program::linkValidateVariablesBase(infoLog, uniformName, vertexUniform, fragmentUniform,
                                            validatePrecision))
    {
        return false;
    }

    // GLSL ES Spec 3.10.4, section 4.4.5.
    if (vertexUniform.binding != -1 && fragmentUniform.binding != -1 &&
        vertexUniform.binding != fragmentUniform.binding)
    {
        infoLog << "Binding layout qualifiers for " << uniformName
                << " differ between vertex and fragment shaders.";
        return false;
    }

    // GLSL ES Spec 3.10.4, section 9.2.1.
    if (vertexUniform.location != -1 && fragmentUniform.location != -1 &&
        vertexUniform.location != fragmentUniform.location)
    {
        infoLog << "Location layout qualifiers for " << uniformName
                << " differ between vertex and fragment shaders.";
        return false;
    }

    return true;
}

bool UniformLinker::indexUniforms(InfoLog &infoLog,
                                  const Program::Bindings &uniformLocationBindings)
{
    // All the locations where another uniform can't be located.
    std::set<GLuint> reservedLocations;
    // Locations which have been allocated for an unused uniform.
    std::set<GLuint> ignoredLocations;

    int maxUniformLocation = -1;

    // Gather uniform locations that have been set either using the bindUniformLocation API or by
    // using a location layout qualifier and check conflicts between them.
    if (!gatherUniformLocationsAndCheckConflicts(infoLog, uniformLocationBindings,
                                                 &reservedLocations, &ignoredLocations,
                                                 &maxUniformLocation))
    {
        return false;
    }

    // Conflicts have been checked, now we can prune non-statically used uniforms. Code further down
    // the line relies on only having statically used uniforms in mUniforms.
    pruneUnusedUniforms();

    // Gather uniforms that have their location pre-set and uniforms that don't yet have a location.
    std::vector<VariableLocation> unlocatedUniforms;
    std::map<GLuint, VariableLocation> preLocatedUniforms;

    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        const LinkedUniform &uniform = mUniforms[uniformIndex];

        if (uniform.isBuiltIn())
        {
            continue;
        }

        int preSetLocation = uniformLocationBindings.getBinding(uniform.name);
        int shaderLocation = uniform.location;

        if (shaderLocation != -1)
        {
            preSetLocation = shaderLocation;
        }

        for (unsigned int arrayIndex = 0; arrayIndex < uniform.elementCount(); arrayIndex++)
        {
            VariableLocation location(uniform.name, arrayIndex,
                                      static_cast<unsigned int>(uniformIndex));

            if ((arrayIndex == 0 && preSetLocation != -1) || shaderLocation != -1)
            {
                int elementLocation                 = preSetLocation + arrayIndex;
                preLocatedUniforms[elementLocation] = location;
            }
            else
            {
                unlocatedUniforms.push_back(location);
            }
        }
    }

    // Make enough space for all uniforms, with pre-set locations or not.
    mUniformLocations.resize(
        std::max(unlocatedUniforms.size() + preLocatedUniforms.size() + ignoredLocations.size(),
                 static_cast<size_t>(maxUniformLocation + 1)));

    // Assign uniforms with pre-set locations
    for (const auto &uniform : preLocatedUniforms)
    {
        mUniformLocations[uniform.first] = uniform.second;
    }

    // Assign ignored uniforms
    for (const auto &ignoredLocation : ignoredLocations)
    {
        mUniformLocations[ignoredLocation].ignored = true;
    }

    // Automatically assign locations for the rest of the uniforms
    size_t nextUniformLocation = 0;
    for (const auto &unlocatedUniform : unlocatedUniforms)
    {
        while (mUniformLocations[nextUniformLocation].used ||
               mUniformLocations[nextUniformLocation].ignored)
        {
            nextUniformLocation++;
        }

        ASSERT(nextUniformLocation < mUniformLocations.size());
        mUniformLocations[nextUniformLocation] = unlocatedUniform;
        nextUniformLocation++;
    }

    return true;
}

bool UniformLinker::gatherUniformLocationsAndCheckConflicts(
    InfoLog &infoLog,
    const Program::Bindings &uniformLocationBindings,
    std::set<GLuint> *reservedLocations,
    std::set<GLuint> *ignoredLocations,
    int *maxUniformLocation)
{
    for (const LinkedUniform &uniform : mUniforms)
    {
        if (uniform.isBuiltIn())
        {
            continue;
        }

        int apiBoundLocation = uniformLocationBindings.getBinding(uniform.name);
        int shaderLocation   = uniform.location;

        if (shaderLocation != -1)
        {
            for (unsigned int arrayIndex = 0; arrayIndex < uniform.elementCount(); arrayIndex++)
            {
                // GLSL ES 3.10 section 4.4.3
                int elementLocation = shaderLocation + arrayIndex;
                *maxUniformLocation = std::max(*maxUniformLocation, elementLocation);
                if (reservedLocations->find(elementLocation) != reservedLocations->end())
                {
                    infoLog << "Multiple uniforms bound to location " << elementLocation << ".";
                    return false;
                }
                reservedLocations->insert(elementLocation);
                if (!uniform.staticUse)
                {
                    ignoredLocations->insert(elementLocation);
                }
            }
        }
        else if (apiBoundLocation != -1 && uniform.staticUse)
        {
            // Only the first location is reserved even if the uniform is an array.
            *maxUniformLocation = std::max(*maxUniformLocation, apiBoundLocation);
            if (reservedLocations->find(apiBoundLocation) != reservedLocations->end())
            {
                infoLog << "Multiple uniforms bound to location " << apiBoundLocation << ".";
                return false;
            }
            reservedLocations->insert(apiBoundLocation);
        }
    }

    // Record the uniform locations that were bound using the API for uniforms that were not found
    // from the shader. Other uniforms should not be assigned to those locations.
    for (const auto &locationBinding : uniformLocationBindings)
    {
        GLuint location = locationBinding.second;
        if (reservedLocations->find(location) == reservedLocations->end())
        {
            ignoredLocations->insert(location);
            *maxUniformLocation = std::max(*maxUniformLocation, static_cast<int>(location));
        }
    }

    return true;
}

void UniformLinker::pruneUnusedUniforms()
{
    auto uniformIter = mUniforms.begin();
    while (uniformIter != mUniforms.end())
    {
        if (uniformIter->staticUse)
        {
            ++uniformIter;
        }
        else
        {
            uniformIter = mUniforms.erase(uniformIter);
        }
    }
}

bool UniformLinker::flattenUniformsAndCheckCapsForShader(
    const Context *context,
    Shader *shader,
    GLuint maxUniformComponents,
    GLuint maxTextureImageUnits,
    GLuint maxImageUnits,
    const std::string &componentsErrorMessage,
    const std::string &samplerErrorMessage,
    const std::string &imageErrorMessage,
    std::vector<LinkedUniform> &samplerUniforms,
    std::vector<LinkedUniform> &imageUniforms,
    InfoLog &infoLog)
{
    ShaderUniformCount shaderUniformCount;
    for (const sh::Uniform &uniform : shader->getUniforms(context))
    {
        shaderUniformCount += flattenUniform(uniform, &samplerUniforms, &imageUniforms);
    }

    if (shaderUniformCount.vectorCount > maxUniformComponents)
    {
        infoLog << componentsErrorMessage << maxUniformComponents << ").";
        return false;
    }

    if (shaderUniformCount.samplerCount > maxTextureImageUnits)
    {
        infoLog << samplerErrorMessage << maxTextureImageUnits << ").";
        return false;
    }

    if (shaderUniformCount.imageCount > maxImageUnits)
    {
        infoLog << imageErrorMessage << maxImageUnits << ").";
        return false;
    }

    return true;
}

bool UniformLinker::flattenUniformsAndCheckCaps(const Context *context, InfoLog &infoLog)
{
    std::vector<LinkedUniform> samplerUniforms;
    std::vector<LinkedUniform> imageUniforms;

    const Caps &caps = context->getCaps();

    if (mState.getAttachedComputeShader())
    {
        Shader *computeShader = mState.getAttachedComputeShader();

        // TODO (mradev): check whether we need finer-grained component counting
        if (!flattenUniformsAndCheckCapsForShader(
                context, computeShader, caps.maxComputeUniformComponents / 4,
                caps.maxComputeTextureImageUnits, caps.maxComputeImageUniforms,
                "Compute shader active uniforms exceed MAX_COMPUTE_UNIFORM_COMPONENTS (",
                "Compute shader sampler count exceeds MAX_COMPUTE_TEXTURE_IMAGE_UNITS (",
                "Compute shader image count exceeds MAX_COMPUTE_IMAGE_UNIFORMS (", samplerUniforms,
                imageUniforms, infoLog))
        {
            return false;
        }
    }
    else
    {
        Shader *vertexShader = mState.getAttachedVertexShader();

        if (!flattenUniformsAndCheckCapsForShader(
                context, vertexShader, caps.maxVertexUniformVectors,
                caps.maxVertexTextureImageUnits, caps.maxVertexImageUniforms,
                "Vertex shader active uniforms exceed MAX_VERTEX_UNIFORM_VECTORS (",
                "Vertex shader sampler count exceeds MAX_VERTEX_TEXTURE_IMAGE_UNITS (",
                "Vertex shader image count exceeds MAX_VERTEX_IMAGE_UNIFORMS (", samplerUniforms,
                imageUniforms, infoLog))
        {
            return false;
        }

        Shader *fragmentShader = mState.getAttachedFragmentShader();

        if (!flattenUniformsAndCheckCapsForShader(
                context, fragmentShader, caps.maxFragmentUniformVectors, caps.maxTextureImageUnits,
                caps.maxFragmentImageUniforms,
                "Fragment shader active uniforms exceed MAX_FRAGMENT_UNIFORM_VECTORS (",
                "Fragment shader sampler count exceeds MAX_TEXTURE_IMAGE_UNITS (",
                "Fragment shader image count exceeds MAX_FRAGMENT_IMAGE_UNIFORMS (",
                samplerUniforms, imageUniforms, infoLog))
        {
            return false;
        }
    }

    mUniforms.insert(mUniforms.end(), samplerUniforms.begin(), samplerUniforms.end());
    mUniforms.insert(mUniforms.end(), imageUniforms.begin(), imageUniforms.end());
    return true;
}

UniformLinker::ShaderUniformCount UniformLinker::flattenUniform(
    const sh::Uniform &uniform,
    std::vector<LinkedUniform> *samplerUniforms,
    std::vector<LinkedUniform> *imageUniforms)
{
    int location                          = uniform.location;
    ShaderUniformCount shaderUniformCount =
        flattenUniformImpl(uniform, uniform.name, samplerUniforms, imageUniforms, uniform.staticUse,
                           uniform.binding, &location);
    if (uniform.staticUse)
    {
        return shaderUniformCount;
    }
    return ShaderUniformCount();
}

UniformLinker::ShaderUniformCount UniformLinker::flattenUniformImpl(
    const sh::ShaderVariable &uniform,
    const std::string &fullName,
    std::vector<LinkedUniform> *samplerUniforms,
    std::vector<LinkedUniform> *imageUniforms,
    bool markStaticUse,
    int binding,
    int *location)
{
    ASSERT(location);
    ShaderUniformCount shaderUniformCount;

    if (uniform.isStruct())
    {
        for (unsigned int elementIndex = 0; elementIndex < uniform.elementCount(); elementIndex++)
        {
            const std::string &elementString = (uniform.isArray() ? ArrayString(elementIndex) : "");

            for (size_t fieldIndex = 0; fieldIndex < uniform.fields.size(); fieldIndex++)
            {
                const sh::ShaderVariable &field  = uniform.fields[fieldIndex];
                const std::string &fieldFullName = (fullName + elementString + "." + field.name);

                shaderUniformCount +=
                    flattenUniformImpl(field, fieldFullName, samplerUniforms, imageUniforms,
                                       markStaticUse, -1, location);
            }
        }

        return shaderUniformCount;
    }

    // Not a struct
    bool isSampler                              = IsSamplerType(uniform.type);
    bool isImage                                = IsImageType(uniform.type);
    std::vector<gl::LinkedUniform> *uniformList = &mUniforms;
    if (isSampler)
    {
        // Store sampler uniforms separately, so we'll append them to the end of the list.
        uniformList = samplerUniforms;
    }
    else if (isImage)
    {
        uniformList = imageUniforms;
    }
    LinkedUniform *existingUniform = FindUniform(*uniformList, fullName);
    if (existingUniform)
    {
        if (binding != -1)
        {
            existingUniform->binding = binding;
        }
        if (*location != -1)
        {
            existingUniform->location = *location;
        }
        if (markStaticUse)
        {
            existingUniform->staticUse = true;
        }
    }
    else
    {
        LinkedUniform linkedUniform(uniform.type, uniform.precision, fullName, uniform.arraySize,
                                    binding, *location, -1,
                                    sh::BlockMemberInfo::getDefaultBlockInfo());
        linkedUniform.staticUse = markStaticUse;
        uniformList->push_back(linkedUniform);
    }

    unsigned int elementCount = uniform.elementCount();

    // Samplers and images aren't "real" uniforms, so they don't count towards register usage.
    // Likewise, don't count "real" uniforms towards sampler and image count.
    shaderUniformCount.vectorCount =
        ((isSampler || isImage) ? 0 : (VariableRegisterCount(uniform.type) * elementCount));
    shaderUniformCount.samplerCount = (isSampler ? elementCount : 0);
    shaderUniformCount.imageCount   = (isImage ? elementCount : 0);

    if (*location != -1)
    {
        *location += elementCount;
    }

    return shaderUniformCount;
}

}  // namespace gl
