//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramImpl.h: Defines the abstract rx::ProgramImpl class.

#ifndef LIBANGLE_RENDERER_PROGRAMIMPL_H_
#define LIBANGLE_RENDERER_PROGRAMIMPL_H_

#include "common/BinaryStream.h"
#include "common/WorkerThread.h"
#include "common/angleutils.h"
#include "libANGLE/Constants.h"
#include "libANGLE/Program.h"
#include "libANGLE/Shader.h"

#include <functional>
#include <map>

namespace gl
{
class Context;
struct ProgramLinkedResources;
}  // namespace gl

namespace sh
{
struct BlockMemberInfo;
}

namespace rx
{
// The link job is split as such:
//
// - Front-end link
// - Back-end link
// - Independent back-end link subtasks (typically native driver compile jobs)
// - Post-link finalization
//
// Each step depends on the previous.  These steps are executed as such:
//
// 1. Program::link calls into ProgramImpl::link
//   - ProgramImpl::link runs whatever needs the Context, such as releasing resources
//   - ProgramImpl::link returns a LinkTask
// 2. Program::link implements a closure that calls the front-end link and passes the results to
//    the backend's LinkTask.
// 3. The LinkTask potentially returns a set of LinkSubTasks to be scheduled by the worker pool
// 4. Once the link is resolved, the post-link finalization is run
//
// In the above, steps 1 and 4 are done under the share group lock.  Steps 2 and 3 can be done in
// threads or without holding the share group lock if the backend supports it.
class LinkSubTask : public angle::Closure
{
  public:
    ~LinkSubTask() override                                                           = default;
    virtual angle::Result getResult(const gl::Context *context, gl::InfoLog &infoLog) = 0;
};
class LinkTask
{
  public:
    virtual ~LinkTask() = default;
    // Used for link()
    virtual std::vector<std::shared_ptr<LinkSubTask>> link(
        const gl::ProgramLinkedResources &resources,
        const gl::ProgramMergedVaryings &mergedVaryings);
    // Used for load()
    virtual std::vector<std::shared_ptr<LinkSubTask>> load();
    virtual angle::Result getResult(const gl::Context *context, gl::InfoLog &infoLog) = 0;

    // Used by the GL backend to query whether the driver is linking in parallel internally.
    virtual bool isLinkingInternally();
};

class ProgramImpl : angle::NonCopyable
{
  public:
    ProgramImpl(const gl::ProgramState &state) : mState(state) {}
    virtual ~ProgramImpl() {}
    virtual void destroy(const gl::Context *context) {}

    virtual angle::Result load(const gl::Context *context,
                               gl::BinaryInputStream *stream,
                               std::shared_ptr<LinkTask> *loadTaskOut)            = 0;
    virtual void save(const gl::Context *context, gl::BinaryOutputStream *stream) = 0;
    virtual void setBinaryRetrievableHint(bool retrievable)                       = 0;
    virtual void setSeparable(bool separable)                                     = 0;

    virtual void prepareForLink(const gl::ShaderMap<ShaderImpl *> &shaders) {}
    virtual angle::Result link(const gl::Context *context,
                               std::shared_ptr<LinkTask> *linkTaskOut) = 0;
    virtual GLboolean validate(const gl::Caps &caps)                   = 0;

    virtual void setUniform1fv(GLint location, GLsizei count, const GLfloat *v) = 0;
    virtual void setUniform2fv(GLint location, GLsizei count, const GLfloat *v) = 0;
    virtual void setUniform3fv(GLint location, GLsizei count, const GLfloat *v) = 0;
    virtual void setUniform4fv(GLint location, GLsizei count, const GLfloat *v) = 0;
    virtual void setUniform1iv(GLint location, GLsizei count, const GLint *v)   = 0;
    virtual void setUniform2iv(GLint location, GLsizei count, const GLint *v)   = 0;
    virtual void setUniform3iv(GLint location, GLsizei count, const GLint *v)   = 0;
    virtual void setUniform4iv(GLint location, GLsizei count, const GLint *v)   = 0;
    virtual void setUniform1uiv(GLint location, GLsizei count, const GLuint *v) = 0;
    virtual void setUniform2uiv(GLint location, GLsizei count, const GLuint *v) = 0;
    virtual void setUniform3uiv(GLint location, GLsizei count, const GLuint *v) = 0;
    virtual void setUniform4uiv(GLint location, GLsizei count, const GLuint *v) = 0;
    virtual void setUniformMatrix2fv(GLint location,
                                     GLsizei count,
                                     GLboolean transpose,
                                     const GLfloat *value)                      = 0;
    virtual void setUniformMatrix3fv(GLint location,
                                     GLsizei count,
                                     GLboolean transpose,
                                     const GLfloat *value)                      = 0;
    virtual void setUniformMatrix4fv(GLint location,
                                     GLsizei count,
                                     GLboolean transpose,
                                     const GLfloat *value)                      = 0;
    virtual void setUniformMatrix2x3fv(GLint location,
                                       GLsizei count,
                                       GLboolean transpose,
                                       const GLfloat *value)                    = 0;
    virtual void setUniformMatrix3x2fv(GLint location,
                                       GLsizei count,
                                       GLboolean transpose,
                                       const GLfloat *value)                    = 0;
    virtual void setUniformMatrix2x4fv(GLint location,
                                       GLsizei count,
                                       GLboolean transpose,
                                       const GLfloat *value)                    = 0;
    virtual void setUniformMatrix4x2fv(GLint location,
                                       GLsizei count,
                                       GLboolean transpose,
                                       const GLfloat *value)                    = 0;
    virtual void setUniformMatrix3x4fv(GLint location,
                                       GLsizei count,
                                       GLboolean transpose,
                                       const GLfloat *value)                    = 0;
    virtual void setUniformMatrix4x3fv(GLint location,
                                       GLsizei count,
                                       GLboolean transpose,
                                       const GLfloat *value)                    = 0;

    // Done in the back-end to avoid having to keep a system copy of uniform data.
    virtual void getUniformfv(const gl::Context *context,
                              GLint location,
                              GLfloat *params) const                                           = 0;
    virtual void getUniformiv(const gl::Context *context, GLint location, GLint *params) const = 0;
    virtual void getUniformuiv(const gl::Context *context,
                               GLint location,
                               GLuint *params) const                                           = 0;

    // Implementation-specific method for ignoring unreferenced uniforms. Some implementations may
    // perform more extensive analysis and ignore some locations that ANGLE doesn't detect as
    // unreferenced. This method is not required to be overriden by a back-end.
    virtual void markUnusedUniformLocations(std::vector<gl::VariableLocation> *uniformLocations,
                                            std::vector<gl::SamplerBinding> *samplerBindings,
                                            std::vector<gl::ImageBinding> *imageBindings)
    {}

    const gl::ProgramState &getState() const { return mState; }

    virtual angle::Result syncState(const gl::Context *context,
                                    const gl::Program::DirtyBits &dirtyBits);

    virtual angle::Result onLabelUpdate(const gl::Context *context);

  protected:
    const gl::ProgramState &mState;
};

inline angle::Result ProgramImpl::syncState(const gl::Context *context,
                                            const gl::Program::DirtyBits &dirtyBits)
{
    return angle::Result::Continue;
}

}  // namespace rx

#endif  // LIBANGLE_RENDERER_PROGRAMIMPL_H_
