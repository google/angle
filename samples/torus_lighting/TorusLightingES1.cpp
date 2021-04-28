//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Based on CubeMapActivity.java from The Android Open Source Project ApiDemos
// https://android.googlesource.com/platform/development/+/refs/heads/master/samples/ApiDemos/src/com/example/android/apis/graphics/CubeMapActivity.java

#include "SampleApplication.h"
#include "torus.h"

class GLES1TorusLightingSample : public SampleApplication
{
  public:
    GLES1TorusLightingSample(int argc, char **argv)
        : SampleApplication("GLES1 Torus Lighting", argc, argv, 1, 0)
    {}

    bool initialize() override
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        glShadeModel(GL_SMOOTH);

        GLfloat light_model_ambient[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_model_ambient);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);

        GenerateTorus(&mVertexBuffer, &mIndexBuffer, &mIndexCount);

        return true;
    }

    void destroy() override
    {
        glDeleteBuffers(1, &mVertexBuffer);
        glDeleteBuffers(1, &mIndexBuffer);
    }

    void draw() override
    {
        glViewport(0, 0, getWindow()->getWidth(), getWindow()->getHeight());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float ratio = (float)getWindow()->getWidth() / (float)getWindow()->getHeight();
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glFrustumf(-ratio, ratio, -1, 1, 1.0f, 20.0f);

        glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glPushMatrix();

        GLfloat lightDir[] = {0.0f, 0.0f, 1.0f, 0.0f};
        glLightfv(GL_LIGHT0, GL_POSITION, lightDir);
        glPopMatrix();

        glTranslatef(0, 0, -5);

        glRotatef(mAngle, 0, 1, 0);
        glRotatef(mAngle * 0.25f, 1, 0, 0);

        glEnableClientState(GL_VERTEX_ARRAY);

        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glVertexPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), nullptr);

        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 6 * sizeof(GLfloat),
                        reinterpret_cast<const void *>(3 * sizeof(GLfloat)));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
        glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_SHORT, 0);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        mAngle++;
    }

  private:
    GLuint mVertexBuffer;
    GLuint mIndexBuffer;
    GLsizei mIndexCount;
    float mAngle = 0;
};

int main(int argc, char **argv)
{
    GLES1TorusLightingSample app(argc, argv);
    return app.run();
}
