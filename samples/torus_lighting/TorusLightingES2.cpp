//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Based on CubeMapActivity.java from The Android Open Source Project ApiDemos
// https://android.googlesource.com/platform/development/+/refs/heads/master/samples/ApiDemos/src/com/example/android/apis/graphics/CubeMapActivity.java

#include "SampleApplication.h"

#include "torus.h"
#include "util/Matrix.h"
#include "util/shader_utils.h"

class GLES2TorusLightingSample : public SampleApplication
{
  public:
    GLES2TorusLightingSample(int argc, char **argv)
        : SampleApplication("GLES2 Torus Lighting", argc, argv, 2, 0)
    {}

    bool initialize() override
    {
        constexpr char kVS[] = R"(uniform mat4 mv;
uniform mat4 mvp;

attribute vec4 position;
attribute vec3 normal;

varying vec3 normal_view;

void main()
{
    normal_view = vec3(mv * vec4(normal, 0.0));
    gl_Position = mvp * position;
})";

        constexpr char kFS[] = R"(precision mediump float;

varying vec3 normal_view;

void main() {
    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0) * dot(vec3(0.0, 0, 1.0), normalize(normal_view));
})";

        mProgram = CompileProgram(kVS, kFS);
        if (!mProgram)
        {
            return false;
        }

        mPositionLoc = glGetAttribLocation(mProgram, "position");
        mNormalLoc   = glGetAttribLocation(mProgram, "normal");

        mMVPMatrixLoc = glGetUniformLocation(mProgram, "mvp");
        mMVMatrixLoc  = glGetUniformLocation(mProgram, "mv");

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glEnable(GL_DEPTH_TEST);

        GenerateTorus(&mVertexBuffer, &mIndexBuffer, &mIndexCount);

        return true;
    }

    void destroy() override
    {
        glDeleteProgram(mProgram);
        glDeleteBuffers(1, &mVertexBuffer);
        glDeleteBuffers(1, &mIndexBuffer);
    }

    void draw() override
    {
        glViewport(0, 0, getWindow()->getWidth(), getWindow()->getHeight());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(mProgram);

        float ratio = (float)getWindow()->getWidth() / (float)getWindow()->getHeight();
        Matrix4 perspectiveMatrix = Matrix4::frustum(-ratio, ratio, -1, 1, 1.0f, 20.0f);

        Matrix4 modelMatrix = Matrix4::translate(angle::Vector3(0, 0, -5)) *
                              Matrix4::rotate(mAngle, angle::Vector3(0.0f, 1.0f, 0.0f)) *
                              Matrix4::rotate(mAngle * 0.25f, angle::Vector3(1.0f, 0.0f, 0.0f));

        Matrix4 mvpMatrix = perspectiveMatrix * modelMatrix;

        glUniformMatrix4fv(mMVMatrixLoc, 1, GL_FALSE, modelMatrix.data);
        glUniformMatrix4fv(mMVPMatrixLoc, 1, GL_FALSE, mvpMatrix.data);

        glEnableVertexAttribArray(mPositionLoc);
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glVertexAttribPointer(mPositionLoc, 3, GL_FLOAT, false, 6 * sizeof(GLfloat), nullptr);

        glVertexAttribPointer(mNormalLoc, 3, GL_FLOAT, false, 6 * sizeof(GLfloat),
                              reinterpret_cast<const void *>(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(mNormalLoc);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
        glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_SHORT, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        mAngle++;
    }

  private:
    GLuint mProgram;

    GLint mPositionLoc;
    GLint mNormalLoc;

    GLuint mMVPMatrixLoc;
    GLuint mMVMatrixLoc;

    GLuint mVertexBuffer;
    GLuint mIndexBuffer;
    GLsizei mIndexCount;

    float mAngle = 0;
};

int main(int argc, char **argv)
{
    GLES2TorusLightingSample app(argc, argv);
    return app.run();
}
