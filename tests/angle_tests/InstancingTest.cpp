#include "ANGLETest.h"

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
// We test on D3D9 and D3D11 9_3 because they use special codepaths when attribute zero is instanced, unlike D3D11.
ANGLE_TYPED_TEST_CASE(InstancingTest, ES2_D3D9, ES2_D3D11, ES2_D3D11_FL9_3);

template<typename T>
class InstancingTest : public ANGLETest
{
  protected:
    InstancingTest() : ANGLETest(T::GetGlesMajorVersion(), T::GetPlatform())
    {
        setWindowWidth(256);
        setWindowHeight(256);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    virtual void SetUp()
    {
        ANGLETest::SetUp();

        mVertexAttribDivisorANGLE = NULL;
        mDrawArraysInstancedANGLE = NULL;
        mDrawElementsInstancedANGLE = NULL;

        char *extensionString = (char*)glGetString(GL_EXTENSIONS);
        if (strstr(extensionString, "GL_ANGLE_instanced_arrays"))
        {
            mVertexAttribDivisorANGLE = (PFNGLVERTEXATTRIBDIVISORANGLEPROC)eglGetProcAddress("glVertexAttribDivisorANGLE");
            mDrawArraysInstancedANGLE = (PFNGLDRAWARRAYSINSTANCEDANGLEPROC)eglGetProcAddress("glDrawArraysInstancedANGLE");
            mDrawElementsInstancedANGLE = (PFNGLDRAWELEMENTSINSTANCEDANGLEPROC)eglGetProcAddress("glDrawElementsInstancedANGLE");
        }

        ASSERT_TRUE(mVertexAttribDivisorANGLE != NULL);
        ASSERT_TRUE(mDrawArraysInstancedANGLE != NULL);
        ASSERT_TRUE(mDrawElementsInstancedANGLE != NULL);

        // Initialize the vertex and index vectors
        GLfloat vertex1[3] = {-quadRadius,  quadRadius, 0.0f};
        GLfloat vertex2[3] = {-quadRadius, -quadRadius, 0.0f};
        GLfloat vertex3[3] = { quadRadius, -quadRadius, 0.0f};
        GLfloat vertex4[3] = { quadRadius,  quadRadius, 0.0f};
        mVertices.insert(mVertices.end(), vertex1, vertex1 + 3);
        mVertices.insert(mVertices.end(), vertex2, vertex2 + 3);
        mVertices.insert(mVertices.end(), vertex3, vertex3 + 3);
        mVertices.insert(mVertices.end(), vertex4, vertex4 + 3);

        GLfloat coord1[2] = {0.0f, 0.0f};
        GLfloat coord2[2] = {0.0f, 1.0f};
        GLfloat coord3[2] = {1.0f, 1.0f};
        GLfloat coord4[2] = {1.0f, 0.0f};
        mTexcoords.insert(mTexcoords.end(), coord1, coord1 + 2);
        mTexcoords.insert(mTexcoords.end(), coord2, coord2 + 2);
        mTexcoords.insert(mTexcoords.end(), coord3, coord3 + 2);
        mTexcoords.insert(mTexcoords.end(), coord4, coord4 + 2);

        mIndices.push_back(0);
        mIndices.push_back(1);
        mIndices.push_back(2);
        mIndices.push_back(0);
        mIndices.push_back(2);
        mIndices.push_back(3);

        // Tile a 3x3 grid of the tiles
        for (float y = -1.0f + quadRadius; y < 1.0f - quadRadius; y += quadRadius * 3)
        {
            for (float x = -1.0f + quadRadius; x < 1.0f - quadRadius; x += quadRadius * 3)
            {
                GLfloat instance[3] = {x + quadRadius, y + quadRadius, 0.0f};
                mInstances.insert(mInstances.end(), instance, instance + 3);
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        ASSERT_GL_NO_ERROR();
    }

    virtual void runTest(std::string vs, bool shouldAttribZeroBeInstanced)
    {
        const std::string fs = SHADER_SOURCE
        (
            precision mediump float;
            void main()
            {
                gl_FragColor = vec4(1.0, 0, 0, 1.0);
            }
        );

        GLuint program = CompileProgram(vs, fs);
        ASSERT_NE(program, 0u);

        // Get the attribute locations
        GLint positionLoc = glGetAttribLocation(program, "a_position");
        GLint instancePosLoc = glGetAttribLocation(program, "a_instancePos");

        // If this ASSERT fails then the vertex shader code should be refactored
        ASSERT_EQ(shouldAttribZeroBeInstanced, (instancePosLoc == 0));

        // Set the viewport
        glViewport(0, 0, getWindowWidth(), getWindowHeight());

        // Clear the color buffer
        glClear(GL_COLOR_BUFFER_BIT);

        // Use the program object
        glUseProgram(program);

        // Load the vertex position
        glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 0, mVertices.data());
        glEnableVertexAttribArray(positionLoc);

        // Load the instance position
        glVertexAttribPointer(instancePosLoc, 3, GL_FLOAT, GL_FALSE, 0, mInstances.data());
        glEnableVertexAttribArray(instancePosLoc);

        // Enable instancing
        mVertexAttribDivisorANGLE(instancePosLoc, 1);

        // Do the instanced draw
        mDrawElementsInstancedANGLE(GL_TRIANGLES, mIndices.size(), GL_UNSIGNED_SHORT, mIndices.data(), mInstances.size());

        ASSERT_GL_NO_ERROR();

        // Check that various pixels are the expected color.
        EXPECT_PIXEL_EQ(quadRadius * getWindowWidth(),            quadRadius * getWindowHeight(),           255, 0, 0, 255);
        EXPECT_PIXEL_EQ((1 - quadRadius) * getWindowWidth(),      (1 - quadRadius) * getWindowHeight(),     255, 0, 0, 255);

        EXPECT_PIXEL_EQ((quadRadius / 2) * getWindowWidth(),      (quadRadius / 2) * getWindowHeight(),     0,   0, 0, 255);
        EXPECT_PIXEL_EQ((1 - quadRadius / 2) * getWindowWidth(),  (1 - quadRadius / 2) * getWindowHeight(), 0,   0, 0, 255);
    }

    // Loaded entry points
    PFNGLVERTEXATTRIBDIVISORANGLEPROC mVertexAttribDivisorANGLE;
    PFNGLDRAWARRAYSINSTANCEDANGLEPROC mDrawArraysInstancedANGLE;
    PFNGLDRAWELEMENTSINSTANCEDANGLEPROC mDrawElementsInstancedANGLE;

    // Vertex data
    std::vector<GLfloat> mVertices;
    std::vector<GLfloat> mTexcoords;
    std::vector<GLfloat> mInstances;
    std::vector<GLushort> mIndices;

    const GLfloat quadRadius = 0.2f;
};

// This test uses a vertex shader with the first attribute (attribute zero) instanced.
// On D3D9 and D3D11 FL9_3, this triggers a special codepath that rearranges the input layout sent to D3D,
// to ensure that slot/stream zero of the input layout doesn't contain per-instance data.
TYPED_TEST(InstancingTest, AttributeZeroInstanced)
{
    const std::string vs = SHADER_SOURCE
    (
        attribute vec3 a_instancePos;
        attribute vec3 a_position;
        void main()
        {
            gl_Position = vec4(a_position.xyz + a_instancePos.xyz, 1.0);
        }
    );

    runTest(vs, true);
}

// Same as AttributeZeroInstanced, but attribute zero is not instanced.
// This ensures the general instancing codepath (i.e. without rearranging the input layout) works as expected.
TYPED_TEST(InstancingTest, AttributeZeroNotInstanced)
{
    const std::string vs = SHADER_SOURCE
    (
        attribute vec3 a_position;
        attribute vec3 a_instancePos;
        void main()
        {
            gl_Position = vec4(a_position.xyz + a_instancePos.xyz, 1.0);
        }
    );

    runTest(vs, false);
}