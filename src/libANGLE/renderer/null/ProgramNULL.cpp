//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ProgramNULL.cpp:
//    Implements the class methods for ProgramNULL.
//

#include "libANGLE/renderer/null/ProgramNULL.h"

#include "common/debug.h"

namespace rx
{
namespace
{
class LinkTaskNULL : public LinkTask
{
  public:
    ~LinkTaskNULL() override = default;
    std::vector<std::shared_ptr<LinkSubTask>> link(
        const gl::ProgramLinkedResources &resources,
        const gl::ProgramMergedVaryings &mergedVaryings) override
    {
        return {};
    }
    angle::Result getResult(const gl::Context *context, gl::InfoLog &infoLog) override
    {
        return angle::Result::Continue;
    }
};
}  // anonymous namespace

ProgramNULL::ProgramNULL(const gl::ProgramState &state) : ProgramImpl(state) {}

ProgramNULL::~ProgramNULL() {}

angle::Result ProgramNULL::load(const gl::Context *context,
                                gl::BinaryInputStream *stream,
                                std::shared_ptr<LinkTask> *loadTaskOut)
{
    *loadTaskOut = {};
    return angle::Result::Continue;
}

void ProgramNULL::save(const gl::Context *context, gl::BinaryOutputStream *stream) {}

void ProgramNULL::setBinaryRetrievableHint(bool retrievable) {}

void ProgramNULL::setSeparable(bool separable) {}

angle::Result ProgramNULL::link(const gl::Context *contextImpl,
                                std::shared_ptr<LinkTask> *linkTaskOut)
{
    *linkTaskOut = std::shared_ptr<LinkTask>(new LinkTaskNULL);
    return angle::Result::Continue;
}

GLboolean ProgramNULL::validate(const gl::Caps &caps)
{
    return GL_TRUE;
}

void ProgramNULL::setUniform1fv(GLint location, GLsizei count, const GLfloat *v) {}

void ProgramNULL::setUniform2fv(GLint location, GLsizei count, const GLfloat *v) {}

void ProgramNULL::setUniform3fv(GLint location, GLsizei count, const GLfloat *v) {}

void ProgramNULL::setUniform4fv(GLint location, GLsizei count, const GLfloat *v) {}

void ProgramNULL::setUniform1iv(GLint location, GLsizei count, const GLint *v) {}

void ProgramNULL::setUniform2iv(GLint location, GLsizei count, const GLint *v) {}

void ProgramNULL::setUniform3iv(GLint location, GLsizei count, const GLint *v) {}

void ProgramNULL::setUniform4iv(GLint location, GLsizei count, const GLint *v) {}

void ProgramNULL::setUniform1uiv(GLint location, GLsizei count, const GLuint *v) {}

void ProgramNULL::setUniform2uiv(GLint location, GLsizei count, const GLuint *v) {}

void ProgramNULL::setUniform3uiv(GLint location, GLsizei count, const GLuint *v) {}

void ProgramNULL::setUniform4uiv(GLint location, GLsizei count, const GLuint *v) {}

void ProgramNULL::setUniformMatrix2fv(GLint location,
                                      GLsizei count,
                                      GLboolean transpose,
                                      const GLfloat *value)
{}

void ProgramNULL::setUniformMatrix3fv(GLint location,
                                      GLsizei count,
                                      GLboolean transpose,
                                      const GLfloat *value)
{}

void ProgramNULL::setUniformMatrix4fv(GLint location,
                                      GLsizei count,
                                      GLboolean transpose,
                                      const GLfloat *value)
{}

void ProgramNULL::setUniformMatrix2x3fv(GLint location,
                                        GLsizei count,
                                        GLboolean transpose,
                                        const GLfloat *value)
{}

void ProgramNULL::setUniformMatrix3x2fv(GLint location,
                                        GLsizei count,
                                        GLboolean transpose,
                                        const GLfloat *value)
{}

void ProgramNULL::setUniformMatrix2x4fv(GLint location,
                                        GLsizei count,
                                        GLboolean transpose,
                                        const GLfloat *value)
{}

void ProgramNULL::setUniformMatrix4x2fv(GLint location,
                                        GLsizei count,
                                        GLboolean transpose,
                                        const GLfloat *value)
{}

void ProgramNULL::setUniformMatrix3x4fv(GLint location,
                                        GLsizei count,
                                        GLboolean transpose,
                                        const GLfloat *value)
{}

void ProgramNULL::setUniformMatrix4x3fv(GLint location,
                                        GLsizei count,
                                        GLboolean transpose,
                                        const GLfloat *value)
{}

void ProgramNULL::getUniformfv(const gl::Context *context, GLint location, GLfloat *params) const
{
    // TODO(jmadill): Write some values.
}

void ProgramNULL::getUniformiv(const gl::Context *context, GLint location, GLint *params) const
{
    // TODO(jmadill): Write some values.
}

void ProgramNULL::getUniformuiv(const gl::Context *context, GLint location, GLuint *params) const
{
    // TODO(jmadill): Write some values.
}

}  // namespace rx
