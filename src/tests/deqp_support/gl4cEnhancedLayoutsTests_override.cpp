/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 * \file  gl4cEnhancedLayoutsTests.cpp
 * \brief Implements conformance tests for "Enhanced Layouts" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cEnhancedLayoutsTests.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluShaderUtil.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <string>
#include <vector>

/* DEBUG */
#define USE_NSIGHT 0
#define DEBUG_ENBALE_MESSAGE_CALLBACK 0
#define DEBUG_NEG_LOG_ERROR 0
#define DEBUG_NEG_REMOVE_ERROR 0
#define DEBUG_REPLACE_TOKEN 0
#define DEBUG_REPEAT_TEST_CASE 0
#define DEBUG_REPEATED_TEST_CASE 0

/* Texture test base */
#define DEBUG_TTB_VERIFICATION_SNIPPET_STAGE 0
#define DEBUG_TTB_VERIFICATION_SNIPPET_VARIABLE 0

/* Tests */
#define DEBUG_VERTEX_ATTRIB_LOCATIONS_TEST_VARIABLE 0

/* WORKAROUNDS */
#define WRKARD_UNIFORMBLOCKMEMBEROFFSETANDALIGNTEST 0
#define WRKARD_UNIFORMBLOCKMEMBERALIGNNONPOWEROF2TEST 0
#define WRKARD_UNIFORMBLOCKALIGNMENT 0
#define WRKARD_VARYINGLOCATIONSTEST 0

using namespace glw;

namespace gl4cts
{
namespace EnhancedLayouts
{
namespace Utils
{
/** Constants used by "random" generators **/
static const GLuint s_rand_start    = 3;
static const GLuint s_rand_max      = 16;
static const GLuint s_rand_max_half = s_rand_max / 2;

/** Seed used by "random" generators **/
static GLuint s_rand = s_rand_start;

/** Get "random" unsigned int value
 *
 * @return Value
 **/
static GLuint GetRandUint()
{
    const GLuint rand = s_rand++;

    if (s_rand_max <= s_rand)
    {
        s_rand = s_rand_start;
    }

    return rand;
}

/** Get "random" int value
 *
 * @return Value
 **/
GLint GetRandInt()
{
    const GLint rand = GetRandUint() - s_rand_max_half;

    return rand;
}

/** Get "random" double value
 *
 * @return Value
 **/
GLdouble GetRandDouble()
{
    const GLint rand = GetRandInt();

    GLdouble result = (GLfloat)rand / (GLdouble)s_rand_max_half;

    return result;
}

/** Get "random" float value
 *
 * @return Value
 **/
GLfloat GetRandFloat()
{
    const GLint rand = GetRandInt();

    GLfloat result = (GLfloat)rand / (GLfloat)s_rand_max_half;

    return result;
}

/** String used by list routines **/
static const GLchar *const g_list = "LIST";

/** Type constants **/
const Type Type::_double = Type::GetType(Type::Double, 1, 1);
const Type Type::dmat2   = Type::GetType(Type::Double, 2, 2);
const Type Type::dmat2x3 = Type::GetType(Type::Double, 2, 3);
const Type Type::dmat2x4 = Type::GetType(Type::Double, 2, 4);
const Type Type::dmat3x2 = Type::GetType(Type::Double, 3, 2);
const Type Type::dmat3   = Type::GetType(Type::Double, 3, 3);
const Type Type::dmat3x4 = Type::GetType(Type::Double, 3, 4);
const Type Type::dmat4x2 = Type::GetType(Type::Double, 4, 2);
const Type Type::dmat4x3 = Type::GetType(Type::Double, 4, 3);
const Type Type::dmat4   = Type::GetType(Type::Double, 4, 4);
const Type Type::dvec2   = Type::GetType(Type::Double, 1, 2);
const Type Type::dvec3   = Type::GetType(Type::Double, 1, 3);
const Type Type::dvec4   = Type::GetType(Type::Double, 1, 4);
const Type Type::_int    = Type::GetType(Type::Int, 1, 1);
const Type Type::ivec2   = Type::GetType(Type::Int, 1, 2);
const Type Type::ivec3   = Type::GetType(Type::Int, 1, 3);
const Type Type::ivec4   = Type::GetType(Type::Int, 1, 4);
const Type Type::_float  = Type::GetType(Type::Float, 1, 1);
const Type Type::mat2    = Type::GetType(Type::Float, 2, 2);
const Type Type::mat2x3  = Type::GetType(Type::Float, 2, 3);
const Type Type::mat2x4  = Type::GetType(Type::Float, 2, 4);
const Type Type::mat3x2  = Type::GetType(Type::Float, 3, 2);
const Type Type::mat3    = Type::GetType(Type::Float, 3, 3);
const Type Type::mat3x4  = Type::GetType(Type::Float, 3, 4);
const Type Type::mat4x2  = Type::GetType(Type::Float, 4, 2);
const Type Type::mat4x3  = Type::GetType(Type::Float, 4, 3);
const Type Type::mat4    = Type::GetType(Type::Float, 4, 4);
const Type Type::vec2    = Type::GetType(Type::Float, 1, 2);
const Type Type::vec3    = Type::GetType(Type::Float, 1, 3);
const Type Type::vec4    = Type::GetType(Type::Float, 1, 4);
const Type Type::uint    = Type::GetType(Type::Uint, 1, 1);
const Type Type::uvec2   = Type::GetType(Type::Uint, 1, 2);
const Type Type::uvec3   = Type::GetType(Type::Uint, 1, 3);
const Type Type::uvec4   = Type::GetType(Type::Uint, 1, 4);

/** Generate data for type. This routine follows STD140 rules
 *
 * @return Vector of bytes filled with data
 **/
std::vector<GLubyte> Type::GenerateData() const
{
    const GLuint alignment = GetActualAlignment(0, false);
    const GLuint padding   = alignment - GetTypeSize(m_basic_type) * m_n_rows;
    const GLuint data_size = alignment * m_n_columns - padding;

    std::vector<GLubyte> data;
    data.resize(data_size);

    for (GLuint column = 0; column < m_n_columns; ++column)
    {
        GLvoid *ptr = (GLvoid *)&data[column * alignment];

        switch (m_basic_type)
        {
            case Double:
            {
                GLdouble *d_ptr = (GLdouble *)ptr;

                for (GLuint i = 0; i < m_n_rows; ++i)
                {
                    d_ptr[i] = GetRandDouble();
                }
            }
            break;
            case Float:
            {
                GLfloat *f_ptr = (GLfloat *)ptr;

                for (GLuint i = 0; i < m_n_rows; ++i)
                {
                    f_ptr[i] = GetRandFloat();
                }
            }
            break;
            case Int:
            {
                GLint *i_ptr = (GLint *)ptr;

                for (GLuint i = 0; i < m_n_rows; ++i)
                {
                    i_ptr[i] = GetRandInt();
                }
            }
            break;
            case Uint:
            {
                GLuint *ui_ptr = (GLuint *)ptr;

                for (GLuint i = 0; i < m_n_rows; ++i)
                {
                    ui_ptr[i] = GetRandUint();
                }
            }
            break;
        }
    }

    return data;
}

/** Generate data for type. This routine packs data tightly.
 *
 * @return Vector of bytes filled with data
 **/
std::vector<GLubyte> Type::GenerateDataPacked() const
{
    const GLuint basic_size = GetTypeSize(m_basic_type);
    const GLuint n_elements = m_n_columns * m_n_rows;
    const GLuint size       = basic_size * n_elements;

    std::vector<GLubyte> data;
    data.resize(size);

    GLvoid *ptr = (GLvoid *)&data[0];

    switch (m_basic_type)
    {
        case Double:
        {
            GLdouble *d_ptr = (GLdouble *)ptr;

            for (GLuint i = 0; i < n_elements; ++i)
            {
                d_ptr[i] = GetRandDouble();
            }
        }
        break;
        case Float:
        {
            GLfloat *f_ptr = (GLfloat *)ptr;

            for (GLuint i = 0; i < n_elements; ++i)
            {
                f_ptr[i] = GetRandFloat();
            }
        }
        break;
        case Int:
        {
            GLint *i_ptr = (GLint *)ptr;

            for (GLuint i = 0; i < n_elements; ++i)
            {
                i_ptr[i] = GetRandInt();
            }
        }
        break;
        case Uint:
        {
            GLuint *ui_ptr = (GLuint *)ptr;

            for (GLuint i = 0; i < n_elements; ++i)
            {
                ui_ptr[i] = GetRandUint();
            }
        }
        break;
    }

    return data;
}

/** Calculate "actual alignment". It work under assumption that align value is valid
 *
 * @param align    Requested alignment, eg with "align" qualifier
 * @param is_array Selects if an array of type or single instance should be considered
 *
 * @return Calculated value
 **/
GLuint Type::GetActualAlignment(GLuint align, bool is_array) const
{
    const GLuint base_alignment = GetBaseAlignment(is_array);

    return std::max(align, base_alignment);
}

/** Align given ofset with specified alignment
 *
 * @param offset    Offset
 * @param alignment Alignment
 *
 * @return Calculated value
 **/
GLuint align(GLuint offset, GLuint alignment)
{
    const GLuint rest = offset % alignment;

    if (0 != rest)
    {
        GLuint missing = alignment - rest;
        offset += missing;
    }

    return offset;
}

/** Calculate "actual offset"
 *
 * @param start_offset     Requested offset
 * @param actual_alignment Actual alignemnt
 *
 * @return Calculated value
 **/
GLuint Type::GetActualOffset(GLuint start_offset, GLuint actual_alignment)
{
    GLuint offset = align(start_offset, actual_alignment);

    return offset;
}

/** Calculate "base alignment" for given type
 *
 * @param is_array Select if array or single instance should be considered
 *
 * @return Calculated value
 **/
GLuint Type::GetBaseAlignment(bool is_array) const
{
    GLuint elements = 1;

    switch (m_n_rows)
    {
        case 2:
            elements = 2;
            break;
        case 3:
        case 4:
            elements = 4;
            break;
        default:
            break;
    }

    GLuint N         = GetTypeSize(m_basic_type);
    GLuint alignment = N * elements;

    if ((true == is_array) || (1 != m_n_columns))
    {
        alignment = align(alignment, 16 /* vec4 alignment */);
    }

    return alignment;
}

/** Returns string representing GLSL constructor of type with arguments provided in data
 *
 * @param data Array of values that will be used as construcotr arguments.
 *             It is interpreted as tightly packed array of type matching this type.
 *
 * @return String in form "Type(args)"
 **/
std::string Type::GetGLSLConstructor(const GLvoid *data) const
{
    const GLchar *type = GetGLSLTypeName();

    std::stringstream stream;

    stream << type << "(";

    /* Scalar or vector */
    if (1 == m_n_columns)
    {
        for (GLuint row = 0; row < m_n_rows; ++row)
        {
            switch (m_basic_type)
            {
                case Double:
                    stream << ((GLdouble *)data)[row];
                    break;
                case Float:
                    stream << ((GLfloat *)data)[row];
                    break;
                case Int:
                    stream << ((GLint *)data)[row];
                    break;
                case Uint:
                    stream << ((GLuint *)data)[row];
                    break;
            }

            if (row + 1 != m_n_rows)
            {
                stream << ", ";
            }
        }
    }
    else /* Matrix: mat(vec(), vec() .. ) */
    {
        const GLuint basic_size = GetTypeSize(m_basic_type);
        // Very indescoverable defect, the column stride should be calculated by rows, such as
        // mat2x3, which is 2, columns 3 rows, its column stride should be 3 * sizeof(float)
        const GLuint column_stride = m_n_rows * basic_size;
        const Type column_type     = GetType(m_basic_type, 1, m_n_rows);

        for (GLuint column = 0; column < m_n_columns; ++column)
        {
            const GLuint column_offset = column * column_stride;
            const GLvoid *column_data  = (GLubyte *)data + column_offset;

            stream << column_type.GetGLSLConstructor(column_data);

            if (column + 1 != m_n_columns)
            {
                stream << ", ";
            }
        }
    }

    stream << ")";

    return stream.str();
}

/** Get glsl name of the type
 *
 * @return Name of glsl type
 **/
const glw::GLchar *Type::GetGLSLTypeName() const
{
    static const GLchar *float_lut[4][4] = {
        {"float", "vec2", "vec3", "vec4"},
        {0, "mat2", "mat2x3", "mat2x4"},
        {0, "mat3x2", "mat3", "mat3x4"},
        {0, "mat4x2", "mat4x3", "mat4"},
    };

    static const GLchar *double_lut[4][4] = {
        {"double", "dvec2", "dvec3", "dvec4"},
        {0, "dmat2", "dmat2x3", "dmat2x4"},
        {0, "dmat3x2", "dmat3", "dmat3x4"},
        {0, "dmat4x2", "dmat4x3", "dmat4"},
    };

    static const GLchar *int_lut[4] = {"int", "ivec2", "ivec3", "ivec4"};

    static const GLchar *uint_lut[4] = {"uint", "uvec2", "uvec3", "uvec4"};

    const GLchar *result = 0;

    if ((1 > m_n_columns) || (1 > m_n_rows) || (4 < m_n_columns) || (4 < m_n_rows))
    {
        return 0;
    }

    switch (m_basic_type)
    {
        case Float:
            result = float_lut[m_n_columns - 1][m_n_rows - 1];
            break;
        case Double:
            result = double_lut[m_n_columns - 1][m_n_rows - 1];
            break;
        case Int:
            result = int_lut[m_n_rows - 1];
            break;
        case Uint:
            result = uint_lut[m_n_rows - 1];
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Get number of locations required for the type
 *
 * @return Number of columns times:
 *          - 2 when type is double with 3 or 4 rows,
 *          - 1 otherwise or if it's a vertex shader input.
 **/
GLuint Type::GetLocations(bool is_vs_input) const
{
    GLuint n_loc_per_column;

    /* 1 or 2 doubles any for rest */
    if ((2 >= m_n_rows) || (Double != m_basic_type) || is_vs_input)
    {
        n_loc_per_column = 1;
    }
    else
    {
        /* 3 and 4 doubles */
        n_loc_per_column = 2;
    }

    return n_loc_per_column * m_n_columns;
}

/** Get size of the type in bytes.
 * Note that this routine doesn't consider arrays and assumes
 * column_major matrices.
 *
 * @return Formula:
 *          - If std140 packaging and matrix; number of columns * base alignment
 *          - Otherwise; number of elements * sizeof(base_type)
 **/
GLuint Type::GetSize(const bool is_std140) const
{
    const GLuint basic_type_size = GetTypeSize(m_basic_type);
    const GLuint n_elements      = m_n_columns * m_n_rows;

    if (is_std140 && m_n_columns > 1)
    {
        return m_n_columns * GetBaseAlignment(false);
    }

    return basic_type_size * n_elements;
}

/** Get GLenum representing the type
 *
 * @return GLenum
 **/
GLenum Type::GetTypeGLenum() const
{
    static const GLenum float_lut[4][4] = {
        {GL_FLOAT, GL_FLOAT_VEC2, GL_FLOAT_VEC3, GL_FLOAT_VEC4},
        {0, GL_FLOAT_MAT2, GL_FLOAT_MAT2x3, GL_FLOAT_MAT2x4},
        {0, GL_FLOAT_MAT3x2, GL_FLOAT_MAT3, GL_FLOAT_MAT3x4},
        {0, GL_FLOAT_MAT4x2, GL_FLOAT_MAT4x3, GL_FLOAT_MAT4},
    };

    static const GLenum double_lut[4][4] = {
        {GL_DOUBLE, GL_DOUBLE_VEC2, GL_DOUBLE_VEC3, GL_DOUBLE_VEC4},
        {0, GL_DOUBLE_MAT2, GL_DOUBLE_MAT2x3, GL_DOUBLE_MAT2x4},
        {0, GL_DOUBLE_MAT3x2, GL_DOUBLE_MAT3, GL_DOUBLE_MAT3x4},
        {0, GL_DOUBLE_MAT4x2, GL_DOUBLE_MAT4x3, GL_DOUBLE_MAT4},
    };

    static const GLenum int_lut[4] = {GL_INT, GL_INT_VEC2, GL_INT_VEC3, GL_INT_VEC4};

    static const GLenum uint_lut[4] = {GL_UNSIGNED_INT, GL_UNSIGNED_INT_VEC2, GL_UNSIGNED_INT_VEC3,
                                       GL_UNSIGNED_INT_VEC4};

    GLenum result = 0;

    if ((1 > m_n_columns) || (1 > m_n_rows) || (4 < m_n_columns) || (4 < m_n_rows))
    {
        return 0;
    }

    switch (m_basic_type)
    {
        case Float:
            result = float_lut[m_n_columns - 1][m_n_rows - 1];
            break;
        case Double:
            result = double_lut[m_n_columns - 1][m_n_rows - 1];
            break;
        case Int:
            result = int_lut[m_n_rows - 1];
            break;
        case Uint:
            result = uint_lut[m_n_rows - 1];
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Calculate the number of components consumed by a type
 *   according to 11.1.2.1 Output Variables
 *
 * @return Calculated number of components for the type
 **/
GLuint Type::GetNumComponents() const
{
    // Rule 3 of Section 7.6.2.2
    // If the member is a three-component vector with components consuming N
    // basic machine units, the base alignment is 4N.
    GLuint num_components = (m_n_rows == 3 ? 4 : m_n_rows) * m_n_columns;

    if (m_basic_type == Double)
    {
        num_components *= 2;
    }

    return num_components;
}

/** Calculate the valid values to use with the component qualifier
 *
 * @return Vector with the valid values, in growing order, or empty if
 *         the component qualifier is not allowed
 **/
std::vector<GLuint> Type::GetValidComponents() const
{
    const GLuint component_size            = Utils::Type::Double == m_basic_type ? 2 : 1;
    const GLuint n_components_per_location = Utils::Type::Double == m_basic_type ? 2 : 4;
    const GLuint n_req_components          = m_n_rows;
    const GLint max_valid_component = (GLint)n_components_per_location - (GLint)n_req_components;
    std::vector<GLuint> data;

    /* The component qualifier cannot be used for matrices */
    if (1 != m_n_columns)
    {
        return data;
    }

    /* The component qualifier cannot be used for dvec3/dvec4 */
    if (max_valid_component < 0)
    {
        return data;
    }

    for (GLuint i = 0; i <= (GLuint)max_valid_component; ++i)
    {
        data.push_back(i * component_size);
    }

    return data;
}

/** Calculate stride for the type according to std140 rules
 *
 * @param alignment        Alignment of type
 * @param n_columns        Number of columns
 * @param n_array_elements Number of elements in array
 *
 * @return Calculated value
 **/
GLuint Type::CalculateStd140Stride(GLuint alignment, GLuint n_columns, GLuint n_array_elements)
{
    GLuint stride = alignment * n_columns;
    if (0 != n_array_elements)
    {
        stride *= n_array_elements;
    }

    return stride;
}

/** Check if glsl support matrices for specific basic type
 *
 * @param type Basic type
 *
 * @return true if matrices of <type> are supported, false otherwise
 **/
bool Type::DoesTypeSupportMatrix(TYPES type)
{
    bool result = false;

    switch (type)
    {
        case Float:
        case Double:
            result = true;
            break;
        case Int:
        case Uint:
            result = false;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Creates instance of Type
 *
 * @param basic_type Select basic type of instance
 * @param n_columns  Number of columns
 * @param n_rows     Number of rows
 *
 * @return Type instance
 **/
Type Type::GetType(TYPES basic_type, glw::GLuint n_columns, glw::GLuint n_rows)
{
    Type type = {basic_type, n_columns, n_rows};

    return type;
}

/** Get Size of given type in bytes
 *
 * @param type
 *
 * @return Size of type
 **/
GLuint Type::GetTypeSize(TYPES type)
{
    GLuint result = 0;

    switch (type)
    {
        case Float:
            result = sizeof(GLfloat);
            break;
        case Double:
            result = sizeof(GLdouble);
            break;
        case Int:
            result = sizeof(GLint);
            break;
        case Uint:
            result = sizeof(GLuint);
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Get GLenum representing given type
 *
 * @param type
 *
 * @return GLenum value
 **/
GLenum Type::GetTypeGLenum(TYPES type)
{
    GLenum result = 0;

    switch (type)
    {
        case Float:
            result = GL_FLOAT;
            break;
        case Double:
            result = GL_DOUBLE;
            break;
        case Int:
            result = GL_INT;
            break;
        case Uint:
            result = GL_UNSIGNED_INT;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Check if two types can share the same location, based on the underlying numerical type and bit
 *width
 *
 * @param first   First type to compare
 * @param second  Second type to compare
 *
 * @return true if the types can share the same location
 **/
bool Type::CanTypesShareLocation(TYPES first, TYPES second)
{
    if (first == second)
    {
        return true;
    }

    if (Float == first || Float == second || Double == first || Double == second)
    {
        return false;
    }

    return true;
}

/** Get proper glUniformNdv routine for vectors with specified number of rows
 *
 * @param gl     GL functions
 * @param n_rows Number of rows
 *
 * @return Function address
 **/
uniformNdv getUniformNdv(const glw::Functions &gl, glw::GLuint n_rows)
{
    uniformNdv result = 0;

    switch (n_rows)
    {
        case 1:
            result = gl.uniform1dv;
            break;
        case 2:
            result = gl.uniform2dv;
            break;
        case 3:
            result = gl.uniform3dv;
            break;
        case 4:
            result = gl.uniform4dv;
            break;
        default:
            TCU_FAIL("Invalid number of rows");
    }

    return result;
}

/** Get proper glUniformNfv routine for vectors with specified number of rows
 *
 * @param gl     GL functions
 * @param n_rows Number of rows
 *
 * @return Function address
 **/
uniformNfv getUniformNfv(const glw::Functions &gl, glw::GLuint n_rows)
{
    uniformNfv result = 0;

    switch (n_rows)
    {
        case 1:
            result = gl.uniform1fv;
            break;
        case 2:
            result = gl.uniform2fv;
            break;
        case 3:
            result = gl.uniform3fv;
            break;
        case 4:
            result = gl.uniform4fv;
            break;
        default:
            TCU_FAIL("Invalid number of rows");
    }

    return result;
}

/** Get proper glUniformNiv routine for vectors with specified number of rows
 *
 * @param gl     GL functions
 * @param n_rows Number of rows
 *
 * @return Function address
 **/
uniformNiv getUniformNiv(const glw::Functions &gl, glw::GLuint n_rows)
{
    uniformNiv result = 0;

    switch (n_rows)
    {
        case 1:
            result = gl.uniform1iv;
            break;
        case 2:
            result = gl.uniform2iv;
            break;
        case 3:
            result = gl.uniform3iv;
            break;
        case 4:
            result = gl.uniform4iv;
            break;
        default:
            TCU_FAIL("Invalid number of rows");
    }

    return result;
}

/** Get proper glUniformNuiv routine for vectors with specified number of rows
 *
 * @param gl     GL functions
 * @param n_rows Number of rows
 *
 * @return Function address
 **/
uniformNuiv getUniformNuiv(const glw::Functions &gl, glw::GLuint n_rows)
{
    uniformNuiv result = 0;

    switch (n_rows)
    {
        case 1:
            result = gl.uniform1uiv;
            break;
        case 2:
            result = gl.uniform2uiv;
            break;
        case 3:
            result = gl.uniform3uiv;
            break;
        case 4:
            result = gl.uniform4uiv;
            break;
        default:
            TCU_FAIL("Invalid number of rows");
    }

    return result;
}

/** Get proper glUniformMatrixNdv routine for matrix with specified number of columns and rows
 *
 * @param gl     GL functions
 * @param n_rows Number of rows
 *
 * @return Function address
 **/
uniformMatrixNdv getUniformMatrixNdv(const glw::Functions &gl,
                                     glw::GLuint n_columns,
                                     glw::GLuint n_rows)
{
    uniformMatrixNdv result = 0;

    switch (n_columns)
    {
        case 2:
            switch (n_rows)
            {
                case 2:
                    result = gl.uniformMatrix2dv;
                    break;
                case 3:
                    result = gl.uniformMatrix2x3dv;
                    break;
                case 4:
                    result = gl.uniformMatrix2x4dv;
                    break;
                default:
                    TCU_FAIL("Invalid number of rows");
            }
            break;
        case 3:
            switch (n_rows)
            {
                case 2:
                    result = gl.uniformMatrix3x2dv;
                    break;
                case 3:
                    result = gl.uniformMatrix3dv;
                    break;
                case 4:
                    result = gl.uniformMatrix3x4dv;
                    break;
                default:
                    TCU_FAIL("Invalid number of rows");
            }
            break;
        case 4:
            switch (n_rows)
            {
                case 2:
                    result = gl.uniformMatrix4x2dv;
                    break;
                case 3:
                    result = gl.uniformMatrix4x3dv;
                    break;
                case 4:
                    result = gl.uniformMatrix4dv;
                    break;
                default:
                    TCU_FAIL("Invalid number of rows");
            }
            break;
        default:
            TCU_FAIL("Invalid number of columns");
    }

    return result;
}

/** Get proper glUniformMatrixNfv routine for vectors with specified number of columns and rows
 *
 * @param gl     GL functions
 * @param n_rows Number of rows
 *
 * @return Function address
 **/
uniformMatrixNfv getUniformMatrixNfv(const glw::Functions &gl,
                                     glw::GLuint n_columns,
                                     glw::GLuint n_rows)
{
    uniformMatrixNfv result = 0;

    switch (n_columns)
    {
        case 2:
            switch (n_rows)
            {
                case 2:
                    result = gl.uniformMatrix2fv;
                    break;
                case 3:
                    result = gl.uniformMatrix2x3fv;
                    break;
                case 4:
                    result = gl.uniformMatrix2x4fv;
                    break;
                default:
                    TCU_FAIL("Invalid number of rows");
            }
            break;
        case 3:
            switch (n_rows)
            {
                case 2:
                    result = gl.uniformMatrix3x2fv;
                    break;
                case 3:
                    result = gl.uniformMatrix3fv;
                    break;
                case 4:
                    result = gl.uniformMatrix3x4fv;
                    break;
                default:
                    TCU_FAIL("Invalid number of rows");
            }
            break;
        case 4:
            switch (n_rows)
            {
                case 2:
                    result = gl.uniformMatrix4x2fv;
                    break;
                case 3:
                    result = gl.uniformMatrix4x3fv;
                    break;
                case 4:
                    result = gl.uniformMatrix4fv;
                    break;
                default:
                    TCU_FAIL("Invalid number of rows");
            }
            break;
        default:
            TCU_FAIL("Invalid number of columns");
    }

    return result;
}

bool verifyVarying(Program &program,
                   const std::string &parent_name,
                   const Variable::Descriptor &desc,
                   std::stringstream &stream,
                   bool is_input)
{
    GLint component  = 0;
    GLuint index     = 0;
    GLenum interface = GL_PROGRAM_INPUT;
    GLint location   = 0;

    if (false == is_input)
    {
        interface = GL_PROGRAM_OUTPUT;
    }

    const std::string &name =
        Utils::Variable::GetReference(parent_name, desc, Utils::Variable::BASIC, 0);

    try
    {
        index = program.GetResourceIndex(name, interface);

        program.GetResource(interface, index, GL_LOCATION, 1 /* size */, &location);
        program.GetResource(interface, index, GL_LOCATION_COMPONENT, 1 /* size */, &component);
    }
    catch (std::exception &exc)
    {
        stream << "Failed to query program for varying: " << desc.m_name
               << ". Reason: " << exc.what() << "\n";

        return false;
    }

    bool result = true;

    if (location != desc.m_expected_location)
    {
        stream << "Attribute: " << desc.m_name << " - invalid location: " << location
               << " expected: " << desc.m_expected_location << std::endl;
        result = false;
    }
    if (component != desc.m_expected_component)
    {
        stream << "Attribute: " << desc.m_name << " - invalid component: " << component
               << " expected: " << desc.m_expected_component << std::endl;
        result = false;
    }

    return result;
}

/** Query program resource for given variable and verify that everything is as expected
 *
 * @param program  Program object
 * @param variable Variable object
 * @param stream   Stream that will be used to log any error
 * @param is_input Selects if varying is input or output
 *
 * @return true if verification is positive, false otherwise
 **/
bool checkVarying(Program &program,
                  Shader::STAGES stage,
                  const Variable &variable,
                  std::stringstream &stream,
                  bool is_input)
{
    bool result = true;

    if (variable.IsBlock())
    {
        Utils::Interface *interface = variable.m_descriptor.m_interface;
        const size_t n_members      = interface->m_members.size();

        for (size_t i = 0; i < n_members; ++i)
        {
            const Variable::Descriptor &member = interface->m_members[i];
            bool member_result =
                verifyVarying(program, interface->m_name, member, stream, is_input);

            if (false == member_result)
            {
                result = false;
            }
        }
    }
    /*
     To query the the location of struct member by glGetProgramResource, we need pass the variable
     name "gs_fs_output[0].single", but in original implementation, the test pass the name
     "Data.single", which can't get any valid result. struct Data { dmat2 single; dmat2 array[1];
     };
     layout (location = 0) in Data gs_fs_output[1];
     */
    else if (variable.IsStruct())
    {
        Utils::Interface *interface = variable.m_descriptor.m_interface;
        const size_t n_members      = interface->m_members.size();
        std::string structVariable  = variable.m_descriptor.m_name;

        switch (Variable::GetFlavour(stage, is_input ? Variable::INPUT : Variable::OUTPUT))
        {
            case Variable::ARRAY:
            case Variable::INDEXED_BY_INVOCATION_ID:
                structVariable.append("[0]");
                break;
            default:
                break;
        }

        // If struct variable is an array
        if (0 != variable.m_descriptor.m_n_array_elements)
        {
            for (GLuint i = 0; i < variable.m_descriptor.m_n_array_elements; i++)
            {
                GLchar buffer[16];
                sprintf(buffer, "%d", i);
                structVariable.append("[");
                structVariable.append(buffer);
                structVariable.append("]");
                for (size_t j = 0; j < n_members; ++j)
                {
                    const Variable::Descriptor &member = interface->m_members[j];
                    bool member_result =
                        verifyVarying(program, structVariable, member, stream, is_input);

                    if (false == member_result)
                    {
                        result = false;
                    }
                }
            }
        }
        else
        {
            for (GLuint i = 0; i < n_members; ++i)
            {
                const Variable::Descriptor &member = interface->m_members[i];
                bool member_result =
                    verifyVarying(program, structVariable, member, stream, is_input);

                if (false == member_result)
                {
                    result = false;
                }
            }
        }
    }
    else
    {
        result = verifyVarying(program, "", variable.m_descriptor, stream, is_input);
    }
    return result;
}

/** Query program resource for given variable and verify that everything is as expected
 *
 * @param program  Program object
 * @param variable Variable object
 * @param stream   Stream that will be used to log any error
 *
 * @return true if verification is positive, false otherwise
 **/
bool checkUniform(Program &program, const Utils::Variable &variable, std::stringstream &stream)
{
    bool result = true;

    if (false == variable.IsBlock())
    {
        TCU_FAIL("Not implemented");
    }
    else
    {
        Utils::Interface *interface = variable.m_descriptor.m_interface;

        size_t size = interface->m_members.size();

        std::vector<GLuint> indices;
        std::vector<const char *> names;
        std::vector<std::string> names_str;
        std::vector<GLint> offsets;

        indices.resize(size);
        names.resize(size);
        names_str.resize(size);
        offsets.resize(size);

        for (size_t i = 0; i < size; ++i)
        {
            indices[i] = 0;
            offsets[i] = 0;

            const std::string &name = Utils::Variable::GetReference(
                interface->m_name, interface->m_members[i], Utils::Variable::BASIC, 0);

            if (Utils::Variable::INTERFACE == interface->m_members[i].m_type)
            {
                const std::string &member_name = Utils::Variable::GetReference(
                    name, interface->m_members[i].m_interface->m_members[0], Utils::Variable::BASIC,
                    0);

                names_str[i] = member_name;
            }
            else
            {
                names_str[i] = name;
            }

            names[i] = names_str[i].c_str();
        }

        try
        {
            program.GetUniformIndices(static_cast<glw::GLsizei>(size), &names[0], &indices[0]);
            program.GetActiveUniformsiv(static_cast<glw::GLsizei>(size), &indices[0],
                                        GL_UNIFORM_OFFSET, &offsets[0]);
        }
        catch (std::exception &exc)
        {
            stream << "Failed to query program for uniforms in block: "
                   << variable.m_descriptor.m_name << ". Reason: " << exc.what() << "\n";

            return false;
        }

        for (size_t i = 0; i < size; ++i)
        {
            Utils::Variable::Descriptor &desc = interface->m_members[i];

            if (offsets[i] != (GLint)desc.m_offset)
            {
                stream << "Uniform: " << desc.m_name << " - invalid offset: " << offsets[i]
                       << " expected: " << desc.m_offset << std::endl;
                result = false;
            }
        }
    }

    return result;
}

/** Query program resource for given variable and verify that everything is as expected
 *
 * @param program  Program object
 * @param variable Variable object
 * @param stream   Stream that will be used to log any error
 *
 * @return true if verification is positive, false otherwise
 **/
bool checkSSB(Program &program, const Utils::Variable &variable, std::stringstream &stream)
{
    bool result = true;

    if (false == variable.IsBlock())
    {
        TCU_FAIL("Not implemented");
    }
    else
    {
        Utils::Interface *interface = variable.m_descriptor.m_interface;

        size_t size = interface->m_members.size();

        for (size_t i = 0; i < size; ++i)
        {
            GLuint index         = 0;
            std::string name_str = "";
            GLint offset         = 0;

            const std::string &name = Utils::Variable::GetReference(
                interface->m_name, interface->m_members[i], Utils::Variable::BASIC, 0);

            if (Utils::Variable::INTERFACE == interface->m_members[i].m_type)
            {
                const std::string &member_name = Utils::Variable::GetReference(
                    name, interface->m_members[i].m_interface->m_members[0], Utils::Variable::BASIC,
                    0);

                name_str = member_name;
            }
            else
            {
                name_str = name;
            }

            try
            {
                index = program.GetResourceIndex(name_str, GL_BUFFER_VARIABLE);

                program.GetResource(GL_BUFFER_VARIABLE, index, GL_OFFSET, 1, &offset);
            }
            catch (std::exception &exc)
            {
                stream << "Failed to query program for buffer variable: "
                       << variable.m_descriptor.m_name << ". Reason: " << exc.what() << "\n";

                return false;
            }

            Utils::Variable::Descriptor &desc = interface->m_members[i];

            if (offset != (GLint)desc.m_offset)
            {
                stream << "Uniform: " << desc.m_name << " - invalid offset: " << offset
                       << " expected: " << desc.m_offset << std::endl;
                result = false;
            }
        }
    }

    return result;
}

/** Query program resources at given stage and verifies results
 *
 * @param program           Program object
 * @param program_interface Definition of program interface
 * @param stage             Stage to be verified
 * @param check_inputs      Select if inputs should be verified
 * @param check_outputs     Select if output should be verified
 * @param check_uniforms    Select if uniforms should be verified
 * @param check_ssbs        Select if buffers should be verified
 * @param stream            Stream that will be used to log any error
 *
 * @return true if verification is positive, false otherwise
 **/
bool checkProgramStage(Program &program,
                       const ProgramInterface &program_interface,
                       Utils::Shader::STAGES stage,
                       bool check_inputs,
                       bool check_outputs,
                       bool check_uniforms,
                       bool check_ssbs,
                       std::stringstream &stream)
{
    typedef Variable::PtrVector::const_iterator const_iterator;

    const ShaderInterface &interface = program_interface.GetShaderInterface(stage);

    bool result = true;

    /* Inputs */
    if (true == check_inputs)
    {
        const Variable::PtrVector &inputs = interface.m_inputs;

        for (const_iterator it = inputs.begin(); it != inputs.end(); ++it)
        {
            if (false == checkVarying(program, stage, **it, stream, true))
            {
                result = false;
            }
        }
    }

    /* Outputs */
    if (true == check_outputs)
    {
        const Variable::PtrVector &outputs = interface.m_outputs;

        for (const_iterator it = outputs.begin(); it != outputs.end(); ++it)
        {
            if (false == checkVarying(program, stage, **it, stream, false))
            {
                result = false;
            }
        }
    }

    /* Uniforms */
    if (true == check_uniforms)
    {
        const Variable::PtrVector &uniforms = interface.m_uniforms;

        for (const_iterator it = uniforms.begin(); it != uniforms.end(); ++it)
        {
            if (false == checkUniform(program, **it, stream))
            {
                result = false;
            }
        }
    }

    /* SSBs */
    if (true == check_ssbs)
    {
        const Variable::PtrVector &ssbs = interface.m_ssb_blocks;

        for (const_iterator it = ssbs.begin(); it != ssbs.end(); ++it)
        {
            if (false == checkSSB(program, **it, stream))
            {
                result = false;
            }
        }
    }

    return result;
}

/** Query resources of monolithic compute program and verifies results
 *
 * @param program           Program object
 * @param program_interface Definition of program interface
 * @param stream            Stream that will be used to log any error
 *
 * @return true if verification is positive, false otherwise
 **/
bool checkMonolithicComputeProgramInterface(Program &program,
                                            const ProgramInterface &program_interface,
                                            std::stringstream &stream)
{
    bool result = true;

    if (false == checkProgramStage(program, program_interface, Shader::COMPUTE, false, false, true,
                                   true, stream))
    {
        result = false;
    }

    /* Done */
    return result;
}

/** Query resources of monolithic draw program and verifies results
 *
 * @param program           Program object
 * @param program_interface Definition of program interface
 * @param stream            Stream that will be used to log any error
 *
 * @return true if verification is positive, false otherwise
 **/
bool checkMonolithicDrawProgramInterface(Program &program,
                                         const ProgramInterface &program_interface,
                                         std::stringstream &stream)
{
    bool result = true;

    if (false == checkProgramStage(program, program_interface, Shader::VERTEX, true, false, true,
                                   true, stream))
    {
        result = false;
    }

    /* Done */
    return result;
}

/** Query resources of separable draw program and verifies results
 *
 * @param program           Program object
 * @param program_interface Definition of program interface
 * @param stream            Stream that will be used to log any error
 *
 * @return true if verification is positive, false otherwise
 **/
bool checkSeparableDrawProgramInterface(Program &program,
                                        const ProgramInterface &program_interface,
                                        Utils::Shader::STAGES stage,
                                        std::stringstream &stream)
{
    bool result = true;

    if (false ==
        checkProgramStage(program, program_interface, stage, true, true, true, true, stream))
    {
        result = false;
    }

    /* Done */
    return result;
}

/** Check if extension is supported
 *
 * @param context        Test context
 * @param extension_name Name of extension
 *
 * @return true if extension is supported, false otherwise
 **/
bool isExtensionSupported(deqp::Context &context, const GLchar *extension_name)
{
    const std::vector<std::string> &extensions = context.getContextInfo().getExtensions();

    if (std::find(extensions.begin(), extensions.end(), extension_name) == extensions.end())
    {
        return false;
    }

    return true;
}

/** Check if GL context meets version requirements
 *
 * @param gl             Functions
 * @param required_major Minimum required MAJOR_VERSION
 * @param required_minor Minimum required MINOR_VERSION
 *
 * @return true if GL context version is at least as requested, false otherwise
 **/
bool isGLVersionAtLeast(const Functions &gl, GLint required_major, GLint required_minor)
{
    glw::GLint major = 0;
    glw::GLint minor = 0;

    gl.getIntegerv(GL_MAJOR_VERSION, &major);
    gl.getIntegerv(GL_MINOR_VERSION, &minor);

    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    if (major > required_major)
    {
        /* Major is higher than required one */
        return true;
    }
    else if (major == required_major)
    {
        if (minor >= required_minor)
        {
            /* Major is equal to required one */
            /* Minor is higher than or equal to required one */
            return true;
        }
        else
        {
            /* Major is equal to required one */
            /* Minor is lower than required one */
            return false;
        }
    }
    else
    {
        /* Major is lower than required one */
        return false;
    }
}

/** Replace first occurance of <token> with <text> in <string> starting at <search_posistion>
 *
 * @param token           Token string
 * @param search_position Position at which find will start, it is updated to position at which
 *replaced text ends
 * @param text            String that will be used as replacement for <token>
 * @param string          String to work on
 **/
void replaceToken(const GLchar *token,
                  size_t &search_position,
                  const GLchar *text,
                  std::string &string)
{
    const size_t text_length    = strlen(text);
    const size_t token_length   = strlen(token);
    const size_t token_position = string.find(token, search_position);

#if DEBUG_REPLACE_TOKEN
    if (std::string::npos == token_position)
    {
        string.append("\n\nInvalid token: ");
        string.append(token);

        TCU_FAIL(string.c_str());
    }
#endif /* DEBUG_REPLACE_TOKEN */

    string.replace(token_position, token_length, text, text_length);

    search_position = token_position + text_length;
}

/** Replace all occurances of <token> with <text> in <string>
 *
 * @param token           Token string
 * @param text            String that will be used as replacement for <token>
 * @param string          String to work on
 **/
void replaceAllTokens(const GLchar *token, const GLchar *text, std::string &string)
{
    const size_t text_length  = strlen(text);
    const size_t token_length = strlen(token);

    size_t search_position = 0;

    while (1)
    {
        const size_t token_position = string.find(token, search_position);

        if (std::string::npos == token_position)
        {
            break;
        }

        search_position = token_position + text_length;

        string.replace(token_position, token_length, text, text_length);
    }
}

/** Rounds up the value to the next power of 2.
 * This routine does not work for 0, see the url for explanations.
 *
 * @param value Starting point
 *
 * @return Calculated value
 **/
glw::GLuint roundUpToPowerOf2(glw::GLuint value)
{
    /* Taken from: graphics.stanford.edu/~seander/bithacks.html */
    --value;

    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;

    ++value;

    return value;
}

/** Insert elements of list into string.
 * List in string is represented either by token "LIST" or "SEPARATORLIST".
 * If SEPARATORLIST is available, than SEPARATOR is replaced with <separator>.
 * LIST is replaced with <element>SEPARATORLIST
 *
 * @param element         Element to be inserted
 * @param separator       Separator inserted between elements
 * @param search_position Position in string, where search for list should start
 * @param string          String
 **/
void insertElementOfList(const GLchar *element,
                         const GLchar *separator,
                         size_t &search_position,
                         std::string &string)
{
    static const char *list     = g_list;
    static const char *sep_list = "SEPARATORLIST";

    /* Try to get "list" positions */
    const size_t list_position     = string.find(list, search_position);
    const size_t sep_list_position = string.find(sep_list, search_position);

    /* There is no list in string */
    if (std::string::npos == list_position)
    {
        return;
    }

    if (9 /* strlen(SEPARATOR) */ == list_position - sep_list_position)
    {
        replaceToken("SEPARATOR", search_position, separator, string);
    }

    /* Save search_position */
    const size_t start_position = search_position;

    /* Prepare new element */
    replaceToken("LIST", search_position, "ELEMENTSEPARATORLIST", string);

    /* Restore search_position */
    search_position = start_position;

    /* Replace element and separator */
    replaceToken("ELEMENT", search_position, element, string);
}

/** Close list in string.
 * If SEPARATORLIST is available, than SEPARATOR is replaced with <separator>
 * LIST is replaced with ""
 *
 * @param separator       Separator inserted between elements
 * @param search_position Position in string, where search for list should start
 * @param string          String
 **/
void endList(const glw::GLchar *separator, size_t &search_position, std::string &string)
{
    const size_t sep_position = string.find("SEPARATOR", search_position);
    if (std::string::npos != sep_position)
    {
        replaceToken("SEPARATOR", search_position, separator, string);
    }

    replaceToken("LIST", search_position, "", string);
}

/* Buffer constants */
const GLuint Buffer::m_invalid_id = -1;

/** Constructor.
 *
 * @param context CTS context.
 **/
Buffer::Buffer(deqp::Context &context) : m_id(m_invalid_id), m_buffer(Array), m_context(context) {}

/** Destructor
 *
 **/
Buffer::~Buffer()
{
    Release();
}

/** Initialize buffer instance
 *
 * @param buffer Buffer type
 * @param usage  Buffer usage enum
 * @param size   <size> parameter
 * @param data   <data> parameter
 **/
void Buffer::Init(BUFFERS buffer, USAGE usage, GLsizeiptr size, GLvoid *data)
{
    /* Delete previous buffer instance */
    Release();

    m_buffer = buffer;

    const Functions &gl = m_context.getRenderContext().getFunctions();

    Generate(gl, m_id);
    Bind(gl, m_id, m_buffer);
    Data(gl, m_buffer, usage, size, data);
}

/** Release buffer instance
 *
 **/
void Buffer::Release()
{
    if (m_invalid_id != m_id)
    {
        const Functions &gl = m_context.getRenderContext().getFunctions();

        gl.deleteBuffers(1, &m_id);
        m_id = m_invalid_id;
    }
}

/** Binds buffer to its target
 *
 **/
void Buffer::Bind() const
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    Bind(gl, m_id, m_buffer);
}

/** Binds indexed buffer
 *
 * @param index <index> parameter
 **/
void Buffer::BindBase(GLuint index) const
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    BindBase(gl, m_id, m_buffer, index);
}

/** Binds range of buffer
 *
 * @param index  <index> parameter
 * @param offset <offset> parameter
 * @param size   <size> parameter
 **/
void Buffer::BindRange(GLuint index, GLintptr offset, GLsizeiptr size) const
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    BindRange(gl, m_id, m_buffer, index, offset, size);
}

/** Allocate memory for buffer and sends initial content
 *
 * @param usage  Buffer usage enum
 * @param size   <size> parameter
 * @param data   <data> parameter
 **/
void Buffer::Data(USAGE usage, glw::GLsizeiptr size, glw::GLvoid *data)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    Data(gl, m_buffer, usage, size, data);
}

/** Maps contents of buffer into CPU space
 *
 * @param access Requested access
 *
 * @return Pointer to memory region available for CPU
 **/
GLvoid *Buffer::Map(ACCESS access)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    return Map(gl, m_buffer, access);
}

/** Allocate memory for buffer and sends initial content
 *
 * @param offset Offset in buffer
 * @param size   <size> parameter
 * @param data   <data> parameter
 **/
void Buffer::SubData(glw::GLintptr offset, glw::GLsizeiptr size, glw::GLvoid *data)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    SubData(gl, m_buffer, offset, size, data);
}

/** Maps contents of buffer into CPU space
 **/
void Buffer::UnMap()
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    return UnMap(gl, m_buffer);
}

/** Bind buffer to given target
 *
 * @param gl     GL functions
 * @param id     Id of buffer
 * @param buffer Buffer enum
 **/
void Buffer::Bind(const Functions &gl, GLuint id, BUFFERS buffer)
{
    GLenum target = GetBufferGLenum(buffer);

    gl.bindBuffer(target, id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");
}

/** Binds indexed buffer
 *
 * @param gl     GL functions
 * @param id     Id of buffer
 * @param buffer Buffer enum
 * @param index  <index> parameter
 **/
void Buffer::BindBase(const Functions &gl, GLuint id, BUFFERS buffer, GLuint index)
{
    GLenum target = GetBufferGLenum(buffer);

    gl.bindBufferBase(target, index, id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferBase");
}

/** Binds buffer range
 *
 * @param gl     GL functions
 * @param id     Id of buffer
 * @param buffer Buffer enum
 * @param index  <index> parameter
 * @param offset <offset> parameter
 * @param size   <size> parameter
 **/
void Buffer::BindRange(const Functions &gl,
                       GLuint id,
                       BUFFERS buffer,
                       GLuint index,
                       GLintptr offset,
                       GLsizeiptr size)
{
    GLenum target = GetBufferGLenum(buffer);

    gl.bindBufferRange(target, index, id, offset, size);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferRange");
}

/** Allocate memory for buffer and sends initial content
 *
 * @param gl     GL functions
 * @param buffer Buffer enum
 * @param usage  Buffer usage enum
 * @param size   <size> parameter
 * @param data   <data> parameter
 **/
void Buffer::Data(const glw::Functions &gl,
                  BUFFERS buffer,
                  USAGE usage,
                  glw::GLsizeiptr size,
                  glw::GLvoid *data)
{
    GLenum target   = GetBufferGLenum(buffer);
    GLenum gl_usage = GetUsageGLenum(usage);

    gl.bufferData(target, size, data, gl_usage);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BufferData");
}

/** Allocate memory for buffer and sends initial content
 *
 * @param gl     GL functions
 * @param buffer Buffer enum
 * @param offset Offset in buffer
 * @param size   <size> parameter
 * @param data   <data> parameter
 **/
void Buffer::SubData(const glw::Functions &gl,
                     BUFFERS buffer,
                     glw::GLintptr offset,
                     glw::GLsizeiptr size,
                     glw::GLvoid *data)
{
    GLenum target = GetBufferGLenum(buffer);

    gl.bufferSubData(target, offset, size, data);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BufferSubData");
}

/** Generate buffer
 *
 * @param gl     GL functions
 * @param out_id Id of buffer
 **/
void Buffer::Generate(const Functions &gl, GLuint &out_id)
{
    GLuint id = m_invalid_id;

    gl.genBuffers(1, &id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");

    if (m_invalid_id == id)
    {
        TCU_FAIL("Got invalid id");
    }

    out_id = id;
}

/** Maps buffer content
 *
 * @param gl     GL functions
 * @param buffer Buffer enum
 * @param access Access rights for mapped region
 *
 * @return Mapped memory
 **/
void *Buffer::Map(const Functions &gl, BUFFERS buffer, ACCESS access)
{
    GLenum target    = GetBufferGLenum(buffer);
    GLenum gl_access = GetAccessGLenum(access);

    void *result = gl.mapBuffer(target, gl_access);
    GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

    return result;
}

/** Unmaps buffer
 *
 **/
void Buffer::UnMap(const Functions &gl, BUFFERS buffer)
{
    GLenum target = GetBufferGLenum(buffer);

    gl.unmapBuffer(target);
    GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");
}

/** Return GLenum representation of requested access
 *
 * @param access Requested access
 *
 * @return GLenum value
 **/
GLenum Buffer::GetAccessGLenum(ACCESS access)
{
    GLenum result = 0;

    switch (access)
    {
        case ReadOnly:
            result = GL_READ_ONLY;
            break;
        case WriteOnly:
            result = GL_WRITE_ONLY;
            break;
        case ReadWrite:
            result = GL_READ_WRITE;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Return GLenum representation of requested buffer type
 *
 * @param buffer Requested buffer type
 *
 * @return GLenum value
 **/
GLenum Buffer::GetBufferGLenum(BUFFERS buffer)
{
    GLenum result = 0;

    switch (buffer)
    {
        case Array:
            result = GL_ARRAY_BUFFER;
            break;
        case Element:
            result = GL_ELEMENT_ARRAY_BUFFER;
            break;
        case Shader_Storage:
            result = GL_SHADER_STORAGE_BUFFER;
            break;
        case Texture:
            result = GL_TEXTURE_BUFFER;
            break;
        case Transform_feedback:
            result = GL_TRANSFORM_FEEDBACK_BUFFER;
            break;
        case Uniform:
            result = GL_UNIFORM_BUFFER;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Return GLenum representation of requested usage
 *
 * @param usage Requested usage
 *
 * @return GLenum value
 **/
GLenum Buffer::GetUsageGLenum(USAGE usage)
{
    GLenum result = 0;

    switch (usage)
    {
        case DynamicCopy:
            result = GL_DYNAMIC_COPY;
            break;
        case DynamicDraw:
            result = GL_DYNAMIC_DRAW;
            break;
        case DynamicRead:
            result = GL_DYNAMIC_READ;
            break;
        case StaticCopy:
            result = GL_STATIC_COPY;
            break;
        case StaticDraw:
            result = GL_STATIC_DRAW;
            break;
        case StaticRead:
            result = GL_STATIC_READ;
            break;
        case StreamCopy:
            result = GL_STREAM_COPY;
            break;
        case StreamDraw:
            result = GL_STREAM_DRAW;
            break;
        case StreamRead:
            result = GL_STREAM_READ;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Returns name of buffer target
 *
 * @param buffer Target enum
 *
 * @return Name of target
 **/
const GLchar *Buffer::GetBufferName(BUFFERS buffer)
{
    const GLchar *name = 0;

    switch (buffer)
    {
        case Array:
            name = "Array";
            break;
        case Element:
            name = "Element";
            break;
        case Shader_Storage:
            name = "Shader_Storage";
            break;
        case Texture:
            name = "Texture";
            break;
        case Transform_feedback:
            name = "Transform_feedback";
            break;
        case Uniform:
            name = "Uniform";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return name;
}

/* Framebuffer constants */
const GLuint Framebuffer::m_invalid_id = -1;

/** Constructor
 *
 * @param context CTS context
 **/
Framebuffer::Framebuffer(deqp::Context &context) : m_id(m_invalid_id), m_context(context)
{
    /* Nothing to be done here */
}

/** Destructor
 *
 **/
Framebuffer::~Framebuffer()
{
    Release();
}

/** Initialize framebuffer instance
 *
 **/
void Framebuffer::Init()
{
    /* Delete previous instance */
    Release();

    const Functions &gl = m_context.getRenderContext().getFunctions();

    Generate(gl, m_id);
}

/** Release framebuffer instance
 *
 **/
void Framebuffer::Release()
{
    if (m_invalid_id != m_id)
    {
        const Functions &gl = m_context.getRenderContext().getFunctions();

        gl.deleteFramebuffers(1, &m_id);
        m_id = m_invalid_id;
    }
}

/** Attach texture to specified attachment
 *
 * @param attachment Attachment
 * @param texture_id Texture id
 * @param width      Texture width
 * @param height     Texture height
 **/
void Framebuffer::AttachTexture(GLenum attachment, GLuint texture_id, GLuint width, GLuint height)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    AttachTexture(gl, attachment, texture_id, width, height);
}

/** Binds framebuffer to DRAW_FRAMEBUFFER
 *
 **/
void Framebuffer::Bind()
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    Bind(gl, m_id);
}

/** Clear framebuffer
 *
 * @param mask <mask> parameter of glClear. Decides which shall be cleared
 **/
void Framebuffer::Clear(GLenum mask)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    Clear(gl, mask);
}

/** Specifies clear color
 *
 * @param red   Red channel
 * @param green Green channel
 * @param blue  Blue channel
 * @param alpha Alpha channel
 **/
void Framebuffer::ClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    ClearColor(gl, red, green, blue, alpha);
}

/** Attach texture to specified attachment
 *
 * @param gl         GL functions
 * @param attachment Attachment
 * @param texture_id Texture id
 * @param width      Texture width
 * @param height     Texture height
 **/
void Framebuffer::AttachTexture(const Functions &gl,
                                GLenum attachment,
                                GLuint texture_id,
                                GLuint width,
                                GLuint height)
{
    gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, attachment, texture_id, 0 /* level */);
    GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture");

    gl.viewport(0 /* x */, 0 /* y */, width, height);
    GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");
}

/** Binds framebuffer to DRAW_FRAMEBUFFER
 *
 * @param gl GL functions
 * @param id ID of framebuffer
 **/
void Framebuffer::Bind(const Functions &gl, GLuint id)
{
    gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");
}

/** Clear framebuffer
 *
 * @param gl   GL functions
 * @param mask <mask> parameter of glClear. Decides which shall be cleared
 **/
void Framebuffer::Clear(const Functions &gl, GLenum mask)
{
    gl.clear(mask);
    GLU_EXPECT_NO_ERROR(gl.getError(), "Clear");
}

/** Specifies clear color
 *
 * @param gl    GL functions
 * @param red   Red channel
 * @param green Green channel
 * @param blue  Blue channel
 * @param alpha Alpha channel
 **/
void Framebuffer::ClearColor(const Functions &gl,
                             GLfloat red,
                             GLfloat green,
                             GLfloat blue,
                             GLfloat alpha)
{
    gl.clearColor(red, green, blue, alpha);
    GLU_EXPECT_NO_ERROR(gl.getError(), "ClearColor");
}

/** Generate framebuffer
 *
 **/
void Framebuffer::Generate(const Functions &gl, GLuint &out_id)
{
    GLuint id = m_invalid_id;

    gl.genFramebuffers(1, &id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GenFramebuffers");

    if (m_invalid_id == id)
    {
        TCU_FAIL("Invalid id");
    }

    out_id = id;
}

/* Shader's constants */
const GLuint Shader::m_invalid_id = 0;

/** Constructor.
 *
 * @param context CTS context.
 **/
Shader::Shader(deqp::Context &context) : m_id(m_invalid_id), m_context(context)
{
    /* Nothing to be done here */
}

/** Destructor
 *
 **/
Shader::~Shader()
{
    Release();
}

/** Initialize shader instance
 *
 * @param stage  Shader stage
 * @param source Source code
 **/
void Shader::Init(STAGES stage, const std::string &source)
{
    if (true == source.empty())
    {
        /* No source == no shader */
        return;
    }

    /* Delete any previous shader */
    Release();

    /* Create, set source and compile */
    const Functions &gl = m_context.getRenderContext().getFunctions();

    Create(gl, stage, m_id);
    Source(gl, m_id, source);

    try
    {
        Compile(gl, m_id);
    }
    catch (const CompilationException &exc)
    {
        throw InvalidSourceException(exc.what(), source, stage);
    }
}

/** Release shader instance
 *
 **/
void Shader::Release()
{
    if (m_invalid_id != m_id)
    {
        const Functions &gl = m_context.getRenderContext().getFunctions();

        gl.deleteShader(m_id);
        m_id = m_invalid_id;
    }
}

/** Compile shader
 *
 * @param gl GL functions
 * @param id Shader id
 **/
void Shader::Compile(const Functions &gl, GLuint id)
{
    GLint status = GL_FALSE;

    /* Compile */
    gl.compileShader(id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "CompileShader");

    /* Get compilation status */
    gl.getShaderiv(id, GL_COMPILE_STATUS, &status);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

    /* Log compilation error */
    if (GL_TRUE != status)
    {
        glw::GLint length = 0;
        std::string message;

        /* Error log length */
        gl.getShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

        /* Prepare storage */
        message.resize(length, 0);

        /* Get error log */
        gl.getShaderInfoLog(id, length, 0, &message[0]);
        GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderInfoLog");

        throw CompilationException(message.c_str());
    }
}

/** Create shader
 *
 * @param gl     GL functions
 * @param stage  Shader stage
 * @param out_id Shader id
 **/
void Shader::Create(const Functions &gl, STAGES stage, GLuint &out_id)
{
    const GLenum shaderType = GetShaderStageGLenum(stage);
    const GLuint id         = gl.createShader(shaderType);
    GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

    if (m_invalid_id == id)
    {
        TCU_FAIL("Failed to create shader");
    }

    out_id = id;
}

/** Set shader's source code
 *
 * @param gl     GL functions
 * @param id     Shader id
 * @param source Shader source code
 **/
void Shader::Source(const Functions &gl, GLuint id, const std::string &source)
{
    const GLchar *code = source.c_str();

    gl.shaderSource(id, 1 /* count */, &code, 0 /* lengths */);
    GLU_EXPECT_NO_ERROR(gl.getError(), "ShaderSource");
}

/** Get GLenum repesenting shader stage
 *
 * @param stage Shader stage
 *
 * @return GLenum
 **/
GLenum Shader::GetShaderStageGLenum(STAGES stage)
{
    GLenum result = 0;

    switch (stage)
    {
        case COMPUTE:
            result = GL_COMPUTE_SHADER;
            break;
        case FRAGMENT:
            result = GL_FRAGMENT_SHADER;
            break;
        case GEOMETRY:
            result = GL_GEOMETRY_SHADER;
            break;
        case TESS_CTRL:
            result = GL_TESS_CONTROL_SHADER;
            break;
        case TESS_EVAL:
            result = GL_TESS_EVALUATION_SHADER;
            break;
        case VERTEX:
            result = GL_VERTEX_SHADER;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Get string representing name of shader stage
 *
 * @param stage Shader stage
 *
 * @return String with name of shader stage
 **/
const glw::GLchar *Shader::GetStageName(STAGES stage)
{
    const GLchar *result = 0;

    switch (stage)
    {
        case COMPUTE:
            result = "compute";
            break;
        case VERTEX:
            result = "vertex";
            break;
        case TESS_CTRL:
            result = "tessellation_control";
            break;
        case TESS_EVAL:
            result = "tessellation_evaluation";
            break;
        case GEOMETRY:
            result = "geometry";
            break;
        case FRAGMENT:
            result = "fragment";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Logs shader source
 *
 * @param context CTS context
 * @param source  Source of shader
 * @param stage   Shader stage
 **/
void Shader::LogSource(deqp::Context &context, const std::string &source, STAGES stage)
{
    /* Skip empty shaders */
    if (true == source.empty())
    {
        return;
    }

    context.getTestContext().getLog()
        << tcu::TestLog::Message << "Shader source. Stage: " << Shader::GetStageName(stage)
        << tcu::TestLog::EndMessage << tcu::TestLog::KernelSource(source);
}

/** Constructor
 *
 * @param message Compilation error message
 **/
Shader::CompilationException::CompilationException(const GLchar *message)
{
    m_message = message;
}

/** Returns error messages
 *
 * @return Compilation error message
 **/
const char *Shader::CompilationException::what() const throw()
{
    return m_message.c_str();
}

/** Constructor
 *
 * @param message Compilation error message
 **/
Shader::InvalidSourceException::InvalidSourceException(const GLchar *error_message,
                                                       const std::string &source,
                                                       STAGES stage)
    : m_message(error_message), m_source(source), m_stage(stage)
{}

/** Returns error messages
 *
 * @return Compilation error message
 **/
const char *Shader::InvalidSourceException::what() const throw()
{
    return "Compilation error";
}

/** Logs error message and shader sources **/
void Shader::InvalidSourceException::log(deqp::Context &context) const
{
    context.getTestContext().getLog()
        << tcu::TestLog::Message << "Failed to compile shader: " << m_message.c_str()
        << tcu::TestLog::EndMessage;

    LogSource(context, m_source, m_stage);
}

/* Program constants */
const GLuint Pipeline::m_invalid_id = 0;

/** Constructor.
 *
 * @param context CTS context.
 **/
Pipeline::Pipeline(deqp::Context &context) : m_id(m_invalid_id), m_context(context)
{
    /* Nothing to be done here */
}

/** Destructor
 *
 **/
Pipeline::~Pipeline()
{
    Release();
}

/** Initialize pipline object
 *
 **/
void Pipeline::Init()
{
    Release();

    const Functions &gl = m_context.getRenderContext().getFunctions();

    /* Generate */
    gl.genProgramPipelines(1, &m_id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GenProgramPipelines");
}

/** Release pipeline object
 *
 **/
void Pipeline::Release()
{
    if (m_invalid_id != m_id)
    {
        const Functions &gl = m_context.getRenderContext().getFunctions();

        /* Generate */
        gl.deleteProgramPipelines(1, &m_id);
        GLU_EXPECT_NO_ERROR(gl.getError(), "DeleteProgramPipelines");

        m_id = m_invalid_id;
    }
}

/** Bind pipeline
 *
 **/
void Pipeline::Bind()
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    Bind(gl, m_id);
}

/** Set which stages should be active
 *
 * @param program_id Id of program
 * @param stages     Logical combination of enums representing stages
 **/
void Pipeline::UseProgramStages(GLuint program_id, GLenum stages)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    UseProgramStages(gl, m_id, program_id, stages);
}

/** Bind pipeline
 *
 * @param gl Functiions
 * @param id Pipeline id
 **/
void Pipeline::Bind(const Functions &gl, GLuint id)
{
    gl.bindProgramPipeline(id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BindProgramPipeline");
}

/** Set which stages should be active
 *
 * @param gl         Functiions
 * @param id         Pipeline id
 * @param program_id Id of program
 * @param stages     Logical combination of enums representing stages
 **/
void Pipeline::UseProgramStages(const Functions &gl, GLuint id, GLuint program_id, GLenum stages)
{
    gl.useProgramStages(id, stages, program_id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgramStages");
}

/* Program constants */
const GLuint Program::m_invalid_id = 0;

/** Constructor.
 *
 * @param context CTS context.
 **/
Program::Program(deqp::Context &context)
    : m_id(m_invalid_id),
      m_compute(context),
      m_fragment(context),
      m_geometry(context),
      m_tess_ctrl(context),
      m_tess_eval(context),
      m_vertex(context),
      m_context(context)
{
    /* Nothing to be done here */
}

/** Destructor
 *
 **/
Program::~Program()
{
    Release();
}

/** Initialize program instance
 *
 * @param compute_shader                    Compute shader source code
 * @param fragment_shader                   Fragment shader source code
 * @param geometry_shader                   Geometry shader source code
 * @param tessellation_control_shader       Tessellation control shader source code
 * @param tessellation_evaluation_shader    Tessellation evaluation shader source code
 * @param vertex_shader                     Vertex shader source code
 * @param captured_varyings                 Vector of variables to be captured with transfrom
 *feedback
 * @param capture_interleaved               Select mode of transform feedback (separate or
 *interleaved)
 * @param is_separable                      Selects if monolithic or separable program should be
 *built. Defaults to false
 **/
void Program::Init(const std::string &compute_shader,
                   const std::string &fragment_shader,
                   const std::string &geometry_shader,
                   const std::string &tessellation_control_shader,
                   const std::string &tessellation_evaluation_shader,
                   const std::string &vertex_shader,
                   const NameVector &captured_varyings,
                   bool capture_interleaved,
                   bool is_separable)
{
    /* Delete previous program */
    Release();

    /* GL entry points */
    const Functions &gl = m_context.getRenderContext().getFunctions();

    /* Initialize shaders */
    m_compute.Init(Shader::COMPUTE, compute_shader);
    m_fragment.Init(Shader::FRAGMENT, fragment_shader);
    m_geometry.Init(Shader::GEOMETRY, geometry_shader);
    m_tess_ctrl.Init(Shader::TESS_CTRL, tessellation_control_shader);
    m_tess_eval.Init(Shader::TESS_EVAL, tessellation_evaluation_shader);
    m_vertex.Init(Shader::VERTEX, vertex_shader);

    /* Create program, set up transform feedback and attach shaders */
    Create(gl, m_id);
    Capture(gl, m_id, captured_varyings, capture_interleaved);
    Attach(gl, m_id, m_compute.m_id);
    Attach(gl, m_id, m_fragment.m_id);
    Attach(gl, m_id, m_geometry.m_id);
    Attach(gl, m_id, m_tess_ctrl.m_id);
    Attach(gl, m_id, m_tess_eval.m_id);
    Attach(gl, m_id, m_vertex.m_id);

    /* Set separable parameter */
    if (true == is_separable)
    {
        gl.programParameteri(m_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
        GLU_EXPECT_NO_ERROR(gl.getError(), "ProgramParameteri");
    }

    try
    {
        /* Link program */
        Link(gl, m_id);
    }
    catch (const LinkageException &exc)
    {
        throw BuildException(exc.what(), compute_shader, fragment_shader, geometry_shader,
                             tessellation_control_shader, tessellation_evaluation_shader,
                             vertex_shader);
    }
}

/** Initialize program instance
 *
 * @param compute_shader                    Compute shader source code
 * @param fragment_shader                   Fragment shader source code
 * @param geometry_shader                   Geometry shader source code
 * @param tessellation_control_shader       Tessellation control shader source code
 * @param tessellation_evaluation_shader    Tessellation evaluation shader source code
 * @param vertex_shader                     Vertex shader source code
 * @param is_separable                      Selects if monolithic or separable program should be
 *built. Defaults to false
 **/
void Program::Init(const std::string &compute_shader,
                   const std::string &fragment_shader,
                   const std::string &geometry_shader,
                   const std::string &tessellation_control_shader,
                   const std::string &tessellation_evaluation_shader,
                   const std::string &vertex_shader,
                   bool is_separable)
{
    NameVector captured_varying;

    Init(compute_shader, fragment_shader, geometry_shader, tessellation_control_shader,
         tessellation_evaluation_shader, vertex_shader, captured_varying, true, is_separable);
}

/** Release program instance
 *
 **/
void Program::Release()
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    if (m_invalid_id != m_id)
    {
        Use(gl, m_invalid_id);

        gl.deleteProgram(m_id);
        m_id = m_invalid_id;
    }

    m_compute.Release();
    m_fragment.Release();
    m_geometry.Release();
    m_tess_ctrl.Release();
    m_tess_eval.Release();
    m_vertex.Release();
}

/** Get <pname> for a set of active uniforms
 *
 * @param count   Number of indices
 * @param indices Indices of uniforms
 * @param pname   Queired pname
 * @param params  Array that will be filled with values of parameters
 **/
void Program::GetActiveUniformsiv(GLsizei count,
                                  const GLuint *indices,
                                  GLenum pname,
                                  GLint *params) const
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    GetActiveUniformsiv(gl, m_id, count, indices, pname, params);
}

/** Get location of attribute
 *
 * @param name Name of attribute
 *
 * @return Result of query
 **/
glw::GLint Program::GetAttribLocation(const std::string &name) const
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    return GetAttribLocation(gl, m_id, name);
}

/** Query resource
 *
 * @param interface Interface to be queried
 * @param index     Index of resource
 * @param property  Property to be queried
 * @param buf_size  Size of <params> buffer
 * @param params    Results of query
 **/
void Program::GetResource(GLenum interface,
                          GLuint index,
                          GLenum property,
                          GLsizei buf_size,
                          GLint *params) const
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    GetResource(gl, m_id, interface, index, property, buf_size, params);
}

/** Query for index of resource
 *
 * @param name      Name of resource
 * @param interface Interface to be queried
 *
 * @return Result of query
 **/
glw::GLuint Program::GetResourceIndex(const std::string &name, GLenum interface) const
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    return GetResourceIndex(gl, m_id, name, interface);
}

/** Get indices for a set of uniforms
 *
 * @param count   Count number of uniforms
 * @param names   Names of uniforms
 * @param indices Buffer that will be filled with indices
 **/
void Program::GetUniformIndices(GLsizei count, const GLchar **names, GLuint *indices) const
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    GetUniformIndices(gl, m_id, count, names, indices);
}

/** Get uniform location
 *
 * @param name Name of uniform
 *
 * @return Results of query
 **/
glw::GLint Program::GetUniformLocation(const std::string &name) const
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    return GetUniformLocation(gl, m_id, name);
}

/** Set program as active
 *
 **/
void Program::Use() const
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    Use(gl, m_id);
}

/** Attach shader to program
 *
 * @param gl         GL functions
 * @param program_id Id of program
 * @param shader_id  Id of shader
 **/
void Program::Attach(const Functions &gl, GLuint program_id, GLuint shader_id)
{
    /* Quick checks */
    if ((m_invalid_id == program_id) || (Shader::m_invalid_id == shader_id))
    {
        return;
    }

    gl.attachShader(program_id, shader_id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
}

/** Set up captured varyings
 *
 * @param gl                  GL functions
 * @param id                  Id of program
 * @param captured_varyings   Vector of varyings
 * @param capture_interleaved Selects if interleaved or separate mode should be used
 **/
void Program::Capture(const Functions &gl,
                      GLuint id,
                      const NameVector &captured_varyings,
                      bool capture_interleaved)
{
    const size_t n_varyings = captured_varyings.size();

    if (0 == n_varyings)
    {
        /* empty list, skip */
        return;
    }

    std::vector<const GLchar *> varying_names;
    varying_names.resize(n_varyings);

    for (size_t i = 0; i < n_varyings; ++i)
    {
        varying_names[i] = captured_varyings[i].c_str();
    }

    GLenum mode = 0;
    if (true == capture_interleaved)
    {
        mode = GL_INTERLEAVED_ATTRIBS;
    }
    else
    {
        mode = GL_SEPARATE_ATTRIBS;
    }

    gl.transformFeedbackVaryings(id, static_cast<GLsizei>(n_varyings), &varying_names[0], mode);
    GLU_EXPECT_NO_ERROR(gl.getError(), "TransformFeedbackVaryings");
}

/** Create program instance
 *
 * @param gl     GL functions
 * @param out_id Id of program
 **/
void Program::Create(const Functions &gl, GLuint &out_id)
{
    const GLuint id = gl.createProgram();
    GLU_EXPECT_NO_ERROR(gl.getError(), "CreateProgram");

    if (m_invalid_id == id)
    {
        TCU_FAIL("Failed to create program");
    }

    out_id = id;
}

/** Get <pname> for a set of active uniforms
 *
 * @param gl         Functions
 * @param program_id Id of program
 * @param count      Number of indices
 * @param indices    Indices of uniforms
 * @param pname      Queired pname
 * @param params     Array that will be filled with values of parameters
 **/
void Program::GetActiveUniformsiv(const Functions &gl,
                                  GLuint program_id,
                                  GLsizei count,
                                  const GLuint *indices,
                                  GLenum pname,
                                  GLint *params)
{
    gl.getActiveUniformsiv(program_id, count, indices, pname, params);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetActiveUniformsiv");
}

/** Get indices for a set of uniforms
 *
 * @param gl         Functions
 * @param program_id Id of program
 * @param count      Count number of uniforms
 * @param names      Names of uniforms
 * @param indices    Buffer that will be filled with indices
 **/
void Program::GetUniformIndices(const Functions &gl,
                                GLuint program_id,
                                GLsizei count,
                                const GLchar **names,
                                GLuint *indices)
{
    gl.getUniformIndices(program_id, count, names, indices);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformIndices");
}

/** Link program
 *
 * @param gl GL functions
 * @param id Id of program
 **/
void Program::Link(const Functions &gl, GLuint id)
{
    GLint status = GL_FALSE;

    gl.linkProgram(id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "LinkProgram");

    /* Get link status */
    gl.getProgramiv(id, GL_LINK_STATUS, &status);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

    /* Log link error */
    if (GL_TRUE != status)
    {
        glw::GLint length = 0;
        std::string message;

        /* Get error log length */
        gl.getProgramiv(id, GL_INFO_LOG_LENGTH, &length);
        GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

        message.resize(length, 0);

        /* Get error log */
        gl.getProgramInfoLog(id, length, 0, &message[0]);
        GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramInfoLog");

        throw LinkageException(message.c_str());
    }
}

/** Set generic uniform
 *
 * @param gl       Functions
 * @param type     Type of uniform
 * @param count    Length of array
 * @param location Location of uniform
 * @param data     Data that will be used
 **/
void Program::Uniform(const Functions &gl,
                      const Type &type,
                      GLsizei count,
                      GLint location,
                      const GLvoid *data)
{
    if (-1 == location)
    {
        TCU_FAIL("Uniform is inactive");
    }

    switch (type.m_basic_type)
    {
        case Type::Double:
            if (1 == type.m_n_columns)
            {
                getUniformNdv(gl, type.m_n_rows)(location, count, (const GLdouble *)data);
                GLU_EXPECT_NO_ERROR(gl.getError(), "UniformNdv");
            }
            else
            {
                getUniformMatrixNdv(gl, type.m_n_columns, type.m_n_rows)(location, count, false,
                                                                         (const GLdouble *)data);
                GLU_EXPECT_NO_ERROR(gl.getError(), "UniformMatrixNdv");
            }
            break;
        case Type::Float:
            if (1 == type.m_n_columns)
            {
                getUniformNfv(gl, type.m_n_rows)(location, count, (const GLfloat *)data);
                GLU_EXPECT_NO_ERROR(gl.getError(), "UniformNfv");
            }
            else
            {
                getUniformMatrixNfv(gl, type.m_n_columns, type.m_n_rows)(location, count, false,
                                                                         (const GLfloat *)data);
                GLU_EXPECT_NO_ERROR(gl.getError(), "UniformMatrixNfv");
            }
            break;
        case Type::Int:
            getUniformNiv(gl, type.m_n_rows)(location, count, (const GLint *)data);
            GLU_EXPECT_NO_ERROR(gl.getError(), "UniformNiv");
            break;
        case Type::Uint:
            getUniformNuiv(gl, type.m_n_rows)(location, count, (const GLuint *)data);
            GLU_EXPECT_NO_ERROR(gl.getError(), "UniformNuiv");
            break;
        default:
            TCU_FAIL("Invalid enum");
    }
}

/** Use program
 *
 * @param gl GL functions
 * @param id Id of program
 **/
void Program::Use(const Functions &gl, GLuint id)
{
    gl.useProgram(id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");
}

/** Get location of attribute
 *
 * @param gl   GL functions
 * @param id   Id of program
 * @param name Name of attribute
 *
 * @return Location of attribute
 **/
GLint Program::GetAttribLocation(const Functions &gl, GLuint id, const std::string &name)
{
    GLint location = gl.getAttribLocation(id, name.c_str());
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetAttribLocation");

    return location;
}

/** Query resource
 *
 * @param gl        GL functions
 * @param id        Id of program
 * @param interface Interface to be queried
 * @param index     Index of resource
 * @param property  Property to be queried
 * @param buf_size  Size of <params> buffer
 * @param params    Results of query
 **/
void Program::GetResource(const Functions &gl,
                          GLuint id,
                          GLenum interface,
                          GLuint index,
                          GLenum property,
                          GLsizei buf_size,
                          GLint *params)
{
    gl.getProgramResourceiv(id, interface, index, 1 /* propCount */, &property,
                            buf_size /* bufSize */, 0 /* length */, params);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramResourceiv");
}

/** Get index of resource
 *
 * @param gl        GL functions
 * @param id        Id of program
 * @param name      Name of resource
 * @param interface Program interface to queried
 *
 * @return Location of attribute
 **/
GLuint Program::GetResourceIndex(const Functions &gl,
                                 GLuint id,
                                 const std::string &name,
                                 GLenum interface)
{
    GLuint index = gl.getProgramResourceIndex(id, interface, name.c_str());
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramResourceIndex");

    return index;
}

/** Get location of attribute
 *
 * @param gl   GL functions
 * @param id   Id of program
 * @param name Name of attribute
 *
 * @return Location of uniform
 **/
GLint Program::GetUniformLocation(const Functions &gl, GLuint id, const std::string &name)
{
    GLint location = gl.getUniformLocation(id, name.c_str());
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformLocation");

    return location;
}

/** Constructor
 *
 * @param error_message    Error message
 * @param compute_shader   Source code for compute stage
 * @param fragment_shader  Source code for fragment stage
 * @param geometry_shader  Source code for geometry stage
 * @param tess_ctrl_shader Source code for tessellation control stage
 * @param tess_eval_shader Source code for tessellation evaluation stage
 * @param vertex_shader    Source code for vertex stage
 **/
Program::BuildException::BuildException(const glw::GLchar *error_message,
                                        const std::string compute_shader,
                                        const std::string fragment_shader,
                                        const std::string geometry_shader,
                                        const std::string tess_ctrl_shader,
                                        const std::string tess_eval_shader,
                                        const std::string vertex_shader)
    : m_error_message(error_message),
      m_compute_shader(compute_shader),
      m_fragment_shader(fragment_shader),
      m_geometry_shader(geometry_shader),
      m_tess_ctrl_shader(tess_ctrl_shader),
      m_tess_eval_shader(tess_eval_shader),
      m_vertex_shader(vertex_shader)
{}

/** Overwrites std::exception::what method
 *
 * @return Message compossed from error message and shader sources
 **/
const char *Program::BuildException::what() const throw()
{
    return "Failed to link program";
}

/** Logs error message and shader sources **/
void Program::BuildException::log(deqp::Context &context) const
{
    context.getTestContext().getLog()
        << tcu::TestLog::Message << "Link failure: " << m_error_message << tcu::TestLog::EndMessage;

    Shader::LogSource(context, m_vertex_shader, Shader::VERTEX);
    Shader::LogSource(context, m_tess_ctrl_shader, Shader::TESS_CTRL);
    Shader::LogSource(context, m_tess_eval_shader, Shader::TESS_EVAL);
    Shader::LogSource(context, m_geometry_shader, Shader::GEOMETRY);
    Shader::LogSource(context, m_fragment_shader, Shader::FRAGMENT);
    Shader::LogSource(context, m_compute_shader, Shader::COMPUTE);
}

/** Constructor
 *
 * @param message Linking error message
 **/
Program::LinkageException::LinkageException(const glw::GLchar *message) : m_error_message(message)
{
    /* Nothing to be done */
}

/** Returns error messages
 *
 * @return Linking error message
 **/
const char *Program::LinkageException::what() const throw()
{
    return m_error_message.c_str();
}

/* Texture constants */
const GLuint Texture::m_invalid_id = -1;

/** Constructor.
 *
 * @param context CTS context.
 **/
Texture::Texture(deqp::Context &context) : m_id(m_invalid_id), m_context(context), m_type(TEX_2D)
{
    /* Nothing to done here */
}

/** Destructor
 *
 **/
Texture::~Texture()
{
    Release();
}

/** Initialize texture instance
 *
 * @param tex_type        Type of texture
 * @param width           Width of texture
 * @param height          Height of texture
 * @param depth           Depth of texture
 * @param internal_format Internal format of texture
 * @param format          Format of texture data
 * @param type            Type of texture data
 * @param data            Texture data
 **/
void Texture::Init(TYPES tex_type,
                   GLuint width,
                   GLuint height,
                   GLuint depth,
                   GLenum internal_format,
                   GLenum format,
                   GLenum type,
                   GLvoid *data)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    /* Delete previous texture */
    Release();

    m_type = tex_type;

    /* Generate, bind, allocate storage and upload data */
    Generate(gl, m_id);
    Bind(gl, m_id, tex_type);
    Storage(gl, tex_type, width, height, depth, internal_format);
    Update(gl, tex_type, width, height, depth, format, type, data);
}

/** Initialize buffer texture
 *
 * @param internal_format Internal format of texture
 * @param buffer_id       Id of buffer that will be used as data source
 **/
void Texture::Init(GLenum internal_format, GLuint buffer_id)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    /* Delete previous texture */
    Release();

    m_type = TEX_BUFFER;

    /* Generate, bind and attach buffer */
    Generate(gl, m_id);
    Bind(gl, m_id, TEX_BUFFER);
    TexBuffer(gl, buffer_id, internal_format);
}

/** Release texture instance
 *
 **/
void Texture::Release()
{
    if (m_invalid_id != m_id)
    {
        const Functions &gl = m_context.getRenderContext().getFunctions();

        gl.deleteTextures(1, &m_id);
        m_id = m_invalid_id;
    }
}

/** Bind texture to its target
 *
 **/
void Texture::Bind() const
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    Bind(gl, m_id, m_type);
}

/** Get texture data
 *
 * @param format   Format of data
 * @param type     Type of data
 * @param out_data Buffer for data
 **/
void Texture::Get(GLenum format, GLenum type, GLvoid *out_data) const
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    Bind(gl, m_id, m_type);
    Get(gl, m_type, format, type, out_data);
}

/** Bind texture to target
 *
 * @param gl       GL functions
 * @param id       Id of texture
 * @param tex_type Type of texture
 **/
void Texture::Bind(const Functions &gl, GLuint id, TYPES tex_type)
{
    GLenum target = GetTargetGLenum(tex_type);

    gl.bindTexture(target, id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
}

/** Generate texture instance
 *
 * @param gl     GL functions
 * @param out_id Id of texture
 **/
void Texture::Generate(const Functions &gl, GLuint &out_id)
{
    GLuint id = m_invalid_id;

    gl.genTextures(1, &id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

    if (m_invalid_id == id)
    {
        TCU_FAIL("Invalid id");
    }

    out_id = id;
}

/** Get texture data
 *
 * @param gl       GL functions
 * @param format   Format of data
 * @param type     Type of data
 * @param out_data Buffer for data
 **/
void Texture::Get(const Functions &gl, TYPES tex_type, GLenum format, GLenum type, GLvoid *out_data)
{
    GLenum target = GetTargetGLenum(tex_type);

    if (TEX_CUBE != tex_type)
    {
        gl.getTexImage(target, 0 /* level */, format, type, out_data);
        GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");
    }
    else
    {
        GLint width;
        GLint height;

        if ((GL_RGBA != format) && (GL_UNSIGNED_BYTE != type))
        {
            TCU_FAIL("Not implemented");
        }

        GLuint texel_size = 4;

        gl.getTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0 /* level */, GL_TEXTURE_WIDTH,
                                  &width);
        GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexLevelParameteriv");

        gl.getTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0 /* level */, GL_TEXTURE_HEIGHT,
                                  &height);
        GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexLevelParameteriv");

        const GLuint image_size = width * height * texel_size;

        gl.getTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0 /* level */, format, type,
                       (GLvoid *)((GLchar *)out_data + (image_size * 0)));
        gl.getTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0 /* level */, format, type,
                       (GLvoid *)((GLchar *)out_data + (image_size * 1)));
        gl.getTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0 /* level */, format, type,
                       (GLvoid *)((GLchar *)out_data + (image_size * 2)));
        gl.getTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0 /* level */, format, type,
                       (GLvoid *)((GLchar *)out_data + (image_size * 3)));
        gl.getTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0 /* level */, format, type,
                       (GLvoid *)((GLchar *)out_data + (image_size * 4)));
        gl.getTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0 /* level */, format, type,
                       (GLvoid *)((GLchar *)out_data + (image_size * 5)));
        GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");
    }
}

/** Allocate storage for texture
 *
 * @param gl              GL functions
 * @param tex_type        Type of texture
 * @param width           Width of texture
 * @param height          Height of texture
 * @param depth           Depth of texture
 * @param internal_format Internal format of texture
 **/
void Texture::Storage(const Functions &gl,
                      TYPES tex_type,
                      GLuint width,
                      GLuint height,
                      GLuint depth,
                      GLenum internal_format)
{
    static const GLuint levels = 1;

    GLenum target = GetTargetGLenum(tex_type);

    switch (tex_type)
    {
        case TEX_1D:
            gl.texStorage1D(target, levels, internal_format, width);
            GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage1D");
            break;
        case TEX_2D:
        case TEX_1D_ARRAY:
        case TEX_2D_RECT:
        case TEX_CUBE:
            gl.texStorage2D(target, levels, internal_format, width, height);
            GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");
            break;
        case TEX_3D:
        case TEX_2D_ARRAY:
            gl.texStorage3D(target, levels, internal_format, width, height, depth);
            GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage3D");
            break;
        default:
            TCU_FAIL("Invalid enum");
    }
}

/** Attach buffer as source of texture buffer data
 *
 * @param gl              GL functions
 * @param internal_format Internal format of texture
 * @param buffer_id       Id of buffer that will be used as data source
 **/
void Texture::TexBuffer(const Functions &gl, GLenum internal_format, GLuint &buffer_id)
{
    gl.texBuffer(GL_TEXTURE_BUFFER, internal_format, buffer_id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "TexBuffer");
}

/** Update contents of texture
 *
 * @param gl       GL functions
 * @param tex_type Type of texture
 * @param width    Width of texture
 * @param height   Height of texture
 * @param format   Format of data
 * @param type     Type of data
 * @param data     Buffer with image data
 **/
void Texture::Update(const Functions &gl,
                     TYPES tex_type,
                     GLuint width,
                     GLuint height,
                     GLuint depth,
                     GLenum format,
                     GLenum type,
                     GLvoid *data)
{
    static const GLuint level = 0;

    GLenum target = GetTargetGLenum(tex_type);

    switch (tex_type)
    {
        case TEX_1D:
            gl.texSubImage1D(target, level, 0 /* x */, width, format, type, data);
            GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage1D");
            break;
        case TEX_2D:
        case TEX_1D_ARRAY:
        case TEX_2D_RECT:
            gl.texSubImage2D(target, level, 0 /* x */, 0 /* y */, width, height, format, type,
                             data);
            GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");
            break;
        case TEX_CUBE:
            gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level, 0 /* x */, 0 /* y */, width,
                             height, format, type, data);
            gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, level, 0 /* x */, 0 /* y */, width,
                             height, format, type, data);
            gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, level, 0 /* x */, 0 /* y */, width,
                             height, format, type, data);
            gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, level, 0 /* x */, 0 /* y */, width,
                             height, format, type, data);
            gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, level, 0 /* x */, 0 /* y */, width,
                             height, format, type, data);
            gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, level, 0 /* x */, 0 /* y */, width,
                             height, format, type, data);
            GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");
            break;
        case TEX_3D:
        case TEX_2D_ARRAY:
            gl.texSubImage3D(target, level, 0 /* x */, 0 /* y */, 0 /* z */, width, height, depth,
                             format, type, data);
            GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage3D");
            break;
        default:
            TCU_FAIL("Invalid enum");
    }
}

/** Get target for given texture type
 *
 * @param type Type of texture
 *
 * @return Target
 **/
GLenum Texture::GetTargetGLenum(TYPES type)
{
    GLenum result = 0;

    switch (type)
    {
        case TEX_BUFFER:
            result = GL_TEXTURE_BUFFER;
            break;
        case TEX_2D:
            result = GL_TEXTURE_2D;
            break;
        case TEX_2D_RECT:
            result = GL_TEXTURE_RECTANGLE;
            break;
        case TEX_2D_ARRAY:
            result = GL_TEXTURE_2D_ARRAY;
            break;
        case TEX_3D:
            result = GL_TEXTURE_3D;
            break;
        case TEX_CUBE:
            result = GL_TEXTURE_CUBE_MAP;
            break;
        case TEX_1D:
            result = GL_TEXTURE_1D;
            break;
        case TEX_1D_ARRAY:
            result = GL_TEXTURE_1D_ARRAY;
            break;
    }

    return result;
}

/* VertexArray constants */
const GLuint VertexArray::m_invalid_id = -1;

/** Constructor.
 *
 * @param context CTS context.
 **/
VertexArray::VertexArray(deqp::Context &context) : m_id(m_invalid_id), m_context(context) {}

/** Destructor
 *
 **/
VertexArray::~VertexArray()
{
    Release();
}

/** Initialize vertex array instance
 *
 **/
void VertexArray::Init()
{
    /* Delete previous instance */
    Release();

    const Functions &gl = m_context.getRenderContext().getFunctions();

    Generate(gl, m_id);
}

/** Release vertex array object instance
 *
 **/
void VertexArray::Release()
{
    if (m_invalid_id != m_id)
    {
        const Functions &gl = m_context.getRenderContext().getFunctions();

        gl.deleteVertexArrays(1, &m_id);

        m_id = m_invalid_id;
    }
}

/** Set attribute in VAO
 *
 * @param index            Index of attribute
 * @param type             Type of attribute
 * @param n_array_elements Arary length
 * @param normalized       Selects if values should be normalized
 * @param stride           Stride
 * @param pointer          Pointer to data, or offset in buffer
 **/
void VertexArray::Attribute(GLuint index,
                            const Type &type,
                            GLuint n_array_elements,
                            GLboolean normalized,
                            GLsizei stride,
                            const GLvoid *pointer)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    AttribPointer(gl, index, type, n_array_elements, normalized, stride, pointer);
    Enable(gl, index, type, n_array_elements);
}

/** Binds Vertex array object
 *
 **/
void VertexArray::Bind()
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    Bind(gl, m_id);
}

/** Set attribute in VAO
 *
 * @param gl               Functions
 * @param index            Index of attribute
 * @param type             Type of attribute
 * @param n_array_elements Arary length
 * @param normalized       Selects if values should be normalized
 * @param stride           Stride
 * @param pointer          Pointer to data, or offset in buffer
 **/
void VertexArray::AttribPointer(const Functions &gl,
                                GLuint index,
                                const Type &type,
                                GLuint n_array_elements,
                                GLboolean normalized,
                                GLsizei stride,
                                const GLvoid *pointer)
{
    const GLuint basic_type_size = Type::GetTypeSize(type.m_basic_type);
    const GLint size             = (GLint)type.m_n_rows;
    const GLuint column_size     = (GLuint)size * basic_type_size;
    const GLenum gl_type         = Type::GetTypeGLenum(type.m_basic_type);

    GLuint offset = 0;

    /* If attribute is not an array */
    if (0 == n_array_elements)
    {
        n_array_elements = 1;
    }

    /* For each element in array */
    for (GLuint element = 0; element < n_array_elements; ++element)
    {
        /* For each column in matrix */
        for (GLuint column = 1; column <= type.m_n_columns; ++column)
        {
            /* Calculate offset */
            const GLvoid *ptr = (GLubyte *)pointer + offset;

            /* Set up attribute */
            switch (type.m_basic_type)
            {
                case Type::Float:
                    gl.vertexAttribPointer(index, size, gl_type, normalized, stride, ptr);
                    GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribPointer");
                    break;
                case Type::Int:
                case Type::Uint:
                    gl.vertexAttribIPointer(index, size, gl_type, stride, ptr);
                    GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribIPointer");
                    break;
                case Type::Double:
                    gl.vertexAttribLPointer(index, size, gl_type, stride, ptr);
                    GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribLPointer");
                    break;
                default:
                    TCU_FAIL("Invalid enum");
            }

            /* Next location */
            offset += column_size;
            index += 1;
        }
    }
}

/** Binds Vertex array object
 *
 * @param gl GL functions
 * @param id ID of vertex array object
 **/
void VertexArray::Bind(const glw::Functions &gl, glw::GLuint id)
{
    gl.bindVertexArray(id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArray");
}

/** Disable attribute in VAO
 *
 * @param gl               Functions
 * @param index            Index of attribute
 * @param type             Type of attribute
 * @param n_array_elements Arary length
 **/
void VertexArray::Disable(const Functions &gl,
                          GLuint index,
                          const Type &type,
                          GLuint n_array_elements)
{
    /* If attribute is not an array */
    if (0 == n_array_elements)
    {
        n_array_elements = 1;
    }

    /* For each element in array */
    for (GLuint element = 0; element < n_array_elements; ++element)
    {
        /* For each column in matrix */
        for (GLuint column = 1; column <= type.m_n_columns; ++column)
        {
            /* Enable attribute array */
            gl.disableVertexAttribArray(index);
            GLU_EXPECT_NO_ERROR(gl.getError(), "DisableVertexAttribArray");

            /* Next location */
            index += 1;
        }
    }
}

/** Set divisor for attribute
 *
 * @param gl               Functions
 * @param index            Index of attribute
 * @param divisor          New divisor value
 **/
void VertexArray::Divisor(const Functions &gl, GLuint index, GLuint divisor)
{
    gl.vertexAttribDivisor(index, divisor);
    GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribDivisor");
}

/** Enables attribute in VAO
 *
 * @param gl               Functions
 * @param index            Index of attribute
 * @param type             Type of attribute
 * @param n_array_elements Arary length
 **/
void VertexArray::Enable(const Functions &gl,
                         GLuint index,
                         const Type &type,
                         GLuint n_array_elements)
{
    /* If attribute is not an array */
    if (0 == n_array_elements)
    {
        n_array_elements = 1;
    }

    /* For each element in array */
    for (GLuint element = 0; element < n_array_elements; ++element)
    {
        /* For each column in matrix */
        for (GLuint column = 1; column <= type.m_n_columns; ++column)
        {
            /* Enable attribute array */
            gl.enableVertexAttribArray(index);
            GLU_EXPECT_NO_ERROR(gl.getError(), "EnableVertexAttribArray");

            /* Next location */
            index += 1;
        }
    }
}

/** Generates Vertex array object
 *
 * @param gl     GL functions
 * @param out_id ID of vertex array object
 **/
void VertexArray::Generate(const glw::Functions &gl, glw::GLuint &out_id)
{
    GLuint id = m_invalid_id;

    gl.genVertexArrays(1, &id);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");

    if (m_invalid_id == id)
    {
        TCU_FAIL("Invalid id");
    }

    out_id = id;
}

/* Constatns used by Variable */
const GLint Variable::m_automatic_location = -1;

/** Copy constructor
 *
 **/
Variable::Variable(const Variable &var)
    : m_data(var.m_data),
      m_data_size(var.m_data_size),
      m_descriptor(var.m_descriptor.m_name.c_str(),
                   var.m_descriptor.m_qualifiers.c_str(),
                   var.m_descriptor.m_expected_component,
                   var.m_descriptor.m_expected_location,
                   var.m_descriptor.m_builtin,
                   var.m_descriptor.m_normalized,
                   var.m_descriptor.m_n_array_elements,
                   var.m_descriptor.m_expected_stride_of_element,
                   var.m_descriptor.m_offset),
      m_storage(var.m_storage)
{
    m_descriptor.m_type = var.m_descriptor.m_type;

    if (BUILTIN != var.m_descriptor.m_type)
    {
        m_descriptor.m_interface = var.m_descriptor.m_interface;
    }
}

/** Get code that defines variable
 *
 * @param flavour Provides info if variable is array or not
 *
 * @return String with code
 **/
std::string Variable::GetDefinition(FLAVOUR flavour) const
{
    return m_descriptor.GetDefinition(flavour, m_storage);
}

/** Calcualtes stride of variable
 *
 * @return Calculated value
 **/
GLuint Variable::GetStride() const
{
    GLint variable_stride = 0;

    if (0 == m_descriptor.m_n_array_elements)
    {
        variable_stride = m_descriptor.m_expected_stride_of_element;
    }
    else
    {
        variable_stride =
            m_descriptor.m_expected_stride_of_element * m_descriptor.m_n_array_elements;
    }

    return variable_stride;
}

/** Check if variable is block
 *
 * @return true if variable type is block, false otherwise
 **/
bool Variable::IsBlock() const
{
    if (BUILTIN == m_descriptor.m_type)
    {
        return false;
    }

    const Interface *interface = m_descriptor.m_interface;
    if (0 == interface)
    {
        TCU_FAIL("Nullptr");
    }

    return (Interface::BLOCK == interface->m_type);
}

/** Check if variable is struct
 *
 * @return true if variable type is struct, false otherwise
 **/
bool Variable::IsStruct() const
{
    if (BUILTIN == m_descriptor.m_type)
    {
        return false;
    }

    const Interface *interface = m_descriptor.m_interface;
    if (0 == interface)
    {
        TCU_FAIL("Nullptr");
    }

    return (Interface::STRUCT == interface->m_type);
}
/** Get code that reference variable
 *
 * @param parent_name Name of parent
 * @param variable    Descriptor of variable
 * @param flavour     Provides info about how variable should be referenced
 * @param array_index Index of array, ignored when variable is not array
 *
 * @return String with code
 **/
std::string Variable::GetReference(const std::string &parent_name,
                                   const Descriptor &variable,
                                   FLAVOUR flavour,
                                   GLuint array_index)
{
    std::string name;

    /* Prepare name */
    if (false == parent_name.empty())
    {
        name = parent_name;
        name.append(".");
        name.append(variable.m_name);
    }
    else
    {
        name = variable.m_name;
    }

    /* */
    switch (flavour)
    {
        case Utils::Variable::BASIC:
            break;

        case Utils::Variable::ARRAY:
            name.append("[0]");
            break;

        case Utils::Variable::INDEXED_BY_INVOCATION_ID:
            name.append("[gl_InvocationID]");
            break;
    }

    /* Assumption that both variables have same lengths */
    if (0 != variable.m_n_array_elements)
    {
        GLchar buffer[16];
        sprintf(buffer, "%d", array_index);
        name.append("[");
        name.append(buffer);
        name.append("]");
    }

    return name;
}

/** Get "flavour" of varying
 *
 * @param stage     Stage of shader
 * @param direction Selects if varying is in or out
 *
 * @return Flavour
 **/
Variable::FLAVOUR Variable::GetFlavour(Shader::STAGES stage, VARYING_DIRECTION direction)
{
    FLAVOUR result = BASIC;

    switch (stage)
    {
        case Shader::GEOMETRY:
        case Shader::TESS_EVAL:
            if (INPUT == direction)
            {
                result = ARRAY;
            }
            break;
        case Shader::TESS_CTRL:
            result = INDEXED_BY_INVOCATION_ID;
            break;
        default:
            break;
    }

    return result;
}

/** Constructor, for built-in types
 *
 * @param name                       Name
 * @param qualifiers                 Qualifiers
 * @param expected_component         Expected component of variable
 * @param expected_location          Expected location
 * @param type                       Type
 * @param normalized                 Selects if data should be normalized
 * @param n_array_elements           Length of array
 * @param expected_stride_of_element Expected stride of element
 * @param offset                     Offset
 **/
Variable::Descriptor::Descriptor(const GLchar *name,
                                 const GLchar *qualifiers,
                                 GLint expected_component,
                                 GLint expected_location,
                                 const Type &type,
                                 GLboolean normalized,
                                 GLuint n_array_elements,
                                 GLint expected_stride_of_element,
                                 GLuint offset)
    : m_expected_component(expected_component),
      m_expected_location(expected_location),
      m_expected_stride_of_element(expected_stride_of_element),
      m_n_array_elements(n_array_elements),
      m_name(name),
      m_normalized(normalized),
      m_offset(offset),
      m_qualifiers(qualifiers),
      m_type(BUILTIN),
      m_builtin(type)
{}

/** Constructor, for interface types
 *
 * @param name                       Name
 * @param qualifiers                 Qualifiers
 * @param expected_component         Expected component of variable
 * @param expected_location          Expected location
 * @param interface                  Interface of variable
 * @param n_array_elements           Length of array
 * @param expected_stride_of_element Expected stride of element
 * @param offset                     Offset
 **/
Variable::Descriptor::Descriptor(const GLchar *name,
                                 const GLchar *qualifiers,
                                 GLint expected_componenet,
                                 GLint expected_location,
                                 Interface *interface,
                                 GLuint n_array_elements,
                                 GLint expected_stride_of_element,
                                 GLuint offset)
    : m_expected_component(expected_componenet),
      m_expected_location(expected_location),
      m_expected_stride_of_element(expected_stride_of_element),
      m_n_array_elements(n_array_elements),
      m_name(name),
      m_normalized(GL_FALSE),
      m_offset(offset),
      m_qualifiers(qualifiers),
      m_type(INTERFACE),
      m_interface(interface)
{}

/** Get definition of variable
 *
 * @param flavour Flavour of variable
 * @param storage Storage used for variable
 *
 * @return code with defintion
 **/
std::string Variable::Descriptor::GetDefinition(FLAVOUR flavour, STORAGE storage) const
{
    static const GLchar *basic_template = "QUALIFIERS STORAGETYPE NAMEARRAY;";
    static const GLchar *array_template = "QUALIFIERS STORAGETYPE NAME[]ARRAY;";
    const GLchar *storage_str           = 0;

    std::string definition;
    size_t position = 0;

    /* Select definition template */
    switch (flavour)
    {
        case BASIC:
            definition = basic_template;
            break;
        case ARRAY:
        case INDEXED_BY_INVOCATION_ID:
            definition = array_template;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    if (BUILTIN != m_type)
    {
        if (0 == m_interface)
        {
            TCU_FAIL("Nullptr");
        }
    }

    /* Qualifiers */
    if (true == m_qualifiers.empty())
    {
        replaceToken("QUALIFIERS ", position, "", definition);
    }
    else
    {
        replaceToken("QUALIFIERS", position, m_qualifiers.c_str(), definition);
    }

    // According to spec: int, uint, and double type must always be declared with flat qualifier
    bool flat_qualifier = false;
    if (m_type != BUILTIN && m_interface != NULL)
    {
        if (m_interface->m_members[0].m_builtin.m_basic_type == Utils::Type::Int ||
            m_interface->m_members[0].m_builtin.m_basic_type == Utils::Type::Uint ||
            m_interface->m_members[0].m_builtin.m_basic_type == Utils::Type::Double)
        {
            flat_qualifier = true;
        }
    }
    /* Storage */
    switch (storage)
    {
        case VARYING_INPUT:
            storage_str = flat_qualifier ? "flat in " : "in ";
            break;
        case VARYING_OUTPUT:
            storage_str = "out ";
            break;
        case UNIFORM:
            storage_str = "uniform ";
            break;
        case SSB:
            storage_str = "buffer ";
            break;
        case MEMBER:
            storage_str = "";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    replaceToken("STORAGE", position, storage_str, definition);

    /* Type */
    if (BUILTIN == m_type)
    {
        replaceToken("TYPE", position, m_builtin.GetGLSLTypeName(), definition);
    }
    else
    {
        if (Interface::STRUCT == m_interface->m_type)
        {
            replaceToken("TYPE", position, m_interface->m_name.c_str(), definition);
        }
        else
        {
            const std::string &block_definition = m_interface->GetDefinition();

            replaceToken("TYPE", position, block_definition.c_str(), definition);
        }
    }

    /* Name */
    replaceToken("NAME", position, m_name.c_str(), definition);

    /* Array size */
    if (0 == m_n_array_elements)
    {
        replaceToken("ARRAY", position, "", definition);
    }
    else
    {
        char buffer[16];
        sprintf(buffer, "[%d]", m_n_array_elements);

        replaceToken("ARRAY", position, buffer, definition);
    }

    /* Done */
    return definition;
}

/** Get definitions for variables collected in vector
 *
 * @param vector  Collection of variables
 * @param flavour Flavour of variables
 *
 * @return Code with definitions
 **/
std::string GetDefinitions(const Variable::PtrVector &vector, Variable::FLAVOUR flavour)
{
    std::string list = Utils::g_list;
    size_t position  = 0;

    for (GLuint i = 0; i < vector.size(); ++i)
    {
        Utils::insertElementOfList(vector[i]->GetDefinition(flavour).c_str(), "\n", position, list);
    }

    Utils::endList("", position, list);

    return list;
}

/** Get definitions for interfaces collected in vector
 *
 * @param vector Collection of interfaces
 *
 * @return Code with definitions
 **/
std::string GetDefinitions(const Interface::PtrVector &vector)
{
    std::string list = Utils::g_list;
    size_t position  = 0;

    for (GLuint i = 0; i < vector.size(); ++i)
    {
        Utils::insertElementOfList(vector[i]->GetDefinition().c_str(), "\n", position, list);
    }

    Utils::endList("", position, list);

    return list;
}

/** Constructor
 *
 * @param name Name
 * @param type Type of interface
 **/
Interface::Interface(const GLchar *name, Interface::TYPE type) : m_name(name), m_type(type) {}

/** Adds member to interface
 *
 * @param member Descriptor of new member
 *
 * @return Pointer to just created member
 **/
Variable::Descriptor *Interface::AddMember(const Variable::Descriptor &member)
{
    m_members.push_back(member);

    return &m_members.back();
}

/** Get definition of interface
 *
 * @param Code with definition
 **/
std::string Interface::GetDefinition() const
{
    std::string definition;
    size_t position = 0;

    const GLchar *member_list = "    MEMBER_DEFINITION\nMEMBER_LIST";

    if (STRUCT == m_type)
    {
        definition = "struct NAME {\nMEMBER_LIST};";
    }
    else
    {
        definition = "NAME {\nMEMBER_LIST}";
    }

    /* Name */
    replaceToken("NAME", position, m_name.c_str(), definition);

    /* Member list */
    for (GLuint i = 0; i < m_members.size(); ++i)
    {
        const size_t start_position = position;
        const std::string &member_definition =
            m_members[i].GetDefinition(Variable::BASIC, Variable::MEMBER);

        /* Member list */
        replaceToken("MEMBER_LIST", position, member_list, definition);

        /* Move back position */
        position = start_position;

        /* Member definition */
        replaceToken("MEMBER_DEFINITION", position, member_definition.c_str(), definition);
    }

    /* Remove last member list */
    replaceToken("MEMBER_LIST", position, "", definition);

    /* Done */
    return definition;
}

/** Adds member of built-in type to interface
 *
 * @param name                       Name
 * @param qualifiers                 Qualifiers
 * @param expected_component         Expected component of variable
 * @param expected_location          Expected location
 * @param type                       Type
 * @param normalized                 Selects if data should be normalized
 * @param n_array_elements           Length of array
 * @param expected_stride_of_element Expected stride of element
 * @param offset                     Offset
 *
 * @return Pointer to just created member
 **/
Variable::Descriptor *Interface::Member(const GLchar *name,
                                        const GLchar *qualifiers,
                                        GLint expected_component,
                                        GLint expected_location,
                                        const Type &type,
                                        GLboolean normalized,
                                        GLuint n_array_elements,
                                        GLint expected_stride_of_element,
                                        GLuint offset)
{
    return AddMember(Variable::Descriptor(name, qualifiers, expected_component, expected_location,
                                          type, normalized, n_array_elements,
                                          expected_stride_of_element, offset));
}

/** Adds member of interface type to interface
 *
 * @param name                       Name
 * @param qualifiers                 Qualifiers
 * @param expected_component         Expected component of variable
 * @param expected_location          Expected location
 * @param type                       Type
 * @param normalized                 Selects if data should be normalized
 * @param n_array_elements           Length of array
 * @param expected_stride_of_element Expected stride of element
 * @param offset                     Offset
 *
 * @return Pointer to just created member
 **/
Variable::Descriptor *Interface::Member(const GLchar *name,
                                        const GLchar *qualifiers,
                                        GLint expected_component,
                                        GLint expected_location,
                                        Interface *nterface,
                                        GLuint n_array_elements,
                                        GLint expected_stride_of_element,
                                        GLuint offset)
{
    return AddMember(Variable::Descriptor(name, qualifiers, expected_component, expected_location,
                                          nterface, n_array_elements, expected_stride_of_element,
                                          offset));
}

/** Clears contents of vector of pointers
 *
 * @tparam T Type of elements
 *
 * @param vector Collection to be cleared
 **/
template <typename T>
void clearPtrVector(std::vector<T *> &vector)
{
    for (size_t i = 0; i < vector.size(); ++i)
    {
        T *t = vector[i];

        vector[i] = 0;

        if (0 != t)
        {
            delete t;
        }
    }

    vector.clear();
}

/** Constructor
 *
 * @param stage Stage described by that interface
 **/
ShaderInterface::ShaderInterface(Shader::STAGES stage) : m_stage(stage)
{
    /* Nothing to be done */
}

/** Get definitions of globals
 *
 * @return Code with definitions
 **/
std::string ShaderInterface::GetDefinitionsGlobals() const
{
    return m_globals;
}

/** Get definitions of inputs
 *
 * @return Code with definitions
 **/
std::string ShaderInterface::GetDefinitionsInputs() const
{
    Variable::FLAVOUR flavour = Variable::GetFlavour(m_stage, Variable::INPUT);

    return GetDefinitions(m_inputs, flavour);
}

/** Get definitions of outputs
 *
 * @return Code with definitions
 **/
std::string ShaderInterface::GetDefinitionsOutputs() const
{
    Variable::FLAVOUR flavour = Variable::GetFlavour(m_stage, Variable::OUTPUT);

    return GetDefinitions(m_outputs, flavour);
}

/** Get definitions of buffers
 *
 * @return Code with definitions
 **/
std::string ShaderInterface::GetDefinitionsSSBs() const
{
    return GetDefinitions(m_ssb_blocks, Variable::BASIC);
}

/** Get definitions of uniforms
 *
 * @return Code with definitions
 **/
std::string ShaderInterface::GetDefinitionsUniforms() const
{
    return GetDefinitions(m_uniforms, Variable::BASIC);
}

/** Constructor
 *
 * @param in  Input variable
 * @param out Output variable
 **/
VaryingConnection::VaryingConnection(Variable *in, Variable *out) : m_in(in), m_out(out)
{
    /* NBothing to be done here */
}

/** Adds new varying connection to given stage
 *
 * @param stage Shader stage
 * @param in    In varying
 * @param out   Out varying
 **/
void VaryingPassthrough::Add(Shader::STAGES stage, Variable *in, Variable *out)
{
    VaryingConnection::Vector &vector = Get(stage);

    vector.push_back(VaryingConnection(in, out));
}

/** Get all passthrough connections for given stage
 *
 * @param stage Shader stage
 *
 * @return Vector of connections
 **/
VaryingConnection::Vector &VaryingPassthrough::Get(Shader::STAGES stage)
{
    VaryingConnection::Vector *result = 0;

    switch (stage)
    {
        case Shader::FRAGMENT:
            result = &m_fragment;
            break;
        case Shader::GEOMETRY:
            result = &m_geometry;
            break;
        case Shader::TESS_CTRL:
            result = &m_tess_ctrl;
            break;
        case Shader::TESS_EVAL:
            result = &m_tess_eval;
            break;
        case Shader::VERTEX:
            result = &m_vertex;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return *result;
}

/** Constructor
 *
 **/
ProgramInterface::ProgramInterface()
    : m_compute(Shader::COMPUTE),
      m_vertex(Shader::VERTEX),
      m_tess_ctrl(Shader::TESS_CTRL),
      m_tess_eval(Shader::TESS_EVAL),
      m_geometry(Shader::GEOMETRY),
      m_fragment(Shader::FRAGMENT)
{}

/** Destructor
 *
 **/
ProgramInterface::~ProgramInterface()
{
    clearPtrVector(m_blocks);
    clearPtrVector(m_structures);
}

/** Adds new interface
 *
 * @param name
 * @param type
 *
 * @return Pointer to created interface
 **/
Interface *ProgramInterface::AddInterface(const GLchar *name, Interface::TYPE type)
{
    Interface *interface = 0;

    if (Interface::STRUCT == type)
    {
        interface = new Interface(name, type);

        m_structures.push_back(interface);
    }
    else
    {
        interface = new Interface(name, type);

        m_blocks.push_back(interface);
    }

    return interface;
}

/** Adds new block interface
 *
 * @param name
 *
 * @return Pointer to created interface
 **/
Interface *ProgramInterface::Block(const GLchar *name)
{
    return AddInterface(name, Interface::BLOCK);
}

/** Get interface of given shader stage
 *
 * @param stage Shader stage
 *
 * @return Reference to stage interface
 **/
ShaderInterface &ProgramInterface::GetShaderInterface(Shader::STAGES stage)
{
    ShaderInterface *interface = 0;

    switch (stage)
    {
        case Shader::COMPUTE:
            interface = &m_compute;
            break;
        case Shader::FRAGMENT:
            interface = &m_fragment;
            break;
        case Shader::GEOMETRY:
            interface = &m_geometry;
            break;
        case Shader::TESS_CTRL:
            interface = &m_tess_ctrl;
            break;
        case Shader::TESS_EVAL:
            interface = &m_tess_eval;
            break;
        case Shader::VERTEX:
            interface = &m_vertex;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return *interface;
}

/** Get interface of given shader stage
 *
 * @param stage Shader stage
 *
 * @return Reference to stage interface
 **/
const ShaderInterface &ProgramInterface::GetShaderInterface(Shader::STAGES stage) const
{
    const ShaderInterface *interface = 0;

    switch (stage)
    {
        case Shader::COMPUTE:
            interface = &m_compute;
            break;
        case Shader::FRAGMENT:
            interface = &m_fragment;
            break;
        case Shader::GEOMETRY:
            interface = &m_geometry;
            break;
        case Shader::TESS_CTRL:
            interface = &m_tess_ctrl;
            break;
        case Shader::TESS_EVAL:
            interface = &m_tess_eval;
            break;
        case Shader::VERTEX:
            interface = &m_vertex;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return *interface;
}

/** Clone interface of Vertex shader stage to other stages
 * It creates matching inputs, outputs, uniforms and buffers in other stages.
 * There are no additional outputs for FRAGMENT shader generated.
 *
 * @param varying_passthrough Collection of varyings connections
 **/
void ProgramInterface::CloneVertexInterface(VaryingPassthrough &varying_passthrough)
{
    /* VS outputs >> TCS inputs >> TCS outputs >> ..  >> FS inputs */
    for (size_t i = 0; i < m_vertex.m_outputs.size(); ++i)
    {
        const Variable &vs_var = *m_vertex.m_outputs[i];
        const GLchar *prefix   = GetStagePrefix(Shader::VERTEX, vs_var.m_storage);

        cloneVariableForStage(vs_var, Shader::TESS_CTRL, prefix, varying_passthrough);
        cloneVariableForStage(vs_var, Shader::TESS_EVAL, prefix, varying_passthrough);
        cloneVariableForStage(vs_var, Shader::GEOMETRY, prefix, varying_passthrough);
        cloneVariableForStage(vs_var, Shader::FRAGMENT, prefix, varying_passthrough);
    }

    /* Copy uniforms from VS to other stages */
    for (size_t i = 0; i < m_vertex.m_uniforms.size(); ++i)
    {
        Variable &vs_var     = *m_vertex.m_uniforms[i];
        const GLchar *prefix = GetStagePrefix(Shader::VERTEX, vs_var.m_storage);

        cloneVariableForStage(vs_var, Shader::COMPUTE, prefix, varying_passthrough);
        cloneVariableForStage(vs_var, Shader::TESS_CTRL, prefix, varying_passthrough);
        cloneVariableForStage(vs_var, Shader::TESS_EVAL, prefix, varying_passthrough);
        cloneVariableForStage(vs_var, Shader::GEOMETRY, prefix, varying_passthrough);
        cloneVariableForStage(vs_var, Shader::FRAGMENT, prefix, varying_passthrough);

        /* Uniform blocks needs unique binding */
        if (true == vs_var.IsBlock())
        {
            replaceBinding(vs_var, Shader::VERTEX);
        }
    }

    /* Copy SSBs from VS to other stages */
    for (size_t i = 0; i < m_vertex.m_ssb_blocks.size(); ++i)
    {
        Variable &vs_var     = *m_vertex.m_ssb_blocks[i];
        const GLchar *prefix = GetStagePrefix(Shader::VERTEX, vs_var.m_storage);

        cloneVariableForStage(vs_var, Shader::COMPUTE, prefix, varying_passthrough);
        cloneVariableForStage(vs_var, Shader::TESS_CTRL, prefix, varying_passthrough);
        cloneVariableForStage(vs_var, Shader::TESS_EVAL, prefix, varying_passthrough);
        cloneVariableForStage(vs_var, Shader::GEOMETRY, prefix, varying_passthrough);
        cloneVariableForStage(vs_var, Shader::FRAGMENT, prefix, varying_passthrough);

        /* SSBs blocks needs unique binding */
        if (true == vs_var.IsBlock())
        {
            replaceBinding(vs_var, Shader::VERTEX);
        }
    }

    m_compute.m_globals   = m_vertex.m_globals;
    m_fragment.m_globals  = m_vertex.m_globals;
    m_geometry.m_globals  = m_vertex.m_globals;
    m_tess_ctrl.m_globals = m_vertex.m_globals;
    m_tess_eval.m_globals = m_vertex.m_globals;
}

/** Clone variable for specific stage
 *
 * @param variable            Variable
 * @param stage               Requested stage
 * @param prefix              Prefix used in variable name that is specific for original stage
 * @param varying_passthrough Collection of varyings connections
 **/
void ProgramInterface::cloneVariableForStage(const Variable &variable,
                                             Shader::STAGES stage,
                                             const GLchar *prefix,
                                             VaryingPassthrough &varying_passthrough)
{
    switch (variable.m_storage)
    {
        case Variable::VARYING_OUTPUT:
        {
            Variable *in = cloneVariableForStage(variable, stage, Variable::VARYING_INPUT, prefix);

            if (Shader::FRAGMENT != stage)
            {
                Variable *out =
                    cloneVariableForStage(variable, stage, Variable::VARYING_OUTPUT, prefix);
                varying_passthrough.Add(stage, in, out);
            }
        }
        break;
        case Variable::UNIFORM:
        case Variable::SSB:
            cloneVariableForStage(variable, stage, variable.m_storage, prefix);
            break;
        default:
            TCU_FAIL("Invalid enum");
    }
}

/** Clone variable for specific stage
 *
 * @param variable Variable
 * @param stage    Requested stage
 * @param storage  Storage used by variable
 * @param prefix   Prefix used in variable name that is specific for original stage
 *
 * @return New variable
 **/
Variable *ProgramInterface::cloneVariableForStage(const Variable &variable,
                                                  Shader::STAGES stage,
                                                  Variable::STORAGE storage,
                                                  const GLchar *prefix)
{
    /* Initialize with original variable */
    Variable *var = new Variable(variable);
    if (0 == var)
    {
        TCU_FAIL("Memory allocation");
    }

    /* Set up storage */
    var->m_storage = storage;

    /* Get name */
    std::string name = variable.m_descriptor.m_name;

    /* Prefix name with stage ID, empty means default block */
    if (false == name.empty())
    {
        size_t position            = 0;
        const GLchar *stage_prefix = GetStagePrefix(stage, storage);
        Utils::replaceToken(prefix, position, stage_prefix, name);
    }
    var->m_descriptor.m_name = name;

    /* Clone block */
    const bool is_block = variable.IsBlock();
    if (true == is_block)
    {
        const Interface *interface = variable.m_descriptor.m_interface;

        Interface *block = CloneBlockForStage(*interface, stage, storage, prefix);

        var->m_descriptor.m_interface = block;
    }

    /* Store variable */
    ShaderInterface &si = GetShaderInterface(stage);
    Variable *result    = 0;

    switch (storage)
    {
        case Variable::VARYING_INPUT:
            si.m_inputs.push_back(var);
            result = si.m_inputs.back();
            break;
        case Variable::VARYING_OUTPUT:
            si.m_outputs.push_back(var);
            result = si.m_outputs.back();
            break;
        case Variable::UNIFORM:
            /* Uniform blocks needs unique binding */
            if (true == is_block)
            {
                replaceBinding(*var, stage);
            }

            si.m_uniforms.push_back(var);
            result = si.m_uniforms.back();
            break;
        case Variable::SSB:
            /* SSBs needs unique binding */
            if (true == is_block)
            {
                replaceBinding(*var, stage);
            }

            si.m_ssb_blocks.push_back(var);
            result = si.m_ssb_blocks.back();
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** clone block to specific stage
 *
 * @param block   Block to be copied
 * @param stage   Specific stage
 * @param storage Storage used by block
 * @param prefix  Prefix used in block name
 *
 * @return New interface
 **/
Interface *ProgramInterface::CloneBlockForStage(const Interface &block,
                                                Shader::STAGES stage,
                                                Variable::STORAGE storage,
                                                const GLchar *prefix)
{
    /* Get name */
    std::string name = block.m_name;

    /* Prefix name with stage ID */
    size_t position            = 0;
    const GLchar *stage_prefix = GetStagePrefix(stage, storage);
    Utils::replaceToken(prefix, position, stage_prefix, name);

    Interface *ptr = GetBlock(name.c_str());

    if (0 == ptr)
    {
        ptr = AddInterface(name.c_str(), Interface::BLOCK);
    }

    ptr->m_members = block.m_members;

    return ptr;
}

/** Get stage specific prefix used in names
 *
 * @param stage   Stage
 * @param storage Storage class
 *
 * @return String
 **/
const GLchar *ProgramInterface::GetStagePrefix(Shader::STAGES stage, Variable::STORAGE storage)
{
    static const GLchar *lut[Shader::STAGE_MAX][Variable::STORAGE_MAX] = {
        /*          IN          OUT         UNIFORM     SSB        MEMBER    */
        /* CS  */ {0, 0, "cs_uni_", "cs_buf_", ""},
        /* VS  */ {"in_vs_", "vs_tcs_", "vs_uni_", "vs_buf_", ""},
        /* TCS */ {"vs_tcs_", "tcs_tes_", "tcs_uni_", "tcs_buf_", ""},
        /* TES */ {"tcs_tes_", "tes_gs_", "tes_uni_", "tes_buf_", ""},
        /* GS  */ {"tes_gs_", "gs_fs_", "gs_uni_", "gs_buf_", ""},
        /* FS  */ {"gs_fs_", "fs_out_", "fs_uni_", "fs_buf_", ""},
    };

    const GLchar *result = 0;

    result = lut[stage][storage];

    return result;
}

/** Get definitions of all structures used in program interface
 *
 * @return String with code
 **/
std::string ProgramInterface::GetDefinitionsStructures() const
{
    return GetDefinitions(m_structures);
}

/** Get interface code for stage
 *
 * @param stage Specific stage
 *
 * @return String with code
 **/
std::string ProgramInterface::GetInterfaceForStage(Shader::STAGES stage) const
{
    size_t position = 0;
    std::string interface =
        "/* Globals */\n"
        "GLOBALS\n"
        "\n"
        "/* Structures */\n"
        "STRUCTURES\n"
        "\n"
        "/* Uniforms */\n"
        "UNIFORMS\n"
        "\n"
        "/* Inputs */\n"
        "INPUTS\n"
        "\n"
        "/* Outputs */\n"
        "OUTPUTS\n"
        "\n"
        "/* Storage */\n"
        "STORAGE\n";

    const ShaderInterface &si = GetShaderInterface(stage);

    const std::string &structures = GetDefinitionsStructures();

    const std::string &globals  = si.GetDefinitionsGlobals();
    const std::string &inputs   = si.GetDefinitionsInputs();
    const std::string &outputs  = si.GetDefinitionsOutputs();
    const std::string &uniforms = si.GetDefinitionsUniforms();
    const std::string &ssbs     = si.GetDefinitionsSSBs();

    replaceToken("GLOBALS", position, globals.c_str(), interface);
    replaceToken("STRUCTURES", position, structures.c_str(), interface);
    replaceToken("UNIFORMS", position, uniforms.c_str(), interface);
    replaceToken("INPUTS", position, inputs.c_str(), interface);
    replaceToken("OUTPUTS", position, outputs.c_str(), interface);
    replaceToken("STORAGE", position, ssbs.c_str(), interface);

    return interface;
}

/** Functional object used in find_if algorithm, in search for interface of given name
 *
 **/
struct matchInterfaceName
{
    matchInterfaceName(const GLchar *name) : m_name(name) {}

    bool operator()(const Interface *interface) { return 0 == interface->m_name.compare(m_name); }

    const GLchar *m_name;
};

/** Finds interface of given name in given vector of interfaces
 *
 * @param vector Collection of interfaces
 * @param name   Requested name
 *
 * @return Pointer to interface if available, 0 otherwise
 **/
static Interface *findInterfaceByName(Interface::PtrVector &vector, const GLchar *name)
{
    Interface::PtrVector::iterator it =
        std::find_if(vector.begin(), vector.end(), matchInterfaceName(name));

    if (vector.end() != it)
    {
        return *it;
    }
    else
    {
        return 0;
    }
}

/** Search for block of given name
 *
 * @param name Name of block
 *
 * @return Pointer to block or 0
 **/
Interface *ProgramInterface::GetBlock(const GLchar *name)
{
    return findInterfaceByName(m_blocks, name);
}

/** Search for structure of given name
 *
 * @param name Name of structure
 *
 * @return Pointer to structure or 0
 **/
Interface *ProgramInterface::GetStructure(const GLchar *name)
{
    return findInterfaceByName(m_structures, name);
}

/** Adds new sturcture to interface
 *
 * @param name Name of structure
 *
 * @return Created structure
 **/
Interface *ProgramInterface::Structure(const GLchar *name)
{
    return AddInterface(name, Interface::STRUCT);
}

/** Replace "BINDING" token in qualifiers string to value specific for given stage
 *
 * @param variable Variable to modify
 * @param stage    Requested stage
 **/
void ProgramInterface::replaceBinding(Variable &variable, Shader::STAGES stage)
{
    GLchar binding[16];
    sprintf(binding, "%d", stage);
    replaceAllTokens("BINDING", binding, variable.m_descriptor.m_qualifiers);
}
}  // namespace Utils

/** Debuging procedure. Logs parameters.
 *
 * @param source   As specified in GL spec.
 * @param type     As specified in GL spec.
 * @param id       As specified in GL spec.
 * @param severity As specified in GL spec.
 * @param ignored
 * @param message  As specified in GL spec.
 * @param info     Pointer to instance of Context used by test.
 */
void GLW_APIENTRY debug_proc(GLenum source,
                             GLenum type,
                             GLuint id,
                             GLenum severity,
                             GLsizei /* length */,
                             const GLchar *message,
                             void *info)
{
    deqp::Context *ctx = (deqp::Context *)info;

    const GLchar *source_str   = "Unknown";
    const GLchar *type_str     = "Unknown";
    const GLchar *severity_str = "Unknown";

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:
            source_str = "API";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            source_str = "APP";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            source_str = "OTR";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source_str = "COM";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source_str = "3RD";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source_str = "WS";
            break;
        default:
            break;
    }

    switch (type)
    {
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            type_str = "DEPRECATED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_ERROR:
            type_str = "ERROR";
            break;
        case GL_DEBUG_TYPE_MARKER:
            type_str = "MARKER";
            break;
        case GL_DEBUG_TYPE_OTHER:
            type_str = "OTHER";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            type_str = "PERFORMANCE";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            type_str = "POP_GROUP";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            type_str = "PORTABILITY";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            type_str = "PUSH_GROUP";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            type_str = "UNDEFINED_BEHAVIOR";
            break;
        default:
            break;
    }

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            severity_str = "H";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            severity_str = "L";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            severity_str = "M";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            severity_str = "N";
            break;
        default:
            break;
    }

    ctx->getTestContext().getLog()
        << tcu::TestLog::Message << "DEBUG_INFO: " << std::setw(3) << source_str << "|"
        << severity_str << "|" << std::setw(18) << type_str << "|" << std::setw(12) << id << ": "
        << message << tcu::TestLog::EndMessage;
}

const glw::GLuint TYPES_NUMBER = 34;

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Test name
 * @param test_description Test description
 **/
TestBase::TestBase(deqp::Context &context, const GLchar *test_name, const GLchar *test_description)
    : TestCase(context, test_name, test_description)
{
    /* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult TestBase::iterate()
{
    bool test_result;

#if DEBUG_ENBALE_MESSAGE_CALLBACK
    const Functions &gl = m_context.getRenderContext().getFunctions();

    gl.debugMessageCallback(debug_proc, &m_context);
    GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

    try
    {
        /* Execute test */
        test_result = test();
    }
    catch (std::exception &exc)
    {
        TCU_FAIL(exc.what());
    }

    /* Set result */
    if (true == test_result)
    {
        m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");
    }
    else
    {
        m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
    }

    /* Done */
    return tcu::TestNode::STOP;
}

/** Get last input location available for given type at specific stage
 *
 * @param stage        Shader stage
 * @param type         Input type
 * @param array_length Length of input array
 *
 * @return Last location index
 **/
GLint TestBase::getLastInputLocation(Utils::Shader::STAGES stage,
                                     const Utils::Type &type,
                                     GLuint array_length,
                                     bool ignore_prev_stage)
{
    GLint divide     = 4; /* 4 components per location */
    GLint param      = 0;
    GLenum pname     = 0;
    GLint paramPrev  = 0;
    GLenum pnamePrev = 0;

    /* Select pnmae */
    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            pname     = GL_MAX_FRAGMENT_INPUT_COMPONENTS;
            pnamePrev = GL_MAX_GEOMETRY_OUTPUT_COMPONENTS;
            break;
        case Utils::Shader::GEOMETRY:
            pname     = GL_MAX_GEOMETRY_INPUT_COMPONENTS;
            pnamePrev = GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS;
            break;
        case Utils::Shader::TESS_CTRL:
            pname     = GL_MAX_TESS_CONTROL_INPUT_COMPONENTS;
            pnamePrev = GL_MAX_VERTEX_OUTPUT_COMPONENTS;
            break;
        case Utils::Shader::TESS_EVAL:
            pname     = GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS;
            pnamePrev = GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS;
            break;
        case Utils::Shader::VERTEX:
            pname  = GL_MAX_VERTEX_ATTRIBS;
            divide = 1;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    /* Zero means no array, but 1 slot is required */
    if (0 == array_length)
    {
        array_length += 1;
    }

    /* Get MAX */
    const Functions &gl = m_context.getRenderContext().getFunctions();

    gl.getIntegerv(pname, &param);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    if (pnamePrev && !ignore_prev_stage)
    {
        gl.getIntegerv(pnamePrev, &paramPrev);
        GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

        /* Don't read from a location that doesn't exist in the previous stage */
        param = de::min(param, paramPrev);
    }

/* Calculate */
#if WRKARD_VARYINGLOCATIONSTEST

    const GLint n_avl_locations = 16;

#else

    const GLint n_avl_locations = param / divide;

#endif

    const GLuint n_req_location = type.GetLocations(stage == Utils::Shader::VERTEX) * array_length;

    return n_avl_locations - n_req_location; /* last is max - 1 */
}

/** Get last output location available for given type at specific stage
 *
 * @param stage        Shader stage
 * @param type         Input type
 * @param array_length Length of input array
 *
 * @return Last location index
 **/
GLint TestBase::getLastOutputLocation(Utils::Shader::STAGES stage,
                                      const Utils::Type &type,
                                      GLuint array_length,
                                      bool ignore_next_stage)
{
    GLint param      = 0;
    GLenum pname     = 0;
    GLint paramNext  = 0;
    GLenum pnameNext = 0;

    /* Select pname */
    switch (stage)
    {
        case Utils::Shader::GEOMETRY:
            pname     = GL_MAX_GEOMETRY_OUTPUT_COMPONENTS;
            pnameNext = GL_MAX_FRAGMENT_INPUT_COMPONENTS;
            break;
        case Utils::Shader::TESS_CTRL:
            pname     = GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS;
            pnameNext = GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS;
            break;
        case Utils::Shader::TESS_EVAL:
            pname     = GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS;
            pnameNext = GL_MAX_GEOMETRY_INPUT_COMPONENTS;
            break;
        case Utils::Shader::VERTEX:
            pname     = GL_MAX_VERTEX_OUTPUT_COMPONENTS;
            pnameNext = GL_MAX_TESS_CONTROL_INPUT_COMPONENTS;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    /* Zero means no array, but 1 slot is required */
    if (0 == array_length)
    {
        array_length += 1;
    }

    /* Get MAX */
    const Functions &gl = m_context.getRenderContext().getFunctions();

    gl.getIntegerv(pname, &param);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

/* Calculate */
#if WRKARD_VARYINGLOCATIONSTEST

    const GLint n_avl_locations = 16;

#else

    /* Don't write to a location that doesn't exist in the next stage */
    if (!ignore_next_stage)
    {
        gl.getIntegerv(pnameNext, &paramNext);
        GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

        param = de::min(param, paramNext);
    }

    const GLint n_avl_locations = param / 4; /* 4 components per location */

#endif

    const GLuint n_req_location = type.GetLocations() * array_length;

    return n_avl_locations - n_req_location; /* last is max - 1 */
}

/** Basic implementation
 *
 * @param ignored
 *
 * @return Empty string
 **/
std::string TestBase::getTestCaseName(GLuint /* test_case_index */)
{
    std::string result;

    return result;
}

/** Basic implementation
 *
 * @return 1
 **/
GLuint TestBase::getTestCaseNumber()
{
    return 1;
}

/** Check if flat qualifier is required for given type, stage and storage
 *
 * @param stage        Shader stage
 * @param type         Input type
 * @param storage      Storage of variable
 *
 * @return Last location index
 **/
bool TestBase::isFlatRequired(Utils::Shader::STAGES stage,
                              const Utils::Type &type,
                              Utils::Variable::STORAGE storage,
                              const bool coherent) const
{
    /* Float types do not need flat at all */
    if (Utils::Type::Float == type.m_basic_type)
    {
        return false;
    }

    /* Inputs to fragment shader */
    if ((Utils::Shader::FRAGMENT == stage) && (Utils::Variable::VARYING_INPUT == storage))
    {
        return true;
    }

    /* Outputs from geometry shader
     *
     * This is not strictly needed since fragment shader input
     * interpolation qualifiers will override whatever comes from the
     * previous stage. However, if we want to have a coherent
     * interface, let's better do it.
     */
    if ((Utils::Shader::GEOMETRY == stage) && (Utils::Variable::VARYING_OUTPUT == storage) &&
        coherent)
    {
        return true;
    }

    return false;
}

/** Basic implementation of testInit method
 *
 **/
void TestBase::testInit() {}

/** Calculate stride for interface
 *
 * @param interface Interface
 *
 * @return Calculated value
 **/
GLuint TestBase::calculateStride(const Utils::Interface &interface) const
{
    const size_t n_members = interface.m_members.size();

    GLuint stride = 0;

    for (size_t i = 0; i < n_members; ++i)
    {
        const Utils::Variable::Descriptor &member = interface.m_members[i];
        const GLuint member_offset                = member.m_offset;
        const GLuint member_stride                = member.m_expected_stride_of_element;
        const GLuint member_ends_at               = member_offset + member_stride;

        stride = std::max(stride, member_ends_at);
    }

    return stride;
}

/** Generate data for interface. This routine is recursive
 *
 * @param interface Interface
 * @param offset    Offset in out_data
 * @param out_data  Buffer to be filled
 **/
void TestBase::generateData(const Utils::Interface &interface,
                            GLuint offset,
                            std::vector<GLubyte> &out_data) const
{
    const size_t n_members = interface.m_members.size();
    GLubyte *ptr           = &out_data[offset];

    for (size_t i = 0; i < n_members; ++i)
    {
        const Utils::Variable::Descriptor &member = interface.m_members[i];
        const GLuint member_offset                = member.m_offset;
        const GLuint n_elements = (0 == member.m_n_array_elements) ? 1 : member.m_n_array_elements;

        for (GLuint element = 0; element < n_elements; ++element)
        {
            const GLuint element_offset = element * member.m_expected_stride_of_element;
            const GLuint data_offfset   = member_offset + element_offset;

            if (Utils::Variable::BUILTIN == member.m_type)
            {
                const std::vector<GLubyte> &data = member.m_builtin.GenerateData();

                memcpy(ptr + data_offfset, &data[0], data.size());
            }
            else
            {
                generateData(*member.m_interface, offset + data_offfset, out_data);
            }
        }
    }
}

/** Get type at index
 *
 * @param index Index of requested type
 *
 * @return Type
 **/
Utils::Type TestBase::getType(GLuint index) const
{
    Utils::Type type;

    switch (index)
    {
        case 0:
            type = Utils::Type::_double;
            break;
        case 1:
            type = Utils::Type::dmat2;
            break;
        case 2:
            type = Utils::Type::dmat2x3;
            break;
        case 3:
            type = Utils::Type::dmat2x4;
            break;
        case 4:
            type = Utils::Type::dmat3;
            break;
        case 5:
            type = Utils::Type::dmat3x2;
            break;
        case 6:
            type = Utils::Type::dmat3x4;
            break;
        case 7:
            type = Utils::Type::dmat4;
            break;
        case 8:
            type = Utils::Type::dmat4x2;
            break;
        case 9:
            type = Utils::Type::dmat4x3;
            break;
        case 10:
            type = Utils::Type::dvec2;
            break;
        case 11:
            type = Utils::Type::dvec3;
            break;
        case 12:
            type = Utils::Type::dvec4;
            break;
        case 13:
            type = Utils::Type::_float;
            break;
        case 14:
            type = Utils::Type::mat2;
            break;
        case 15:
            type = Utils::Type::mat2x3;
            break;
        case 16:
            type = Utils::Type::mat2x4;
            break;
        case 17:
            type = Utils::Type::mat3;
            break;
        case 18:
            type = Utils::Type::mat3x2;
            break;
        case 19:
            type = Utils::Type::mat3x4;
            break;
        case 20:
            type = Utils::Type::mat4;
            break;
        case 21:
            type = Utils::Type::mat4x2;
            break;
        case 22:
            type = Utils::Type::mat4x3;
            break;
        case 23:
            type = Utils::Type::vec2;
            break;
        case 24:
            type = Utils::Type::vec3;
            break;
        case 25:
            type = Utils::Type::vec4;
            break;
        case 26:
            type = Utils::Type::_int;
            break;
        case 27:
            type = Utils::Type::ivec2;
            break;
        case 28:
            type = Utils::Type::ivec3;
            break;
        case 29:
            type = Utils::Type::ivec4;
            break;
        case 30:
            type = Utils::Type::uint;
            break;
        case 31:
            type = Utils::Type::uvec2;
            break;
        case 32:
            type = Utils::Type::uvec3;
            break;
        case 33:
            type = Utils::Type::uvec4;
            break;
        default:
            TCU_FAIL("invalid enum");
    }

    return type;
}

/** Get name of type at index
 *
 * @param index Index of type
 *
 * @return Name
 **/
std::string TestBase::getTypeName(GLuint index) const
{
    std::string name = getType(index).GetGLSLTypeName();

    return name;
}

/** Get number of types
 *
 * @return 34
 **/
glw::GLuint TestBase::getTypesNumber() const
{
    // return 34;
    return TYPES_NUMBER;
}

/** Execute test
 *
 * @return true if test pass, false otherwise
 **/
bool TestBase::test()
{
    bool result         = true;
    GLuint n_test_cases = 0;

    /* Prepare test */
    testInit();

    /* GL entry points */
    const Functions &gl = m_context.getRenderContext().getFunctions();

    /* Tessellation patch set up */
    gl.patchParameteri(GL_PATCH_VERTICES, 1);
    GLU_EXPECT_NO_ERROR(gl.getError(), "PatchParameteri");

    /* Get number of test cases */
    n_test_cases = getTestCaseNumber();

#if DEBUG_REPEAT_TEST_CASE

    while (1)
    {
        GLuint test_case = DEBUG_REPEATED_TEST_CASE;

#else  /* DEBUG_REPEAT_TEST_CASE */

    for (GLuint test_case = 0; test_case < n_test_cases; ++test_case)
    {
#endif /* DEBUG_REPEAT_TEST_CASE */

        /* Execute case */
        if (!testCase(test_case))
        {
            const std::string &test_case_name = getTestCaseName(test_case);

            if (false == test_case_name.empty())
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message << "Test case (" << test_case_name << ") failed."
                    << tcu::TestLog::EndMessage;
            }
            else
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message << "Test case (" << test_case << ") failed."
                    << tcu::TestLog::EndMessage;
            }

            result = false;
        }
    }

    /* Done */
    return result;
}

/* Constants used by BufferTestBase */
const GLuint BufferTestBase::bufferDescriptor::m_non_indexed = -1;

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Name of test
 * @param test_description Description of test
 **/
BufferTestBase::BufferTestBase(deqp::Context &context,
                               const GLchar *test_name,
                               const GLchar *test_description)
    : TestBase(context, test_name, test_description)
{}

/** Execute drawArrays for single vertex
 *
 * @param ignored
 *
 * @return true
 **/
bool BufferTestBase::executeDrawCall(bool tesEnabled, GLuint /* test_case_index */)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    gl.disable(GL_RASTERIZER_DISCARD);
    GLU_EXPECT_NO_ERROR(gl.getError(), "Disable");

    gl.beginTransformFeedback(GL_POINTS);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

    // Only TES is existed, glDrawArray can use the parameter GL_PATCHES
    if (tesEnabled == false)
    {
        gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
    }
    else
    {
        gl.drawArrays(GL_PATCHES, 0 /* first */, 1 /* count */);
    }

    GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

    gl.endTransformFeedback();
    GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

    return true;
}

/** Get descriptors of buffers necessary for test
 *
 * @param ignored
 * @param ignored
 **/
void BufferTestBase::getBufferDescriptors(glw::GLuint /* test_case_index */,
                                          bufferDescriptor::Vector & /* out_descriptors */)
{
    /* Nothhing to be done */
}

/** Get list of names of varyings that will be registered with TransformFeedbackVaryings
 *
 * @param ignored
 * @param ignored
 **/
void BufferTestBase::getCapturedVaryings(glw::GLuint /* test_case_index */,
                                         Utils::Program::NameVector & /* captured_varyings */,
                                         GLint * /* xfb_components */)
{
    /* Nothing to be done */
}

/** Get body of main function for given shader stage
 *
 * @param ignored
 * @param ignored
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void BufferTestBase::getShaderBody(glw::GLuint /* test_case_index */,
                                   Utils::Shader::STAGES /* stage */,
                                   std::string &out_assignments,
                                   std::string &out_calculations)
{
    out_assignments  = "";
    out_calculations = "";
}

/** Get interface of shader
 *
 * @param ignored
 * @param ignored
 * @param out_interface Set to ""
 **/
void BufferTestBase::getShaderInterface(glw::GLuint /* test_case_index */,
                                        Utils::Shader::STAGES /* stage */,
                                        std::string &out_interface)
{
    out_interface = "";
}

/** Get source code of shader
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Source
 **/
std::string BufferTestBase::getShaderSource(glw::GLuint test_case_index,
                                            Utils::Shader::STAGES stage)
{
    std::string assignments;
    std::string calculations;
    std::string interface;

    /* */
    getShaderBody(test_case_index, stage, assignments, calculations);
    getShaderInterface(test_case_index, stage, interface);

    /* */
    std::string source = getShaderTemplate(stage);

    /* */
    size_t position = 0;
    Utils::replaceToken("INTERFACE", position, interface.c_str(), source);
    Utils::replaceToken("CALCULATIONS", position, calculations.c_str(), source);
    Utils::replaceToken("ASSIGNMENTS", position, assignments.c_str(), source);

    /* */
    return source;
}

/** Inspects program to check if all resources are as expected
 *
 * @param ignored
 * @param ignored
 * @param ignored
 *
 * @return true
 **/
bool BufferTestBase::inspectProgram(GLuint /* test_case_index */,
                                    Utils::Program & /* program */,
                                    std::stringstream & /* out_stream */)
{
    return true;
}

/** Runs test case
 *
 * @param test_case_index Id of test case
 *
 * @return true if test case pass, false otherwise
 **/
bool BufferTestBase::testCase(GLuint test_case_index)
{
    try
    {
        bufferCollection buffers;
        Utils::Program::NameVector captured_varyings;
        bufferDescriptor::Vector descriptors;
        Utils::Program program(m_context);
        Utils::VertexArray vao(m_context);

        /* Get captured varyings */
        GLint xfb_components;
        getCapturedVaryings(test_case_index, captured_varyings, &xfb_components);

        /* Don't generate shaders that try to capture more XFB components than the implementation's
         * limit */
        if (captured_varyings.size() > 0)
        {
            const Functions &gl = m_context.getRenderContext().getFunctions();

            GLint max_xfb_components;
            gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &max_xfb_components);
            GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

            if (xfb_components > max_xfb_components)
            {
                return true;
            }
        }

        /* Get shader sources */
        const std::string &fragment_shader =
            getShaderSource(test_case_index, Utils::Shader::FRAGMENT);
        const std::string &geometry_shader =
            getShaderSource(test_case_index, Utils::Shader::GEOMETRY);
        const std::string &tess_ctrl_shader =
            getShaderSource(test_case_index, Utils::Shader::TESS_CTRL);
        const std::string &tess_eval_shader =
            getShaderSource(test_case_index, Utils::Shader::TESS_EVAL);
        const std::string &vertex_shader = getShaderSource(test_case_index, Utils::Shader::VERTEX);

        /* Set up program */
        program.Init("" /* compute_shader */, fragment_shader, geometry_shader, tess_ctrl_shader,
                     tess_eval_shader, vertex_shader, captured_varyings, true,
                     false /* is_separable */);

        /* Inspection */
        {
            std::stringstream stream;
            if (false == inspectProgram(test_case_index, program, stream))
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message
                    << "Program inspection failed. Test case: " << getTestCaseName(test_case_index)
                    << ". Reason: " << stream.str() << tcu::TestLog::EndMessage
                    << tcu::TestLog::KernelSource(vertex_shader)
                    << tcu::TestLog::KernelSource(tess_ctrl_shader)
                    << tcu::TestLog::KernelSource(tess_eval_shader)
                    << tcu::TestLog::KernelSource(geometry_shader)
                    << tcu::TestLog::KernelSource(fragment_shader);

                return false;
            }
        }

        program.Use();

        /* Set up buffers */
        getBufferDescriptors(test_case_index, descriptors);
        cleanBuffers();
        prepareBuffers(descriptors, buffers);

        /* Set up vao */
        vao.Init();
        vao.Bind();

        /* Draw */
        bool result = executeDrawCall((program.m_tess_eval.m_id != 0), test_case_index);

#if USE_NSIGHT
        m_context.getRenderContext().postIterate();
#endif

        if (false == result)
        {
            m_context.getTestContext().getLog() << tcu::TestLog::KernelSource(vertex_shader)
                                                << tcu::TestLog::KernelSource(tess_ctrl_shader)
                                                << tcu::TestLog::KernelSource(tess_eval_shader)
                                                << tcu::TestLog::KernelSource(geometry_shader)
                                                << tcu::TestLog::KernelSource(fragment_shader);

            return false;
        }

        /* Verify result */
        if (false == verifyBuffers(buffers))
        {
            m_context.getTestContext().getLog() << tcu::TestLog::KernelSource(vertex_shader)
                                                << tcu::TestLog::KernelSource(tess_ctrl_shader)
                                                << tcu::TestLog::KernelSource(tess_eval_shader)
                                                << tcu::TestLog::KernelSource(geometry_shader)
                                                << tcu::TestLog::KernelSource(fragment_shader);

            return false;
        }
    }
    catch (Utils::Shader::InvalidSourceException &exc)
    {
        exc.log(m_context);
        TCU_FAIL(exc.what());
    }
    catch (Utils::Program::BuildException &exc)
    {
        exc.log(m_context);
        TCU_FAIL(exc.what());
    }

    /* Done */
    return true;
}

/** Verify contents of buffers
 *
 * @param buffers Collection of buffers to be verified
 *
 * @return true if everything is as expected, false otherwise
 **/
bool BufferTestBase::verifyBuffers(bufferCollection &buffers)
{
    bool result = true;

    for (bufferCollection::Vector::iterator it  = buffers.m_vector.begin(),
                                            end = buffers.m_vector.end();
         end != it; ++it)
    {
        bufferCollection::pair &pair = *it;
        Utils::Buffer *buffer        = pair.m_buffer;
        bufferDescriptor *descriptor = pair.m_descriptor;
        size_t size                  = descriptor->m_expected_data.size();

        /* Skip buffers that have no expected data */
        if (0 == size)
        {
            continue;
        }

        /* Get pointer to contents of buffer */
        buffer->Bind();
        GLvoid *buffer_data = buffer->Map(Utils::Buffer::ReadOnly);

        /* Get pointer to expected data */
        GLvoid *expected_data = &descriptor->m_expected_data[0];

        /* Compare */
        int res = memcmp(buffer_data, expected_data, size);

        if (0 != res)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message
                << "Invalid result. Buffer: " << Utils::Buffer::GetBufferName(descriptor->m_target)
                << ". Index: " << descriptor->m_index << tcu::TestLog::EndMessage;

            result = false;
        }

        /* Release buffer mapping */
        buffer->UnMap();
    }

    return result;
}

/** Unbinds all uniforms and xfb
 *
 **/
void BufferTestBase::cleanBuffers()
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    GLint max_uni = 0;
    GLint max_xfb = 0;

    gl.getIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &max_uni);
    gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, &max_xfb);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    for (GLint i = 0; i < max_uni; ++i)
    {
        Utils::Buffer::BindBase(gl, 0, Utils::Buffer::Uniform, i);
    }

    for (GLint i = 0; i < max_xfb; ++i)
    {
        Utils::Buffer::BindBase(gl, 0, Utils::Buffer::Transform_feedback, i);
    }
}

/** Get template of shader for given stage
 *
 * @param stage Stage
 *
 * @return Template of shader source
 **/
std::string BufferTestBase::getShaderTemplate(Utils::Shader::STAGES stage)
{
    static const GLchar *compute_shader_template =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
        "\n"
        "writeonly uniform uimage2D uni_image;\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "CALCULATIONS"
        "\n"
        "ASSIGNMENTS"
        "}\n"
        "\n";

    static const GLchar *fragment_shader_template =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "CALCULATIONS"
        "\n"
        "ASSIGNMENTS"
        "}\n"
        "\n";

    // max_vertices is set to 3 for the test case "xfb_vertex_streams" declares 3 streams in
    // geometry shader, according to spec, max_vertices should be no less than 3 if there are 3
    // streams in GS.
    static const GLchar *geometry_shader_template =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                   in;\n"
        "layout(points, max_vertices = 3) out;\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "CALCULATIONS"
        "\n"
        "\n"
        "ASSIGNMENTS"
        "    gl_Position  = vec4(0, 0, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";

    static const GLchar *tess_ctrl_shader_template =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "CALCULATIONS"
        "\n"
        "ASSIGNMENTS"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";

    static const GLchar *tess_eval_shader_template =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "CALCULATIONS"
        "\n"
        "ASSIGNMENTS"
        "}\n"
        "\n";

    static const GLchar *vertex_shader_template =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "CALCULATIONS"
        "\n"
        "ASSIGNMENTS"
        "}\n"
        "\n";

    const GLchar *result = 0;

    switch (stage)
    {
        case Utils::Shader::COMPUTE:
            result = compute_shader_template;
            break;
        case Utils::Shader::FRAGMENT:
            result = fragment_shader_template;
            break;
        case Utils::Shader::GEOMETRY:
            result = geometry_shader_template;
            break;
        case Utils::Shader::TESS_CTRL:
            result = tess_ctrl_shader_template;
            break;
        case Utils::Shader::TESS_EVAL:
            result = tess_eval_shader_template;
            break;
        case Utils::Shader::VERTEX:
            result = vertex_shader_template;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Prepare buffer according to descriptor
 *
 * @param buffer Buffer to prepare
 * @param desc   Descriptor
 **/
void BufferTestBase::prepareBuffer(Utils::Buffer &buffer, bufferDescriptor &desc)
{
    GLsizeiptr size = 0;
    GLvoid *data    = 0;

    if (false == desc.m_initial_data.empty())
    {
        size = desc.m_initial_data.size();
        data = &desc.m_initial_data[0];
    }
    else if (false == desc.m_expected_data.empty())
    {
        size = desc.m_expected_data.size();
    }

    buffer.Init(desc.m_target, Utils::Buffer::StaticDraw, size, data);

    if (bufferDescriptor::m_non_indexed != desc.m_index)
    {
        buffer.BindBase(desc.m_index);
    }
    else
    {
        buffer.Bind();
    }
}

/** Prepare collection of buffer
 *
 * @param descriptors Collection of descriptors
 * @param out_buffers Collection of buffers
 **/
void BufferTestBase::prepareBuffers(bufferDescriptor::Vector &descriptors,
                                    bufferCollection &out_buffers)
{
    for (bufferDescriptor::Vector::iterator it = descriptors.begin(), end = descriptors.end();
         end != it; ++it)
    {
        bufferCollection::pair pair;

        pair.m_buffer = new Utils::Buffer(m_context);
        if (0 == pair.m_buffer)
        {
            TCU_FAIL("Memory allocation failed");
        }

        pair.m_descriptor = &(*it);

        prepareBuffer(*pair.m_buffer, *pair.m_descriptor);

        out_buffers.m_vector.push_back(pair);
    }
}

/** Destructor
 *
 **/
BufferTestBase::bufferCollection::~bufferCollection()
{
    for (Vector::iterator it = m_vector.begin(), end = m_vector.end(); end != it; ++it)
    {
        if (0 != it->m_buffer)
        {
            delete it->m_buffer;
            it->m_buffer = 0;
        }
    }
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Name of test
 * @param test_description Description of test
 **/
NegativeTestBase::NegativeTestBase(deqp::Context &context,
                                   const GLchar *test_name,
                                   const GLchar *test_description)
    : TestBase(context, test_name, test_description)
{}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return true
 **/
bool NegativeTestBase::isComputeRelevant(GLuint /* test_case_index */)
{
    return true;
}

/** Selects if compilation failure is expected result
 *
 * @param ignored
 *
 * @return true
 **/
bool NegativeTestBase::isFailureExpected(GLuint /* test_case_index */)
{
    return true;
}

/** Selects if the test case should use a separable program
 *
 * @param ignored
 *
 * @return false
 **/
bool NegativeTestBase::isSeparable(const GLuint /* test_case_index */)
{
    return false;
}

/** Runs test case
 *
 * @param test_case_index Id of test case
 *
 * @return true if test case pass, false otherwise
 **/
bool NegativeTestBase::testCase(GLuint test_case_index)
{
    bool test_case_result = true;

    /* Compute */
    if (true == isComputeRelevant(test_case_index))
    {
        const std::string &cs_source   = getShaderSource(test_case_index, Utils::Shader::COMPUTE);
        bool is_build_error            = false;
        const bool is_failure_expected = isFailureExpected(test_case_index);
        Utils::Program program(m_context);

        try
        {
            program.Init(cs_source, "" /* fs */, "" /* gs */, "" /* tcs */, "" /* tes */,
                         "" /* vs */, false /* separable */);
        }
        catch (Utils::Shader::InvalidSourceException &exc)
        {
            if (false == is_failure_expected)
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message
                    << "Unexpected error in shader compilation: " << tcu::TestLog::EndMessage;
                exc.log(m_context);
            }

#if DEBUG_NEG_LOG_ERROR

            else
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message
                    << "Error in shader compilation was expected, logged for verification: "
                    << tcu::TestLog::EndMessage;
                exc.log(m_context);
            }

#endif /* DEBUG_NEG_LOG_ERROR */

            is_build_error = true;
        }
        catch (Utils::Program::BuildException &exc)
        {
            if (false == is_failure_expected)
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message
                    << "Unexpected error in program linking: " << tcu::TestLog::EndMessage;
                exc.log(m_context);
            }

#if DEBUG_NEG_LOG_ERROR

            else
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message
                    << "Error in program linking was expected, logged for verification: "
                    << tcu::TestLog::EndMessage;
                exc.log(m_context);
            }

#endif /* DEBUG_NEG_LOG_ERROR */

            is_build_error = true;
        }

        if (is_build_error != is_failure_expected)
        {
            if (!is_build_error)
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message << "Unexpected success: " << tcu::TestLog::EndMessage;
                Utils::Shader::LogSource(m_context, cs_source, Utils::Shader::COMPUTE);
            }
            test_case_result = false;
        }
    }
    else /* Draw */
    {
        const std::string &fs_source   = getShaderSource(test_case_index, Utils::Shader::FRAGMENT);
        const std::string &gs_source   = getShaderSource(test_case_index, Utils::Shader::GEOMETRY);
        bool is_build_error            = false;
        const bool is_failure_expected = isFailureExpected(test_case_index);
        Utils::Program program(m_context);
        const std::string &tcs_source = getShaderSource(test_case_index, Utils::Shader::TESS_CTRL);
        const std::string &tes_source = getShaderSource(test_case_index, Utils::Shader::TESS_EVAL);
        const std::string &vs_source  = getShaderSource(test_case_index, Utils::Shader::VERTEX);

        try
        {
            if (isSeparable(test_case_index))
            {
                program.Init("" /*cs*/, fs_source, "" /*gs_source*/, "" /*tcs_source*/,
                             "" /*tes_source*/, "" /*vs_source*/, true /* separable */);
                program.Init("" /*cs*/, "" /*fs_source*/, gs_source, "" /*tcs_source*/,
                             "" /*tes_source*/, "" /*vs_source*/, true /* separable */);
                program.Init("" /*cs*/, "" /*fs_source*/, "" /*gs_source*/, tcs_source,
                             "" /*tes_source*/, "" /*vs_source*/, true /* separable */);
                program.Init("" /*cs*/, "" /*fs_source*/, "" /*gs_source*/, "" /*tcs_source*/,
                             tes_source, "" /*vs_source*/, true /* separable */);
                program.Init("" /*cs*/, "" /*fs_source*/, "" /*gs_source*/, "" /*tcs_source*/,
                             "" /*tes_source*/, vs_source, true /* separable */);
            }
            else
            {
                program.Init("" /* cs */, fs_source, gs_source, tcs_source, tes_source, vs_source,
                             false /* separable */);
            }
        }
        catch (Utils::Shader::InvalidSourceException &exc)
        {
            if (false == is_failure_expected)
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message
                    << "Unexpected error in shader compilation: " << tcu::TestLog::EndMessage;
                exc.log(m_context);
            }

#if DEBUG_NEG_LOG_ERROR

            else
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message
                    << "Error in shader compilation was expected, logged for verification: "
                    << tcu::TestLog::EndMessage;
                exc.log(m_context);
            }

#endif /* DEBUG_NEG_LOG_ERROR */

            is_build_error = true;
        }
        catch (Utils::Program::BuildException &exc)
        {
            if (false == is_failure_expected)
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message
                    << "Unexpected error in program linking: " << tcu::TestLog::EndMessage;
                exc.log(m_context);
            }

#if DEBUG_NEG_LOG_ERROR

            else
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message
                    << "Error in program linking was expected, logged for verification: "
                    << tcu::TestLog::EndMessage;
                exc.log(m_context);
            }

#endif /* DEBUG_NEG_LOG_ERROR */

            is_build_error = true;
        }

        if (is_build_error != is_failure_expected)
        {
            if (!is_build_error)
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message << "Unexpected success: " << tcu::TestLog::EndMessage;
                Utils::Shader::LogSource(m_context, vs_source, Utils::Shader::VERTEX);
                Utils::Shader::LogSource(m_context, tcs_source, Utils::Shader::TESS_CTRL);
                Utils::Shader::LogSource(m_context, tes_source, Utils::Shader::TESS_EVAL);
                Utils::Shader::LogSource(m_context, gs_source, Utils::Shader::GEOMETRY);
                Utils::Shader::LogSource(m_context, fs_source, Utils::Shader::FRAGMENT);
            }
            test_case_result = false;
        }
    }

    return test_case_result;
}

/* Constants used by TextureTestBase */
const glw::GLuint TextureTestBase::m_width  = 16;
const glw::GLuint TextureTestBase::m_height = 16;

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Name of test
 * @param test_description Description of test
 **/
TextureTestBase::TextureTestBase(deqp::Context &context,
                                 const GLchar *test_name,
                                 const GLchar *test_description)
    : TestBase(context, test_name, test_description)
{}

/** Get locations for all inputs with automatic_location
 *
 * @param program           Program object
 * @param program_interface Interface of program
 **/
void TextureTestBase::prepareAttribLocation(Utils::Program &program,
                                            Utils::ProgramInterface &program_interface)
{
    Utils::ShaderInterface &si = program_interface.GetShaderInterface(Utils::Shader::VERTEX);

    Utils::Variable::PtrVector &inputs = si.m_inputs;

    for (Utils::Variable::PtrVector::iterator it = inputs.begin(); inputs.end() != it; ++it)
    {
        /* Test does not specify location, query value and set */
        if (Utils::Variable::m_automatic_location == (*it)->m_descriptor.m_expected_location)
        {
            GLuint index   = program.GetResourceIndex((*it)->m_descriptor.m_name, GL_PROGRAM_INPUT);
            GLint location = 0;

            program.GetResource(GL_PROGRAM_INPUT, index, GL_LOCATION, 1 /* size */, &location);

            (*it)->m_descriptor.m_expected_location = location;
        }
    }
}

/** Verifies contents of drawn image
 *
 * @param ignored
 * @param color_0 Verified image
 *
 * @return true if image is filled with 1, false otherwise
 **/
bool TextureTestBase::checkResults(glw::GLuint /* test_case_index */, Utils::Texture &color_0)
{
    static const GLuint size           = m_width * m_height;
    static const GLuint expected_color = 1;

    std::vector<GLuint> data;
    data.resize(size);

    color_0.Get(GL_RED_INTEGER, GL_UNSIGNED_INT, &data[0]);

    for (GLuint i = 0; i < size; ++i)
    {
        const GLuint color = data[i];

        if (expected_color != color)
        {
            m_context.getTestContext().getLog() << tcu::TestLog::Message << "R32UI[" << i
                                                << "]:" << color << tcu::TestLog::EndMessage;
            return false;
        }
    }

    return true;
}

/** Execute dispatch compute for 16x16x1
 *
 * @param ignored
 **/
void TextureTestBase::executeDispatchCall(GLuint /* test_case_index */)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    gl.dispatchCompute(16 /* x */, 16 /* y */, 1 /* z */);
    GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");
}

/** Execute drawArrays for single vertex
 *
 * @param ignored
 **/
void TextureTestBase::executeDrawCall(GLuint /* test_case_index */)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    gl.drawArrays(GL_PATCHES, 0 /* first */, 1 /* count */);
    GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");
}

/** Prepare code snippet that will pass in variables to out variables
 *
 * @param ignored
 * @param varying_passthrough Collection of connections between in and out variables
 * @param stage               Shader stage
 *
 * @return Code that pass in variables to next stage
 **/
std::string TextureTestBase::getPassSnippet(GLuint /* test_case_index */,
                                            Utils::VaryingPassthrough &varying_passthrough,
                                            Utils::Shader::STAGES stage)
{
    static const GLchar *separator = "\n    ";

    /* Skip for compute shader */
    if (Utils::Shader::COMPUTE == stage)
    {
        return "";
    }

    Utils::VaryingConnection::Vector &vector = varying_passthrough.Get(stage);

    std::string result = Utils::g_list;
    size_t position    = 0;

    for (GLuint i = 0; i < vector.size(); ++i)
    {

        Utils::VaryingConnection &connection = vector[i];

        Utils::Variable *in  = connection.m_in;
        Utils::Variable *out = connection.m_out;

        Utils::Variable::FLAVOUR in_flavour =
            Utils::Variable::GetFlavour(stage, Utils::Variable::INPUT);
        Utils::Variable::FLAVOUR out_flavour =
            Utils::Variable::GetFlavour(stage, Utils::Variable::OUTPUT);

        const std::string passthrough = getVariablePassthrough("", in->m_descriptor, in_flavour, "",
                                                               out->m_descriptor, out_flavour);

        Utils::insertElementOfList(passthrough.c_str(), separator, position, result);
    }

    Utils::endList("", position, result);

    return result;
}

/** Basic implementation of method getProgramInterface
 *
 * @param ignored
 * @param ignored
 * @param ignored
 **/
void TextureTestBase::getProgramInterface(GLuint /* test_case_index */,
                                          Utils::ProgramInterface & /* program_interface */,
                                          Utils::VaryingPassthrough & /* varying_passthrough */)
{}

/** Prepare code snippet that will verify in and uniform variables
 *
 * @param ignored
 * @param program_interface Interface of program
 * @param stage             Shader stage
 *
 * @return Code that verify variables
 **/
std::string TextureTestBase::getVerificationSnippet(GLuint /* test_case_index */,
                                                    Utils::ProgramInterface &program_interface,
                                                    Utils::Shader::STAGES stage)
{
    static const GLchar *separator = " ||\n        ";

    std::string verification =
        "if (LIST)\n"
        "    {\n"
        "        result = 0u;\n"
        "    }\n";

    /* Get flavour of in and out variables */
    Utils::Variable::FLAVOUR in_flavour =
        Utils::Variable::GetFlavour(stage, Utils::Variable::INPUT);

    /* Get interface for shader stage */
    Utils::ShaderInterface &si = program_interface.GetShaderInterface(stage);

    /* There are no varialbes to verify */
    if ((0 == si.m_inputs.size()) && (0 == si.m_uniforms.size()) && (0 == si.m_ssb_blocks.size()))
    {
        return "";
    }

    /* For each in variable insert verification code */
    size_t position = 0;

    for (GLuint i = 0; i < si.m_inputs.size(); ++i)
    {
        const Utils::Variable &var = *si.m_inputs[i];
        const std::string &var_verification =
            getVariableVerification("", var.m_data, var.m_descriptor, in_flavour);

        Utils::insertElementOfList(var_verification.c_str(), separator, position, verification);
    }

    /* For each unifrom variable insert verification code */
    for (GLuint i = 0; i < si.m_uniforms.size(); ++i)
    {
        const Utils::Variable &var = *si.m_uniforms[i];
        const std::string &var_verification =
            getVariableVerification("", var.m_data, var.m_descriptor, Utils::Variable::BASIC);

        Utils::insertElementOfList(var_verification.c_str(), separator, position, verification);
    }

    /* For each ssb variable insert verification code */
    for (GLuint i = 0; i < si.m_ssb_blocks.size(); ++i)
    {
        const Utils::Variable &var = *si.m_ssb_blocks[i];
        const std::string &var_verification =
            getVariableVerification("", var.m_data, var.m_descriptor, Utils::Variable::BASIC);

        Utils::insertElementOfList(var_verification.c_str(), separator, position, verification);
    }

    Utils::endList("", position, verification);

#if DEBUG_TTB_VERIFICATION_SNIPPET_STAGE

    {
        GLchar buffer[16];
        sprintf(buffer, "%d", stage + 10);
        Utils::replaceToken("0u", position, buffer, verification);
    }

#elif DEBUG_TTB_VERIFICATION_SNIPPET_VARIABLE

    if (Utils::Shader::VERTEX == stage)
    {
        Utils::replaceToken("0u", position, "in_vs_first.x", verification);
    }
    else
    {
        Utils::replaceToken("0u", position, "31u", verification);
    }

#endif

    /* Done */
    return verification;
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return true
 **/
bool TextureTestBase::isComputeRelevant(GLuint /* test_case_index */)
{
    return true;
}

/** Selects if "draw" stages are relevant for test
 *
 * @param ignored
 *
 * @return true
 **/
bool TextureTestBase::isDrawRelevant(GLuint /* test_case_index */)
{
    return true;
}

/** Prepare code that will do assignment of single in to single out
 *
 * @param in_parent_name  Name of parent in variable
 * @param in_variable     Descriptor of in variable
 * @param in_flavour      Flavoud of in variable
 * @param out_parent_name Name of parent out variable
 * @param out_variable    Descriptor of out variable
 * @param out_flavour     Flavoud of out variable
 *
 * @return Code that does OUT = IN
 **/
std::string TextureTestBase::getVariablePassthrough(const std::string &in_parent_name,
                                                    const Utils::Variable::Descriptor &in_variable,
                                                    Utils::Variable::FLAVOUR in_flavour,
                                                    const std::string &out_parent_name,
                                                    const Utils::Variable::Descriptor &out_variable,
                                                    Utils::Variable::FLAVOUR out_flavour)
{
    bool done                      = false;
    GLuint index                   = 0;
    GLuint member_index            = 0;
    size_t position                = 0;
    std::string result             = Utils::g_list;
    static const GLchar *separator = ";\n    ";

    /* For each member of each array element */
    do
    {
        const std::string in_name =
            Utils::Variable::GetReference(in_parent_name, in_variable, in_flavour, index);
        const std::string out_name =
            Utils::Variable::GetReference(out_parent_name, out_variable, out_flavour, index);
        std::string passthrough;

        /* Prepare verification */
        if (Utils::Variable::BUILTIN == in_variable.m_type)
        {
            size_t pass_position = 0;

            passthrough = "OUT = IN;";

            Utils::replaceToken("OUT", pass_position, out_name.c_str(), passthrough);
            Utils::replaceToken("IN", pass_position, in_name.c_str(), passthrough);

            /* Increment index */
            ++index;
        }
        else
        {
            const Utils::Interface *in_interface  = in_variable.m_interface;
            const Utils::Interface *out_interface = out_variable.m_interface;

            if ((0 == in_interface) || (0 == out_interface))
            {
                TCU_FAIL("Nullptr");
            }

            const Utils::Variable::Descriptor &in_member  = in_interface->m_members[member_index];
            const Utils::Variable::Descriptor &out_member = out_interface->m_members[member_index];

            passthrough = getVariablePassthrough(in_name, in_member, Utils::Variable::BASIC,
                                                 out_name, out_member, Utils::Variable::BASIC);

            /* Increment member_index */
            ++member_index;

            /* Increment index and reset member_index if all members were processed */
            if (in_interface->m_members.size() == member_index)
            {
                ++index;
                member_index = 0;
            }
        }

        /* Check if loop should end */
        if ((index >= in_variable.m_n_array_elements) && (0 == member_index))
        {
            done = true;
        }

        Utils::insertElementOfList(passthrough.c_str(), separator, position, result);

    } while (true != done);

    Utils::endList("", position, result);

    /* Done */
    return result;
}

/** Get verification of single variable
 *
 * @param parent_name Name of parent variable
 * @param data        Data that should be used as EXPECTED
 * @param variable    Descriptor of variable
 * @param flavour     Flavour of variable
 *
 * @return Code that does (EXPECTED != VALUE) ||
 **/
std::string TextureTestBase::getVariableVerification(const std::string &parent_name,
                                                     const GLvoid *data,
                                                     const Utils::Variable::Descriptor &variable,
                                                     Utils::Variable::FLAVOUR flavour)
{
    static const GLchar *logic_op = " ||\n        ";
    const GLuint n_elements = (0 == variable.m_n_array_elements) ? 1 : variable.m_n_array_elements;
    size_t position         = 0;
    std::string result      = Utils::g_list;
    GLint stride            = variable.m_expected_stride_of_element;

    /* For each each array element */
    for (GLuint element = 0; element < n_elements; ++element)
    {
        const std::string name =
            Utils::Variable::GetReference(parent_name, variable, flavour, element);

        /* Calculate data pointer */
        GLvoid *data_ptr = (GLvoid *)((GLubyte *)data + element * stride);

        /* Prepare verification */
        if (Utils::Variable::BUILTIN == variable.m_type)
        {
            const std::string &expected = variable.m_builtin.GetGLSLConstructor(data_ptr);
            std::string verification;
            size_t verification_position = 0;

            verification = "(EXPECTED != NAME)";

            Utils::replaceToken("EXPECTED", verification_position, expected.c_str(), verification);
            Utils::replaceToken("NAME", verification_position, name.c_str(), verification);

            Utils::insertElementOfList(verification.c_str(), logic_op, position, result);
        }
        else
        {
            const Utils::Interface *interface = variable.m_interface;

            if (0 == interface)
            {
                TCU_FAIL("Nullptr");
            }

            const GLuint n_members = static_cast<GLuint>(interface->m_members.size());

            /* for each member */
            for (GLuint member_index = 0; member_index < n_members; ++member_index)
            {
                const Utils::Variable::Descriptor &member = interface->m_members[member_index];

                /* Get verification of member */
                const std::string &verification = getVariableVerification(
                    name, (GLubyte *)data_ptr + member.m_offset, member, Utils::Variable::BASIC);

                Utils::insertElementOfList(verification.c_str(), logic_op, position, result);
            }
        }
    }

    Utils::endList("", position, result);

    return result;
}

/** Prepare attributes, vertex array object and array buffer
 *
 * @param test_case_index   Index of test case
 * @param program_interface Interface of program
 * @param buffer            Array buffer
 * @param vao               Vertex array object
 **/
void TextureTestBase::prepareAttributes(GLuint test_case_index,
                                        Utils::ProgramInterface &program_interface,
                                        Utils::Buffer &buffer,
                                        Utils::VertexArray &vao)
{
    const bool use_component_qualifier = useComponentQualifier(test_case_index);

    /* Get shader interface */
    const Utils::ShaderInterface &si = program_interface.GetShaderInterface(Utils::Shader::VERTEX);

    /* Bind vao and buffer */
    vao.Bind();
    buffer.Bind();

    /* Skip if there are no input variables in vertex shader */
    if (0 == si.m_inputs.size())
    {
        return;
    }

    const Functions &gl = m_context.getRenderContext().getFunctions();

    /* Calculate vertex stride and check */
    GLint max_inputs;
    gl.getIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_inputs);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    /* dvec3/4 vertex inputs use a single location but require 2x16B slots */
    const GLuint max_slots = max_inputs * 2;

    /* Compute used slots */
    std::vector<GLuint> slot_sizes(max_slots, 0);
    for (GLuint i = 0; i < si.m_inputs.size(); ++i)
    {
        const Utils::Variable &variable = *si.m_inputs[i];

        const GLuint variable_size = static_cast<GLuint>(variable.m_data_size);

        const GLuint base_slot =
            variable.m_descriptor.m_expected_location + variable.m_descriptor.m_offset / 16;
        const GLuint ends_at = variable.m_descriptor.m_offset % 16 + variable_size;

        const GLuint array_length = std::max(1u, variable.m_descriptor.m_n_array_elements);
        for (GLuint loc = 0; loc < array_length; loc++)
        {
            const GLuint slot = base_slot + loc;
            slot_sizes[slot]  = std::max(slot_sizes[slot], ends_at);
        }
    }

    /* Compute the offsets where we need to put vertex buffer data for each slot */
    std::vector<GLint> slot_offsets(max_slots, -1);
    GLuint buffer_size = 0;
    for (GLuint i = 0; i < max_slots; i++)
    {
        if (slot_sizes[i] == 0)
        {
            continue;
        }
        slot_offsets[i] = buffer_size;
        buffer_size += slot_sizes[i];
    }

    /* Prepare buffer data and set up vao */
    std::vector<GLubyte> buffer_data(buffer_size);

    GLubyte *ptr = &buffer_data[0];

    for (GLuint i = 0; i < si.m_inputs.size(); ++i)
    {
        const Utils::Variable &variable = *si.m_inputs[i];

        const GLuint base_slot =
            variable.m_descriptor.m_expected_location + variable.m_descriptor.m_offset / 16;
        const GLuint variable_offset = variable.m_descriptor.m_offset % 16;
        const GLuint array_length    = std::max(1u, variable.m_descriptor.m_n_array_elements);
        for (GLuint loc = 0; loc < array_length; loc++)
        {
            const GLuint slot = base_slot + loc;
            memcpy(ptr + slot_offsets[slot] + variable_offset, variable.m_data,
                   variable.m_data_size);
        }

        if (!use_component_qualifier)
        {
            vao.Attribute(variable.m_descriptor.m_expected_location,
                          variable.m_descriptor.m_builtin, variable.m_descriptor.m_n_array_elements,
                          variable.m_descriptor.m_normalized, variable.GetStride(),
                          (GLvoid *)(intptr_t)(slot_offsets[base_slot] + variable_offset));
        }
        else if (0 == variable.m_descriptor.m_expected_component)
        {
            /* Components can only be applied to types not surpassing
             * the bounds of a single slot. Therefore, we calculate
             * the amount of used components in the varying based on
             * the calculated slot sizes.
             */
            const GLuint n_component_size =
                Utils::Type::Double == variable.m_descriptor.m_builtin.m_basic_type ? 8 : 4;
            const GLuint n_rows = slot_sizes[base_slot] / n_component_size;

            const Utils::Type &type =
                Utils::Type::GetType(variable.m_descriptor.m_builtin.m_basic_type,
                                     1 /* n_columns */, n_rows /* n_rows */);

            vao.Attribute(variable.m_descriptor.m_expected_location, type,
                          variable.m_descriptor.m_n_array_elements,
                          variable.m_descriptor.m_normalized, variable.GetStride(),
                          (GLvoid *)(intptr_t)(slot_offsets[base_slot] + variable_offset));
        }
    }

    /* Update buffer */
    buffer.Data(Utils::Buffer::StaticDraw, buffer_size, ptr);
}

/** Get locations for all outputs with automatic_location
 *
 * @param program           Program object
 * @param program_interface Interface of program
 **/
void TextureTestBase::prepareFragmentDataLoc(Utils::Program &program,
                                             Utils::ProgramInterface &program_interface)
{
    Utils::ShaderInterface &si = program_interface.GetShaderInterface(Utils::Shader::FRAGMENT);
    Utils::Variable::PtrVector &outputs = si.m_outputs;

    for (Utils::Variable::PtrVector::iterator it = outputs.begin(); outputs.end() != it; ++it)
    {
        /* Test does not specify location, query value and set */
        if (Utils::Variable::m_automatic_location == (*it)->m_descriptor.m_expected_location)
        {
            GLuint index = program.GetResourceIndex((*it)->m_descriptor.m_name, GL_PROGRAM_OUTPUT);
            GLint location = 0;

            program.GetResource(GL_PROGRAM_OUTPUT, index, GL_LOCATION, 1 /* size */, &location);

            (*it)->m_descriptor.m_expected_location = location;
        }
    }
}

/** Prepare framebuffer with single texture as color attachment
 *
 * @param framebuffer     Framebuffer
 * @param color_0_texture Texture that will used as color attachment
 **/
void TextureTestBase::prepareFramebuffer(Utils::Framebuffer &framebuffer,
                                         Utils::Texture &color_0_texture)
{
    /* Prepare data */
    std::vector<GLuint> texture_data;
    texture_data.resize(m_width * m_height);

    for (GLuint i = 0; i < texture_data.size(); ++i)
    {
        texture_data[i] = 0x20406080;
    }

    /* Prepare texture */
    color_0_texture.Init(Utils::Texture::TEX_2D, m_width, m_height, 0, GL_R32UI, GL_RED_INTEGER,
                         GL_UNSIGNED_INT, &texture_data[0]);

    /* Prepare framebuffer */
    framebuffer.Init();
    framebuffer.Bind();
    framebuffer.AttachTexture(GL_COLOR_ATTACHMENT0, color_0_texture.m_id, m_width, m_height);

    framebuffer.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    framebuffer.Clear(GL_COLOR_BUFFER_BIT);
}

/** Prepare iamge unit for compute shader
 *
 * @param location      Uniform location
 * @param image_texture Texture that will used as color attachment
 **/
void TextureTestBase::prepareImage(GLint location, Utils::Texture &image_texture) const
{
    static const GLuint image_unit = 0;

    std::vector<GLuint> texture_data;
    texture_data.resize(m_width * m_height);

    for (GLuint i = 0; i < texture_data.size(); ++i)
    {
        texture_data[i] = 0x20406080;
    }

    image_texture.Init(Utils::Texture::TEX_2D, m_width, m_height, 0, GL_R32UI, GL_RED_INTEGER,
                       GL_UNSIGNED_INT, &texture_data[0]);

    const Functions &gl = m_context.getRenderContext().getFunctions();

    gl.bindImageTexture(image_unit, image_texture.m_id, 0 /* level */, GL_FALSE /* layered */,
                        0 /* Layer */, GL_WRITE_ONLY, GL_R32UI);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

    Utils::Program::Uniform(gl, Utils::Type::_int, 1 /* count */, location, &image_unit);
}

/** Basic implementation
 *
 * @param ignored
 * @param si        Shader interface
 * @param program   Program
 * @param cs_buffer Buffer for ssb blocks
 **/
void TextureTestBase::prepareSSBs(GLuint /* test_case_index */,
                                  Utils::ShaderInterface &si,
                                  Utils::Program &program,
                                  Utils::Buffer &buffer)
{
    /* Skip if there are no input variables in vertex shader */
    if (0 == si.m_ssb_blocks.size())
    {
        return;
    }

    /* Calculate vertex stride */
    GLint ssbs_stride = 0;

    for (GLuint i = 0; i < si.m_ssb_blocks.size(); ++i)
    {
        Utils::Variable &variable = *si.m_ssb_blocks[i];

        if (false == variable.IsBlock())
        {
            continue;
        }

        GLint variable_stride = variable.GetStride();

        GLint ends_at = variable_stride + variable.m_descriptor.m_offset;

        ssbs_stride = std::max(ssbs_stride, ends_at);
    }

    /* Set active program */
    program.Use();

    /* Allocate */
    buffer.Bind();
    buffer.Data(Utils::Buffer::StaticDraw, ssbs_stride, 0);

    /* Set up uniforms */
    for (GLuint i = 0; i < si.m_ssb_blocks.size(); ++i)
    {
        Utils::Variable &variable = *si.m_ssb_blocks[i];

        /* prepareUnifor should work fine for ssb blocks */
        prepareUniform(program, variable, buffer);
    }
}

/** Basic implementation
 *
 * @param test_case_index   Test case index
 * @param program_interface Program interface
 * @param program           Program
 * @param cs_buffer         Buffer for compute shader stage
 **/
void TextureTestBase::prepareSSBs(GLuint test_case_index,
                                  Utils::ProgramInterface &program_interface,
                                  Utils::Program &program,
                                  Utils::Buffer &cs_buffer)
{
    cs_buffer.Init(Utils::Buffer::Shader_Storage, Utils::Buffer::StaticDraw, 0, 0);

    Utils::ShaderInterface &cs = program_interface.GetShaderInterface(Utils::Shader::COMPUTE);

    prepareSSBs(test_case_index, cs, program, cs_buffer);

    cs_buffer.BindBase(Utils::Shader::COMPUTE);
}

/** Basic implementation
 *
 * @param test_case_index   Test case index
 * @param program_interface Program interface
 * @param program           Program
 * @param fs_buffer         Buffer for fragment shader stage
 * @param gs_buffer         Buffer for geometry shader stage
 * @param tcs_buffer        Buffer for tessellation control shader stage
 * @param tes_buffer        Buffer for tessellation evaluation shader stage
 * @param vs_buffer         Buffer for vertex shader stage
 **/
void TextureTestBase::prepareSSBs(GLuint test_case_index,
                                  Utils::ProgramInterface &program_interface,
                                  Utils::Program &program,
                                  Utils::Buffer &fs_buffer,
                                  Utils::Buffer &gs_buffer,
                                  Utils::Buffer &tcs_buffer,
                                  Utils::Buffer &tes_buffer,
                                  Utils::Buffer &vs_buffer)
{
    fs_buffer.Init(Utils::Buffer::Shader_Storage, Utils::Buffer::StaticDraw, 0, 0);
    gs_buffer.Init(Utils::Buffer::Shader_Storage, Utils::Buffer::StaticDraw, 0, 0);
    tcs_buffer.Init(Utils::Buffer::Shader_Storage, Utils::Buffer::StaticDraw, 0, 0);
    tes_buffer.Init(Utils::Buffer::Shader_Storage, Utils::Buffer::StaticDraw, 0, 0);
    vs_buffer.Init(Utils::Buffer::Shader_Storage, Utils::Buffer::StaticDraw, 0, 0);

    Utils::ShaderInterface &fs  = program_interface.GetShaderInterface(Utils::Shader::FRAGMENT);
    Utils::ShaderInterface &gs  = program_interface.GetShaderInterface(Utils::Shader::GEOMETRY);
    Utils::ShaderInterface &tcs = program_interface.GetShaderInterface(Utils::Shader::TESS_CTRL);
    Utils::ShaderInterface &tes = program_interface.GetShaderInterface(Utils::Shader::TESS_EVAL);
    Utils::ShaderInterface &vs  = program_interface.GetShaderInterface(Utils::Shader::VERTEX);

    prepareSSBs(test_case_index, fs, program, fs_buffer);
    prepareSSBs(test_case_index, gs, program, gs_buffer);
    prepareSSBs(test_case_index, tcs, program, tcs_buffer);
    prepareSSBs(test_case_index, tes, program, tes_buffer);
    prepareSSBs(test_case_index, vs, program, vs_buffer);

    fs_buffer.BindBase(Utils::Shader::FRAGMENT);
    gs_buffer.BindBase(Utils::Shader::GEOMETRY);
    tcs_buffer.BindBase(Utils::Shader::TESS_CTRL);
    tes_buffer.BindBase(Utils::Shader::TESS_EVAL);
    vs_buffer.BindBase(Utils::Shader::VERTEX);
}

/** Updates buffer data with variable
 *
 * @param program  Program object
 * @param variable Variable
 * @param buffer   Buffer
 **/
void TextureTestBase::prepareUniform(Utils::Program &program,
                                     Utils::Variable &variable,
                                     Utils::Buffer &buffer)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    GLsizei count = variable.m_descriptor.m_n_array_elements;
    if (0 == count)
    {
        count = 1;
    }

    if (Utils::Variable::BUILTIN == variable.m_descriptor.m_type)
    {
        program.Uniform(gl, variable.m_descriptor.m_builtin, count,
                        variable.m_descriptor.m_expected_location, variable.m_data);
    }
    else
    {
        const bool is_block = variable.IsBlock();

        if (false == is_block)
        {
            TCU_FAIL("Not implemented");
        }
        else
        {
            buffer.SubData(variable.m_descriptor.m_offset,
                           variable.m_descriptor.m_expected_stride_of_element * count,
                           variable.m_data);
        }
    }
}

/** Basic implementation
 *
 * @param ignored
 * @param si        Shader interface
 * @param program   Program
 * @param cs_buffer Buffer for uniform blocks
 **/
void TextureTestBase::prepareUniforms(GLuint /* test_case_index */,
                                      Utils::ShaderInterface &si,
                                      Utils::Program &program,
                                      Utils::Buffer &buffer)
{
    /* Skip if there are no input variables in vertex shader */
    if (0 == si.m_uniforms.size())
    {
        return;
    }

    /* Calculate vertex stride */
    GLint uniforms_stride = 0;

    for (GLuint i = 0; i < si.m_uniforms.size(); ++i)
    {
        Utils::Variable &variable = *si.m_uniforms[i];

        if (false == variable.IsBlock())
        {
            continue;
        }

        GLint variable_stride = variable.GetStride();

        GLint ends_at = variable_stride + variable.m_descriptor.m_offset;

        uniforms_stride = std::max(uniforms_stride, ends_at);
    }

    /* Set active program */
    program.Use();

    /* Allocate */
    buffer.Bind();
    buffer.Data(Utils::Buffer::StaticDraw, uniforms_stride, 0);

    /* Set up uniforms */
    for (GLuint i = 0; i < si.m_uniforms.size(); ++i)
    {
        Utils::Variable &variable = *si.m_uniforms[i];

        prepareUniform(program, variable, buffer);
    }
}

/** Basic implementation
 *
 * @param test_case_index   Test case index
 * @param program_interface Program interface
 * @param program           Program
 * @param cs_buffer         Buffer for compute shader stage
 **/
void TextureTestBase::prepareUniforms(GLuint test_case_index,
                                      Utils::ProgramInterface &program_interface,
                                      Utils::Program &program,
                                      Utils::Buffer &cs_buffer)
{
    cs_buffer.Init(Utils::Buffer::Uniform, Utils::Buffer::StaticDraw, 0, 0);

    Utils::ShaderInterface &cs = program_interface.GetShaderInterface(Utils::Shader::COMPUTE);

    prepareUniforms(test_case_index, cs, program, cs_buffer);

    cs_buffer.BindBase(Utils::Shader::COMPUTE);
}

/** Basic implementation
 *
 * @param test_case_index   Test case index
 * @param program_interface Program interface
 * @param program           Program
 * @param fs_buffer         Buffer for fragment shader stage
 * @param gs_buffer         Buffer for geometry shader stage
 * @param tcs_buffer        Buffer for tessellation control shader stage
 * @param tes_buffer        Buffer for tessellation evaluation shader stage
 * @param vs_buffer         Buffer for vertex shader stage
 **/
void TextureTestBase::prepareUniforms(GLuint test_case_index,
                                      Utils::ProgramInterface &program_interface,
                                      Utils::Program &program,
                                      Utils::Buffer &fs_buffer,
                                      Utils::Buffer &gs_buffer,
                                      Utils::Buffer &tcs_buffer,
                                      Utils::Buffer &tes_buffer,
                                      Utils::Buffer &vs_buffer)
{
    fs_buffer.Init(Utils::Buffer::Uniform, Utils::Buffer::StaticDraw, 0, 0);
    gs_buffer.Init(Utils::Buffer::Uniform, Utils::Buffer::StaticDraw, 0, 0);
    tcs_buffer.Init(Utils::Buffer::Uniform, Utils::Buffer::StaticDraw, 0, 0);
    tes_buffer.Init(Utils::Buffer::Uniform, Utils::Buffer::StaticDraw, 0, 0);
    vs_buffer.Init(Utils::Buffer::Uniform, Utils::Buffer::StaticDraw, 0, 0);

    Utils::ShaderInterface &fs  = program_interface.GetShaderInterface(Utils::Shader::FRAGMENT);
    Utils::ShaderInterface &gs  = program_interface.GetShaderInterface(Utils::Shader::GEOMETRY);
    Utils::ShaderInterface &tcs = program_interface.GetShaderInterface(Utils::Shader::TESS_CTRL);
    Utils::ShaderInterface &tes = program_interface.GetShaderInterface(Utils::Shader::TESS_EVAL);
    Utils::ShaderInterface &vs  = program_interface.GetShaderInterface(Utils::Shader::VERTEX);

    prepareUniforms(test_case_index, fs, program, fs_buffer);
    prepareUniforms(test_case_index, gs, program, gs_buffer);
    prepareUniforms(test_case_index, tcs, program, tcs_buffer);
    prepareUniforms(test_case_index, tes, program, tes_buffer);
    prepareUniforms(test_case_index, vs, program, vs_buffer);

    fs_buffer.BindBase(Utils::Shader::FRAGMENT);
    gs_buffer.BindBase(Utils::Shader::GEOMETRY);
    tcs_buffer.BindBase(Utils::Shader::TESS_CTRL);
    tes_buffer.BindBase(Utils::Shader::TESS_EVAL);
    vs_buffer.BindBase(Utils::Shader::VERTEX);
}

/** Basic implementation
 *
 * @param test_case_index   Test case index
 * @param program_interface Program interface
 * @param program           Program
 * @param fs_buffer         Buffer for fragment shader stage
 * @param gs_buffer         Buffer for geometry shader stage
 * @param tcs_buffer        Buffer for tessellation control shader stage
 * @param tes_buffer        Buffer for tessellation evaluation shader stage
 * @param vs_buffer         Buffer for vertex shader stage
 **/
void TextureTestBase::prepareUniforms(GLuint test_case_index,
                                      Utils::ProgramInterface &program_interface,
                                      Utils::Program &fs_program,
                                      Utils::Program &gs_program,
                                      Utils::Program &tcs_program,
                                      Utils::Program &tes_program,
                                      Utils::Program &vs_program,
                                      Utils::Buffer &fs_buffer,
                                      Utils::Buffer &gs_buffer,
                                      Utils::Buffer &tcs_buffer,
                                      Utils::Buffer &tes_buffer,
                                      Utils::Buffer &vs_buffer)
{
    fs_buffer.Init(Utils::Buffer::Uniform, Utils::Buffer::StaticDraw, 0, 0);
    gs_buffer.Init(Utils::Buffer::Uniform, Utils::Buffer::StaticDraw, 0, 0);
    tcs_buffer.Init(Utils::Buffer::Uniform, Utils::Buffer::StaticDraw, 0, 0);
    tes_buffer.Init(Utils::Buffer::Uniform, Utils::Buffer::StaticDraw, 0, 0);
    vs_buffer.Init(Utils::Buffer::Uniform, Utils::Buffer::StaticDraw, 0, 0);

    Utils::ShaderInterface &fs  = program_interface.GetShaderInterface(Utils::Shader::FRAGMENT);
    Utils::ShaderInterface &gs  = program_interface.GetShaderInterface(Utils::Shader::GEOMETRY);
    Utils::ShaderInterface &tcs = program_interface.GetShaderInterface(Utils::Shader::TESS_CTRL);
    Utils::ShaderInterface &tes = program_interface.GetShaderInterface(Utils::Shader::TESS_EVAL);
    Utils::ShaderInterface &vs  = program_interface.GetShaderInterface(Utils::Shader::VERTEX);

    prepareUniforms(test_case_index, fs, fs_program, fs_buffer);
    fs_buffer.BindBase(Utils::Shader::FRAGMENT);

    prepareUniforms(test_case_index, gs, gs_program, gs_buffer);
    gs_buffer.BindBase(Utils::Shader::GEOMETRY);

    prepareUniforms(test_case_index, tcs, tcs_program, tcs_buffer);
    tcs_buffer.BindBase(Utils::Shader::TESS_CTRL);

    prepareUniforms(test_case_index, tes, tes_program, tes_buffer);
    tes_buffer.BindBase(Utils::Shader::TESS_EVAL);

    prepareUniforms(test_case_index, vs, vs_program, vs_buffer);
    vs_buffer.BindBase(Utils::Shader::VERTEX);
}

/** Prepare source for shader
 *
 * @param test_case_index     Index of test case
 * @param program_interface   Interface of program
 * @param varying_passthrough Collection of connection between in and out variables
 * @param stage               Shader stage
 *
 * @return Source of shader
 **/
std::string TextureTestBase::getShaderSource(GLuint test_case_index,
                                             Utils::ProgramInterface &program_interface,
                                             Utils::VaryingPassthrough &varying_passthrough,
                                             Utils::Shader::STAGES stage)
{
    /* Get strings */
    const GLchar *shader_template = getShaderTemplate(stage);
    glu::GLSLVersion glslVersion =
        glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
    const char *shader_version          = glu::getGLSLVersionDeclaration(glslVersion);
    const std::string &shader_interface = program_interface.GetInterfaceForStage(stage);
    const std::string &verification =
        getVerificationSnippet(test_case_index, program_interface, stage);
    const std::string &passthrough = getPassSnippet(test_case_index, varying_passthrough, stage);

    const GLchar *per_vertex = "";

    std::string source = shader_template;
    size_t position    = 0;

    Utils::replaceToken("VERSION", position, shader_version, source);

    /* Replace tokens in template */
    if (Utils::Shader::GEOMETRY == stage)
    {
        if (false == useMonolithicProgram(test_case_index))
        {
            per_vertex =
                "out gl_PerVertex {\n"
                "vec4 gl_Position;\n"
                "};\n"
                "\n";
        }

        Utils::replaceToken("PERVERTEX", position, per_vertex, source);
    }

    Utils::replaceToken("INTERFACE", position, shader_interface.c_str(), source);
    Utils::replaceToken("VERIFICATION", position, verification.c_str(), source);

    if (false == verification.empty())
    {
        Utils::replaceAllTokens("ELSE", "    else ", source);
    }
    else
    {
        Utils::replaceAllTokens("ELSE", "", source);
    }

    Utils::replaceAllTokens("PASSTHROUGH", passthrough.c_str(), source);

    /* Done */
    return source;
}

/** Returns template of shader for given stage
 *
 * @param stage Shade stage
 *
 * @return Proper template
 **/
const GLchar *TextureTestBase::getShaderTemplate(Utils::Shader::STAGES stage)
{

    static const GLchar *compute_shader_template =
        "VERSION\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
        "\n"
        "writeonly uniform uimage2D uni_image;\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "    uint result = 1u;\n"
        "\n"
        "    VERIFICATION"
        "\n"
        "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), uvec4(result, 0, 0, 0));\n"
        "}\n"
        "\n";

    static const GLchar *fragment_shader_template =
        "VERSION\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "flat in  uint gs_fs_result;\n"
        "     out uint fs_out_result;\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "    uint result = 1u;\n"
        "\n"
        "    if (1u != gs_fs_result)\n"
        "    {\n"
        "         result = gs_fs_result;\n"
        "    }\n"
        "ELSEVERIFICATION"
        "\n"
        "    fs_out_result = result;\n"
        "    PASSTHROUGH\n"
        "}\n"
        "\n";

    static const GLchar *geometry_shader_template =
        "VERSION\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "     in  uint tes_gs_result[];\n"
        "     flat out uint gs_fs_result;\n"
        "\n"
        "PERVERTEX" /* Separable programs require explicit declaration of gl_PerVertex */
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "    uint result = 1u;\n"
        "\n"
        "    if (1u != tes_gs_result[0])\n"
        "    {\n"
        "         result = tes_gs_result[0];\n"
        "    }\n"
        "ELSEVERIFICATION"
        "\n"
        "    gs_fs_result = result;\n"
        "    PASSTHROUGH\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs_result = result;\n"
        "    PASSTHROUGH\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs_result = result;\n"
        "    PASSTHROUGH\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs_result = result;\n"
        "    PASSTHROUGH\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";

    static const GLchar *tess_ctrl_shader_template =
        "VERSION\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  uint vs_tcs_result[];\n"
        "out uint tcs_tes_result[];\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "    uint result = 1u;\n"
        "\n"
        "    if (1u != vs_tcs_result[gl_InvocationID])\n"
        "    {\n"
        "         result = vs_tcs_result[gl_InvocationID];\n"
        "    }\n"
        "ELSEVERIFICATION"
        "\n"
        "    tcs_tes_result[gl_InvocationID] = result;\n"
        "\n"
        "    PASSTHROUGH\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";

    static const GLchar *tess_eval_shader_template =
        "VERSION\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  uint tcs_tes_result[];\n"
        "out uint tes_gs_result;\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "    uint result = 1u;\n"
        "\n"
        "    if (1u != tcs_tes_result[0])\n"
        "    {\n"
        "         result = tcs_tes_result[0];\n"
        "    }\n"
        "ELSEVERIFICATION"
        "\n"
        "    tes_gs_result = result;\n"
        "\n"
        "    PASSTHROUGH\n"
        "}\n"
        "\n";

    static const GLchar *vertex_shader_template =
        "VERSION\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "out uint vs_tcs_result;\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "    uint result = 1u;\n"
        "\n"
        "    VERIFICATION\n"
        "\n"
        "    vs_tcs_result = result;\n"
        "\n"
        "    PASSTHROUGH\n"
        "}\n"
        "\n";

    const GLchar *result = 0;

    switch (stage)
    {
        case Utils::Shader::COMPUTE:
            result = compute_shader_template;
            break;
        case Utils::Shader::FRAGMENT:
            result = fragment_shader_template;
            break;
        case Utils::Shader::GEOMETRY:
            result = geometry_shader_template;
            break;
        case Utils::Shader::TESS_CTRL:
            result = tess_ctrl_shader_template;
            break;
        case Utils::Shader::TESS_EVAL:
            result = tess_eval_shader_template;
            break;
        case Utils::Shader::VERTEX:
            result = vertex_shader_template;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Runs test case
 *
 * @param test_case_index Id of test case
 *
 * @return true if test case pass, false otherwise
 **/
bool TextureTestBase::testCase(GLuint test_case_index)
{
    try
    {
        if (true == useMonolithicProgram(test_case_index))
        {
            return testMonolithic(test_case_index);
        }
        else
        {
            return testSeparable(test_case_index);
        }
    }
    catch (tcu::NotSupportedError &exc)
    {
        // Write message to log and return true to continue with rest of test cases
        m_context.getTestContext().getLog().writeMessage(exc.what());
        return true;
    }
    catch (Utils::Shader::InvalidSourceException &exc)
    {
        exc.log(m_context);
        TCU_FAIL(exc.what());
    }
    catch (Utils::Program::BuildException &exc)
    {
        exc.log(m_context);
        TCU_FAIL(exc.what());
    }
}

/** Runs "draw" test with monolithic program
 *
 * @param test_case_index Id of test case
 **/
bool TextureTestBase::testMonolithic(GLuint test_case_index)
{
    Utils::ProgramInterface program_interface;
    Utils::VaryingPassthrough varying_passthrough;

    /* */
    const std::string &test_name = getTestCaseName(test_case_index);

    /* */
    getProgramInterface(test_case_index, program_interface, varying_passthrough);

    bool result = true;
    /* Draw */
    if (true == isDrawRelevant(test_case_index))
    {
        Utils::Buffer buffer_attr(m_context);
        Utils::Buffer buffer_ssb_fs(m_context);
        Utils::Buffer buffer_ssb_gs(m_context);
        Utils::Buffer buffer_ssb_tcs(m_context);
        Utils::Buffer buffer_ssb_tes(m_context);
        Utils::Buffer buffer_ssb_vs(m_context);
        Utils::Buffer buffer_u_fs(m_context);
        Utils::Buffer buffer_u_gs(m_context);
        Utils::Buffer buffer_u_tcs(m_context);
        Utils::Buffer buffer_u_tes(m_context);
        Utils::Buffer buffer_u_vs(m_context);
        Utils::Framebuffer framebuffer(m_context);
        Utils::Program program(m_context);
        Utils::Texture texture_fb(m_context);
        Utils::VertexArray vao(m_context);

        /* */
        const std::string &fragment_shader = getShaderSource(
            test_case_index, program_interface, varying_passthrough, Utils::Shader::FRAGMENT);
        const std::string &geometry_shader = getShaderSource(
            test_case_index, program_interface, varying_passthrough, Utils::Shader::GEOMETRY);
        const std::string &tess_ctrl_shader = getShaderSource(
            test_case_index, program_interface, varying_passthrough, Utils::Shader::TESS_CTRL);
        const std::string &tess_eval_shader = getShaderSource(
            test_case_index, program_interface, varying_passthrough, Utils::Shader::TESS_EVAL);
        const std::string &vertex_shader = getShaderSource(
            test_case_index, program_interface, varying_passthrough, Utils::Shader::VERTEX);

        program.Init("" /* compute_shader */, fragment_shader, geometry_shader, tess_ctrl_shader,
                     tess_eval_shader, vertex_shader, false /* is_separable */);

        /* */
        prepareAttribLocation(program, program_interface);
        prepareFragmentDataLoc(program, program_interface);

        /* */
        std::stringstream stream;
        if (false == Utils::checkMonolithicDrawProgramInterface(program, program_interface, stream))
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "FAILURE. Test case: " << test_name
                << ". Inspection of draw program interface failed:\n"
                << stream.str() << tcu::TestLog::EndMessage
                << tcu::TestLog::KernelSource(vertex_shader)
                << tcu::TestLog::KernelSource(tess_ctrl_shader)
                << tcu::TestLog::KernelSource(tess_eval_shader)
                << tcu::TestLog::KernelSource(geometry_shader)
                << tcu::TestLog::KernelSource(fragment_shader);

            return false;
        }

        /* */
        program.Use();

        /* */
        buffer_attr.Init(Utils::Buffer::Array, Utils::Buffer::StaticDraw, 0, 0);
        vao.Init();
        prepareAttributes(test_case_index, program_interface, buffer_attr, vao);

        /* */
        prepareUniforms(test_case_index, program_interface, program, buffer_u_fs, buffer_u_gs,
                        buffer_u_tcs, buffer_u_tes, buffer_u_vs);

        prepareSSBs(test_case_index, program_interface, program, buffer_ssb_fs, buffer_ssb_gs,
                    buffer_ssb_tcs, buffer_ssb_tes, buffer_ssb_vs);

        /* */
        prepareFramebuffer(framebuffer, texture_fb);

        /* Draw */
        executeDrawCall(test_case_index);

#if USE_NSIGHT
        m_context.getRenderContext().postIterate();
#endif

        /* Check results */
        if (false == checkResults(test_case_index, texture_fb))
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "FAILURE. Test case: " << test_name
                << ". Draw - invalid results." << tcu::TestLog::EndMessage
                << tcu::TestLog::KernelSource(vertex_shader)
                << tcu::TestLog::KernelSource(tess_ctrl_shader)
                << tcu::TestLog::KernelSource(tess_eval_shader)
                << tcu::TestLog::KernelSource(geometry_shader)
                << tcu::TestLog::KernelSource(fragment_shader);

            result = false;
        }
    }

    /* Compute */
    if (true == isComputeRelevant(test_case_index))
    {
        Utils::Buffer buffer_ssb_cs(m_context);
        Utils::Buffer buffer_u_cs(m_context);
        Utils::Program program(m_context);
        Utils::Texture texture_im(m_context);
        Utils::VertexArray vao(m_context);

        /* */
        const std::string &compute_shader = getShaderSource(
            test_case_index, program_interface, varying_passthrough, Utils::Shader::COMPUTE);

        program.Init(compute_shader, "" /* fragment_shader */, "" /* geometry_shader */,
                     "" /* tess_ctrl_shader */, "" /* tess_eval_shader */, "" /* vertex_shader */,
                     false /* is_separable */);

        /* */
        {
            std::stringstream stream;

            if (false ==
                Utils::checkMonolithicComputeProgramInterface(program, program_interface, stream))
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message << "FAILURE. Test case: " << test_name
                    << ". Inspection of compute program interface failed:\n"
                    << stream.str() << tcu::TestLog::EndMessage;

                return false;
            }
        }

        /* */
        program.Use();

        /* */
        vao.Init();
        vao.Bind();

        /* */
        prepareUniforms(test_case_index, program_interface, program, buffer_u_cs);

        prepareSSBs(test_case_index, program_interface, program, buffer_ssb_cs);

        /* */
        GLint image_location = program.GetUniformLocation("uni_image");
        prepareImage(image_location, texture_im);

        /* Draw */
        executeDispatchCall(test_case_index);

#if USE_NSIGHT
        m_context.getRenderContext().postIterate();
#endif

        /* Check results */
        if (false == checkResults(test_case_index, texture_im))
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "FAILURE. Test case: " << test_name
                << ". Compute - invalid results." << tcu::TestLog::EndMessage
                << tcu::TestLog::KernelSource(compute_shader);

            result = false;
        }
    }

    return result;
}

/** Runs "draw" test with separable program
 *
 * @param test_case_index Id of test case
 **/
bool TextureTestBase::testSeparable(GLuint test_case_index)
{
    Utils::ProgramInterface program_interface;
    Utils::VaryingPassthrough varying_passthrough;

    /* */
    const std::string &test_name = getTestCaseName(test_case_index);

    /* */
    getProgramInterface(test_case_index, program_interface, varying_passthrough);

    bool result = true;
    /* Draw */
    if (true == isDrawRelevant(test_case_index))
    {
        Utils::Buffer buffer_attr(m_context);
        Utils::Buffer buffer_u_fs(m_context);
        Utils::Buffer buffer_u_gs(m_context);
        Utils::Buffer buffer_u_tcs(m_context);
        Utils::Buffer buffer_u_tes(m_context);
        Utils::Buffer buffer_u_vs(m_context);
        Utils::Framebuffer framebuffer(m_context);
        Utils::Pipeline pipeline(m_context);
        Utils::Program program_fs(m_context);
        Utils::Program program_gs(m_context);
        Utils::Program program_tcs(m_context);
        Utils::Program program_tes(m_context);
        Utils::Program program_vs(m_context);
        Utils::Texture texture_fb(m_context);
        Utils::VertexArray vao(m_context);

        /* */
        const std::string &fs  = getShaderSource(test_case_index, program_interface,
                                                 varying_passthrough, Utils::Shader::FRAGMENT);
        const std::string &gs  = getShaderSource(test_case_index, program_interface,
                                                 varying_passthrough, Utils::Shader::GEOMETRY);
        const std::string &tcs = getShaderSource(test_case_index, program_interface,
                                                 varying_passthrough, Utils::Shader::TESS_CTRL);
        const std::string &tes = getShaderSource(test_case_index, program_interface,
                                                 varying_passthrough, Utils::Shader::TESS_EVAL);
        const std::string &vs  = getShaderSource(test_case_index, program_interface,
                                                 varying_passthrough, Utils::Shader::VERTEX);

        program_fs.Init("" /*cs*/, fs, "" /*gs*/, "" /*tcs*/, "" /*tes*/, "" /*vs*/,
                        true /* is_separable */);
        program_gs.Init("" /*cs*/, "" /*fs*/, gs, "" /*tcs*/, "" /*tes*/, "" /*vs*/,
                        true /* is_separable */);
        program_tcs.Init("" /*cs*/, "" /*fs*/, "" /*gs*/, tcs, "" /*tes*/, "" /*vs*/,
                         true /* is_separable */);
        program_tes.Init("" /*cs*/, "" /*fs*/, "" /*gs*/, "" /*tcs*/, tes, "" /*vs*/,
                         true /* is_separable */);
        program_vs.Init("" /*cs*/, "" /*fs*/, "" /*gs*/, "" /*tcs*/, "" /*tes*/, vs,
                        true /* is_separable */);

        /* */
        prepareAttribLocation(program_vs, program_interface);
        prepareFragmentDataLoc(program_vs, program_interface);

        /* */
        std::stringstream stream;
        if ((false == Utils::checkSeparableDrawProgramInterface(program_vs, program_interface,
                                                                Utils::Shader::VERTEX, stream)) ||
            (false == Utils::checkSeparableDrawProgramInterface(program_fs, program_interface,
                                                                Utils::Shader::FRAGMENT, stream)) ||
            (false == Utils::checkSeparableDrawProgramInterface(program_gs, program_interface,
                                                                Utils::Shader::GEOMETRY, stream)) ||
            (false == Utils::checkSeparableDrawProgramInterface(
                          program_tcs, program_interface, Utils::Shader::TESS_CTRL, stream)) ||
            (false == Utils::checkSeparableDrawProgramInterface(program_tes, program_interface,
                                                                Utils::Shader::TESS_EVAL, stream)))
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "FAILURE. Test case: " << test_name
                << ". Inspection of separable draw program interface failed:\n"
                << stream.str() << tcu::TestLog::EndMessage << tcu::TestLog::KernelSource(vs)
                << tcu::TestLog::KernelSource(tcs) << tcu::TestLog::KernelSource(tes)
                << tcu::TestLog::KernelSource(gs) << tcu::TestLog::KernelSource(fs);

            return false;
        }

        /* */
        pipeline.Init();
        pipeline.UseProgramStages(program_fs.m_id, GL_FRAGMENT_SHADER_BIT);
        pipeline.UseProgramStages(program_gs.m_id, GL_GEOMETRY_SHADER_BIT);
        pipeline.UseProgramStages(program_tcs.m_id, GL_TESS_CONTROL_SHADER_BIT);
        pipeline.UseProgramStages(program_tes.m_id, GL_TESS_EVALUATION_SHADER_BIT);
        pipeline.UseProgramStages(program_vs.m_id, GL_VERTEX_SHADER_BIT);
        pipeline.Bind();

        /* */

        buffer_attr.Init(Utils::Buffer::Array, Utils::Buffer::StaticDraw, 0, 0);
        vao.Init();
        prepareAttributes(test_case_index, program_interface, buffer_attr, vao);

        /* */
        prepareUniforms(test_case_index, program_interface, program_fs, program_gs, program_tcs,
                        program_tes, program_vs, buffer_u_fs, buffer_u_gs, buffer_u_tcs,
                        buffer_u_tes, buffer_u_vs);

        Utils::Program::Use(m_context.getRenderContext().getFunctions(),
                            Utils::Program::m_invalid_id);

        /* */
        prepareFramebuffer(framebuffer, texture_fb);

        /* Draw */
        executeDrawCall(test_case_index);

#if USE_NSIGHT
        m_context.getRenderContext().postIterate();
#endif

        /* Check results */
        if (false == checkResults(test_case_index, texture_fb))
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "FAILURE. Test case: " << test_name
                << ". Draw - invalid results." << tcu::TestLog::EndMessage
                << tcu::TestLog::KernelSource(vs) << tcu::TestLog::KernelSource(tcs)
                << tcu::TestLog::KernelSource(tes) << tcu::TestLog::KernelSource(gs)
                << tcu::TestLog::KernelSource(fs);

            result = false;
        }
        else
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "Success." << tcu::TestLog::EndMessage
                << tcu::TestLog::KernelSource(vs) << tcu::TestLog::KernelSource(tcs)
                << tcu::TestLog::KernelSource(tes) << tcu::TestLog::KernelSource(gs)
                << tcu::TestLog::KernelSource(fs);
        }
    }

    /* Compute */
    if (true == isComputeRelevant(test_case_index))
    {
        Utils::Buffer buffer_u_cs(m_context);
        Utils::Program program(m_context);
        Utils::Texture texture_im(m_context);
        Utils::VertexArray vao(m_context);

        /* */
        const std::string &compute_shader = getShaderSource(
            test_case_index, program_interface, varying_passthrough, Utils::Shader::COMPUTE);

        program.Init(compute_shader, "" /* fragment_shader */, "" /* geometry_shader */,
                     "" /* tess_ctrl_shader */, "" /* tess_eval_shader */, "" /* vertex_shader */,
                     false /* is_separable */);

        /* */
        {
            std::stringstream stream;

            if (false ==
                Utils::checkMonolithicComputeProgramInterface(program, program_interface, stream))
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message << "FAILURE. Test case: " << test_name
                    << ". Inspection of compute program interface failed:\n"
                    << stream.str() << tcu::TestLog::EndMessage;

                return false;
            }
        }

        /* */
        program.Use();

        /* */
        vao.Init();
        vao.Bind();

        /* */
        prepareUniforms(test_case_index, program_interface, program, buffer_u_cs);

        /* */
        GLint image_location = program.GetUniformLocation("uni_image");
        prepareImage(image_location, texture_im);

        /* Draw */
        executeDispatchCall(test_case_index);

#if USE_NSIGHT
        m_context.getRenderContext().postIterate();
#endif

        /* Check results */
        if (false == checkResults(test_case_index, texture_im))
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "FAILURE. Test case: " << test_name
                << ". Compute - invalid results." << tcu::TestLog::EndMessage
                << tcu::TestLog::KernelSource(compute_shader);

            result = false;
        }
    }

    return result;
}

/** Basic implementation
 *
 * @param ignored
 *
 * @return false
 **/
bool TextureTestBase::useComponentQualifier(glw::GLuint /* test_case_index */)
{
    return false;
}

/** Basic implementation
 *
 * @param ignored
 *
 * @return true
 **/
bool TextureTestBase::useMonolithicProgram(GLuint /* test_case_index */)
{
    return true;
}

/** Constructor
 *
 * @param context Test framework context
 **/
APIConstantValuesTest::APIConstantValuesTest(deqp::Context &context)
    : TestCase(context, "api_constant_values", "Test verifies values of api constants")
{
    /* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult APIConstantValuesTest::iterate()
{
    static const GLuint expected_comp = 64;
    static const GLuint expected_xfb  = 4;
    static const GLuint expected_sep  = 4;
    GLint max_comp                    = 0;
    GLint max_xfb                     = 0;
    GLint max_sep                     = 0;
    bool test_result                  = true;

    const Functions &gl = m_context.getRenderContext().getFunctions();

    gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, &max_xfb);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");
    gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &max_comp);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");
    gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS, &max_sep);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    if (expected_xfb > (GLuint)max_xfb)
    {
        m_context.getTestContext().getLog()
            << tcu::TestLog::Message << "Invalid GL_MAX_TRANSFORM_FEEDBACK_BUFFERS. Got " << max_xfb
            << " Expected at least " << expected_xfb << tcu::TestLog::EndMessage;

        test_result = false;
    }

    if (expected_comp > (GLuint)max_comp)
    {
        m_context.getTestContext().getLog()
            << tcu::TestLog::Message
            << "Invalid GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS. Got " << max_comp
            << " Expected at least " << expected_comp << tcu::TestLog::EndMessage;

        test_result = false;
    }

    if (expected_sep > (GLuint)max_sep)
    {
        m_context.getTestContext().getLog()
            << tcu::TestLog::Message
            << "Invalid GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS. Got " << max_comp
            << " Expected at least " << expected_comp << tcu::TestLog::EndMessage;

        test_result = false;
    }

    /* Set result */
    if (true == test_result)
    {
        m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");
    }
    else
    {
        m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
    }

    /* Done */
    return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context Test framework context
 **/
APIErrorsTest::APIErrorsTest(deqp::Context &context)
    : TestCase(context, "api_errors", "Test verifies errors reeturned by api")
{
    /* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult APIErrorsTest::iterate()
{
    GLint length = 0;
    GLchar name[64];
    GLint param = 0;
    Utils::Program program(m_context);
    bool test_result = true;

    const Functions &gl = m_context.getRenderContext().getFunctions();

    try
    {
        program.Init("" /* cs */,
                     "#version 430 core\n"
                     "#extension GL_ARB_enhanced_layouts : require\n"
                     "\n"
                     "in  vec4 vs_fs;\n"
                     "out vec4 fs_out;\n"
                     "\n"
                     "void main()\n"
                     "{\n"
                     "    fs_out = vs_fs;\n"
                     "}\n"
                     "\n" /* fs */,
                     "" /* gs */, "" /* tcs */, "" /* tes */,
                     "#version 430 core\n"
                     "#extension GL_ARB_enhanced_layouts : require\n"
                     "\n"
                     "in  vec4 in_vs;\n"
                     "layout (xfb_offset = 16) out vec4 vs_fs;\n"
                     "\n"
                     "void main()\n"
                     "{\n"
                     "    vs_fs = in_vs;\n"
                     "}\n"
                     "\n" /* vs */,
                     false /* separable */);
    }
    catch (Utils::Shader::InvalidSourceException &exc)
    {
        exc.log(m_context);
        TCU_FAIL(exc.what());
    }
    catch (Utils::Program::BuildException &exc)
    {
        TCU_FAIL(exc.what());
    }

    /*
     * - GetProgramInterfaceiv should generate INVALID_OPERATION when
     * <programInterface> is TRANSFORM_FEEDBACK_BUFFER and <pname> is one of the
     * following:
     *   * MAX_NAME_LENGTH,
     *   * MAX_NUM_ACTIVE_VARIABLES;
     */
    gl.getProgramInterfaceiv(program.m_id, GL_TRANSFORM_FEEDBACK_BUFFER, GL_MAX_NAME_LENGTH,
                             &param);
    checkError(GL_INVALID_OPERATION,
               "GetProgramInterfaceiv(GL_TRANSFORM_FEEDBACK_BUFFER, GL_MAX_NAME_LENGTH)",
               test_result);

    /*
     * - GetProgramResourceIndex should generate INVALID_ENUM when
     * <programInterface> is TRANSFORM_FEEDBACK_BUFFER;
     */
    gl.getProgramResourceIndex(program.m_id, GL_TRANSFORM_FEEDBACK_BUFFER, "0");
    checkError(GL_INVALID_ENUM, "GetProgramResourceIndex(GL_TRANSFORM_FEEDBACK_BUFFER)",
               test_result);
    /*
     * - GetProgramResourceName should generate INVALID_ENUM when
     * <programInterface> is TRANSFORM_FEEDBACK_BUFFER;
     */
    gl.getProgramResourceName(program.m_id, GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */,
                              64 /* bufSize */, &length, name);
    checkError(GL_INVALID_ENUM, "GetProgramResourceName(GL_TRANSFORM_FEEDBACK_BUFFER)",
               test_result);

    /* Set result */
    if (true == test_result)
    {
        m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");
    }
    else
    {
        m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
    }

    /* Done */
    return tcu::TestNode::STOP;
}

/** Check if error is the expected one.
 *
 * @param expected_error Expected error
 * @param message        Message to log in case of error
 * @param test_result    Test result, set to false in case of invalid error
 **/
void APIErrorsTest::checkError(GLenum expected_error, const GLchar *message, bool &test_result)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    GLenum error = gl.getError();

    if (error != expected_error)
    {
        m_context.getTestContext().getLog()
            << tcu::TestLog::Message << "Failure. Invalid error. Got " << glu::getErrorStr(error)
            << " expected " << glu::getErrorStr(expected_error) << " Msg: " << message
            << tcu::TestLog::EndMessage;

        test_result = false;
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
GLSLContantImmutablityTest::GLSLContantImmutablityTest(deqp::Context &context,
                                                       GLuint constant,
                                                       GLuint stage)
    : NegativeTestBase(context,
                       "glsl_contant_immutablity",
                       "Test verifies that glsl constants cannot be modified"),
      m_constant(constant),
      m_stage(stage)
{
    std::string name = ("glsl_contant_immutablity_");
    name.append(EnhancedLayouts::GLSLContantImmutablityTest::getConstantName(
        (EnhancedLayouts::GLSLContantImmutablityTest::CONSTANTS)constant));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string GLSLContantImmutablityTest::getShaderSource(GLuint test_case_index,
                                                        Utils::Shader::STAGES stage)
{
    static const GLchar *cs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
        "\n"
        "writeonly uniform uimage2D uni_image;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    uint result = 1u;\n"
        "    CONSTANT = 3;\n"
        "\n"
        "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), uvec4(result, 0, 0, 0));\n"
        "}\n"
        "\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "ASSIGNMENT"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "ASSIGNMENT"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "ASSIGNMENT"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "ASSIGNMENT"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "ASSIGNMENT"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (Utils::Shader::COMPUTE == test_case.m_stage)
    {
        size_t position = 0;

        source = cs;

        Utils::replaceToken("CONSTANT", position, getConstantName(test_case.m_constant), source);
    }
    else
    {
        std::string assignment = "    CONSTANT = 3;\n";
        size_t position        = 0;

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        if (test_case.m_stage == stage)
        {
            Utils::replaceToken("CONSTANT", position, getConstantName(test_case.m_constant),
                                assignment);
        }
        else
        {
            assignment = "";
        }

        position = 0;
        Utils::replaceToken("ASSIGNMENT", position, assignment.c_str(), source);
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Constant name
 **/
std::string GLSLContantImmutablityTest::getTestCaseName(GLuint test_case_index)
{
    std::string result = getConstantName(m_test_cases[test_case_index].m_constant);

    return result;
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint GLSLContantImmutablityTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param test_case_index Index of test case
 *
 * @return true when tested stage is compute
 **/
bool GLSLContantImmutablityTest::isComputeRelevant(GLuint test_case_index)
{
    return (Utils::Shader::COMPUTE == m_test_cases[test_case_index].m_stage);
}

/** Prepare all test cases
 *
 **/
void GLSLContantImmutablityTest::testInit()
{
    testCase test_case = {(CONSTANTS)m_constant, (Utils::Shader::STAGES)m_stage};

    m_test_cases.push_back(test_case);
}

/** Get name of glsl constant
 *
 * @param Constant id
 *
 * @return Name of constant used in GLSL
 **/
const GLchar *GLSLContantImmutablityTest::getConstantName(CONSTANTS constant)
{
    const GLchar *name = "";

    switch (constant)
    {
        case GL_ARB_ENHANCED_LAYOUTS:
            name = "GL_ARB_enhanced_layouts";
            break;
        case GL_MAX_XFB:
            name = "gl_MaxTransformFeedbackBuffers";
            break;
        case GL_MAX_XFB_INT_COMP:
            name = "gl_MaxTransformFeedbackInterleavedComponents";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return name;
}

/** Constructor
 *
 * @param context Test framework context
 **/
GLSLContantValuesTest::GLSLContantValuesTest(deqp::Context &context)
    : TextureTestBase(context, "glsl_contant_values", "Test verifies values of constant symbols")
{}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool GLSLContantValuesTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare code snippet that will verify in and uniform variables
 *
 * @param ignored
 * @param ignored
 * @param stage   Shader stage
 *
 * @return Code that verify variables
 **/
std::string GLSLContantValuesTest::getVerificationSnippet(
    GLuint /* test_case_index */,
    Utils::ProgramInterface & /* program_interface */,
    Utils::Shader::STAGES stage)
{
    /* Get constants */
    const Functions &gl = m_context.getRenderContext().getFunctions();

    GLint max_transform_feedback_buffers                = 0;
    GLint max_transform_feedback_interleaved_components = 0;

    gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, &max_transform_feedback_buffers);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");
    gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS,
                   &max_transform_feedback_interleaved_components);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    std::string verification;

    if (Utils::Shader::VERTEX == stage)
    {
        verification =
            "if (1 != GL_ARB_enhanced_layouts)\n"
            "    {\n"
            "        result = 0;\n"
            "    }\n"
            "    else if (MAX_TRANSFORM_FEEDBACK_BUFFERS\n"
            "        != gl_MaxTransformFeedbackBuffers)\n"
            "    {\n"
            "        result = 0;\n"
            "    }\n"
            "    else if (MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS \n"
            "        != gl_MaxTransformFeedbackInterleavedComponents)\n"
            "    {\n"
            "        result = 0;\n"
            "    }\n";

        size_t position = 0;
        GLchar buffer[16];

        sprintf(buffer, "%d", max_transform_feedback_buffers);
        Utils::replaceToken("MAX_TRANSFORM_FEEDBACK_BUFFERS", position, buffer, verification);

        sprintf(buffer, "%d", max_transform_feedback_interleaved_components);
        Utils::replaceToken("MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS", position, buffer,
                            verification);
    }
    else
    {
        verification = "";
    }

    return verification;
}

/** Constructor
 *
 * @param context Test framework context
 **/
GLSLConstantIntegralExpressionTest::GLSLConstantIntegralExpressionTest(deqp::Context &context)
    : TextureTestBase(context,
                      "glsl_constant_integral_expression",
                      "Test verifies that symbols can be used as constant integral expressions")
{}

/** Get interface of program
 *
 * @param ignored
 * @param program_interface Interface of program
 * @param ignored
 **/
void GLSLConstantIntegralExpressionTest::getProgramInterface(
    GLuint /* test_case_index */,
    Utils::ProgramInterface &program_interface,
    Utils::VaryingPassthrough & /* varying_passthrough */)
{
    /* Get constants */
    const Functions &gl = m_context.getRenderContext().getFunctions();

    GLint max_transform_feedback_buffers                = 0;
    GLint max_transform_feedback_interleaved_components = 0;

    gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, &max_transform_feedback_buffers);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");
    gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS,
                   &max_transform_feedback_interleaved_components);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    GLuint gohan_div = std::max(1, max_transform_feedback_buffers / 16);
    GLuint goten_div = std::max(1, max_transform_feedback_interleaved_components / 16);

    m_gohan_length = max_transform_feedback_buffers / gohan_div;
    m_goten_length = max_transform_feedback_interleaved_components / goten_div;

    /* Globals */
    std::string globals =
        "uniform uint goku [GL_ARB_enhanced_layouts / 1];\n"
        "uniform uint gohan[gl_MaxTransformFeedbackBuffers / GOHAN_DIV];\n"
        "uniform uint goten[gl_MaxTransformFeedbackInterleavedComponents / GOTEN_DIV];\n";

    size_t position = 0;
    GLchar buffer[16];

    sprintf(buffer, "%d", gohan_div);
    Utils::replaceToken("GOHAN_DIV", position, buffer, globals);

    sprintf(buffer, "%d", goten_div);
    Utils::replaceToken("GOTEN_DIV", position, buffer, globals);

    program_interface.m_vertex.m_globals    = globals;
    program_interface.m_tess_ctrl.m_globals = globals;
    program_interface.m_tess_eval.m_globals = globals;
    program_interface.m_geometry.m_globals  = globals;
    program_interface.m_fragment.m_globals  = globals;
    program_interface.m_compute.m_globals   = globals;
}

/** Prepare code snippet that will verify in and uniform variables
 *
 * @param ignored
 * @param ignored
 * @param ignored
 *
 * @return Code that verify variables
 **/
std::string GLSLConstantIntegralExpressionTest::getVerificationSnippet(
    GLuint /* test_case_index */,
    Utils::ProgramInterface & /* program_interface */,
    Utils::Shader::STAGES /* stage */)
{
    std::string verification =
        "{\n"
        "        uint goku_sum = 0;\n"
        "        uint gohan_sum = 0;\n"
        "        uint goten_sum = 0;\n"
        "\n"
        "        for (uint i = 0u; i < goku.length(); ++i)\n"
        "        {\n"
        "            goku_sum += goku[i];\n"
        "        }\n"
        "\n"
        "        for (uint i = 0u; i < gohan.length(); ++i)\n"
        "        {\n"
        "            gohan_sum += gohan[i];\n"
        "        }\n"
        "\n"
        "        for (uint i = 0u; i < goten.length(); ++i)\n"
        "        {\n"
        "            goten_sum += goten[i];\n"
        "        }\n"
        "\n"
        "        if ( (1u != goku_sum)  &&\n"
        "             (EXPECTED_GOHAN_SUMu != gohan_sum) ||\n"
        "             (EXPECTED_GOTEN_SUMu != goten_sum) )\n"
        "        {\n"
        "            result = 0u;\n"
        "        }\n"
        "    }\n";

    size_t position = 0;
    GLchar buffer[16];

    sprintf(buffer, "%d", m_gohan_length);
    Utils::replaceToken("EXPECTED_GOHAN_SUM", position, buffer, verification);

    sprintf(buffer, "%d", m_goten_length);
    Utils::replaceToken("EXPECTED_GOTEN_SUM", position, buffer, verification);

    return verification;
}

/** Prepare unifroms
 *
 * @param ignored
 * @param ignored
 * @param program Program object
 * @param ignored
 **/
void GLSLConstantIntegralExpressionTest::prepareUniforms(
    GLuint /* test_case_index */,
    Utils::ProgramInterface & /* program_interface */,
    Utils::Program &program,
    Utils::Buffer & /* cs_buffer */)
{
    static const GLuint uniform_data[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    const Functions &gl = m_context.getRenderContext().getFunctions();

    GLint goku_location  = program.GetUniformLocation("goku");
    GLint gohan_location = program.GetUniformLocation("gohan");
    GLint goten_location = program.GetUniformLocation("goten");

    program.Uniform(gl, Utils::Type::uint, 1 /* count */, goku_location, uniform_data);
    program.Uniform(gl, Utils::Type::uint, m_gohan_length, gohan_location, uniform_data);
    program.Uniform(gl, Utils::Type::uint, m_goten_length, goten_location, uniform_data);
}

/** Prepare unifroms
 *
 * @param test_case_index   Pass as param to first implemetnation
 * @param program_interface Pass as param to first implemetnation
 * @param program           Pass as param to first implemetnation
 * @param ignored
 * @param ignored
 * @param ignored
 * @param ignored
 * @param vs_buffer         Pass as param to first implemetnation
 **/
void GLSLConstantIntegralExpressionTest::prepareUniforms(GLuint test_case_index,
                                                         Utils::ProgramInterface &program_interface,
                                                         Utils::Program &program,
                                                         Utils::Buffer & /* fs_buffer */,
                                                         Utils::Buffer & /* gs_buffer */,
                                                         Utils::Buffer & /* tcs_buffer */,
                                                         Utils::Buffer & /* tes_buffer */,
                                                         Utils::Buffer &vs_buffer)
{
    /* Call first implementation */
    prepareUniforms(test_case_index, program_interface, program, vs_buffer);
}

/** Constructor
 *
 * @param context Test framework context
 **/
UniformBlockMemberOffsetAndAlignTest::UniformBlockMemberOffsetAndAlignTest(deqp::Context &context)
    : TextureTestBase(context,
                      "uniform_block_member_offset_and_align",
                      "Test verifies offsets and alignment of uniform buffer members")
{}

/** Get interface of program
 *
 * @param test_case_index     Test case index
 * @param program_interface   Interface of program
 * @param varying_passthrough Collection of connections between in and out variables
 **/
void UniformBlockMemberOffsetAndAlignTest::getProgramInterface(
    GLuint test_case_index,
    Utils::ProgramInterface &program_interface,
    Utils::VaryingPassthrough &varying_passthrough)
{
    std::string globals =
        "const int basic_size = BASIC_SIZE;\n"
        "const int type_align = TYPE_ALIGN;\n"
        "const int type_size  = TYPE_SIZE;\n";

    Utils::Type type         = getType(test_case_index);
    GLuint basic_size        = Utils::Type::GetTypeSize(type.m_basic_type);
    const GLuint base_align  = type.GetBaseAlignment(false);
    const GLuint array_align = type.GetBaseAlignment(true);
    const GLuint base_stride = Utils::Type::CalculateStd140Stride(base_align, type.m_n_columns, 0);
    const GLuint type_align  = Utils::roundUpToPowerOf2(base_stride);

    /* Calculate offsets */
    const GLuint first_offset  = 0;
    const GLuint second_offset = type.GetActualOffset(base_stride, basic_size / 2);

#if WRKARD_UNIFORMBLOCKMEMBEROFFSETANDALIGNTEST

    const GLuint third_offset   = type.GetActualOffset(second_offset + base_stride, base_align);
    const GLuint fourth_offset  = type.GetActualOffset(third_offset + base_stride, base_align);
    const GLuint fifth_offset   = type.GetActualOffset(fourth_offset + base_stride, base_align);
    const GLuint sixth_offset   = type.GetActualOffset(fifth_offset + base_stride, array_align);
    const GLuint seventh_offset = type.GetActualOffset(sixth_offset + base_stride, array_align);
    const GLuint eigth_offset   = type.GetActualOffset(seventh_offset + base_stride, array_align);

#else /* WRKARD_UNIFORMBLOCKMEMBEROFFSETANDALIGNTEST */

    const GLuint third_offset   = type.GetActualOffset(second_offset + base_stride, 2 * type_align);
    const GLuint fourth_offset  = type.GetActualOffset(3 * type_align + base_stride, base_align);
    const GLuint fifth_offset   = type.GetActualOffset(fourth_offset + base_stride, base_align);
    const GLuint sixth_offset   = type.GetActualOffset(fifth_offset + base_stride, array_align);
    const GLuint seventh_offset = type.GetActualOffset(sixth_offset + base_stride, array_align);
    const GLuint eigth_offset = type.GetActualOffset(seventh_offset + base_stride, 8 * basic_size);

#endif /* WRKARD_UNIFORMBLOCKMEMBEROFFSETANDALIGNTEST */

    /* Prepare data */
    const std::vector<GLubyte> &first  = type.GenerateData();
    const std::vector<GLubyte> &second = type.GenerateData();
    const std::vector<GLubyte> &third  = type.GenerateData();
    const std::vector<GLubyte> &fourth = type.GenerateData();

    m_data.resize(eigth_offset + base_stride);
    GLubyte *ptr = &m_data[0];
    memcpy(ptr + first_offset, &first[0], first.size());
    memcpy(ptr + second_offset, &second[0], second.size());
    memcpy(ptr + third_offset, &third[0], third.size());
    memcpy(ptr + fourth_offset, &fourth[0], fourth.size());
    memcpy(ptr + fifth_offset, &fourth[0], fourth.size());
    memcpy(ptr + sixth_offset, &third[0], third.size());
    memcpy(ptr + seventh_offset, &second[0], second.size());
    memcpy(ptr + eigth_offset, &first[0], first.size());

    /* Prepare globals */
    size_t position = 0;
    GLchar buffer[16];

    sprintf(buffer, "%d", basic_size);
    Utils::replaceToken("BASIC_SIZE", position, buffer, globals);

    sprintf(buffer, "%d", type_align);
    Utils::replaceToken("TYPE_ALIGN", position, buffer, globals);

    sprintf(buffer, "%d", base_stride);
    Utils::replaceToken("TYPE_SIZE", position, buffer, globals);

    /* Prepare Block */
    Utils::Interface *vs_uni_block = program_interface.Block("vs_uni_Block");

    vs_uni_block->Member("at_first_offset", "layout(offset = 0, align = 8 * basic_size)",
                         0 /* expected_component */, 0 /* expected_location */, type,
                         false /* normalized */, 0 /* n_array_elements */, base_stride,
                         first_offset);

    vs_uni_block->Member("at_second_offset", "layout(offset = type_size, align = basic_size / 2)",
                         0 /* expected_component */, 0 /* expected_location */, type,
                         false /* normalized */, 0 /* n_array_elements */, base_stride,
                         second_offset);

    vs_uni_block->Member("at_third_offset", "layout(align = 2 * type_align)",
                         0 /* expected_component */, 0 /* expected_location */, type,
                         false /* normalized */, 0 /* n_array_elements */, base_stride,
                         third_offset);

    vs_uni_block->Member("at_fourth_offset", "layout(offset = 3 * type_align + type_size)",
                         0 /* expected_component */, 0 /* expected_location */, type,
                         false /* normalized */, 0 /* n_array_elements */, base_stride,
                         fourth_offset);

    vs_uni_block->Member("at_fifth_offset", "", 0 /* expected_component */,
                         0 /* expected_location */, type, false /* normalized */,
                         0 /* n_array_elements */, base_stride, fifth_offset);

    vs_uni_block->Member("at_sixth_offset", "", 0 /* expected_component */,
                         0 /* expected_location */, type, false /* normalized */,
                         2 /* n_array_elements */, array_align * 2, sixth_offset);

    vs_uni_block->Member("at_eigth_offset", "layout(align = 8 * basic_size)",
                         0 /* expected_component */, 0 /* expected_location */, type,
                         false /* normalized */, 0 /* n_array_elements */, base_stride,
                         eigth_offset);

    Utils::ShaderInterface &vs_si = program_interface.GetShaderInterface(Utils::Shader::VERTEX);

    /* Add globals */
    vs_si.m_globals = globals;

    /* Add uniform BLOCK */
    vs_si.Uniform("vs_uni_block", "layout (std140, binding = BINDING)", 0, 0, vs_uni_block, 0,
                  static_cast<glw::GLint>(m_data.size()), 0, &m_data[0], m_data.size());

    /* */
    program_interface.CloneVertexInterface(varying_passthrough);
}

/** Get type name
 *
 * @param test_case_index Index of test case
 *
 * @return Name of type test in test_case_index
 **/
std::string UniformBlockMemberOffsetAndAlignTest::getTestCaseName(glw::GLuint test_case_index)
{
    return getTypeName(test_case_index);
}

/** Returns number of types to test
 *
 * @return Number of types, 34
 **/
glw::GLuint UniformBlockMemberOffsetAndAlignTest::getTestCaseNumber()
{
    return getTypesNumber();
}

/** Prepare code snippet that will verify in and uniform variables
 *
 * @param ignored
 * @param ignored
 * @param stage   Shader stage
 *
 * @return Code that verify variables
 **/
std::string UniformBlockMemberOffsetAndAlignTest::getVerificationSnippet(
    GLuint /* test_case_index */,
    Utils::ProgramInterface & /* program_interface */,
    Utils::Shader::STAGES stage)
{
    std::string verification =
        "if ( (PREFIXblock.at_first_offset  != PREFIXblock.at_eigth_offset   ) ||\n"
        "         (PREFIXblock.at_second_offset != PREFIXblock.at_sixth_offset[1]) ||\n"
        "         (PREFIXblock.at_third_offset  != PREFIXblock.at_sixth_offset[0]) ||\n"
        "         (PREFIXblock.at_fourth_offset != PREFIXblock.at_fifth_offset   )  )\n"
        "    {\n"
        "        result = 0;\n"
        "    }";

    const GLchar *prefix = Utils::ProgramInterface::GetStagePrefix(stage, Utils::Variable::UNIFORM);

    Utils::replaceAllTokens("PREFIX", prefix, verification);

    return verification;
}

/** Constructor
 *
 * @param context Test framework context
 **/
UniformBlockLayoutQualifierConflictTest::UniformBlockLayoutQualifierConflictTest(
    deqp::Context &context,
    glw::GLuint qualifier,
    glw::GLuint stage)
    : NegativeTestBase(context,
                       "uniform_block_layout_qualifier_conflict",
                       "Test verifies that std140 is required when offset and/or align qualifiers "
                       "are used with uniform block"),
      m_qualifier(qualifier),
      m_stage(stage)
{
    std::string name = ("uniform_block_layout_qualifier_conflict_");
    name.append(EnhancedLayouts::UniformBlockLayoutQualifierConflictTest::getQualifierName(
        (EnhancedLayouts::UniformBlockLayoutQualifierConflictTest::QUALIFIERS)qualifier));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string UniformBlockLayoutQualifierConflictTest::getShaderSource(GLuint test_case_index,
                                                                     Utils::Shader::STAGES stage)
{
    static const GLchar *cs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
        "\n"
        "LAYOUTuniform Block {\n"
        "    layout(offset = 16) vec4 b;\n"
        "    layout(align  = 64) vec4 a;\n"
        "} uni_block;\n"
        "\n"
        "writeonly uniform image2D uni_image;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = uni_block.b + uni_block.a;\n"
        "\n"
        "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
        "}\n"
        "\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "LAYOUTuniform Block {\n"
        "    layout(offset = 16) vec4 b;\n"
        "    layout(align  = 64) vec4 a;\n"
        "} uni_block;\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs + uni_block.b + uni_block.a;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "LAYOUTuniform Block {\n"
        "    layout(offset = 16) vec4 b;\n"
        "    layout(align  = 64) vec4 a;\n"
        "} uni_block;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0] + uni_block.b + uni_block.a;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0] + uni_block.b + uni_block.a;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0] + uni_block.b + uni_block.a;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0] + uni_block.b + uni_block.a;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "LAYOUTuniform Block {\n"
        "    layout(offset = 16) vec4 b;\n"
        "    layout(align  = 64) vec4 a;\n"
        "} uni_block;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID] + uni_block.b + uni_block.a;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "LAYOUTuniform Block {\n"
        "    layout(offset = 16) vec4 b;\n"
        "    layout(align  = 64) vec4 a;\n"
        "} uni_block;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0] + uni_block.b + uni_block.a;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "LAYOUTuniform Block {\n"
        "    layout(offset = 16) vec4 b;\n"
        "    layout(align  = 64) vec4 a;\n"
        "} uni_block;\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs + uni_block.b + uni_block.a;\n"
        "}\n"
        "\n";

    std::string layout      = "";
    size_t position         = 0;
    testCase &test_case     = m_test_cases[test_case_index];
    const GLchar *qualifier = getQualifierName(test_case.m_qualifier);
    std::string source;

    if (0 != qualifier[0])
    {
        size_t layout_position = 0;

        layout = "layout (QUALIFIER) ";

        Utils::replaceToken("QUALIFIER", layout_position, qualifier, layout);
    }

    switch (stage)
    {
        case Utils::Shader::COMPUTE:
            source = cs;
            break;
        case Utils::Shader::FRAGMENT:
            source = fs;
            break;
        case Utils::Shader::GEOMETRY:
            source = gs;
            break;
        case Utils::Shader::TESS_CTRL:
            source = tcs;
            break;
        case Utils::Shader::TESS_EVAL:
            source = tes;
            break;
        case Utils::Shader::VERTEX:
            source = vs;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    if (test_case.m_stage == stage)
    {
        Utils::replaceToken("LAYOUT", position, layout.c_str(), source);
    }
    else
    {
        Utils::replaceToken("LAYOUT", position, "layout (std140) ", source);
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Qualifier name
 **/
std::string UniformBlockLayoutQualifierConflictTest::getTestCaseName(GLuint test_case_index)
{
    std::string result = getQualifierName(m_test_cases[test_case_index].m_qualifier);

    return result;
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint UniformBlockLayoutQualifierConflictTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param test_case_index Index of test case
 *
 * @return true when tested stage is compute
 **/
bool UniformBlockLayoutQualifierConflictTest::isComputeRelevant(GLuint test_case_index)
{
    return (Utils::Shader::COMPUTE == m_test_cases[test_case_index].m_stage);
}

/** Selects if compilation failure is expected result
 *
 * @param test_case_index Index of test case
 *
 * @return false for STD140 cases, true otherwise
 **/
bool UniformBlockLayoutQualifierConflictTest::isFailureExpected(GLuint test_case_index)
{
    return (STD140 != m_test_cases[test_case_index].m_qualifier);
}

/** Prepare all test cases
 *
 **/
void UniformBlockLayoutQualifierConflictTest::testInit()
{
    testCase test_case = {(QUALIFIERS)m_qualifier, (Utils::Shader::STAGES)m_stage};
    m_test_cases.push_back(test_case);
}

/** Get name of glsl constant
 *
 * @param Constant id
 *
 * @return Name of constant used in GLSL
 **/
const GLchar *UniformBlockLayoutQualifierConflictTest::getQualifierName(QUALIFIERS qualifier)
{
    const GLchar *name = "";

    switch (qualifier)
    {
        case DEFAULT:
            name = "default";
            break;
        case STD140:
            name = "std140";
            break;
        case SHARED:
            name = "shared";
            break;
        case PACKED:
            name = "packed";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return name;
}

/** Constructor
 *
 * @param context Test framework context
 **/
UniformBlockMemberInvalidOffsetAlignmentTest::UniformBlockMemberInvalidOffsetAlignmentTest(
    deqp::Context &context,
    GLuint type,
    GLuint stage)
    : NegativeTestBase(
          context,
          "uniform_block_member_invalid_offset_alignment",
          "Test verifies that invalid alignment of offset qualifiers cause compilation failure"),
      m_type(type),
      m_stage(stage)
{
    std::string name = ("uniform_block_member_invalid_offset_alignment_");
    name.append(getTypeName(type));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Constructor
 *
 * @param context     Test framework context
 * @param name        Test name
 * @param description Test description
 **/
UniformBlockMemberInvalidOffsetAlignmentTest::UniformBlockMemberInvalidOffsetAlignmentTest(
    deqp::Context &context,
    const glw::GLchar *name,
    const glw::GLchar *description,
    GLuint type,
    GLuint stage)
    : NegativeTestBase(context, name, description), m_type(type), m_stage(stage)
{
    /* Nothing to be done here */
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string UniformBlockMemberInvalidOffsetAlignmentTest::getShaderSource(
    GLuint test_case_index,
    Utils::Shader::STAGES stage)
{
    static const GLchar *cs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    layout (offset = OFFSET) TYPE member;\n"
        "} block;\n"
        "\n"
        "writeonly uniform image2D uni_image;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vec4(1, 0, 0.5, 1);\n"
        "\n"
        "    if (TYPE(1) == block.member)\n"
        "    {\n"
        "        result = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
        "}\n"
        "\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    layout (offset = OFFSET) TYPE member;\n"
        "} block;\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(1) == block.member)\n"
        "    {\n"
        "        fs_out = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    fs_out += gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    layout (offset = OFFSET) TYPE member;\n"
        "} block;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(1) == block.member)\n"
        "    {\n"
        "        gs_fs = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    layout (offset = OFFSET) TYPE member;\n"
        "} block;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(1) == block.member)\n"
        "    {\n"
        "        tcs_tes[gl_InvocationID] = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "\n"
        "    tcs_tes[gl_InvocationID] += vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    layout (offset = OFFSET) TYPE member;\n"
        "} block;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(1) == block.member)\n"
        "    {\n"
        "        tes_gs = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    tes_gs += tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    layout (offset = OFFSET) TYPE member;\n"
        "} block;\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(1) == block.member)\n"
        "    {\n"
        "        vs_tcs = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    vs_tcs += in_vs;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        GLchar buffer[16];
        const GLuint offset     = test_case.m_offset;
        size_t position         = 0;
        const Utils::Type &type = test_case.m_type;
        const GLchar *type_name = type.GetGLSLTypeName();

        sprintf(buffer, "%d", offset);

        switch (stage)
        {
            case Utils::Shader::COMPUTE:
                source = cs;
                break;
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("OFFSET", position, buffer, source);
        Utils::replaceToken("TYPE", position, type_name, source);
        Utils::replaceToken("TYPE", position, type_name, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Type name and offset
 **/
std::string UniformBlockMemberInvalidOffsetAlignmentTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Type: " << test_case.m_type.GetGLSLTypeName() << ", offset: " << test_case.m_offset;

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint UniformBlockMemberInvalidOffsetAlignmentTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Get the maximum size for an uniform block
 *
 * @return The maximum size in basic machine units of a uniform block.
 **/
GLint UniformBlockMemberInvalidOffsetAlignmentTest::getMaxBlockSize()
{
    const Functions &gl = m_context.getRenderContext().getFunctions();
    GLint max_size      = 0;

    gl.getIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_size);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    return max_size;
}

/** Selects if "compute" stage is relevant for test
 *
 * @param test_case_index Index of test case
 *
 * @return true when tested stage is compute
 **/
bool UniformBlockMemberInvalidOffsetAlignmentTest::isComputeRelevant(GLuint test_case_index)
{
    return (Utils::Shader::COMPUTE == m_test_cases[test_case_index].m_stage);
}

/** Selects if compilation failure is expected result
 *
 * @param test_case_index Index of test case
 *
 * @return should_fail field from testCase
 **/
bool UniformBlockMemberInvalidOffsetAlignmentTest::isFailureExpected(GLuint test_case_index)
{
    return m_test_cases[test_case_index].m_should_fail;
}

/** Checks if stage is supported
 *
 * @param stage ignored
 *
 * @return true
 **/
bool UniformBlockMemberInvalidOffsetAlignmentTest::isStageSupported(
    Utils::Shader::STAGES /* stage */)
{
    return true;
}

/** Prepare all test cases
 *
 **/
void UniformBlockMemberInvalidOffsetAlignmentTest::testInit()
{
    const Utils::Type &type = getType(m_type);
    const GLuint alignment  = type.GetBaseAlignment(false);
    const GLuint type_size  = type.GetSize(true);
    const GLuint sec_to_end = getMaxBlockSize() - 2 * type_size;

    for (GLuint offset = 0; offset <= type_size; ++offset)
    {
        const GLuint modulo    = offset % alignment;
        const bool is_aligned  = (0 == modulo) ? true : false;
        const bool should_fail = !is_aligned;

        testCase test_case = {offset, should_fail, (Utils::Shader::STAGES)m_stage, type};

        m_test_cases.push_back(test_case);
    }

    for (GLuint offset = sec_to_end; offset <= sec_to_end + type_size; ++offset)
    {
        const GLuint modulo    = offset % alignment;
        const bool is_aligned  = (0 == modulo) ? true : false;
        const bool should_fail = !is_aligned;

        testCase test_case = {offset, should_fail, (Utils::Shader::STAGES)m_stage, type};

        m_test_cases.push_back(test_case);
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
UniformBlockMemberOverlappingOffsetsTest::UniformBlockMemberOverlappingOffsetsTest(
    deqp::Context &context,
    GLuint type_i,
    GLuint type_j)
    : NegativeTestBase(
          context,
          "uniform_block_member_overlapping_offsets",
          "Test verifies that overlapping offsets qualifiers cause compilation failure"),
      m_type_i(type_i),
      m_type_j(type_j)
{
    std::string name = ("uniform_block_member_overlapping_offsets_");
    name.append(getTypeName(type_i));
    name.append("_");
    name.append(getTypeName(type_j));

    NegativeTestBase::m_name = name.c_str();
}

/** Constructor
 *
 * @param context Test framework context
 * @param name        Test name
 * @param description Test description
 **/
UniformBlockMemberOverlappingOffsetsTest::UniformBlockMemberOverlappingOffsetsTest(
    deqp::Context &context,
    const glw::GLchar *name,
    const glw::GLchar *description,
    GLuint type_i,
    GLuint type_j)
    : NegativeTestBase(context, name, description), m_type_i(type_i), m_type_j(type_j)
{
    /* Nothing to be done here */
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string UniformBlockMemberOverlappingOffsetsTest::getShaderSource(GLuint test_case_index,
                                                                      Utils::Shader::STAGES stage)
{
    static const GLchar *cs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    layout (offset = B_OFFSET) B_TYPE b;\n"
        "    layout (offset = A_OFFSET) A_TYPE a;\n"
        "} block;\n"
        "\n"
        "writeonly uniform image2D uni_image;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vec4(1, 0, 0.5, 1);\n"
        "\n"
        "    if ((B_TYPE(1) == block.b) ||\n"
        "        (A_TYPE(0) == block.a) )\n"
        "    {\n"
        "        result = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
        "}\n"
        "\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    layout (offset = B_OFFSET) B_TYPE b;\n"
        "    layout (offset = A_OFFSET) A_TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if ((B_TYPE(1) == block.b) ||\n"
        "        (A_TYPE(0) == block.a) )\n"
        "    {\n"
        "        fs_out = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    fs_out += gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    layout (offset = B_OFFSET) B_TYPE b;\n"
        "    layout (offset = A_OFFSET) A_TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if ((B_TYPE(1) == block.b) ||\n"
        "        (A_TYPE(0) == block.a) )\n"
        "    {\n"
        "        gs_fs = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    layout (offset = B_OFFSET) B_TYPE b;\n"
        "    layout (offset = A_OFFSET) A_TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if ((B_TYPE(1) == block.b) ||\n"
        "        (A_TYPE(0) == block.a) )\n"
        "    {\n"
        "        tcs_tes[gl_InvocationID] = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "\n"
        "    tcs_tes[gl_InvocationID] += vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    layout (offset = B_OFFSET) B_TYPE b;\n"
        "    layout (offset = A_OFFSET) A_TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if ((B_TYPE(1) == block.b) ||\n"
        "        (A_TYPE(0) == block.a) )\n"
        "    {\n"
        "        tes_gs = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    tes_gs += tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    layout (offset = B_OFFSET) B_TYPE b;\n"
        "    layout (offset = A_OFFSET) A_TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if ((B_TYPE(1) == block.b) ||\n"
        "        (A_TYPE(0) == block.a) )\n"
        "    {\n"
        "        vs_tcs = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    vs_tcs += in_vs;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        GLchar buffer[16];
        const GLuint b_offset     = test_case.m_b_offset;
        const Utils::Type &b_type = test_case.m_b_type;
        const GLchar *b_type_name = b_type.GetGLSLTypeName();
        const GLuint a_offset     = test_case.m_a_offset;
        const Utils::Type &a_type = test_case.m_a_type;
        const GLchar *a_type_name = a_type.GetGLSLTypeName();
        size_t position           = 0;

        switch (stage)
        {
            case Utils::Shader::COMPUTE:
                source = cs;
                break;
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        sprintf(buffer, "%d", b_offset);
        Utils::replaceToken("B_OFFSET", position, buffer, source);
        Utils::replaceToken("B_TYPE", position, b_type_name, source);
        sprintf(buffer, "%d", a_offset);
        Utils::replaceToken("A_OFFSET", position, buffer, source);
        Utils::replaceToken("A_TYPE", position, a_type_name, source);
        Utils::replaceToken("B_TYPE", position, b_type_name, source);
        Utils::replaceToken("A_TYPE", position, a_type_name, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Type name and offset
 **/
std::string UniformBlockMemberOverlappingOffsetsTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Type: " << test_case.m_b_type.GetGLSLTypeName()
           << ", offset: " << test_case.m_b_offset
           << ". Type: " << test_case.m_a_type.GetGLSLTypeName()
           << ", offset: " << test_case.m_a_offset;

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint UniformBlockMemberOverlappingOffsetsTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param test_case_index Index of test case
 *
 * @return true when tested stage is compute
 **/
bool UniformBlockMemberOverlappingOffsetsTest::isComputeRelevant(GLuint test_case_index)
{
    return (Utils::Shader::COMPUTE == m_test_cases[test_case_index].m_stage);
}

/** Checks if stage is supported
 *
 * @param stage ignored
 *
 * @return true
 **/
bool UniformBlockMemberOverlappingOffsetsTest::isStageSupported(Utils::Shader::STAGES /* stage */)
{
    return true;
}

/** Prepare all test cases
 *
 **/
void UniformBlockMemberOverlappingOffsetsTest::testInit()
{
    bool stage_support[Utils::Shader::STAGE_MAX];

    for (GLuint stage = 0; stage < Utils::Shader::STAGE_MAX; ++stage)
    {
        stage_support[stage] = isStageSupported((Utils::Shader::STAGES)stage);
    }

    const Utils::Type &b_type = getType(m_type_i);
    const GLuint b_size       = b_type.GetActualAlignment(1 /* align */, false /* is_array*/);

    const Utils::Type &a_type = getType(m_type_j);
    const GLuint a_align      = a_type.GetBaseAlignment(false);
    const GLuint a_size       = a_type.GetActualAlignment(1 /* align */, false /* is_array*/);

    const GLuint b_offset       = lcm(b_size, a_size);
    const GLuint a_after_start  = b_offset + 1;
    const GLuint a_after_off    = a_type.GetActualOffset(a_after_start, a_size);
    const GLuint a_before_start = b_offset - a_align;
    const GLuint a_before_off   = a_type.GetActualOffset(a_before_start, a_size);

    for (GLuint stage = 0; stage < Utils::Shader::STAGE_MAX; ++stage)
    {
        if (false == stage_support[stage])
        {
            continue;
        }

        if ((b_offset > a_before_off) && (b_offset < a_before_off + a_size))
        {
            testCase test_case = {b_offset, b_type, a_before_off, a_type,
                                  (Utils::Shader::STAGES)stage};

            m_test_cases.push_back(test_case);
        }

        if ((b_offset < a_after_off) && (b_offset + b_size > a_after_off))
        {
            testCase test_case = {b_offset, b_type, a_after_off, a_type,
                                  (Utils::Shader::STAGES)stage};

            m_test_cases.push_back(test_case);
        }

        /* b offset, should be fine for both types */
        testCase test_case = {b_offset, b_type, b_offset, a_type, (Utils::Shader::STAGES)stage};

        m_test_cases.push_back(test_case);
    }
}

/** Find greatest common divisor for a and b
 *
 * @param a A argument
 * @param b B argument
 *
 * @return Found gcd value
 **/
GLuint UniformBlockMemberOverlappingOffsetsTest::gcd(GLuint a, GLuint b)
{
    if ((0 != a) && (0 == b))
    {
        return a;
    }
    else
    {
        GLuint greater = std::max(a, b);
        GLuint lesser  = std::min(a, b);

        return gcd(lesser, greater % lesser);
    }
}

/** Find lowest common multiple for a and b
 *
 * @param a A argument
 * @param b B argument
 *
 * @return Found gcd value
 **/
GLuint UniformBlockMemberOverlappingOffsetsTest::lcm(GLuint a, GLuint b)
{
    return (a * b) / gcd(a, b);
}

/** Constructor
 *
 * @param context Test framework context
 **/
UniformBlockMemberAlignNonPowerOf2Test::UniformBlockMemberAlignNonPowerOf2Test(
    deqp::Context &context,
    glw::GLuint type)
    : NegativeTestBase(context,
                       "uniform_block_member_align_non_power_of_2",
                       "Test verifies that align qualifier requires value that is a power of 2"),
      m_type(type)
{
    std::string name = ("uniform_block_member_align_non_power_of_2_");
    name.append(getTypeName(type));

    NegativeTestBase::m_name = name.c_str();
}

/** Constructor
 *
 * @param context Test framework context
 * @param name        Test name
 * @param description Test description
 **/
UniformBlockMemberAlignNonPowerOf2Test::UniformBlockMemberAlignNonPowerOf2Test(
    deqp::Context &context,
    const glw::GLchar *name,
    const glw::GLchar *description,
    glw::GLuint type)
    : NegativeTestBase(context, name, description), m_type(type)
{
    /* Nothing to be done here */
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string UniformBlockMemberAlignNonPowerOf2Test::getShaderSource(GLuint test_case_index,
                                                                    Utils::Shader::STAGES stage)
{
    static const GLchar *cs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    vec4 b;\n"
        "    layout (align = ALIGN) TYPE a;\n"
        "} block;\n"
        "\n"
        "writeonly uniform image2D uni_image;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vec4(1, 0, 0.5, 1);\n"
        "\n"
        "    if (TYPE(0) == block.a)\n"
        "    {\n"
        "        result = vec4(1, 1, 1, 1) - block.b;\n"
        "    }\n"
        "\n"
        "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
        "}\n"
        "\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    vec4 b;\n"
        "    layout (align = ALIGN) TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(0) == block.a)\n"
        "    {\n"
        "        fs_out = block.b;\n"
        "    }\n"
        "\n"
        "    fs_out += gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    vec4 b;\n"
        "    layout (align = ALIGN) TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(0) == block.a)\n"
        "    {\n"
        "        gs_fs = block.b;\n"
        "    }\n"
        "\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    vec4 b;\n"
        "    layout (align = ALIGN) TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(0) == block.a)\n"
        "    {\n"
        "        tcs_tes[gl_InvocationID] = block.b;\n"
        "    }\n"
        "\n"
        "\n"
        "    tcs_tes[gl_InvocationID] += vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    vec4 b;\n"
        "    layout (align = ALIGN) TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(0) == block.a)\n"
        "    {\n"
        "        tes_gs = block.b;\n"
        "    }\n"
        "\n"
        "    tes_gs += tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (std140) uniform Block {\n"
        "    vec4 b;\n"
        "    layout (align = ALIGN) TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(0) == block.a)\n"
        "    {\n"
        "        vs_tcs = block.b;\n"
        "    }\n"
        "\n"
        "    vs_tcs += in_vs;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        GLchar buffer[16];
        const GLuint alignment  = test_case.m_alignment;
        const Utils::Type &type = test_case.m_type;
        const GLchar *type_name = type.GetGLSLTypeName();
        size_t position         = 0;

        switch (stage)
        {
            case Utils::Shader::COMPUTE:
                source = cs;
                break;
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        sprintf(buffer, "%d", alignment);
        Utils::replaceToken("ALIGN", position, buffer, source);
        Utils::replaceToken("TYPE", position, type_name, source);
        Utils::replaceToken("TYPE", position, type_name, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Type name and offset
 **/
std::string UniformBlockMemberAlignNonPowerOf2Test::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Type: " << test_case.m_type.GetGLSLTypeName()
           << ", align: " << test_case.m_alignment;

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint UniformBlockMemberAlignNonPowerOf2Test::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param test_case_index Index of test case
 *
 * @return true when tested stage is compute
 **/
bool UniformBlockMemberAlignNonPowerOf2Test::isComputeRelevant(GLuint test_case_index)
{
    return (Utils::Shader::COMPUTE == m_test_cases[test_case_index].m_stage);
}

/** Checks if stage is supported
 *
 * @param ignored
 *
 * @return true
 **/
bool UniformBlockMemberAlignNonPowerOf2Test::isStageSupported(Utils::Shader::STAGES /* stage */)
{
    return true;
}

/** Selects if compilation failure is expected result
 *
 * @param test_case_index Index of test case
 *
 * @return should_fail field from testCase
 **/
bool UniformBlockMemberAlignNonPowerOf2Test::isFailureExpected(GLuint test_case_index)
{
    return m_test_cases[test_case_index].m_should_fail;
}

/** Prepare all test cases
 *
 **/
void UniformBlockMemberAlignNonPowerOf2Test::testInit()
{
    static const GLuint dmat4_size = 128;
    bool stage_support[Utils::Shader::STAGE_MAX];

    for (GLuint stage = 0; stage < Utils::Shader::STAGE_MAX; ++stage)
    {
        stage_support[stage] = isStageSupported((Utils::Shader::STAGES)stage);
    }

    const Utils::Type &type = getType(m_type);

    for (GLuint align = 0; align <= dmat4_size; ++align)
    {

#if WRKARD_UNIFORMBLOCKMEMBERALIGNNONPOWEROF2TEST

        const bool should_fail = (0 == align) ? false : !isPowerOf2(align);

#else /* WRKARD_UNIFORMBLOCKMEMBERALIGNNONPOWEROF2TEST */

        const bool should_fail = !isPowerOf2(align);

#endif /* WRKARD_UNIFORMBLOCKMEMBERALIGNNONPOWEROF2TEST */

        for (GLuint stage = 0; stage < Utils::Shader::STAGE_MAX; ++stage)
        {
            if (false == stage_support[stage])
            {
                continue;
            }

            testCase test_case = {align, type, should_fail, (Utils::Shader::STAGES)stage};

            m_test_cases.push_back(test_case);
        }
    }
}

/** Check if value is power of 2
 *
 * @param val Tested value
 *
 * @return true if val is power of 2, false otherwise
 **/
bool UniformBlockMemberAlignNonPowerOf2Test::isPowerOf2(GLuint val)
{
    if (0 == val)
    {
        return false;
    }

    return (0 == (val & (val - 1)));
}

/** Constructor
 *
 * @param context Test framework context
 **/
UniformBlockAlignmentTest::UniformBlockAlignmentTest(deqp::Context &context)
    : TextureTestBase(context,
                      "uniform_block_alignment",
                      "Test verifies offset and alignment of uniform buffer")
{}

/** Get interface of program
 *
 * @param ignored
 * @param program_interface Interface of program
 * @param varying_passthrough Collection of connections between in and out variables
 **/
void UniformBlockAlignmentTest::getProgramInterface(GLuint /* test_case_index */,
                                                    Utils::ProgramInterface &program_interface,
                                                    Utils::VaryingPassthrough &varying_passthrough)
{
    static const Utils::Type vec4 = Utils::Type::vec4;

#if WRKARD_UNIFORMBLOCKALIGNMENT

    static const GLuint block_align = 16;

#else /* WRKARD_UNIFORMBLOCKALIGNMENT */

    static const GLuint block_align = 64;

#endif /* WRKARD_UNIFORMBLOCKALIGNMENT */

    static const GLuint vec4_stride = 16;
    static const GLuint data_stride = vec4_stride * 2; /* one vec4 + one scalar aligned to 16 */

    /*Fixed a test issue, the fifth_offset should be calculated by block_align, instead of
     fifth_align, according to spec, the actual alignment of a member will be the greater of the
     specified alignment and the base aligment for the member type
     */
    const GLuint first_offset = 0; /* vec4 at 0 */
    const GLuint second_offset =
        Utils::Type::GetActualOffset(first_offset + vec4_stride, block_align); /* Data at 32 */
    const GLuint third_offset =
        Utils::Type::GetActualOffset(second_offset + data_stride, block_align); /* Data[2] at 64 */
    const GLuint fourth_offset = Utils::Type::GetActualOffset(third_offset + data_stride * 2,
                                                              block_align); /* vec4[3] at 96 */
    const GLuint fifth_offset  = Utils::Type::GetActualOffset(fourth_offset + vec4_stride * 3,
                                                              block_align); /* vec4[2] at 160 */
    const GLuint sixth_offset =
        Utils::Type::GetActualOffset(fifth_offset + vec4_stride * 2, block_align); /* Data at 192 */

    Utils::Interface *structure = program_interface.Structure("Data");

    structure->Member("vector", "", 0 /* expected_component */, 0 /* expected_location */,
                      Utils::Type::vec4, false /* normalized */, 0 /* n_array_elements */,
                      Utils::Type::vec4.GetSize(), 0 /* offset */);

    structure->Member("scalar", "", 0 /* expected_component */, 0 /* expected_location */,
                      Utils::Type::_float, false /* normalized */, 0 /* n_array_elements */,
                      Utils::Type::_float.GetSize(), Utils::Type::vec4.GetSize() /* offset */);

    /* Prepare Block */
    Utils::Interface *vs_uni_block = program_interface.Block("vs_uni_Block");

    vs_uni_block->Member("first", "", 0 /* expected_component */, 0 /* expected_location */,
                         Utils::Type::vec4, false /* normalized */, 0 /* n_array_elements */,
                         vec4_stride, first_offset /* offset */);

    vs_uni_block->Member("second", "", 0 /* expected_component */, 0 /* expected_location */,
                         structure, 0 /* n_array_elements */, data_stride, second_offset);

    vs_uni_block->Member("third", "", 0 /* expected_component */, 0 /* expected_location */,
                         structure, 2 /* n_array_elements */, data_stride, third_offset);

    vs_uni_block->Member("fourth", "", 0 /* expected_component */, 0 /* expected_location */, vec4,
                         false /* normalized */, 3 /* n_array_elements */, vec4_stride,
                         fourth_offset);

    vs_uni_block->Member("fifth", "layout(align = 64)", 0 /* expected_component */,
                         0 /* expected_location */, vec4, false /* normalized */,
                         2 /* n_array_elements */, vec4_stride, fifth_offset);

    vs_uni_block->Member("sixth", "", 0 /* expected_component */, 0 /* expected_location */,
                         structure, 0 /* n_array_elements */, data_stride, sixth_offset);

    const GLuint stride = calculateStride(*vs_uni_block);
    m_data.resize(stride);
    generateData(*vs_uni_block, 0, m_data);

    Utils::ShaderInterface &vs_si = program_interface.GetShaderInterface(Utils::Shader::VERTEX);

/* Add uniform BLOCK */
#if WRKARD_UNIFORMBLOCKALIGNMENT
    vs_si.Uniform("vs_uni_block", "layout (std140, binding = BINDING)", 0, 0, vs_uni_block, 0,
                  static_cast<GLuint>(m_data.size()), 0, &m_data[0], m_data.size());
#else  /* WRKARD_UNIFORMBLOCKALIGNMENT */
    vs_si.Uniform("vs_uni_block", "layout (std140, binding = BINDING, align = 64)", 0, 0,
                  vs_uni_block, 0, static_cast<GLuint>(m_data.size()), 0, &m_data[0],
                  m_data.size());
#endif /* WRKARD_UNIFORMBLOCKALIGNMENT */

    program_interface.CloneVertexInterface(varying_passthrough);
}

/** Constructor
 *
 * @param context Test framework context
 **/
SSBMemberOffsetAndAlignTest::SSBMemberOffsetAndAlignTest(deqp::Context &context)
    : TextureTestBase(context,
                      "ssb_member_offset_and_align",
                      "Test verifies offsets and alignment of storage buffer members")
{}

/** Get interface of program
 *
 * @param test_case_index     Test case index
 * @param program_interface   Interface of program
 * @param varying_passthrough Collection of connections between in and out variables
 **/
void SSBMemberOffsetAndAlignTest::getProgramInterface(
    GLuint test_case_index,
    Utils::ProgramInterface &program_interface,
    Utils::VaryingPassthrough &varying_passthrough)
{
    std::string globals =
        "const int basic_size = BASIC_SIZE;\n"
        "const int type_align = TYPE_ALIGN;\n"
        "const int type_size  = TYPE_SIZE;\n";

    Utils::Type type         = getType(test_case_index);
    GLuint basic_size        = Utils::Type::GetTypeSize(type.m_basic_type);
    const GLuint base_align  = type.GetBaseAlignment(false);
    const GLuint array_align = type.GetBaseAlignment(true);
    const GLuint base_stride = Utils::Type::CalculateStd140Stride(base_align, type.m_n_columns, 0);
    const GLuint type_align  = Utils::roundUpToPowerOf2(base_stride);

    /* Calculate offsets */
    const GLuint first_offset  = 0;
    const GLuint second_offset = type.GetActualOffset(base_stride, basic_size / 2);

#if WRKARD_UNIFORMBLOCKMEMBEROFFSETANDALIGNTEST

    const GLuint third_offset   = type.GetActualOffset(second_offset + base_stride, base_align);
    const GLuint fourth_offset  = type.GetActualOffset(third_offset + base_stride, base_align);
    const GLuint fifth_offset   = type.GetActualOffset(fourth_offset + base_stride, base_align);
    const GLuint sixth_offset   = type.GetActualOffset(fifth_offset + base_stride, array_align);
    const GLuint seventh_offset = type.GetActualOffset(sixth_offset + base_stride, array_align);
    const GLuint eigth_offset   = type.GetActualOffset(seventh_offset + base_stride, array_align);

#else /* WRKARD_UNIFORMBLOCKMEMBEROFFSETANDALIGNTEST */

    const GLuint third_offset   = type.GetActualOffset(second_offset + base_stride, 2 * type_align);
    const GLuint fourth_offset  = type.GetActualOffset(3 * type_align + base_stride, base_align);
    const GLuint fifth_offset   = type.GetActualOffset(fourth_offset + base_stride, base_align);
    const GLuint sixth_offset   = type.GetActualOffset(fifth_offset + base_stride, array_align);
    const GLuint seventh_offset = type.GetActualOffset(sixth_offset + base_stride, array_align);
    const GLuint eigth_offset = type.GetActualOffset(seventh_offset + base_stride, 8 * basic_size);

#endif /* WRKARD_UNIFORMBLOCKMEMBEROFFSETANDALIGNTEST */

    /* Prepare data */
    const std::vector<GLubyte> &first  = type.GenerateData();
    const std::vector<GLubyte> &second = type.GenerateData();
    const std::vector<GLubyte> &third  = type.GenerateData();
    const std::vector<GLubyte> &fourth = type.GenerateData();

    m_data.resize(eigth_offset + base_stride);
    GLubyte *ptr = &m_data[0];
    memcpy(ptr + first_offset, &first[0], first.size());
    memcpy(ptr + second_offset, &second[0], second.size());
    memcpy(ptr + third_offset, &third[0], third.size());
    memcpy(ptr + fourth_offset, &fourth[0], fourth.size());
    memcpy(ptr + fifth_offset, &fourth[0], fourth.size());
    memcpy(ptr + sixth_offset, &third[0], third.size());
    memcpy(ptr + seventh_offset, &second[0], second.size());
    memcpy(ptr + eigth_offset, &first[0], first.size());

    /* Prepare globals */
    size_t position = 0;
    GLchar buffer[16];

    sprintf(buffer, "%d", basic_size);
    Utils::replaceToken("BASIC_SIZE", position, buffer, globals);

    sprintf(buffer, "%d", type_align);
    Utils::replaceToken("TYPE_ALIGN", position, buffer, globals);

    sprintf(buffer, "%d", base_stride);
    Utils::replaceToken("TYPE_SIZE", position, buffer, globals);

    /* Prepare Block */
    Utils::Interface *vs_buf_block = program_interface.Block("vs_buf_Block");

    vs_buf_block->Member("at_first_offset", "layout(offset = 0, align = 8 * basic_size)",
                         0 /* expected_component */, 0 /* expected_location */, type,
                         false /* normalized */, 0 /* n_array_elements */, base_stride,
                         first_offset);

    vs_buf_block->Member("at_second_offset", "layout(offset = type_size, align = basic_size / 2)",
                         0 /* expected_component */, 0 /* expected_location */, type,
                         false /* normalized */, 0 /* n_array_elements */, base_stride,
                         second_offset);

    vs_buf_block->Member("at_third_offset", "layout(align = 2 * type_align)",
                         0 /* expected_component */, 0 /* expected_location */, type,
                         false /* normalized */, 0 /* n_array_elements */, base_stride,
                         third_offset);

    vs_buf_block->Member("at_fourth_offset", "layout(offset = 3 * type_align + type_size)",
                         0 /* expected_component */, 0 /* expected_location */, type,
                         false /* normalized */, 0 /* n_array_elements */, base_stride,
                         fourth_offset);

    vs_buf_block->Member("at_fifth_offset", "", 0 /* expected_component */,
                         0 /* expected_location */, type, false /* normalized */,
                         0 /* n_array_elements */, base_stride, fifth_offset);

    vs_buf_block->Member("at_sixth_offset", "", 0 /* expected_component */,
                         0 /* expected_location */, type, false /* normalized */,
                         2 /* n_array_elements */, array_align * 2, sixth_offset);

    vs_buf_block->Member("at_eigth_offset", "layout(align = 8 * basic_size)",
                         0 /* expected_component */, 0 /* expected_location */, type,
                         false /* normalized */, 0 /* n_array_elements */, base_stride,
                         eigth_offset);

    Utils::ShaderInterface &vs_si = program_interface.GetShaderInterface(Utils::Shader::VERTEX);

    /* Add globals */
    vs_si.m_globals = globals;

    /* Add uniform BLOCK */
    vs_si.SSB("vs_buf_block", "layout (std140, binding = BINDING)", 0, 0, vs_buf_block, 0,
              static_cast<GLuint>(m_data.size()), 0, &m_data[0], m_data.size());

    /* */
    program_interface.CloneVertexInterface(varying_passthrough);
}

/** Get type name
 *
 * @param test_case_index Index of test case
 *
 * @return Name of type test in test_case_index
 **/
std::string SSBMemberOffsetAndAlignTest::getTestCaseName(glw::GLuint test_case_index)
{
    return getTypeName(test_case_index);
}

/** Returns number of types to test
 *
 * @return Number of types, 34
 **/
glw::GLuint SSBMemberOffsetAndAlignTest::getTestCaseNumber()
{
    return getTypesNumber();
}

/** Prepare code snippet that will verify in and uniform variables
 *
 * @param ignored
 * @param ignored
 * @param stage   Shader stage
 *
 * @return Code that verify variables
 **/
std::string SSBMemberOffsetAndAlignTest::getVerificationSnippet(
    GLuint /* test_case_index */,
    Utils::ProgramInterface & /* program_interface */,
    Utils::Shader::STAGES stage)
{
    std::string verification =
        "if ( (PREFIXblock.at_first_offset  != PREFIXblock.at_eigth_offset   ) ||\n"
        "         (PREFIXblock.at_second_offset != PREFIXblock.at_sixth_offset[1]) ||\n"
        "         (PREFIXblock.at_third_offset  != PREFIXblock.at_sixth_offset[0]) ||\n"
        "         (PREFIXblock.at_fourth_offset != PREFIXblock.at_fifth_offset   )  )\n"
        "    {\n"
        "        result = 0;\n"
        "    }";

    const GLchar *prefix = Utils::ProgramInterface::GetStagePrefix(stage, Utils::Variable::SSB);

    Utils::replaceAllTokens("PREFIX", prefix, verification);

    return verification;
}

/** Selects if "draw" stages are relevant for test
 *
 * @param ignored
 *
 * @return true if all stages support shader storage buffers, false otherwise
 **/
bool SSBMemberOffsetAndAlignTest::isDrawRelevant(GLuint /* test_case_index */)
{
    const Functions &gl         = m_context.getRenderContext().getFunctions();
    GLint gs_supported_buffers  = 0;
    GLint tcs_supported_buffers = 0;
    GLint tes_supported_buffers = 0;
    GLint vs_supported_buffers  = 0;

    gl.getIntegerv(GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, &gs_supported_buffers);
    gl.getIntegerv(GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS, &tcs_supported_buffers);
    gl.getIntegerv(GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS, &tes_supported_buffers);
    gl.getIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &vs_supported_buffers);

    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    return ((1 <= gs_supported_buffers) && (1 <= tcs_supported_buffers) &&
            (1 <= tes_supported_buffers) && (1 <= vs_supported_buffers));
}

/** Constructor
 *
 * @param context Test framework context
 **/
SSBLayoutQualifierConflictTest::SSBLayoutQualifierConflictTest(deqp::Context &context,
                                                               GLuint qualifier,
                                                               GLuint stage)
    : NegativeTestBase(context,
                       "ssb_layout_qualifier_conflict",
                       "Test verifies that std140 or std430 is required when "
                       "offset and/or align qualifiers are used with storage "
                       "block"),
      m_qualifier(qualifier),
      m_stage(stage)
{
    std::string name = ("ssb_layout_qualifier_conflict_");
    name.append(EnhancedLayouts::SSBLayoutQualifierConflictTest::getQualifierName(
        (EnhancedLayouts::SSBLayoutQualifierConflictTest::QUALIFIERS)qualifier));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string SSBLayoutQualifierConflictTest::getShaderSource(GLuint test_case_index,
                                                            Utils::Shader::STAGES stage)
{
    static const GLchar *cs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
        "\n"
        "layout (QUALIFIERbinding = BINDING) buffer cs_Block {\n"
        "    layout(offset = 16) vec4 b;\n"
        "    layout(align  = 64) vec4 a;\n"
        "} uni_block;\n"
        "\n"
        "writeonly uniform image2D uni_image;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = uni_block.b + uni_block.a;\n"
        "\n"
        "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
        "}\n"
        "\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (QUALIFIERbinding = BINDING) buffer Block {\n"
        "    layout(offset = 16) vec4 b;\n"
        "    layout(align  = 64) vec4 a;\n"
        "} uni_block;\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs + uni_block.b + uni_block.a;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "layout (QUALIFIERbinding = BINDING) buffer gs_Block {\n"
        "    layout(offset = 16) vec4 b;\n"
        "    layout(align  = 64) vec4 a;\n"
        "} uni_block;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0] + uni_block.b + uni_block.a;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0] + uni_block.b + uni_block.a;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0] + uni_block.b + uni_block.a;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0] + uni_block.b + uni_block.a;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "layout (QUALIFIERbinding = BINDING) buffer tcs_Block {\n"
        "    layout(offset = 16) vec4 b;\n"
        "    layout(align  = 64) vec4 a;\n"
        "} uni_block;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID] + uni_block.b + uni_block.a;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "layout (QUALIFIERbinding = BINDING) buffer tes_Block {\n"
        "    layout(offset = 16) vec4 b;\n"
        "    layout(align  = 64) vec4 a;\n"
        "} uni_block;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0] + uni_block.b + uni_block.a;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (QUALIFIERbinding = BINDING) buffer vs_Block {\n"
        "    layout(offset = 16) vec4 b;\n"
        "    layout(align  = 64) vec4 a;\n"
        "} uni_block;\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs + uni_block.b + uni_block.a;\n"
        "}\n"
        "\n";

    GLchar buffer[16];
    size_t position = 0;
    std::string source;
    testCase &test_case   = m_test_cases[test_case_index];
    std::string qualifier = getQualifierName(test_case.m_qualifier);

    if (false == qualifier.empty())
    {
        qualifier.append(", ");
    }

    sprintf(buffer, "%d", stage);

    switch (stage)
    {
        case Utils::Shader::COMPUTE:
            source = cs;
            break;
        case Utils::Shader::FRAGMENT:
            source = fs;
            break;
        case Utils::Shader::GEOMETRY:
            source = gs;
            break;
        case Utils::Shader::TESS_CTRL:
            source = tcs;
            break;
        case Utils::Shader::TESS_EVAL:
            source = tes;
            break;
        case Utils::Shader::VERTEX:
            source = vs;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    if (test_case.m_stage == stage)
    {
        Utils::replaceToken("QUALIFIER", position, qualifier.c_str(), source);
    }
    else
    {
        Utils::replaceToken("QUALIFIER", position, "std140, ", source);
    }

    Utils::replaceToken("BINDING", position, buffer, source);

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Qualifier name
 **/
std::string SSBLayoutQualifierConflictTest::getTestCaseName(GLuint test_case_index)
{
    std::string result = getQualifierName(m_test_cases[test_case_index].m_qualifier);

    return result;
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint SSBLayoutQualifierConflictTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param test_case_index Index of test case
 *
 * @return true when tested stage is compute
 **/
bool SSBLayoutQualifierConflictTest::isComputeRelevant(GLuint test_case_index)
{
    return (Utils::Shader::COMPUTE == m_test_cases[test_case_index].m_stage);
}

/** Selects if compilation failure is expected result
 *
 * @param test_case_index Index of test case
 *
 * @return false for STD140 and STD430 cases, true otherwise
 **/
bool SSBLayoutQualifierConflictTest::isFailureExpected(GLuint test_case_index)
{
    const QUALIFIERS qualifier = m_test_cases[test_case_index].m_qualifier;

    return !((STD140 == qualifier) || (STD430 == qualifier));
}

/** Checks if stage is supported
 *
 * @param stage Shader stage
 *
 * @return true if supported, false otherwise
 **/
bool SSBLayoutQualifierConflictTest::isStageSupported(Utils::Shader::STAGES stage)
{
    const Functions &gl         = m_context.getRenderContext().getFunctions();
    GLint max_supported_buffers = 0;
    GLenum pname                = 0;

    switch (stage)
    {
        case Utils::Shader::COMPUTE:
            pname = GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::FRAGMENT:
            pname = GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::GEOMETRY:
            pname = GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::TESS_CTRL:
            pname = GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::TESS_EVAL:
            pname = GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::VERTEX:
            pname = GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    gl.getIntegerv(pname, &max_supported_buffers);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    return 1 <= max_supported_buffers;
}

/** Prepare all test cases
 *
 **/
void SSBLayoutQualifierConflictTest::testInit()
{
    testCase test_case = {(QUALIFIERS)m_qualifier, (Utils::Shader::STAGES)m_stage};

    m_test_cases.push_back(test_case);
}

/** Get name of glsl constant
 *
 * @param Constant id
 *
 * @return Name of constant used in GLSL
 **/
const GLchar *SSBLayoutQualifierConflictTest::getQualifierName(QUALIFIERS qualifier)
{
    const GLchar *name = "";

    switch (qualifier)
    {
        case DEFAULT:
            name = "";
            break;
        case STD140:
            name = "std140";
            break;
        case STD430:
            name = "std430";
            break;
        case SHARED:
            name = "shared";
            break;
        case PACKED:
            name = "packed";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return name;
}

/** Constructor
 *
 * @param context Test framework context
 **/
SSBMemberInvalidOffsetAlignmentTest::SSBMemberInvalidOffsetAlignmentTest(deqp::Context &context,
                                                                         GLuint type,
                                                                         GLuint stage)
    : UniformBlockMemberInvalidOffsetAlignmentTest(
          context,
          "ssb_member_invalid_offset_alignment",
          "Test verifies that invalid alignment of offset qualifiers cause compilation failure",
          type,
          stage)
{
    std::string name = ("ssb_member_invalid_offset_alignment_");
    name.append(getTypeName(type));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Get the maximum size for a shader storage block
 *
 * @return The maximum size in basic machine units of a shader storage block.
 **/
GLint SSBMemberInvalidOffsetAlignmentTest::getMaxBlockSize()
{
    const Functions &gl = m_context.getRenderContext().getFunctions();
    GLint max_size      = 0;

    gl.getIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &max_size);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    return max_size;
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string SSBMemberInvalidOffsetAlignmentTest::getShaderSource(GLuint test_case_index,
                                                                 Utils::Shader::STAGES stage)
{
    static const GLchar *cs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    layout (offset = OFFSET) TYPE member;\n"
        "} block;\n"
        "\n"
        "writeonly uniform image2D uni_image;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vec4(1, 0, 0.5, 1);\n"
        "\n"
        "    if (TYPE(1) == block.member)\n"
        "    {\n"
        "        result = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
        "}\n"
        "\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    layout (offset = OFFSET) TYPE member;\n"
        "} block;\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(1) == block.member)\n"
        "    {\n"
        "        fs_out = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    fs_out += gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    layout (offset = OFFSET) TYPE member;\n"
        "} block;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(1) == block.member)\n"
        "    {\n"
        "        gs_fs = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    layout (offset = OFFSET) TYPE member;\n"
        "} block;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(1) == block.member)\n"
        "    {\n"
        "        tcs_tes[gl_InvocationID] = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "\n"
        "    tcs_tes[gl_InvocationID] += vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    layout (offset = OFFSET) TYPE member;\n"
        "} block;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(1) == block.member)\n"
        "    {\n"
        "        tes_gs = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    tes_gs += tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    layout (offset = OFFSET) TYPE member;\n"
        "} block;\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(1) == block.member)\n"
        "    {\n"
        "        vs_tcs = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    vs_tcs += in_vs;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        GLchar buffer[16];
        const GLuint offset     = test_case.m_offset;
        size_t position         = 0;
        const Utils::Type &type = test_case.m_type;
        const GLchar *type_name = type.GetGLSLTypeName();

        sprintf(buffer, "%d", offset);

        switch (stage)
        {
            case Utils::Shader::COMPUTE:
                source = cs;
                break;
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("OFFSET", position, buffer, source);
        Utils::replaceToken("TYPE", position, type_name, source);
        Utils::replaceToken("TYPE", position, type_name, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Checks if stage is supported
 *
 * @param stage Shader stage
 *
 * @return true if supported, false otherwise
 **/
bool SSBMemberInvalidOffsetAlignmentTest::isStageSupported(Utils::Shader::STAGES stage)
{
    const Functions &gl         = m_context.getRenderContext().getFunctions();
    GLint max_supported_buffers = 0;
    GLenum pname                = 0;

    switch (stage)
    {
        case Utils::Shader::COMPUTE:
            pname = GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::FRAGMENT:
            pname = GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::GEOMETRY:
            pname = GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::TESS_CTRL:
            pname = GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::TESS_EVAL:
            pname = GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::VERTEX:
            pname = GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    gl.getIntegerv(pname, &max_supported_buffers);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    return 1 <= max_supported_buffers;
}

/** Constructor
 *
 * @param context Test framework context
 **/
SSBMemberOverlappingOffsetsTest::SSBMemberOverlappingOffsetsTest(deqp::Context &context,
                                                                 GLuint type_i,
                                                                 GLuint type_j)
    : UniformBlockMemberOverlappingOffsetsTest(
          context,
          "ssb_member_overlapping_offsets",
          "Test verifies that overlapping offsets qualifiers cause compilation failure",
          type_i,
          type_j)
{
    std::string name = ("ssb_member_overlapping_offsets_");
    name.append(getTypeName(type_i));
    name.append("_");
    name.append(getTypeName(type_j));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string SSBMemberOverlappingOffsetsTest::getShaderSource(GLuint test_case_index,
                                                             Utils::Shader::STAGES stage)
{
    static const GLchar *cs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    layout (offset = B_OFFSET) B_TYPE b;\n"
        "    layout (offset = A_OFFSET) A_TYPE a;\n"
        "} block;\n"
        "\n"
        "writeonly uniform image2D uni_image;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vec4(1, 0, 0.5, 1);\n"
        "\n"
        "    if ((B_TYPE(1) == block.b) ||\n"
        "        (A_TYPE(0) == block.a) )\n"
        "    {\n"
        "        result = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
        "}\n"
        "\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    layout (offset = B_OFFSET) B_TYPE b;\n"
        "    layout (offset = A_OFFSET) A_TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if ((B_TYPE(1) == block.b) ||\n"
        "        (A_TYPE(0) == block.a) )\n"
        "    {\n"
        "        fs_out = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    fs_out += gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    layout (offset = B_OFFSET) B_TYPE b;\n"
        "    layout (offset = A_OFFSET) A_TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if ((B_TYPE(1) == block.b) ||\n"
        "        (A_TYPE(0) == block.a) )\n"
        "    {\n"
        "        gs_fs = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    layout (offset = B_OFFSET) B_TYPE b;\n"
        "    layout (offset = A_OFFSET) A_TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if ((B_TYPE(1) == block.b) ||\n"
        "        (A_TYPE(0) == block.a) )\n"
        "    {\n"
        "        tcs_tes[gl_InvocationID] = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "\n"
        "    tcs_tes[gl_InvocationID] += vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    layout (offset = B_OFFSET) B_TYPE b;\n"
        "    layout (offset = A_OFFSET) A_TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if ((B_TYPE(1) == block.b) ||\n"
        "        (A_TYPE(0) == block.a) )\n"
        "    {\n"
        "        tes_gs = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    tes_gs += tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    layout (offset = B_OFFSET) B_TYPE b;\n"
        "    layout (offset = A_OFFSET) A_TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if ((B_TYPE(1) == block.b) ||\n"
        "        (A_TYPE(0) == block.a) )\n"
        "    {\n"
        "        vs_tcs = vec4(1, 1, 1, 1);\n"
        "    }\n"
        "\n"
        "    vs_tcs += in_vs;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        GLchar buffer[16];
        const GLuint b_offset     = test_case.m_b_offset;
        const Utils::Type &b_type = test_case.m_b_type;
        const GLchar *b_type_name = b_type.GetGLSLTypeName();
        const GLuint a_offset     = test_case.m_a_offset;
        const Utils::Type &a_type = test_case.m_a_type;
        const GLchar *a_type_name = a_type.GetGLSLTypeName();
        size_t position           = 0;

        switch (stage)
        {
            case Utils::Shader::COMPUTE:
                source = cs;
                break;
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        sprintf(buffer, "%d", b_offset);
        Utils::replaceToken("B_OFFSET", position, buffer, source);
        Utils::replaceToken("B_TYPE", position, b_type_name, source);
        sprintf(buffer, "%d", a_offset);
        Utils::replaceToken("A_OFFSET", position, buffer, source);
        Utils::replaceToken("A_TYPE", position, a_type_name, source);
        Utils::replaceToken("B_TYPE", position, b_type_name, source);
        Utils::replaceToken("A_TYPE", position, a_type_name, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Checks if stage is supported
 *
 * @param stage Shader stage
 *
 * @return true if supported, false otherwise
 **/
bool SSBMemberOverlappingOffsetsTest::isStageSupported(Utils::Shader::STAGES stage)
{
    const Functions &gl         = m_context.getRenderContext().getFunctions();
    GLint max_supported_buffers = 0;
    GLenum pname                = 0;

    switch (stage)
    {
        case Utils::Shader::COMPUTE:
            pname = GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::FRAGMENT:
            pname = GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::GEOMETRY:
            pname = GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::TESS_CTRL:
            pname = GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::TESS_EVAL:
            pname = GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::VERTEX:
            pname = GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    gl.getIntegerv(pname, &max_supported_buffers);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    return 1 <= max_supported_buffers;
}

/** Constructor
 *
 * @param context Test framework context
 **/
SSBMemberAlignNonPowerOf2Test::SSBMemberAlignNonPowerOf2Test(deqp::Context &context, GLuint type)
    : UniformBlockMemberAlignNonPowerOf2Test(
          context,
          "ssb_member_align_non_power_of_2",
          "Test verifies that align qualifier requires value that is a power of 2",
          type)
{
    std::string name = ("ssb_member_align_non_power_of_2_");
    name.append(getTypeName(type));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string SSBMemberAlignNonPowerOf2Test::getShaderSource(GLuint test_case_index,
                                                           Utils::Shader::STAGES stage)
{
    static const GLchar *cs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    vec4 b;\n"
        "    layout (align = ALIGN) TYPE a;\n"
        "} block;\n"
        "\n"
        "writeonly uniform image2D uni_image;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vec4(1, 0, 0.5, 1);\n"
        "\n"
        "    if (TYPE(0) == block.a)\n"
        "    {\n"
        "        result = vec4(1, 1, 1, 1) - block.b;\n"
        "    }\n"
        "\n"
        "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
        "}\n"
        "\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    vec4 b;\n"
        "    layout (align = ALIGN) TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(0) == block.a)\n"
        "    {\n"
        "        fs_out = block.b;\n"
        "    }\n"
        "\n"
        "    fs_out += gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    vec4 b;\n"
        "    layout (align = ALIGN) TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(0) == block.a)\n"
        "    {\n"
        "        gs_fs = block.b;\n"
        "    }\n"
        "\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs += tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    vec4 b;\n"
        "    layout (align = ALIGN) TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(0) == block.a)\n"
        "    {\n"
        "        tcs_tes[gl_InvocationID] = block.b;\n"
        "    }\n"
        "\n"
        "\n"
        "    tcs_tes[gl_InvocationID] += vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    vec4 b;\n"
        "    layout (align = ALIGN) TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(0) == block.a)\n"
        "    {\n"
        "        tes_gs = block.b;\n"
        "    }\n"
        "\n"
        "    tes_gs += tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (std140) buffer Block {\n"
        "    vec4 b;\n"
        "    layout (align = ALIGN) TYPE a;\n"
        "} block;\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    if (TYPE(0) == block.a)\n"
        "    {\n"
        "        vs_tcs = block.b;\n"
        "    }\n"
        "\n"
        "    vs_tcs += in_vs;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        GLchar buffer[16];
        const GLuint alignment  = test_case.m_alignment;
        const Utils::Type &type = test_case.m_type;
        const GLchar *type_name = type.GetGLSLTypeName();
        size_t position         = 0;

        switch (stage)
        {
            case Utils::Shader::COMPUTE:
                source = cs;
                break;
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        sprintf(buffer, "%d", alignment);
        Utils::replaceToken("ALIGN", position, buffer, source);
        Utils::replaceToken("TYPE", position, type_name, source);
        Utils::replaceToken("TYPE", position, type_name, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Checks if stage is supported
 *
 * @param stage Shader stage
 *
 * @return true if supported, false otherwise
 **/
bool SSBMemberAlignNonPowerOf2Test::isStageSupported(Utils::Shader::STAGES stage)
{
    const Functions &gl         = m_context.getRenderContext().getFunctions();
    GLint max_supported_buffers = 0;
    GLenum pname                = 0;

    switch (stage)
    {
        case Utils::Shader::COMPUTE:
            pname = GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::FRAGMENT:
            pname = GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::GEOMETRY:
            pname = GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::TESS_CTRL:
            pname = GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::TESS_EVAL:
            pname = GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS;
            break;
        case Utils::Shader::VERTEX:
            pname = GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    gl.getIntegerv(pname, &max_supported_buffers);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    return 1 <= max_supported_buffers;
}

/** Constructor
 *
 * @param context Test framework context
 **/
SSBAlignmentTest::SSBAlignmentTest(deqp::Context &context)
    : TextureTestBase(context, "ssb_alignment", "Test verifies offset and alignment of ssb buffer")
{}

/** Get interface of program
 *
 * @param ignored
 * @param program_interface Interface of program
 * @param varying_passthrough Collection of connections between in and out variables
 **/
void SSBAlignmentTest::getProgramInterface(GLuint /* test_case_index */,
                                           Utils::ProgramInterface &program_interface,
                                           Utils::VaryingPassthrough &varying_passthrough)
{
    static const Utils::Type vec4 = Utils::Type::vec4;

#if WRKARD_UNIFORMBLOCKALIGNMENT

    static const GLuint block_align = 16;

#else /* WRKARD_UNIFORMBLOCKALIGNMENT */

    static const GLuint block_align = 64;

#endif /* WRKARD_UNIFORMBLOCKALIGNMENT */

    static const GLuint fifth_align = 16;
    static const GLuint vec4_stride = 16;
    static const GLuint data_stride = vec4_stride * 2; /* one vec4 + one scalar aligned to 16 */

    const GLuint first_offset = 0; /* vec4 at 0 */
    const GLuint second_offset =
        Utils::Type::GetActualOffset(first_offset + vec4_stride, block_align); /* Data at 32 */
    const GLuint third_offset =
        Utils::Type::GetActualOffset(second_offset + data_stride, block_align); /* Data[2] at 64 */
    const GLuint fourth_offset = Utils::Type::GetActualOffset(third_offset + data_stride * 2,
                                                              block_align); /* vec4[3] at 96 */
    const GLuint fifth_offset  = Utils::Type::GetActualOffset(fourth_offset + vec4_stride * 3,
                                                              fifth_align); /* vec4[2] at 160 */
    const GLuint sixth_offset =
        Utils::Type::GetActualOffset(fifth_offset + vec4_stride * 2, block_align); /* Data at 192 */

    Utils::Interface *structure = program_interface.Structure("Data");

    structure->Member("vector", "", 0 /* expected_component */, 0 /* expected_location */,
                      Utils::Type::vec4, false /* normalized */, 0 /* n_array_elements */,
                      Utils::Type::vec4.GetSize(), 0 /* offset */);

    structure->Member("scalar", "", 0 /* expected_component */, 0 /* expected_location */,
                      Utils::Type::_float, false /* normalized */, 0 /* n_array_elements */,
                      Utils::Type::_float.GetSize(), Utils::Type::vec4.GetSize() /* offset */);

    /* Prepare Block */
    Utils::Interface *vs_buf_Block = program_interface.Block("vs_buf_Block");

    vs_buf_Block->Member("first", "", 0 /* expected_component */, 0 /* expected_location */,
                         Utils::Type::vec4, false /* normalized */, 0 /* n_array_elements */,
                         vec4_stride, first_offset /* offset */);

    vs_buf_Block->Member("second", "", 0 /* expected_component */, 0 /* expected_location */,
                         structure, 0 /* n_array_elements */, data_stride, second_offset);

    vs_buf_Block->Member("third", "", 0 /* expected_component */, 0 /* expected_location */,
                         structure, 2 /* n_array_elements */, data_stride, third_offset);

    vs_buf_Block->Member("fourth", "", 0 /* expected_component */, 0 /* expected_location */, vec4,
                         false /* normalized */, 3 /* n_array_elements */, vec4_stride,
                         fourth_offset);

    vs_buf_Block->Member("fifth", "layout(align = 16)", 0 /* expected_component */,
                         0 /* expected_location */, vec4, false /* normalized */,
                         2 /* n_array_elements */, vec4_stride, fifth_offset);

    vs_buf_Block->Member("sixth", "", 0 /* expected_component */, 0 /* expected_location */,
                         structure, 0 /* n_array_elements */, data_stride, sixth_offset);

    const GLuint stride = calculateStride(*vs_buf_Block);
    m_data.resize(stride);
    generateData(*vs_buf_Block, 0, m_data);

    Utils::ShaderInterface &vs_si = program_interface.GetShaderInterface(Utils::Shader::VERTEX);

/* Add uniform BLOCK */
#if WRKARD_UNIFORMBLOCKALIGNMENT
    vs_si.SSB("vs_buf_block", "layout (std140, binding = BINDING)", 0, 0, vs_buf_Block, 0,
              static_cast<GLuint>(m_data.size()), 0, &m_data[0], m_data.size());
#else  /* WRKARD_UNIFORMBLOCKALIGNMENT */
    vs_si.SSB("vs_buf_block", "layout (std140, binding = BINDING, align = 64)", 0, 0, vs_buf_Block,
              0, static_cast<GLuint>(m_data.size()), 0, &m_data[0], m_data.size());
#endif /* WRKARD_UNIFORMBLOCKALIGNMENT */

    program_interface.CloneVertexInterface(varying_passthrough);
}

/** Selects if "draw" stages are relevant for test
 *
 * @param ignored
 *
 * @return true if all stages support shader storage buffers, false otherwise
 **/
bool SSBAlignmentTest::isDrawRelevant(GLuint /* test_case_index */)
{
    const Functions &gl         = m_context.getRenderContext().getFunctions();
    GLint gs_supported_buffers  = 0;
    GLint tcs_supported_buffers = 0;
    GLint tes_supported_buffers = 0;
    GLint vs_supported_buffers  = 0;

    gl.getIntegerv(GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, &gs_supported_buffers);
    gl.getIntegerv(GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS, &tcs_supported_buffers);
    gl.getIntegerv(GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS, &tes_supported_buffers);
    gl.getIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &vs_supported_buffers);

    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    return ((1 <= gs_supported_buffers) && (1 <= tcs_supported_buffers) &&
            (1 <= tes_supported_buffers) && (1 <= vs_supported_buffers));
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingLocationsTest::VaryingLocationsTest(deqp::Context &context)
    : TextureTestBase(context,
                      "varying_locations",
                      "Test verifies that input and output locations are respected"),
      m_gl_max_geometry_input_components(0)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();
    gl.getIntegerv(GL_MAX_GEOMETRY_INPUT_COMPONENTS, &m_gl_max_geometry_input_components);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Name of test
 * @param test_description Description of test
 **/
VaryingLocationsTest::VaryingLocationsTest(deqp::Context &context,
                                           const glw::GLchar *test_name,
                                           const glw::GLchar *test_description)
    : TextureTestBase(context, test_name, test_description)
{}

/** Get interface of program
 *
 * @param test_case_index     Test case
 * @param program_interface   Interface of program
 * @param varying_passthrough Collection of connections between in and out variables
 **/
void VaryingLocationsTest::getProgramInterface(GLuint test_case_index,
                                               Utils::ProgramInterface &program_interface,
                                               Utils::VaryingPassthrough &varying_passthrough)
{
    const Utils::Type type = getType(test_case_index);

    GLint totalComponents = 2 * type.m_n_columns * type.m_n_rows;
    if (type.m_basic_type == Utils::Type::Double)
    {
        totalComponents = totalComponents * 2;
    }
    if (totalComponents >= m_gl_max_geometry_input_components)
    {
        throw tcu::NotSupportedError("Test case index " + std::to_string(test_case_index) +
                                     " for type " + type.GetGLSLTypeName() + " not supported");
    }
    m_first_data = type.GenerateDataPacked();
    m_last_data  = type.GenerateDataPacked();

    prepareShaderStage(Utils::Shader::FRAGMENT, type, program_interface, varying_passthrough);
    prepareShaderStage(Utils::Shader::GEOMETRY, type, program_interface, varying_passthrough);
    prepareShaderStage(Utils::Shader::TESS_CTRL, type, program_interface, varying_passthrough);
    prepareShaderStage(Utils::Shader::TESS_EVAL, type, program_interface, varying_passthrough);
    prepareShaderStage(Utils::Shader::VERTEX, type, program_interface, varying_passthrough);
}

/** Get type name
 *
 * @param test_case_index Index of test case
 *
 * @return Name of type test in test_case_index
 **/
std::string VaryingLocationsTest::getTestCaseName(glw::GLuint test_case_index)
{
    return getTypeName(test_case_index);
}

/** Returns number of types to test
 *
 * @return Number of types, 34
 **/
glw::GLuint VaryingLocationsTest::getTestCaseNumber()
{
    return getTypesNumber();
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingLocationsTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/**
 *
 *
 **/
std::string VaryingLocationsTest::prepareGlobals(GLint last_in_loc, GLint last_out_loc)
{
    GLchar buffer[16];
    std::string globals =
        "const uint first_input_location  = 0u;\n"
        "const uint first_output_location = 0u;\n"
        "const uint last_input_location   = LAST_INPUTu;\n"
        "const uint last_output_location  = LAST_OUTPUTu;\n";
    size_t position = 100; /* Skip first part */

    sprintf(buffer, "%d", last_in_loc);
    Utils::replaceToken("LAST_INPUT", position, buffer, globals);

    sprintf(buffer, "%d", last_out_loc);
    Utils::replaceToken("LAST_OUTPUT", position, buffer, globals);

    return globals;
}

/**
 *
 **/
void VaryingLocationsTest::prepareShaderStage(Utils::Shader::STAGES stage,
                                              const Utils::Type &type,
                                              Utils::ProgramInterface &program_interface,
                                              Utils::VaryingPassthrough &varying_passthrough)
{
    const GLuint array_length  = 1;
    const GLuint first_in_loc  = 0;
    const GLuint first_out_loc = 0;
    const GLuint last_in_loc   = getLastInputLocation(stage, type, array_length, false);
    size_t position            = 0;

    const GLchar *prefix_in =
        Utils::ProgramInterface::GetStagePrefix(stage, Utils::Variable::VARYING_INPUT);

    const GLchar *prefix_out =
        Utils::ProgramInterface::GetStagePrefix(stage, Utils::Variable::VARYING_OUTPUT);

    const GLchar *qual_first_in  = "layout (location = first_input_location)";
    const GLchar *qual_first_out = "layout (location = first_output_location)";
    const GLchar *qual_last_in   = "layout (location = last_input_location)";
    const GLchar *qual_last_out  = "layout (location = last_output_location)";

    Utils::ShaderInterface &si = program_interface.GetShaderInterface(stage);
    const GLuint type_size     = type.GetSize();

    std::string first_in_name  = "PREFIXfirst";
    std::string first_out_name = "PREFIXfirst";
    std::string last_in_name   = "PREFIXlast";
    std::string last_out_name  = "PREFIXlast";

    Utils::replaceToken("PREFIX", position, prefix_in, first_in_name);
    position = 0;
    Utils::replaceToken("PREFIX", position, prefix_out, first_out_name);
    position = 0;
    Utils::replaceToken("PREFIX", position, prefix_in, last_in_name);
    position = 0;
    Utils::replaceToken("PREFIX", position, prefix_out, last_out_name);

    if (Utils::Shader::FRAGMENT == stage)
    {
        qual_first_in = "layout (location = first_input_location) flat";
        qual_last_in  = "layout (location = last_input_location)  flat";
    }
    if (Utils::Shader::GEOMETRY == stage)
    {
        qual_first_out = "layout (location = first_output_location) flat";
        qual_last_out  = "layout (location = last_output_location)  flat";
    }

    Utils::Variable *first_in =
        si.Input(first_in_name.c_str(), qual_first_in /* qualifiers */, 0 /* expected_componenet */,
                 first_in_loc /* expected_location */, type /* type */, GL_FALSE /* normalized */,
                 0u /* n_array_elements */, 0u /* stride */, 0u /* offset */,
                 (GLvoid *)&m_first_data[0] /* data */, m_first_data.size() /* data_size */);

    Utils::Variable *last_in =
        si.Input(last_in_name.c_str(), qual_last_in /* qualifiers */, 0 /* expected_componenet */,
                 last_in_loc /* expected_location */, type /* type */, GL_FALSE /* normalized */,
                 0u /* n_array_elements */, 0u /* stride */, type_size /* offset */,
                 (GLvoid *)&m_last_data[0] /* data */, m_last_data.size() /* data_size */);

    if (Utils::Shader::FRAGMENT != stage)
    {
        const GLuint last_out_loc = getLastOutputLocation(stage, type, array_length, false);

        Utils::Variable *first_out = si.Output(
            first_out_name.c_str(), qual_first_out /* qualifiers */, 0 /* expected_componenet */,
            first_out_loc /* expected_location */, type /* type */, GL_FALSE /* normalized */,
            0u /* n_array_elements */, 0u /* stride */, 0u /* offset */,
            (GLvoid *)&m_first_data[0] /* data */, m_first_data.size() /* data_size */);

        Utils::Variable *last_out = si.Output(
            last_out_name.c_str(), qual_last_out /* qualifiers */, 0 /* expected_componenet */,
            last_out_loc /* expected_location */, type /* type */, GL_FALSE /* normalized */,
            0u /* n_array_elements */, 0u /* stride */, 0u /* offset */,
            (GLvoid *)&m_last_data[0] /* data */, m_last_data.size() /* data_size */);

        si.m_globals = prepareGlobals(last_in_loc, last_out_loc);

        varying_passthrough.Add(stage, first_in, first_out);
        varying_passthrough.Add(stage, last_in, last_out);
    }
    else
    {
        /* No outputs for fragment shader, so last_output_location can be 0 */
        si.m_globals = prepareGlobals(last_in_loc, 0);
    }
}

/** This test should be run with separable programs
 *
 * @param ignored
 *
 * @return true
 **/
bool VaryingLocationsTest::useMonolithicProgram(GLuint /* test_case_index */)
{
    return false;
}

/* Constants used by VertexAttribLocationsTest */
const GLuint VertexAttribLocationsTest::m_base_vertex   = 4;
const GLuint VertexAttribLocationsTest::m_base_instance = 2;
const GLuint VertexAttribLocationsTest::m_loc_vertex    = 2;
const GLuint VertexAttribLocationsTest::m_loc_instance  = 5;
const GLuint VertexAttribLocationsTest::m_n_instances   = 4;

/** Constructor
 *
 * @param context Test framework context
 **/
VertexAttribLocationsTest::VertexAttribLocationsTest(deqp::Context &context)
    : TextureTestBase(context,
                      "vertex_attrib_locations",
                      "Test verifies that attribute locations are respected by drawing operations")
{}

/** Execute proper draw command for test case
 *
 * @param test_case_index Index of test case
 **/
void VertexAttribLocationsTest::executeDrawCall(GLuint test_case_index)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    switch (test_case_index)
    {
        case DRAWARRAYS:
            gl.drawArrays(GL_PATCHES, 0 /* first */, 1 /* count */);
            GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");
            break;
        case DRAWARRAYSINSTANCED:
            gl.drawArraysInstanced(GL_PATCHES, 0 /* first */, 1 /* count */, m_n_instances);
            GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArraysInstanced");
            break;
        case DRAWELEMENTS:
            gl.drawElements(GL_PATCHES, 1 /* count */, GL_UNSIGNED_BYTE, nullptr);
            GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElements");
            break;
        case DRAWELEMENTSBASEVERTEX:
            gl.drawElementsBaseVertex(GL_PATCHES, 1 /* count */, GL_UNSIGNED_BYTE, nullptr,
                                      m_base_vertex);
            GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElementsBaseVertex");
            break;
        case DRAWELEMENTSINSTANCED:
            gl.drawElementsInstanced(GL_PATCHES, 1 /* count */, GL_UNSIGNED_BYTE, nullptr,
                                     m_n_instances);
            GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElementsInstanced");
            break;
        case DRAWELEMENTSINSTANCEDBASEINSTANCE:
            gl.drawElementsInstancedBaseInstance(GL_PATCHES, 1 /* count */, GL_UNSIGNED_BYTE,
                                                 nullptr, m_n_instances, m_base_instance);
            GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElementsInstancedBaseInstance");
            break;
        case DRAWELEMENTSINSTANCEDBASEVERTEX:
            gl.drawElementsInstancedBaseVertex(GL_PATCHES, 1 /* count */, GL_UNSIGNED_BYTE, nullptr,
                                               m_n_instances, m_base_vertex);
            GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElementsInstancedBaseVertex");
            break;
        case DRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE:
            gl.drawElementsInstancedBaseVertexBaseInstance(GL_PATCHES, 1 /* count */,
                                                           GL_UNSIGNED_BYTE, nullptr, m_n_instances,
                                                           m_base_vertex, m_base_instance);
            GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElementsInstancedBaseVertexBaseInstance");
            break;
        default:
            TCU_FAIL("Invalid enum");
    }
}

/** Get interface of program
 *
 * @param ignored
 * @param program_interface   Interface of program
 * @param ignored
 **/
void VertexAttribLocationsTest::getProgramInterface(
    GLuint /* test_case_index */,
    Utils::ProgramInterface &program_interface,
    Utils::VaryingPassthrough & /* varying_passthrough */)
{
    Utils::ShaderInterface &si = program_interface.GetShaderInterface(Utils::Shader::VERTEX);

    /* Globals */
    si.m_globals =
        "const uint vertex_index_location   = 2;\n"
        "const uint instance_index_location = 5;\n";

    /* Attributes */
    si.Input("vertex_index" /* name */,
             "layout (location = vertex_index_location)" /* qualifiers */,
             0 /* expected_componenet */, m_loc_vertex /* expected_location */,
             Utils::Type::uint /* type */, GL_FALSE /* normalized */, 0u /* n_array_elements */,
             16 /* stride */, 0u /* offset */, (GLvoid *)0 /* data */, 0 /* data_size */);
    si.Input("instance_index" /* name */,
             "layout (location = instance_index_location)" /* qualifiers */,
             0 /* expected_componenet */, m_loc_instance /* expected_location */,
             Utils::Type::uint /* type */, GL_FALSE /* normalized */, 0u /* n_array_elements */,
             16 /* stride */, 16u /* offset */, (GLvoid *)0 /* data */, 0 /* data_size */);
}

/** Get name of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Name of test case
 **/
std::string VertexAttribLocationsTest::getTestCaseName(glw::GLuint test_case_index)
{
    std::string result;

    switch (test_case_index)
    {
        case DRAWARRAYS:
            result = "DrawArrays";
            break;
        case DRAWARRAYSINSTANCED:
            result = "DrawArraysInstanced";
            break;
        case DRAWELEMENTS:
            result = "DrawElements";
            break;
        case DRAWELEMENTSBASEVERTEX:
            result = "DrawElementsBaseVertex";
            break;
        case DRAWELEMENTSINSTANCED:
            result = "DrawElementsInstanced";
            break;
        case DRAWELEMENTSINSTANCEDBASEINSTANCE:
            result = "DrawElementsInstancedBaseInstance";
            break;
        case DRAWELEMENTSINSTANCEDBASEVERTEX:
            result = "DrawElementsInstancedBaseVertex";
            break;
        case DRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE:
            result = "DrawElementsInstancedBaseVertexBaseInstance";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint VertexAttribLocationsTest::getTestCaseNumber()
{
    return TESTCASES_MAX;
}

/** Prepare code snippet that will verify in and uniform variables
 *
 * @param ignored
 * @param ignored
 * @param stage   Shader stage
 *
 * @return Code that verify variables
 **/
std::string VertexAttribLocationsTest::getVerificationSnippet(
    GLuint /* test_case_index */,
    Utils::ProgramInterface & /* program_interface */,
    Utils::Shader::STAGES stage)
{
    std::string verification;

    if (Utils::Shader::VERTEX == stage)
    {

#if DEBUG_VERTEX_ATTRIB_LOCATIONS_TEST_VARIABLE

        verification =
            "if (gl_InstanceID != instance_index)\n"
            "    {\n"
            "        result = 12u;\n"
            "    }\n"
            "    else if (gl_VertexID != vertex_index)\n"
            "    {\n"
            "        result = 11u;\n"
            "    }\n";

#else

        verification =
            "if ((gl_VertexID   != vertex_index)  ||\n"
            "        (gl_InstanceID != instance_index) )\n"
            "    {\n"
            "        result = 0u;\n"
            "    }\n";

#endif
    }
    else
    {
        verification = "";
    }

    return verification;
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VertexAttribLocationsTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare attributes, vertex array object and array buffer
 *
 * @param ignored
 * @param ignored Interface of program
 * @param buffer  Array buffer
 * @param vao     Vertex array object
 **/
void VertexAttribLocationsTest::prepareAttributes(GLuint test_case_index /* test_case_index */,
                                                  Utils::ProgramInterface & /* program_interface */,
                                                  Utils::Buffer &buffer,
                                                  Utils::VertexArray &vao)
{
    static const GLuint vertex_index_data[8]   = {0, 1, 2, 3, 4, 5, 6, 7};
    static const GLuint instance_index_data[8] = {0, 1, 2, 3, 4, 5, 6, 7};

    std::vector<GLuint> buffer_data;
    buffer_data.resize(8 + 8); /* vertex_index_data + instance_index_data */

    GLubyte *ptr = (GLubyte *)&buffer_data[0];

    /*
     When case index >=2, the test calls glDrawElement*(), such as glDrawElementsBaseVertex(),
     glDrawElementsInstanced(), glDrawElementsInstancedBaseInstance() and so on, So we need to
     change the buffer type as GL_ELEMENT_ARRAY_BUFFER
     */
    if (test_case_index >= 2)
    {
        buffer.m_buffer = Utils::Buffer::Element;
    }
    vao.Bind();
    buffer.Bind();

    vao.Attribute(m_loc_vertex /* vertex_index */, Utils::Type::uint, 0 /* array_elements */,
                  false /* normalized */, 0 /* stride */, 0 /* offset */);

    vao.Attribute(m_loc_instance /* instance_index */, Utils::Type::uint, 0 /* array_elements */,
                  false /* normalized */, 0 /* stride */,
                  (GLvoid *)sizeof(vertex_index_data) /* offset */);
    // when test_case_index is 5 or 7, the draw call is glDrawElementsInstancedBaseInstance,
    // glDrawElementsInstancedBaseVertexBaseInstance the instancecount is 4, the baseinstance is 2,
    // the divisor should be set 2
    bool isBaseInstanced = (test_case_index == DRAWELEMENTSINSTANCEDBASEINSTANCE ||
                            test_case_index == DRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE);
    vao.Divisor(m_context.getRenderContext().getFunctions() /* gl */,
                m_loc_instance /* instance_index */,
                isBaseInstanced ? 2 : 1 /* divisor. 1 - advance once per instance */);

    memcpy(ptr + 0, vertex_index_data, sizeof(vertex_index_data));
    memcpy(ptr + sizeof(vertex_index_data), instance_index_data, sizeof(instance_index_data));

    buffer.Data(Utils::Buffer::StaticDraw, buffer_data.size() * sizeof(GLuint), ptr);
}

/** This test should be run with separable programs
 *
 * @param ignored
 *
 * @return true
 **/
bool VertexAttribLocationsTest::useMonolithicProgram(GLuint /* test_case_index */)
{
    return false;
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingArrayLocationsTest::VaryingArrayLocationsTest(deqp::Context &context)
    : VaryingLocationsTest(context,
                           "varying_array_locations",
                           "Test verifies that input and output locations are respected for arrays")
{}

/**
 *
 **/
void VaryingArrayLocationsTest::prepareShaderStage(Utils::Shader::STAGES stage,
                                                   const Utils::Type &type,
                                                   Utils::ProgramInterface &program_interface,
                                                   Utils::VaryingPassthrough &varying_passthrough)
{
    const GLuint array_length  = 1u;
    const GLuint first_in_loc  = 0;
    const GLuint first_out_loc = 0;
    const GLuint last_in_loc   = getLastInputLocation(stage, type, array_length, false);
    size_t position            = 0;

    const GLchar *prefix_in =
        Utils::ProgramInterface::GetStagePrefix(stage, Utils::Variable::VARYING_INPUT);

    const GLchar *prefix_out =
        Utils::ProgramInterface::GetStagePrefix(stage, Utils::Variable::VARYING_OUTPUT);

    const GLchar *qual_first_in  = "layout (location = first_input_location)";
    const GLchar *qual_first_out = "layout (location = first_output_location)";
    const GLchar *qual_last_in   = "layout (location = last_input_location)";
    const GLchar *qual_last_out  = "layout (location = last_output_location)";

    Utils::ShaderInterface &si = program_interface.GetShaderInterface(stage);
    const GLuint type_size     = type.GetSize();

    std::string first_in_name  = "PREFIXfirst";
    std::string first_out_name = "PREFIXfirst";
    std::string last_in_name   = "PREFIXlast";
    std::string last_out_name  = "PREFIXlast";

    Utils::replaceToken("PREFIX", position, prefix_in, first_in_name);
    position = 0;
    Utils::replaceToken("PREFIX", position, prefix_out, first_out_name);
    position = 0;
    Utils::replaceToken("PREFIX", position, prefix_in, last_in_name);
    position = 0;
    Utils::replaceToken("PREFIX", position, prefix_out, last_out_name);

    if (Utils::Shader::FRAGMENT == stage)
    {
        qual_first_in = "layout (location = first_input_location) flat";
        qual_last_in  = "layout (location = last_input_location)  flat";
    }
    if (Utils::Shader::GEOMETRY == stage)
    {
        qual_first_out = "layout (location = first_output_location) flat";
        qual_last_out  = "layout (location = last_output_location)  flat";
    }

    Utils::Variable *first_in =
        si.Input(first_in_name.c_str(), qual_first_in /* qualifiers */, 0 /* expected_componenet */,
                 first_in_loc /* expected_location */, type /* type */, GL_FALSE /* normalized */,
                 array_length /* n_array_elements */, 0u /* stride */, 0u /* offset */,
                 (GLvoid *)&m_first_data[0] /* data */, m_first_data.size() /* data_size */);

    Utils::Variable *last_in =
        si.Input(last_in_name.c_str(), qual_last_in /* qualifiers */, 0 /* expected_componenet */,
                 last_in_loc /* expected_location */, type /* type */, GL_FALSE /* normalized */,
                 array_length /* n_array_elements */, 0u /* stride */, type_size /* offset */,
                 (GLvoid *)&m_last_data[0] /* data */, m_last_data.size() /* data_size */);

    if (Utils::Shader::FRAGMENT != stage)
    {
        const GLuint last_out_loc = getLastOutputLocation(stage, type, array_length, false);

        Utils::Variable *first_out = si.Output(
            first_out_name.c_str(), qual_first_out /* qualifiers */, 0 /* expected_componenet */,
            first_out_loc /* expected_location */, type /* type */, GL_FALSE /* normalized */,
            array_length /* n_array_elements */, 0u /* stride */, 0u /* offset */,
            (GLvoid *)&m_first_data[0] /* data */, m_first_data.size() /* data_size */);

        Utils::Variable *last_out = si.Output(
            last_out_name.c_str(), qual_last_out /* qualifiers */, 0 /* expected_componenet */,
            last_out_loc /* expected_location */, type /* type */, GL_FALSE /* normalized */,
            array_length /* n_array_elements */, 0u /* stride */, 0u /* offset */,
            (GLvoid *)&m_last_data[0] /* data */, m_last_data.size() /* data_size */);

        si.m_globals = prepareGlobals(last_in_loc, last_out_loc);

        varying_passthrough.Add(stage, first_in, first_out);
        varying_passthrough.Add(stage, last_in, last_out);
    }
    else
    {
        /* No outputs for fragment shader, so last_output_location can be 0 */
        si.m_globals = prepareGlobals(last_in_loc, 0);
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingStructureLocationsTest::VaryingStructureLocationsTest(deqp::Context &context)
    : TextureTestBase(
          context,
          "varying_structure_locations",
          "Test verifies that locations are respected when structures are used as in and out ")
{}

/** Prepare code snippet that will pass in variables to out variables
 *
 * @param ignored
 * @param varying_passthrough Collection of connections between in and out variables
 * @param stage               Shader stage
 *
 * @return Code that pass in variables to next stage
 **/
std::string VaryingStructureLocationsTest::getPassSnippet(
    GLuint /* test_case_index */,
    Utils::VaryingPassthrough &varying_passthrough,
    Utils::Shader::STAGES stage)
{
    std::string result;

    if (Utils::Shader::VERTEX != stage)
    {
        result = TextureTestBase::getPassSnippet(0, varying_passthrough, stage);
    }
    else
    {
        result =
            "    vs_tcs_output[0].single   = vs_in_single[0];\n"
            "    vs_tcs_output[0].array[0] = vs_in_array[0];\n";
    }

    return result;
}

/** Get interface of program
 *
 * @param test_case_index     Test case
 * @param program_interface   Interface of program
 * @param varying_passthrough Collection of connections between in and out variables
 **/
void VaryingStructureLocationsTest::getProgramInterface(
    GLuint test_case_index,
    Utils::ProgramInterface &program_interface,
    Utils::VaryingPassthrough &varying_passthrough)
{
    Utils::ShaderInterface &si = program_interface.GetShaderInterface(Utils::Shader::VERTEX);
    const Utils::Type type     = getType(test_case_index);

    /* Prepare data */
    // We should call GenerateDataPacked() to generate data, which can make sure the data in shader
    // is correct
    m_single_data = type.GenerateDataPacked();
    m_array_data  = type.GenerateDataPacked();

    m_data.resize(m_single_data.size() + m_array_data.size());
    GLubyte *ptr = (GLubyte *)&m_data[0];
    memcpy(ptr, &m_single_data[0], m_single_data.size());
    memcpy(ptr + m_single_data.size(), &m_array_data[0], m_array_data.size());

    Utils::Interface *structure = program_interface.Structure("Data");

    structure->Member("single", "" /* qualifiers */, 0 /* component */, 0 /* location */, type,
                      false /* normalized */, 0u /* n_array_elements */, 0u /* stride */,
                      0u /* offset */);

    // the second struct member 's location should not be 0, it is based on by how many the
    // locations the first struct member consumed.
    structure->Member("array", "" /* qualifiers */, 0 /* component */,
                      type.GetLocations() /* location */, type, false /* normalized */,
                      1u /* n_array_elements */, 0u /* stride */, type.GetSize() /* offset */);

    si.Input("vs_in_single", "layout (location = 0)", 0 /* component */, 0 /* location */, type,
             false /* normalized */, 1u /* n_array_elements */, 0u /* stride */, 0u /* offset */,
             (GLvoid *)&m_single_data[0] /* data */, m_single_data.size() /* data_size */);

    si.Input("vs_in_array", "layout (location = 8)", 0 /* component */, 8 /* location */, type,
             false /* normalized */, 1u /* n_array_elements */, 0u /* stride */,
             type.GetSize() /* offset */, (GLvoid *)&m_array_data[0] /* data */,
             m_array_data.size() /* data_size */);

    si.Output("vs_tcs_output", "layout (location = 0)", 0 /* component */, 0 /* location */,
              structure, 1u /* n_array_elements */, 0u /* stride */, 0u /* offset */,
              (GLvoid *)&m_data[0] /* data */, m_data.size() /* data_size */);

    program_interface.CloneVertexInterface(varying_passthrough);
}

/** Get type name
 *
 * @param test_case_index Index of test case
 *
 * @return Name of type test in test_case_index
 **/
std::string VaryingStructureLocationsTest::getTestCaseName(glw::GLuint test_case_index)
{
    return getTypeName(test_case_index);
}

/** Returns number of types to test
 *
 * @return Number of types, 34
 **/
glw::GLuint VaryingStructureLocationsTest::getTestCaseNumber()
{
    return getTypesNumber();
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingStructureLocationsTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** This test should be run with separable programs
 *
 * @param ignored
 *
 * @return true
 **/
bool VaryingStructureLocationsTest::useMonolithicProgram(GLuint /* test_case_index */)
{
    return false;
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Name of test
 * @param test_description Description of test
 **/
VaryingStructureMemberLocationTest::VaryingStructureMemberLocationTest(deqp::Context &context,
                                                                       GLuint stage)
    : NegativeTestBase(
          context,
          "varying_structure_member_location",
          "Test verifies that compiler does not allow location qualifier on member of structure"),
      m_stage(stage)
{
    std::string name = ("varying_structure_member_location_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string VaryingStructureMemberLocationTest::getShaderSource(GLuint test_case_index,
                                                                Utils::Shader::STAGES stage)
{
    static const GLchar *struct_definition =
        "struct Data {\n"
        "    vec4 gohan;\n"
#if DEBUG_NEG_REMOVE_ERROR
        "    /* layout (location = 4) */ vec4 goten;\n"
#else
        "    layout (location = 4) vec4 goten;\n"
#endif /* DEBUG_NEG_REMOVE_ERROR */
        "};\n";
    static const GLchar *input_use  = "    result += data.gohan + data.goten;\n";
    static const GLchar *input_var  = "in Data dataARRAY;\n";
    static const GLchar *output_var = "out Data dataARRAY;\n";
    static const GLchar *output_use =
        "    dataINDEX.gohan = result / 2;\n"
        "    dataINDEX.goten = result / 4 - dataINDEX.gohan;\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "STRUCT_DEFINITION"
        "\n"
        "VARIABLE_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    fs_out += result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "STRUCT_DEFINITION"
        "\n"
        "VARIABLE_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "STRUCT_DEFINITION"
        "\n"
        "VARIABLE_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "STRUCT_DEFINITION"
        "\n"
        "VARIABLE_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "STRUCT_DEFINITION"
        "\n"
        "VARIABLE_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs += result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case          = m_test_cases[test_case_index];
    const GLchar *var_definition = input_var;
    const GLchar *var_use        = Utils::Shader::VERTEX == test_case.m_stage ? input_use : "\n";
    const GLchar *array          = "";
    const GLchar *index          = "";

    if (!test_case.m_is_input)
    {
        var_definition = output_var;
        var_use        = output_use;
    }

    if (test_case.m_stage == stage)
    {
        size_t position = 0;
        size_t temp     = 0;

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                array  = "[]";
                index  = "[gl_InvocationID]";
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("STRUCT_DEFINITION", position, struct_definition, source);
        temp = position;
        Utils::replaceToken("VARIABLE_DEFINITION", position, var_definition, source);
        position = temp;
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("INDEX", index, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string VaryingStructureMemberLocationTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage) << ", direction: ";

    if (true == test_case.m_is_input)
    {
        stream << "input";
    }
    else
    {
        stream << "output";
    }

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint VaryingStructureMemberLocationTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingStructureMemberLocationTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void VaryingStructureMemberLocationTest::testInit()
{
    testCase test_case_in  = {true, (Utils::Shader::STAGES)m_stage};
    testCase test_case_out = {false, (Utils::Shader::STAGES)m_stage};

    /* It is a compile-time error to declare a struct as a VS input */
    if (Utils::Shader::VERTEX != m_stage)
    {
        m_test_cases.push_back(test_case_in);
    }

    if (Utils::Shader::FRAGMENT != m_stage)
    {
        m_test_cases.push_back(test_case_out);
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingBlockLocationsTest::VaryingBlockLocationsTest(deqp::Context &context)
    : TextureTestBase(
          context,
          "varying_block_locations",
          "Test verifies that locations are respected when blocks are used as in and out ")
{}

/** Prepare code snippet that will pass in variables to out variables
 *
 * @param ignored
 * @param varying_passthrough Collection of connections between in and out variables
 * @param stage               Shader stage
 *
 * @return Code that pass in variables to next stage
 **/
std::string VaryingBlockLocationsTest::getPassSnippet(
    GLuint /* test_case_index */,
    Utils::VaryingPassthrough &varying_passthrough,
    Utils::Shader::STAGES stage)
{
    std::string result;

    if (Utils::Shader::VERTEX != stage)
    {
        result = TextureTestBase::getPassSnippet(0, varying_passthrough, stage);
    }
    else
    {
        result =
            "vs_tcs_block.third  = vs_in_third;\n"
            "    vs_tcs_block.fourth = vs_in_fourth;\n"
            "    vs_tcs_block.fifth  = vs_in_fifth;\n";
    }

    return result;
}

/** Get interface of program
 *
 * @param ignored
 * @param program_interface   Interface of program
 * @param varying_passthrough Collection of connections between in and out variables
 **/
void VaryingBlockLocationsTest::getProgramInterface(GLuint /* test_case_index */,
                                                    Utils::ProgramInterface &program_interface,
                                                    Utils::VaryingPassthrough &varying_passthrough)
{
    Utils::ShaderInterface &si = program_interface.GetShaderInterface(Utils::Shader::VERTEX);
    const Utils::Type vec4     = Utils::Type::vec4;

    /* Prepare data */
    m_third_data  = vec4.GenerateData();
    m_fourth_data = vec4.GenerateData();
    m_fifth_data  = vec4.GenerateData();

    /* Memory layout is different from location layout */
    const GLuint fifth_offset  = 0u;
    const GLuint third_offset  = static_cast<GLuint>(fifth_offset + m_fifth_data.size());
    const GLuint fourth_offset = static_cast<GLuint>(third_offset + m_fourth_data.size());

    m_data.resize(fourth_offset + m_fourth_data.size());
    GLubyte *ptr = (GLubyte *)&m_data[0];
    memcpy(ptr + third_offset, &m_third_data[0], m_third_data.size());
    memcpy(ptr + fourth_offset, &m_fourth_data[0], m_fourth_data.size());
    memcpy(ptr + fifth_offset, &m_fifth_data[0], m_fifth_data.size());

    Utils::Interface *block = program_interface.Block("vs_tcs_Block");

    block->Member("fifth", "" /* qualifiers */, 0 /* component */, 4 /* location */, vec4,
                  false /* normalized */, 0u /* n_array_elements */, 0u /* stride */,
                  fifth_offset /* offset */);

    block->Member("third", "layout (location = 2)" /* qualifiers */, 0 /* component */,
                  2 /* location */, vec4, false /* normalized */, 0u /* n_array_elements */,
                  0u /* stride */, third_offset /* offset */);

    block->Member("fourth", "" /* qualifiers */, 0 /* component */, 3 /* location */, vec4,
                  false /* normalized */, 0u /* n_array_elements */, 0u /* stride */,
                  fourth_offset /* offset */);

    si.Output("vs_tcs_block", "layout (location = 4)", 0 /* component */, 4 /* location */, block,
              0u /* n_array_elements */, 0u /* stride */, 0u /* offset */,
              (GLvoid *)&m_data[0] /* data */, m_data.size() /* data_size */);

    si.Input("vs_in_third", "layout (location = 0)", 0 /* component */, 0 /* location */, vec4,
             false /* normalized */, 0u /* n_array_elements */, 0u /* stride */,
             third_offset /* offset */, (GLvoid *)&m_third_data[0] /* data */,
             m_third_data.size() /* data_size */);

    si.Input("vs_in_fourth", "layout (location = 1)", 0 /* component */, 1 /* location */, vec4,
             false /* normalized */, 0u /* n_array_elements */, 0u /* stride */,
             fourth_offset /* offset */, (GLvoid *)&m_fourth_data[0] /* data */,
             m_fourth_data.size() /* data_size */);

    si.Input("vs_in_fifth", "layout (location = 2)", 0 /* component */, 2 /* location */, vec4,
             false /* normalized */, 0u /* n_array_elements */, 0u /* stride */,
             fifth_offset /* offset */, (GLvoid *)&m_fifth_data[0] /* data */,
             m_fifth_data.size() /* data_size */);

    program_interface.CloneVertexInterface(varying_passthrough);
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingBlockLocationsTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** This test should be run with separable programs
 *
 * @param ignored
 *
 * @return true
 **/
bool VaryingBlockLocationsTest::useMonolithicProgram(GLuint /* test_case_index */)
{
    return false;
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingBlockMemberLocationsTest::VaryingBlockMemberLocationsTest(deqp::Context &context,
                                                                 GLuint stage)
    : NegativeTestBase(context,
                       "varying_block_member_locations",
                       "Test verifies that compilation error is reported when not all members of "
                       "block are qualified with location"),
      m_stage(stage)
{
    std::string name = ("varying_block_member_locations_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string VaryingBlockMemberLocationsTest::getShaderSource(GLuint test_case_index,
                                                             Utils::Shader::STAGES stage)
{
    static const GLchar *block_definition_all =
        "Goku {\n"
        "    layout (location = 2) vec4 gohan;\n"
        "    layout (location = 4) vec4 goten;\n"
        "    layout (location = 6) vec4 chichi;\n"
        "} gokuARRAY;\n";
    static const GLchar *block_definition_one =
        "Goku {\n"
        "    vec4 gohan;\n"
#if DEBUG_NEG_REMOVE_ERROR
        "    /* layout (location = 4) */ vec4 goten;\n"
#else
        "    layout (location = 4) vec4 goten;\n"
#endif /* DEBUG_NEG_REMOVE_ERROR */
        "    vec4 chichi;\n"
        "} gokuARRAY;\n";
    static const GLchar *input_use =
        "    result += gokuINDEX.gohan + gokuINDEX.goten + gokuINDEX.chichi;\n";
    static const GLchar *output_use =
        "    gokuINDEX.gohan  = result / 2;\n"
        "    gokuINDEX.goten  = result / 4 - gokuINDEX.gohan;\n"
        "    gokuINDEX.chichi = result / 8 - gokuINDEX.goten;\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "DIRECTION BLOCK_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    fs_out = result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "DIRECTION BLOCK_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "DIRECTION BLOCK_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "DIRECTION BLOCK_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs = result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "DIRECTION BLOCK_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs = result;\n"
        "}\n"
        "\n";

    const GLchar *array     = "";
    const GLchar *direction = "in";
    const GLchar *index     = "";
    std::string source;
    testCase &test_case   = m_test_cases[test_case_index];
    const GLchar *var_use = Utils::Shader::VERTEX == test_case.m_stage ? input_use : "\n";
    const GLchar *definition =
        test_case.m_qualify_all ? block_definition_all : block_definition_one;

    if (!test_case.m_is_input)
    {
        direction = "out";
        var_use   = output_use;
    }

    if (test_case.m_stage == stage)
    {
        size_t position = 0;
        size_t temp     = 0;

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                array  = "[]";
                index  = "[gl_InvocationID]";
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("DIRECTION", position, direction, source);
        temp = position;
        Utils::replaceToken("BLOCK_DEFINITION", position, definition, source);
        position = temp;
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("INDEX", index, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string VaryingBlockMemberLocationsTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage) << ", direction: ";

    if (true == test_case.m_is_input)
    {
        stream << "input";
    }
    else
    {
        stream << "output";
    }

    if (true == test_case.m_qualify_all)
    {
        stream << ", all members qualified";
    }
    else
    {
        stream << ", not all members qualified";
    }

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint VaryingBlockMemberLocationsTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingBlockMemberLocationsTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Selects if compilation failure is expected result
 *
 * @param test_case_index Index of test case
 *
 * @return false when all members are qualified, true otherwise
 **/
bool VaryingBlockMemberLocationsTest::isFailureExpected(GLuint test_case_index)
{
    return (true != m_test_cases[test_case_index].m_qualify_all);
}

/** Prepare all test cases
 *
 **/
void VaryingBlockMemberLocationsTest::testInit()
{
    testCase test_case_in_all  = {true, true, (Utils::Shader::STAGES)m_stage};
    testCase test_case_in_one  = {true, false, (Utils::Shader::STAGES)m_stage};
    testCase test_case_out_all = {false, true, (Utils::Shader::STAGES)m_stage};
    testCase test_case_out_one = {false, false, (Utils::Shader::STAGES)m_stage};

    if (Utils::Shader::VERTEX != m_stage)
    {
        m_test_cases.push_back(test_case_in_all);
        m_test_cases.push_back(test_case_in_one);
    }

    if (Utils::Shader::FRAGMENT != m_stage)
    {
        m_test_cases.push_back(test_case_out_all);
        m_test_cases.push_back(test_case_out_one);
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingBlockAutomaticMemberLocationsTest::VaryingBlockAutomaticMemberLocationsTest(
    deqp::Context &context,
    GLuint stage)
    : NegativeTestBase(context,
                       "varying_block_automatic_member_locations",
                       "Test verifies that compiler assigns subsequent locations to block members, "
                       "even if this causes errors"),
      m_stage(stage)
{
    std::string name = ("varying_block_automatic_member_locations_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string VaryingBlockAutomaticMemberLocationsTest::getShaderSource(GLuint test_case_index,
                                                                      Utils::Shader::STAGES stage)
{
    static const GLchar *block_definition =
        "layout (location = 2) DIRECTION DBZ {\n"
        "    vec4 goku;\n"
        "    vec4 gohan[4];\n"
        "    vec4 goten;\n"
#if DEBUG_NEG_REMOVE_ERROR
        "    /* layout (location = 1) */ vec4 chichi;\n"
#else
        "    layout (location = 1) vec4 chichi;\n"
#endif /* DEBUG_NEG_REMOVE_ERROR */
        "    vec4 pan;\n"
        "} dbzARRAY;\n";
    static const GLchar *input_use =
        "    result += dbzINDEX.goku + dbzINDEX.gohan[0] + dbzINDEX.gohan[1] + "
        "dbzINDEX.gohan[3] + dbzINDEX.gohan[2] + dbzINDEX.goten + dbzINDEX.chichi + "
        "dbzINDEX.pan;\n";
    static const GLchar *output_use =
        "    dbzINDEX.goku     = result;\n"
        "    dbzINDEX.gohan[0] = result / 2;\n"
        "    dbzINDEX.gohan[1] = result / 2.25;\n"
        "    dbzINDEX.gohan[2] = result / 2.5;\n"
        "    dbzINDEX.gohan[3] = result / 2.75;\n"
        "    dbzINDEX.goten    = result / 4  - dbzINDEX.gohan[0] - dbzINDEX.gohan[1] - "
        "dbzINDEX.gohan[2] - dbzINDEX.gohan[3];\n"
        "    dbzINDEX.chichi   = result / 8  - dbzINDEX.goten;\n"
        "    dbzINDEX.pan      = result / 16 - dbzINDEX.chichi;\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "BLOCK_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    fs_out += result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "BLOCK_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "BLOCK_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "BLOCK_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "BLOCK_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs += result;\n"
        "}\n"
        "\n";

    const GLchar *array     = "";
    const GLchar *direction = "in";
    const GLchar *index     = "";
    std::string source;
    testCase &test_case   = m_test_cases[test_case_index];
    const GLchar *var_use = Utils::Shader::VERTEX == test_case.m_stage ? input_use : "\n";

    if (!test_case.m_is_input)
    {
        direction = "out";
        var_use   = output_use;
    }

    bool needs_vertex_output = stage == Utils::Shader::VERTEX && true == test_case.m_is_input &&
                               test_case.m_stage != Utils::Shader::VERTEX;
    bool needs_fragment_input = stage == Utils::Shader::FRAGMENT && false == test_case.m_is_input &&
                                test_case.m_stage != Utils::Shader::FRAGMENT;

    if (test_case.m_stage == stage || needs_vertex_output || needs_fragment_input)
    {
        size_t position = 0;

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                direction = "in";
                var_use   = input_use;
                source    = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                array  = "[]";
                index  = "[gl_InvocationID]";
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::VERTEX:
                direction = "out";
                var_use   = output_use;
                source    = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("BLOCK_DEFINITION", position, block_definition, source);
        position = 0;
        Utils::replaceToken("DIRECTION", position, direction, source);
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("INDEX", index, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string VaryingBlockAutomaticMemberLocationsTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage) << ", direction: ";

    if (true == test_case.m_is_input)
    {
        stream << "input";
    }
    else
    {
        stream << "output";
    }

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint VaryingBlockAutomaticMemberLocationsTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingBlockAutomaticMemberLocationsTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void VaryingBlockAutomaticMemberLocationsTest::testInit()
{
    testCase test_case_in  = {true, (Utils::Shader::STAGES)m_stage};
    testCase test_case_out = {false, (Utils::Shader::STAGES)m_stage};

    if (Utils::Shader::VERTEX != m_stage)
    {
        m_test_cases.push_back(test_case_in);
    }

    if (Utils::Shader::FRAGMENT != m_stage)
    {
        m_test_cases.push_back(test_case_out);
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingLocationLimitTest::VaryingLocationLimitTest(deqp::Context &context,
                                                   GLuint type,
                                                   GLuint stage)
    : NegativeTestBase(
          context,
          "varying_location_limit",
          "Test verifies that compiler reports error when location qualifier exceeds limits"),
      m_type(type),
      m_stage(stage)
{
    std::string name = ("varying_location_limit_");
    name.append(getTypeName(m_type));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string VaryingLocationLimitTest::getShaderSource(GLuint test_case_index,
                                                      Utils::Shader::STAGES stage)
{
#if DEBUG_NEG_REMOVE_ERROR
    static const GLchar *var_definition =
        "layout (location = LAST /* + 1 */) FLAT DIRECTION TYPE gokuARRAY;\n";
#else
    static const GLchar *var_definition =
        "layout (location = LAST + 1) FLAT DIRECTION TYPE gokuARRAY;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *input_use =
        "    if (TYPE(0) == gokuINDEX)\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *output_use =
        "    gokuINDEX = TYPE(0);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gokuINDEX = TYPE(1);\n"
        "    }\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    fs_out += result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "PERVERTEX" /* Separable programs require explicit declaration of gl_PerVertex */
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "PERVERTEX" /* Separable programs require explicit declaration of gl_PerVertex */
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs += result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case      = m_test_cases[test_case_index];
    size_t position          = 0;
    const GLchar *per_vertex = !isSeparable(test_case_index) ? ""
                                                             : "out gl_PerVertex {\n"
                                                               "vec4 gl_Position;\n"
                                                               "};\n"
                                                               "\n";

    bool needs_vertex_output = stage == Utils::Shader::VERTEX && true == test_case.m_is_input &&
                               test_case.m_stage != Utils::Shader::VERTEX;
    bool needs_fragment_input = stage == Utils::Shader::FRAGMENT && false == test_case.m_is_input &&
                                test_case.m_stage != Utils::Shader::FRAGMENT;

    if (test_case.m_stage == stage || needs_fragment_input || needs_vertex_output)
    {
        const GLchar *array = "";
        GLchar buffer[16];
        const GLchar *direction          = "in ";
        const GLchar *flat               = "";
        const GLchar *index              = "";
        GLuint last                      = getLastInputLocation(stage, test_case.m_type, 0, true);
        const GLchar *type_name          = test_case.m_type.GetGLSLTypeName();
        Utils::Variable::STORAGE storage = Utils::Variable::VARYING_INPUT;
        const GLchar *var_use            = input_use;

        if (false == test_case.m_is_input && !needs_fragment_input)
        {
            direction = "out";
            last      = getLastOutputLocation(stage, test_case.m_type, 0, true);
            storage   = Utils::Variable::VARYING_OUTPUT;
            var_use   = output_use;
        }

        if (isFlatRequired(stage, test_case.m_type, storage))
        {
            flat = "flat";
        }

        sprintf(buffer, "%d", last);

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                direction = "in ";
                last      = getLastInputLocation(stage, test_case.m_type, 0, true);
                storage   = Utils::Variable::VARYING_INPUT;
                var_use   = input_use;

                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                Utils::replaceToken("PERVERTEX", position, per_vertex, source);
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                array  = "[]";
                index  = "[gl_InvocationID]";
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::VERTEX:
                direction = "out";
                last      = getLastOutputLocation(stage, test_case.m_type, 0, true);
                storage   = Utils::Variable::VARYING_OUTPUT;
                var_use   = output_use;

                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("LAST", position, buffer, source);
        Utils::replaceToken("FLAT", position, flat, source);
        Utils::replaceToken("DIRECTION", position, direction, source);
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("TYPE", type_name, source);
        Utils::replaceAllTokens("INDEX", index, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                Utils::replaceToken("PERVERTEX", position, per_vertex, source);
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string VaryingLocationLimitTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage)
           << " type: " << test_case.m_type.GetGLSLTypeName() << ", direction: ";

    if (true == test_case.m_is_input)
    {
        stream << "input";
    }
    else
    {
        stream << "output";
    }

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint VaryingLocationLimitTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingLocationLimitTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Selects if the test case should use a separable program
 *
 * @param test_case_index Id of test case
 *
 * @return whether the test should use separable programs or not
 **/
bool VaryingLocationLimitTest::isSeparable(const GLuint test_case_index)
{
    const testCase &test_case = m_test_cases[test_case_index];

    return test_case.m_is_input && test_case.m_stage != Utils::Shader::VERTEX;
}

/** Prepare all test cases
 *
 **/
void VaryingLocationLimitTest::testInit()
{
    const Utils::Type &type = getType(m_type);

    testCase test_case_in  = {true, type, (Utils::Shader::STAGES)m_stage};
    testCase test_case_out = {false, type, (Utils::Shader::STAGES)m_stage};

    if (Utils::Shader::VERTEX != m_stage)
    {
        m_test_cases.push_back(test_case_in);
    }

    if (Utils::Shader::FRAGMENT != m_stage)
    {
        m_test_cases.push_back(test_case_out);
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingComponentsTest::VaryingComponentsTest(deqp::Context &context,
                                             COMPONENTS_LAYOUT layout,
                                             Utils::Type::TYPES type)
    : VaryingLocationsTest(context,
                           "varying_components",
                           "Test verifies that input and output components are respected"),
      m_layout(layout),
      m_type(type)
{
    std::string name = ("varying_components_");
    name.append(getComponentsLayoutName(m_layout));
    name.append("_");
    name.append(getTypesName(type));

    VaryingLocationsTest::m_name = name.c_str();
}

/** Constructor
 *
 * @param context          Test framework context
 * @param test_name        Name of test
 * @param test_description Description of test
 **/
VaryingComponentsTest::VaryingComponentsTest(deqp::Context &context,
                                             const glw::GLchar *test_name,
                                             const glw::GLchar *test_description,
                                             COMPONENTS_LAYOUT layout,
                                             Utils::Type::TYPES type)
    : VaryingLocationsTest(context, test_name, test_description), m_layout(layout), m_type(type)
{
    std::string name = ("varying_components_");
    name.append(getComponentsLayoutName(m_layout));
    name.append("_");
    name.append(getTypesName(type));

    VaryingLocationsTest::m_name = name.c_str();
}

/** Get interface of program
 *
 * @param test_case_index     Test case
 * @param program_interface   Interface of program
 * @param varying_passthrough Collection of connections between in and out variables
 **/
void VaryingComponentsTest::getProgramInterface(GLuint test_case_index,
                                                Utils::ProgramInterface &program_interface,
                                                Utils::VaryingPassthrough &varying_passthrough)
{
    GLuint array_length           = getArrayLength();
    const testCase &test_case     = m_test_cases[test_case_index];
    const Utils::Type vector_type = Utils::Type::GetType(test_case.m_type, 1, 4);
    Utils::ShaderInterface si     = program_interface.GetShaderInterface(Utils::Shader::VERTEX);

    /* Zero means no array, however we still need at least 1 slot of data */
    if (0 == array_length)
    {
        array_length += 1;
    }

    /* Generate data */
    const std::vector<GLubyte> &data = vector_type.GenerateDataPacked();
    const size_t data_size           = data.size();

    /* Prepare data for variables */
    m_data.resize(array_length * data_size);

    GLubyte *dst       = &m_data[0];
    const GLubyte *src = &data[0];

    for (GLuint i = 0; i < array_length; ++i)
    {
        memcpy(dst + data_size * i, src, data_size);
    }

    /* Prepare interface for each stage */
    prepareShaderStage(Utils::Shader::FRAGMENT, vector_type, program_interface, test_case,
                       varying_passthrough);
    prepareShaderStage(Utils::Shader::GEOMETRY, vector_type, program_interface, test_case,
                       varying_passthrough);
    prepareShaderStage(Utils::Shader::TESS_CTRL, vector_type, program_interface, test_case,
                       varying_passthrough);
    prepareShaderStage(Utils::Shader::TESS_EVAL, vector_type, program_interface, test_case,
                       varying_passthrough);
    prepareShaderStage(Utils::Shader::VERTEX, vector_type, program_interface, test_case,
                       varying_passthrough);
}

/** Get type name
 *
 * @param test_case_index Index of test case
 *
 * @return Name of type test in test_case_index
 **/
std::string VaryingComponentsTest::getTestCaseName(glw::GLuint test_case_index)
{
    std::string name;

    const testCase &test_case = m_test_cases[test_case_index];

    name = "Type: ";

    switch (test_case.m_type)
    {
        case Utils::Type::Double:
            name.append(Utils::Type::_double.GetGLSLTypeName());
            break;
        case Utils::Type::Float:
            name.append(Utils::Type::_float.GetGLSLTypeName());
            break;
        case Utils::Type::Int:
            name.append(Utils::Type::_int.GetGLSLTypeName());
            break;
        case Utils::Type::Uint:
            name.append(Utils::Type::uint.GetGLSLTypeName());
            break;
    }

    name.append(", layout: ");

    switch (test_case.m_layout)
    {
        case G64VEC2:
            name.append("G64VEC2");
            break;
        case G64SCALAR_G64SCALAR:
            name.append("G64SCALAR_G64SCALAR");
            break;
        case GVEC4:
            name.append("GVEC4");
            break;
        case SCALAR_GVEC3:
            name.append("SCALAR_GVEC3");
            break;
        case GVEC3_SCALAR:
            name.append("GVEC3_SCALAR");
            break;
        case GVEC2_GVEC2:
            name.append("GVEC2_GVEC2");
            break;
        case GVEC2_SCALAR_SCALAR:
            name.append("GVEC2_SCALAR_SCALAR");
            break;
        case SCALAR_GVEC2_SCALAR:
            name.append("SCALAR_GVEC2_SCALAR");
            break;
        case SCALAR_SCALAR_GVEC2:
            name.append("SCALAR_SCALAR_GVEC2");
            break;
        case SCALAR_SCALAR_SCALAR_SCALAR:
            name.append("SCALAR_SCALAR_SCALAR_SCALAR");
            break;
    }

    return name;
}

/** Returns number of types to test
 *
 * @return Number of types
 **/
glw::GLuint VaryingComponentsTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/* Prepare test cases */
void VaryingComponentsTest::testInit()
{
    m_test_cases.push_back(testCase(m_layout, m_type));
}

std::string VaryingComponentsTest::getComponentsLayoutName(COMPONENTS_LAYOUT layout)
{
    switch (layout)
    {
        case G64VEC2:
            return "g64vec2";
        case G64SCALAR_G64SCALAR:
            return "g64scalar_g64scalar";
        case GVEC4:
            return "gvec4";
        case SCALAR_GVEC3:
            return "scalar_gvec3";
        case GVEC3_SCALAR:
            return "gvec3_scalar";
        case GVEC2_GVEC2:
            return "gvec2_gvec2";
        case GVEC2_SCALAR_SCALAR:
            return "gvec2_scalar_scalar";
        case SCALAR_GVEC2_SCALAR:
            return "scalar_gvec2_scalar";
        case SCALAR_SCALAR_GVEC2:
            return "scalar_scalar_gvec2";
        case SCALAR_SCALAR_SCALAR_SCALAR:
            return "scalar_scalar_scalar_scalar";
        default:
            return "default";
    }
}

std::string VaryingComponentsTest::getTypesName(Utils::Type::TYPES type)
{
    switch (type)
    {
        case Utils::Type::Double:
            return "double";
        case Utils::Type::Float:
            return "float";
        case Utils::Type::Int:
            return "int";
        case Utils::Type::Uint:
            return "uint";
        default:
            return "default";
    }
}

/** Inform that test use components
 *
 * @param ignored
 *
 * @return true
 **/
bool VaryingComponentsTest::useComponentQualifier(glw::GLuint /* test_case_index */)
{
    return true;
}

/** Get length of arrays that should be used during test
 *
 * @return 0u - no array at all
 **/
GLuint VaryingComponentsTest::getArrayLength()
{
    return 0;
}

std::string VaryingComponentsTest::prepareGlobals(GLuint last_in_location, GLuint last_out_location)
{
    std::string globals = VaryingLocationsTest::prepareGlobals(last_in_location, last_out_location);

    globals.append(
        "const uint comp_x = 0u;\n"
        "const uint comp_y = 1u;\n"
        "const uint comp_z = 2u;\n"
        "const uint comp_w = 3u;\n");

    return globals;
}

/**
 *
 **/
std::string VaryingComponentsTest::prepareName(const glw::GLchar *name,
                                               glw::GLint location,
                                               glw::GLint component,
                                               Utils::Shader::STAGES stage,
                                               Utils::Variable::STORAGE storage)
{
    GLchar buffer[16];
    std::string result   = "PREFIXNAME_lLOCATION_cCOMPONENT";
    size_t position      = 0;
    const GLchar *prefix = Utils::ProgramInterface::GetStagePrefix(stage, storage);

    Utils::replaceToken("PREFIX", position, prefix, result);
    Utils::replaceToken("NAME", position, name, result);

    sprintf(buffer, "%d", location);
    Utils::replaceToken("LOCATION", position, buffer, result);

    sprintf(buffer, "%d", component);
    Utils::replaceToken("COMPONENT", position, buffer, result);

    return result;
}

std::string VaryingComponentsTest::prepareQualifiers(const glw::GLchar *location,
                                                     const glw::GLchar *component,
                                                     const glw::GLchar *interpolation)
{
    size_t position        = 0;
    std::string qualifiers = "layout (location = LOCATION, component = COMPONENT) INTERPOLATION";

    Utils::replaceToken("LOCATION", position, location, qualifiers);
    Utils::replaceToken("COMPONENT", position, component, qualifiers);
    Utils::replaceToken("INTERPOLATION", position, interpolation, qualifiers);

    return qualifiers;
}

/**
 *
 **/
void VaryingComponentsTest::prepareShaderStage(Utils::Shader::STAGES stage,
                                               const Utils::Type &vector_type,
                                               Utils::ProgramInterface &program_interface,
                                               const testCase &test_case,
                                               Utils::VaryingPassthrough &varying_passthrough)
{
    const GLuint array_length = getArrayLength();
    const Utils::Type &basic_type =
        Utils::Type::GetType(vector_type.m_basic_type, 1 /* n_cols */, 1 /* n_rows */);
    descriptor desc_in[8];
    descriptor desc_out[8];
    const GLuint first_in_loc   = 0;
    const GLuint first_out_loc  = 0;
    const GLchar *interpolation = "";
    const GLuint last_in_loc    = getLastInputLocation(stage, vector_type, array_length, false);
    GLuint last_out_loc         = 0;
    GLuint n_desc               = 0;
    Utils::ShaderInterface &si  = program_interface.GetShaderInterface(stage);

    /* Select interpolation */
    if ((Utils::Shader::FRAGMENT == stage) || (Utils::Shader::GEOMETRY == stage))
    {
        interpolation = " flat";
    }

    if (Utils::Shader::FRAGMENT != stage)
    {
        last_out_loc = getLastOutputLocation(stage, vector_type, array_length, false);
    }

    switch (test_case.m_layout)
    {
        case G64VEC2:
            n_desc = 2;
            desc_in[0].assign(0, "comp_x", first_in_loc, "first_input_location", 2, "g64vec2");
            desc_in[1].assign(0, "comp_x", last_in_loc, "last_input_location", 2, "g64vec2");

            desc_out[0].assign(0, "comp_x", first_out_loc, "first_output_location", 2, "g64vec2");
            desc_out[1].assign(0, "comp_x", last_out_loc, "last_output_location", 2, "g64vec2");
            break;

        case G64SCALAR_G64SCALAR:
            n_desc = 4;
            desc_in[0].assign(0, "comp_x", first_in_loc, "first_input_location", 1, "g64scalar");
            desc_in[1].assign(0, "comp_x", last_in_loc, "last_input_location", 1, "g64scalar");
            desc_in[2].assign(2, "comp_z", first_in_loc, "first_input_location", 1, "g64scalar");
            desc_in[3].assign(2, "comp_z", last_in_loc, "last_input_location", 1, "g64scalar");

            desc_out[0].assign(0, "comp_x", first_out_loc, "first_output_location", 1, "g64scalar");
            desc_out[1].assign(0, "comp_x", last_out_loc, "last_output_location", 1, "g64scalar");
            desc_out[2].assign(2, "comp_z", first_out_loc, "first_output_location", 1, "g64scalar");
            desc_out[3].assign(2, "comp_z", last_out_loc, "last_output_location", 1, "g64scalar");
            break;
        case GVEC4:
            n_desc = 2;
            desc_in[0].assign(0, "comp_x", first_in_loc, "first_input_location", 4, "gvec4");
            desc_in[1].assign(0, "comp_x", last_in_loc, "last_input_location", 4, "gvec4");

            desc_out[0].assign(0, "comp_x", first_out_loc, "first_output_location", 4, "gvec4");
            desc_out[1].assign(0, "comp_x", last_out_loc, "last_output_location", 4, "gvec4");
            break;
        case SCALAR_GVEC3:
            n_desc = 4;
            desc_in[0].assign(0, "comp_x", first_in_loc, "first_input_location", 1, "scalar");
            desc_in[1].assign(0, "comp_x", last_in_loc, "last_input_location", 1, "scalar");
            desc_in[2].assign(1, "comp_y", first_in_loc, "first_input_location", 3, "gvec3");
            desc_in[3].assign(1, "comp_y", last_in_loc, "last_input_location", 3, "gvec3");

            desc_out[0].assign(0, "comp_x", first_out_loc, "first_output_location", 1, "scalar");
            desc_out[1].assign(0, "comp_x", last_out_loc, "last_output_location", 1, "scalar");
            desc_out[2].assign(1, "comp_y", first_out_loc, "first_output_location", 3, "gvec3");
            desc_out[3].assign(1, "comp_y", last_out_loc, "last_output_location", 3, "gvec3");
            break;
        case GVEC3_SCALAR:
            n_desc = 4;
            desc_in[0].assign(0, "comp_x", first_in_loc, "first_input_location", 3, "gvec3");
            desc_in[1].assign(0, "comp_x", last_in_loc, "last_input_location", 3, "gvec3");
            desc_in[2].assign(3, "comp_w", first_in_loc, "first_input_location", 1, "scalar");
            desc_in[3].assign(3, "comp_w", last_in_loc, "last_input_location", 1, "scalar");

            desc_out[0].assign(0, "comp_x", first_out_loc, "first_output_location", 3, "gvec3");
            desc_out[1].assign(0, "comp_x", last_out_loc, "last_output_location", 3, "gvec3");
            desc_out[2].assign(3, "comp_w", first_out_loc, "first_output_location", 1, "scalar");
            desc_out[3].assign(3, "comp_w", last_out_loc, "last_output_location", 1, "scalar");
            break;
        case GVEC2_GVEC2:
            n_desc = 4;
            desc_in[0].assign(0, "comp_x", first_in_loc, "first_input_location", 2, "gvec2");
            desc_in[1].assign(0, "comp_x", last_in_loc, "last_input_location", 2, "gvec2");
            desc_in[2].assign(2, "comp_z", first_in_loc, "first_input_location", 2, "gvec2");
            desc_in[3].assign(2, "comp_z", last_in_loc, "last_input_location", 2, "gvec2");

            desc_out[0].assign(0, "comp_x", first_out_loc, "first_output_location", 2, "gvec2");
            desc_out[1].assign(0, "comp_x", last_out_loc, "last_output_location", 2, "gvec2");
            desc_out[2].assign(2, "comp_z", first_out_loc, "first_output_location", 2, "gvec2");
            desc_out[3].assign(2, "comp_z", last_out_loc, "last_output_location", 2, "gvec2");
            break;
        case GVEC2_SCALAR_SCALAR:
            n_desc = 6;
            desc_in[0].assign(0, "comp_x", first_in_loc, "first_input_location", 2, "gvec2");
            desc_in[1].assign(0, "comp_x", last_in_loc, "last_input_location", 2, "gvec2");
            desc_in[2].assign(2, "comp_z", first_in_loc, "first_input_location", 1, "scalar");
            desc_in[3].assign(2, "comp_z", last_in_loc, "last_input_location", 1, "scalar");
            desc_in[4].assign(3, "comp_w", first_in_loc, "first_input_location", 1, "scalar");
            desc_in[5].assign(3, "comp_w", last_in_loc, "last_input_location", 1, "scalar");

            desc_out[0].assign(0, "comp_x", first_out_loc, "first_output_location", 2, "gvec2");
            desc_out[1].assign(0, "comp_x", last_out_loc, "last_output_location", 2, "gvec2");
            desc_out[2].assign(2, "comp_z", first_out_loc, "first_output_location", 1, "scalar");
            desc_out[3].assign(2, "comp_z", last_out_loc, "last_output_location", 1, "scalar");
            desc_out[4].assign(3, "comp_w", first_out_loc, "first_output_location", 1, "scalar");
            desc_out[5].assign(3, "comp_w", last_out_loc, "last_output_location", 1, "scalar");
            break;
        case SCALAR_GVEC2_SCALAR:
            n_desc = 6;
            desc_in[0].assign(0, "comp_x", first_in_loc, "first_input_location", 1, "scalar");
            desc_in[1].assign(0, "comp_x", last_in_loc, "last_input_location", 1, "scalar");
            desc_in[2].assign(1, "comp_y", first_in_loc, "first_input_location", 2, "gvec2");
            desc_in[3].assign(1, "comp_y", last_in_loc, "last_input_location", 2, "gvec2");
            desc_in[4].assign(3, "comp_w", first_in_loc, "first_input_location", 1, "scalar");
            desc_in[5].assign(3, "comp_w", last_in_loc, "last_input_location", 1, "scalar");

            desc_out[0].assign(0, "comp_x", first_out_loc, "first_output_location", 1, "scalar");
            desc_out[1].assign(0, "comp_x", last_out_loc, "last_output_location", 1, "scalar");
            desc_out[2].assign(1, "comp_y", first_out_loc, "first_output_location", 2, "gvec2");
            desc_out[3].assign(1, "comp_y", last_out_loc, "last_output_location", 2, "gvec2");
            desc_out[4].assign(3, "comp_w", first_out_loc, "first_output_location", 1, "scalar");
            desc_out[5].assign(3, "comp_w", last_out_loc, "last_output_location", 1, "scalar");
            break;
        case SCALAR_SCALAR_GVEC2:
            n_desc = 6;
            desc_in[0].assign(0, "comp_x", first_in_loc, "first_input_location", 1, "scalar");
            desc_in[1].assign(0, "comp_x", last_in_loc, "last_input_location", 1, "scalar");
            desc_in[2].assign(1, "comp_y", first_in_loc, "first_input_location", 1, "scalar");
            desc_in[3].assign(1, "comp_y", last_in_loc, "last_input_location", 1, "scalar");
            desc_in[4].assign(2, "comp_z", first_in_loc, "first_input_location", 2, "gvec2");
            desc_in[5].assign(2, "comp_z", last_in_loc, "last_input_location", 2, "gvec2");

            desc_out[0].assign(0, "comp_x", first_out_loc, "first_output_location", 1, "scalar");
            desc_out[1].assign(0, "comp_x", last_out_loc, "last_output_location", 1, "scalar");
            desc_out[2].assign(1, "comp_y", first_out_loc, "first_output_location", 1, "scalar");
            desc_out[3].assign(1, "comp_y", last_out_loc, "last_output_location", 1, "scalar");
            desc_out[4].assign(2, "comp_z", first_out_loc, "first_output_location", 2, "gvec2");
            desc_out[5].assign(2, "comp_z", last_out_loc, "last_output_location", 2, "gvec2");
            break;
        case SCALAR_SCALAR_SCALAR_SCALAR:
            n_desc = 8;
            desc_in[0].assign(0, "comp_x", first_in_loc, "first_input_location", 1, "scalar");
            desc_in[1].assign(0, "comp_x", last_in_loc, "last_input_location", 1, "scalar");
            desc_in[2].assign(1, "comp_y", first_in_loc, "first_input_location", 1, "scalar");
            desc_in[3].assign(1, "comp_y", last_in_loc, "last_input_location", 1, "scalar");
            desc_in[4].assign(2, "comp_z", first_in_loc, "first_input_location", 1, "scalar");
            desc_in[5].assign(2, "comp_z", last_in_loc, "last_input_location", 1, "scalar");
            desc_in[6].assign(3, "comp_w", first_in_loc, "first_input_location", 1, "scalar");
            desc_in[7].assign(3, "comp_w", last_in_loc, "last_input_location", 1, "scalar");

            desc_out[0].assign(0, "comp_x", first_out_loc, "first_output_location", 1, "scalar");
            desc_out[1].assign(0, "comp_x", last_out_loc, "last_output_location", 1, "scalar");
            desc_out[2].assign(1, "comp_y", first_out_loc, "first_output_location", 1, "scalar");
            desc_out[3].assign(1, "comp_y", last_out_loc, "last_output_location", 1, "scalar");
            desc_out[4].assign(2, "comp_z", first_out_loc, "first_output_location", 1, "scalar");
            desc_out[5].assign(2, "comp_z", last_out_loc, "last_output_location", 1, "scalar");
            desc_out[6].assign(3, "comp_w", first_out_loc, "first_output_location", 1, "scalar");
            desc_out[7].assign(3, "comp_w", last_out_loc, "last_output_location", 1, "scalar");
            break;
    }

    for (GLuint i = 0; i < n_desc; ++i)
    {
        const descriptor &in_desc = desc_in[i];

        Utils::Variable *in = prepareVarying(basic_type, in_desc, interpolation, si, stage,
                                             Utils::Variable::VARYING_INPUT);

        if (Utils::Shader::FRAGMENT != stage)
        {
            const descriptor &out_desc = desc_out[i];

            Utils::Variable *out = prepareVarying(basic_type, out_desc, interpolation, si, stage,
                                                  Utils::Variable::VARYING_OUTPUT);

            varying_passthrough.Add(stage, in, out);
        }
    }

    si.m_globals = prepareGlobals(last_in_loc, last_out_loc);
}

/**
 *
 **/
Utils::Variable *VaryingComponentsTest::prepareVarying(const Utils::Type &basic_type,
                                                       const descriptor &desc,
                                                       const GLchar *interpolation,
                                                       Utils::ShaderInterface &si,
                                                       Utils::Shader::STAGES stage,
                                                       Utils::Variable::STORAGE storage)
{
    const GLuint array_length   = getArrayLength();
    const GLuint component_size = Utils::Type::_float.GetSize();
    const std::string &name =
        prepareName(desc.m_name, desc.m_location, desc.m_component, stage, storage);
    const GLuint offset = desc.m_component * component_size;
    const std::string &qual =
        prepareQualifiers(desc.m_location_str, desc.m_component_str, interpolation);
    const GLuint size = desc.m_n_rows * basic_type.GetSize();
    const Utils::Type &type =
        Utils::Type::GetType(basic_type.m_basic_type, 1 /* n_columns */, desc.m_n_rows);
    Utils::Variable *var = 0;

    if (Utils::Variable::VARYING_INPUT == storage)
    {
        var = si.Input(
            name.c_str(), qual.c_str() /* qualifiers */, desc.m_component /* expected_componenet */,
            desc.m_location /* expected_location */, type, /* built_in_type */
            GL_FALSE /* normalized */, array_length /* n_array_elements */, 0u /* stride */,
            offset /* offset */, (GLvoid *)&m_data[offset] /* data */, size /* data_size */);
    }
    else
    {
        var = si.Output(
            name.c_str(), qual.c_str() /* qualifiers */, desc.m_component /* expected_componenet */,
            desc.m_location /* expected_location */, type, /* built_in_type */
            GL_FALSE /* normalized */, array_length /* n_array_elements */, 0u /* stride */,
            offset /* offset */, (GLvoid *)&m_data[offset] /* data */, size /* data_size */);
    }

    return var;
}

void VaryingComponentsTest::descriptor::assign(glw::GLint component,
                                               const glw::GLchar *component_str,
                                               glw::GLint location,
                                               const glw::GLchar *location_str,
                                               glw::GLuint n_rows,
                                               const glw::GLchar *name)
{
    m_component     = component;
    m_component_str = component_str;
    m_location      = location;
    m_location_str  = location_str;
    m_n_rows        = n_rows;
    m_name          = name;
}

VaryingComponentsTest::testCase::testCase(COMPONENTS_LAYOUT layout, Utils::Type::TYPES type)
    : m_layout(layout), m_type(type)
{}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingArrayComponentsTest::VaryingArrayComponentsTest(deqp::Context &context,
                                                       COMPONENTS_LAYOUT layout,
                                                       Utils::Type::TYPES type)
    : VaryingComponentsTest(
          context,
          "varying_array_components",
          "Test verifies that input and output components are respected for arrays",
          layout,
          type)
{
    std::string name = ("varying_array_components_");
    name.append(getComponentsLayoutName(m_layout));
    name.append("_");
    name.append(getTypesName(type));

    VaryingLocationsTest::m_name = name.c_str();
}

/** Get length of arrays that should be used during test
 *
 * @return 4u
 **/
GLuint VaryingArrayComponentsTest::getArrayLength()
{
    return 4u;
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingInvalidValueComponentTest::VaryingInvalidValueComponentTest(deqp::Context &context,
                                                                   GLuint type)
    : NegativeTestBase(context,
                       "varying_invalid_value_component",
                       "Test verifies that compiler reports error when "
                       "using an invalid value in the component "
                       "qualification for a specific type"),
      m_type(type)
{
    std::string name = ("varying_invalid_value_component_");
    name.append(getTypeName(type));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string VaryingInvalidValueComponentTest::getShaderSource(GLuint test_case_index,
                                                              Utils::Shader::STAGES stage)
{
#if DEBUG_NEG_REMOVE_ERROR
    static const GLchar *var_definition_arr =
        "layout (location = 1 /*, component = COMPONENT */) FLAT DIRECTION TYPE gokuARRAY[1];\n";
    static const GLchar *var_definition_one =
        "layout (location = 1 /*, component = COMPONENT */) FLAT DIRECTION TYPE gokuARRAY;\n";
#else
    static const GLchar *var_definition_arr =
        "layout (location = 1, component = COMPONENT) FLAT DIRECTION TYPE gokuARRAY[1];\n";
    static const GLchar *var_definition_one =
        "layout (location = 1, component = COMPONENT) FLAT DIRECTION TYPE gokuARRAY;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *input_use_arr =
        "    if (TYPE(0) == gokuINDEX[0])\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *input_use_one =
        "    if (TYPE(0) == gokuINDEX)\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *output_use_arr =
        "    gokuINDEX[0] = TYPE(0);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gokuINDEX[0] = TYPE(1);\n"
        "    }\n";
    static const GLchar *output_use_one =
        "    gokuINDEX = TYPE(0);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gokuINDEX = TYPE(1);\n"
        "    }\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    fs_out += result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs += result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        const GLchar *array = "";
        GLchar buffer[16];
        const GLchar *var_definition     = 0;
        const GLchar *direction          = "in";
        const GLchar *index              = "";
        size_t position                  = 0;
        const GLchar *type_name          = test_case.m_type.GetGLSLTypeName();
        const GLchar *var_use            = 0;
        Utils::Variable::STORAGE storage = Utils::Variable::VARYING_INPUT;
        const GLchar *flat               = "";

        if (false == test_case.m_is_input)
        {
            direction = "out";
            storage   = Utils::Variable::VARYING_OUTPUT;

            if (false == test_case.m_is_array)
            {
                var_definition = var_definition_one;
                var_use        = output_use_one;
            }
            else
            {
                var_definition = var_definition_arr;
                var_use        = output_use_arr;
            }
        }
        else
        {
            if (false == test_case.m_is_array)
            {
                var_definition = var_definition_one;
                var_use        = Utils::Shader::VERTEX == stage ? input_use_one : "\n";
            }
            else
            {
                var_definition = var_definition_arr;
                var_use        = Utils::Shader::VERTEX == stage ? input_use_arr : "\n";
            }
        }

        if (isFlatRequired(stage, test_case.m_type, storage, true))
        {
            flat = "flat";
        }

        sprintf(buffer, "%d", test_case.m_component);

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                array  = "[]";
                index  = "[gl_InvocationID]";
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("COMPONENT", position, buffer, source);
        Utils::replaceToken("FLAT", position, flat, source);
        Utils::replaceToken("DIRECTION", position, direction, source);
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("TYPE", type_name, source);
        Utils::replaceAllTokens("INDEX", index, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string VaryingInvalidValueComponentTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage)
           << " type: " << test_case.m_type.GetGLSLTypeName();

    if (true == test_case.m_is_array)
    {
        stream << "[1]";
    }

    stream << ", direction: ";

    if (true == test_case.m_is_input)
    {
        stream << "input";
    }
    else
    {
        stream << "output";
    }

    stream << ", component: " << test_case.m_component;

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint VaryingInvalidValueComponentTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingInvalidValueComponentTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void VaryingInvalidValueComponentTest::testInit()
{
    const Utils::Type &type                     = getType(m_type);
    const std::vector<GLuint> &valid_components = type.GetValidComponents();

    std::vector<GLuint> every_component(4, 0);
    every_component[1] = 1;
    every_component[2] = 2;
    every_component[3] = 3;
    std::vector<GLuint> invalid_components;

    std::set_symmetric_difference(every_component.begin(), every_component.end(),
                                  valid_components.begin(), valid_components.end(),
                                  std::back_inserter(invalid_components));

    for (std::vector<GLuint>::const_iterator it_invalid_components = invalid_components.begin();
         it_invalid_components != invalid_components.end(); ++it_invalid_components)
    {
        for (GLuint stage = 0; stage < Utils::Shader::STAGE_MAX; ++stage)
        {
            if (Utils::Shader::COMPUTE == stage)
            {
                continue;
            }

            testCase test_case_in_arr  = {*it_invalid_components, true, true,
                                          (Utils::Shader::STAGES)stage, type};
            testCase test_case_in_one  = {*it_invalid_components, true, false,
                                          (Utils::Shader::STAGES)stage, type};
            testCase test_case_out_arr = {*it_invalid_components, false, true,
                                          (Utils::Shader::STAGES)stage, type};
            testCase test_case_out_one = {*it_invalid_components, false, false,
                                          (Utils::Shader::STAGES)stage, type};

            m_test_cases.push_back(test_case_in_arr);
            m_test_cases.push_back(test_case_in_one);

            if (Utils::Shader::FRAGMENT != stage)
            {
                m_test_cases.push_back(test_case_out_arr);
                m_test_cases.push_back(test_case_out_one);
            }
        }
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingExceedingComponentsTest::VaryingExceedingComponentsTest(deqp::Context &context, GLuint type)
    : NegativeTestBase(
          context,
          "varying_exceeding_components",
          "Test verifies that compiler reports error when component qualifier exceeds limits"),
      m_type(type)
{
    std::string name = ("varying_exceeding_components_");
    name.append(getTypeName(type));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string VaryingExceedingComponentsTest::getShaderSource(GLuint test_case_index,
                                                            Utils::Shader::STAGES stage)
{
#if DEBUG_NEG_REMOVE_ERROR
    static const GLchar *var_definition_arr =
        "layout (location = 1 /*, component = 4 */) FLAT DIRECTION TYPE gokuARRAY[1];\n";
    static const GLchar *var_definition_one =
        "layout (location = 1 /*, component = 4 */) FLAT DIRECTION TYPE gokuARRAY;\n";
#else
    static const GLchar *var_definition_arr =
        "layout (location = 1, component = 4) FLAT DIRECTION TYPE gokuARRAY[1];\n";
    static const GLchar *var_definition_one =
        "layout (location = 1, component = 4) FLAT DIRECTION TYPE gokuARRAY;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *input_use_arr =
        "    if (TYPE(0) == gokuINDEX[0])\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *input_use_one =
        "    if (TYPE(0) == gokuINDEX)\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *output_use_arr =
        "    gokuINDEX[0] = TYPE(0);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gokuINDEX[0] = TYPE(1);\n"
        "    }\n";
    static const GLchar *output_use_one =
        "    gokuINDEX = TYPE(0);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gokuINDEX = TYPE(1);\n"
        "    }\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    fs_out += result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs += result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        const GLchar *array              = "";
        const GLchar *var_definition     = 0;
        const GLchar *direction          = "in";
        const GLchar *index              = "";
        size_t position                  = 0;
        const GLchar *type_name          = test_case.m_type.GetGLSLTypeName();
        const GLchar *var_use            = 0;
        Utils::Variable::STORAGE storage = Utils::Variable::VARYING_INPUT;
        const GLchar *flat               = "";

        if (false == test_case.m_is_input)
        {
            direction = "out";
            storage   = Utils::Variable::VARYING_OUTPUT;

            if (false == test_case.m_is_array)
            {
                var_definition = var_definition_one;
                var_use        = output_use_one;
            }
            else
            {
                var_definition = var_definition_arr;
                var_use        = output_use_arr;
            }
        }
        else
        {
            if (false == test_case.m_is_array)
            {
                var_definition = var_definition_one;
                var_use        = Utils::Shader::VERTEX == stage ? input_use_one : "\n";
            }
            else
            {
                var_definition = var_definition_arr;
                var_use        = Utils::Shader::VERTEX == stage ? input_use_arr : "\n";
            }
        }

        if (isFlatRequired(stage, test_case.m_type, storage, true))
        {
            flat = "flat";
        }

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                array  = "[]";
                index  = "[gl_InvocationID]";
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("FLAT", position, flat, source);
        Utils::replaceToken("DIRECTION", position, direction, source);
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("TYPE", type_name, source);
        Utils::replaceAllTokens("INDEX", index, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string VaryingExceedingComponentsTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage)
           << " type: " << test_case.m_type.GetGLSLTypeName();

    if (true == test_case.m_is_array)
    {
        stream << "[1]";
    }

    stream << ", direction: ";

    if (true == test_case.m_is_input)
    {
        stream << "input";
    }
    else
    {
        stream << "output";
    }

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint VaryingExceedingComponentsTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingExceedingComponentsTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void VaryingExceedingComponentsTest::testInit()
{
    const Utils::Type &type = getType(m_type);

    for (GLuint stage = 0; stage < Utils::Shader::STAGE_MAX; ++stage)
    {
        if (Utils::Shader::COMPUTE == stage)
        {
            continue;
        }

        testCase test_case_in_arr  = {true, true, (Utils::Shader::STAGES)stage, type};
        testCase test_case_in_one  = {true, false, (Utils::Shader::STAGES)stage, type};
        testCase test_case_out_arr = {false, true, (Utils::Shader::STAGES)stage, type};
        testCase test_case_out_one = {false, false, (Utils::Shader::STAGES)stage, type};

        m_test_cases.push_back(test_case_in_arr);
        m_test_cases.push_back(test_case_in_one);

        if (Utils::Shader::FRAGMENT != stage)
        {
            m_test_cases.push_back(test_case_out_arr);
            m_test_cases.push_back(test_case_out_one);
        }
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingComponentWithoutLocationTest::VaryingComponentWithoutLocationTest(deqp::Context &context,
                                                                         GLuint type)
    : NegativeTestBase(context,
                       "varying_component_without_location",
                       "Test verifies that compiler reports error when component qualifier is used "
                       "without location"),
      m_type(type)
{
    std::string name = ("varying_component_without_location_");
    name.append(getTypeName(type));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string VaryingComponentWithoutLocationTest::getShaderSource(GLuint test_case_index,
                                                                 Utils::Shader::STAGES stage)
{
#if DEBUG_NEG_REMOVE_ERROR
    static const GLchar *var_definition =
        "/* layout (component = COMPONENT) */ FLAT DIRECTION TYPE gokuARRAY;\n";
#else
    static const GLchar *var_definition =
        "layout (component = COMPONENT) FLAT DIRECTION TYPE gokuARRAY;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *input_use =
        "    if (TYPE(0) == gokuINDEX)\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *output_use =
        "    gokuINDEX = TYPE(0);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gokuINDEX = TYPE(1);\n"
        "    }\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    fs_out = result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs = result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs = result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        const GLchar *array = "";
        GLchar buffer[16];
        const GLchar *direction          = "in";
        const GLchar *index              = "";
        size_t position                  = 0;
        const GLchar *type_name          = test_case.m_type.GetGLSLTypeName();
        const GLchar *var_use            = Utils::Shader::VERTEX == stage ? input_use : "\n";
        Utils::Variable::STORAGE storage = Utils::Variable::VARYING_INPUT;
        const GLchar *flat               = "";

        if (false == test_case.m_is_input)
        {
            direction = "out";
            storage   = Utils::Variable::VARYING_OUTPUT;
            var_use   = output_use;
        }

        if (isFlatRequired(stage, test_case.m_type, storage, true))
        {
            flat = "flat";
        }

        sprintf(buffer, "%d", test_case.m_component);

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                array  = "[]";
                index  = "[gl_InvocationID]";
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("COMPONENT", position, buffer, source);
        Utils::replaceToken("FLAT", position, flat, source);
        Utils::replaceToken("DIRECTION", position, direction, source);
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("TYPE", type_name, source);
        Utils::replaceAllTokens("INDEX", index, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string VaryingComponentWithoutLocationTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage)
           << " type: " << test_case.m_type.GetGLSLTypeName() << ", direction: ";

    if (true == test_case.m_is_input)
    {
        stream << "input";
    }
    else
    {
        stream << "output";
    }

    stream << ", component: " << test_case.m_component;

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint VaryingComponentWithoutLocationTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingComponentWithoutLocationTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void VaryingComponentWithoutLocationTest::testInit()
{
    const Utils::Type &type                     = getType(m_type);
    const std::vector<GLuint> &valid_components = type.GetValidComponents();

    for (GLuint stage = 0; stage < Utils::Shader::STAGE_MAX; ++stage)
    {
        if (Utils::Shader::COMPUTE == stage)
        {
            continue;
        }

        testCase test_case_in = {valid_components.back(), true, (Utils::Shader::STAGES)stage, type};
        testCase test_case_out = {valid_components.back(), false, (Utils::Shader::STAGES)stage,
                                  type};

        m_test_cases.push_back(test_case_in);

        if (Utils::Shader::FRAGMENT != stage)
        {
            m_test_cases.push_back(test_case_out);
        }
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingComponentOfInvalidTypeTest::VaryingComponentOfInvalidTypeTest(deqp::Context &context,
                                                                     GLuint type,
                                                                     GLuint stage)
    : NegativeTestBase(context,
                       "varying_component_of_invalid_type",
                       "Test verifies that compiler reports error when component qualifier is used "
                       "for invalid type"),
      m_type(type),
      m_stage(stage)
{
    std::string name = ("varying_component_of_invalid_type_");
    name.append(getTypeName(type));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string VaryingComponentOfInvalidTypeTest::getShaderSource(GLuint test_case_index,
                                                               Utils::Shader::STAGES stage)
{
    static const GLchar *block_definition_arr =
        "layout (location = 1COMPONENT) DIRECTION Goku {\n"
        "    FLAT TYPE member;\n"
        "} gokuARRAY[1];\n";
    static const GLchar *block_definition_one =
        "layout (location = 1COMPONENT) DIRECTION Goku {\n"
        "    FLAT TYPE member;\n"
        "} gokuARRAY;\n";
    static const GLchar *matrix_dvec3_dvec4_definition_arr =
        "layout (location = 1COMPONENT) FLAT DIRECTION TYPE gokuARRAY[1];\n";
    static const GLchar *matrix_dvec3_dvec4_definition_one =
        "layout (location = 1COMPONENT) FLAT DIRECTION TYPE gokuARRAY;\n";
    static const GLchar *struct_definition_arr =
        "struct Goku {\n"
        "    TYPE member;\n"
        "};\n"
        "\n"
        "layout (location = 1COMPONENT) FLAT DIRECTION Goku gokuARRAY[1];\n";
    static const GLchar *struct_definition_one =
        "struct Goku {\n"
        "    TYPE member;\n"
        "};\n"
        "\n"
        "layout (location = 1COMPONENT) FLAT DIRECTION Goku gokuARRAY;\n";
    static const GLchar *matrix_dvec3_dvec4_input_use_arr =
        "    if (TYPE(0) == gokuINDEX[0])\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *matrix_dvec3_dvec4_input_use_one =
        "    if (TYPE(0) == gokuINDEX)\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *matrix_dvec3_dvec4_output_use_arr =
        "    gokuINDEX[0] = TYPE(0);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gokuINDEX[0] = TYPE(1);\n"
        "    }\n";
    static const GLchar *matrix_dvec3_dvec4_output_use_one =
        "    gokuINDEX = TYPE(0);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gokuINDEX = TYPE(1);\n"
        "    }\n";
    static const GLchar *member_input_use_arr =
        "    if (TYPE(0) == gokuINDEX[0].member)\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *member_input_use_one =
        "    if (TYPE(0) == gokuINDEX.member)\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *member_output_use_arr =
        "    gokuINDEX[0].member = TYPE(0);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gokuINDEX[0].member = TYPE(1);\n"
        "    }\n";
    static const GLchar *member_output_use_one =
        "    gokuINDEX.member = TYPE(0);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gokuINDEX.member = TYPE(1);\n"
        "    }\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    fs_out += result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs += result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        const GLchar *array = "";
        GLchar buffer[32];
        const GLchar *var_definition     = 0;
        const GLchar *direction          = "in ";
        const GLchar *index              = "";
        size_t position                  = 0;
        const GLchar *type_name          = test_case.m_type.GetGLSLTypeName();
        const GLchar *var_use            = 0;
        Utils::Variable::STORAGE storage = Utils::Variable::VARYING_INPUT;
        const GLchar *flat               = "";

        if (false == test_case.m_is_input)
        {
            direction = "out";
            storage   = Utils::Variable::VARYING_OUTPUT;

            if (false == test_case.m_is_array)
            {
                switch (test_case.m_case)
                {
                    case BLOCK:
                        var_definition = block_definition_one;
                        var_use        = member_output_use_one;
                        break;
                    case MATRIX:
                    case DVEC3_DVEC4:
                        var_definition = matrix_dvec3_dvec4_definition_one;
                        var_use        = matrix_dvec3_dvec4_output_use_one;
                        break;
                    case STRUCT:
                        var_definition = struct_definition_one;
                        var_use        = member_output_use_one;
                        break;
                    default:
                        TCU_FAIL("Invalid enum");
                }
            }
            else
            {
                switch (test_case.m_case)
                {
                    case BLOCK:
                        var_definition = block_definition_arr;
                        var_use        = member_output_use_arr;
                        break;
                    case MATRIX:
                    case DVEC3_DVEC4:
                        var_definition = matrix_dvec3_dvec4_definition_arr;
                        var_use        = matrix_dvec3_dvec4_output_use_arr;
                        break;
                    case STRUCT:
                        var_definition = struct_definition_arr;
                        var_use        = member_output_use_arr;
                        break;
                    default:
                        TCU_FAIL("Invalid enum");
                }
            }
        }
        else
        {
            if (false == test_case.m_is_array)
            {
                switch (test_case.m_case)
                {
                    case BLOCK:
                        var_definition = block_definition_one;
                        var_use = Utils::Shader::VERTEX == stage ? member_input_use_one : "\n";
                        break;
                    case MATRIX:
                    case DVEC3_DVEC4:
                        var_definition = matrix_dvec3_dvec4_definition_one;
                        var_use = Utils::Shader::VERTEX == stage ? matrix_dvec3_dvec4_input_use_one
                                                                 : "\n";
                        break;
                    case STRUCT:
                        var_definition = struct_definition_one;
                        var_use = Utils::Shader::VERTEX == stage ? member_input_use_one : "\n";
                        break;
                    default:
                        TCU_FAIL("Invalid enum");
                }
            }
            else
            {
                switch (test_case.m_case)
                {
                    case BLOCK:
                        var_definition = block_definition_arr;
                        var_use = Utils::Shader::VERTEX == stage ? member_input_use_arr : "\n";
                        break;
                    case MATRIX:
                    case DVEC3_DVEC4:
                        var_definition = matrix_dvec3_dvec4_definition_arr;
                        var_use = Utils::Shader::VERTEX == stage ? matrix_dvec3_dvec4_input_use_arr
                                                                 : "\n";
                        break;
                    case STRUCT:
                        var_definition = struct_definition_arr;
                        var_use = Utils::Shader::VERTEX == stage ? member_input_use_arr : "\n";
                        break;
                    default:
                        TCU_FAIL("Invalid enum");
                }
            }
        }

        if (isFlatRequired(stage, test_case.m_type, storage))
        {
            flat = "flat";
        }

#if DEBUG_NEG_REMOVE_ERROR
        sprintf(buffer, " /* , component = %d */", test_case.m_component);
#else
        sprintf(buffer, ", component = %d", test_case.m_component);
#endif /* DEBUG_NEG_REMOVE_ERROR */

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                array  = "[]";
                index  = "[gl_InvocationID]";
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("COMPONENT", position, buffer, source);
        Utils::replaceToken("DIRECTION", position, direction, source);
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("FLAT", flat, source);
        Utils::replaceAllTokens("TYPE", type_name, source);
        Utils::replaceAllTokens("INDEX", index, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string VaryingComponentOfInvalidTypeTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage)
           << " type: " << test_case.m_type.GetGLSLTypeName();

    if (true == test_case.m_is_array)
    {
        stream << "[1]";
    }

    stream << ", direction: ";

    if (true == test_case.m_is_input)
    {
        stream << "input";
    }
    else
    {
        stream << "output";
    }

    stream << ", component: " << test_case.m_component;

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint VaryingComponentOfInvalidTypeTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingComponentOfInvalidTypeTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void VaryingComponentOfInvalidTypeTest::testInit()
{
    const Utils::Type &type                     = getType(m_type);
    const std::vector<GLuint> &valid_components = type.GetValidComponents();

    /* matrices */
    if (1 != type.m_n_columns)
    {
        testCase test_case_in_arr  = {MATRIX, 0, true, true, (Utils::Shader::STAGES)m_stage, type};
        testCase test_case_in_one  = {MATRIX, 0, false, true, (Utils::Shader::STAGES)m_stage, type};
        testCase test_case_out_arr = {MATRIX, 0, true, false, (Utils::Shader::STAGES)m_stage, type};
        testCase test_case_out_one = {MATRIX, 0, false, false, (Utils::Shader::STAGES)m_stage,
                                      type};

        m_test_cases.push_back(test_case_in_arr);
        m_test_cases.push_back(test_case_in_one);

        if (Utils::Shader::FRAGMENT != m_stage)
        {
            m_test_cases.push_back(test_case_out_arr);
            m_test_cases.push_back(test_case_out_one);
        }
    }
    else if (Utils::Type::Double == type.m_basic_type && 2 < type.m_n_rows) /* dvec3 and dvec4 */
    {
        testCase test_case_in_arr  = {DVEC3_DVEC4, 0, true, true, (Utils::Shader::STAGES)m_stage,
                                      type};
        testCase test_case_in_one  = {DVEC3_DVEC4, 0, false, true, (Utils::Shader::STAGES)m_stage,
                                      type};
        testCase test_case_out_arr = {DVEC3_DVEC4, 0, true, false, (Utils::Shader::STAGES)m_stage,
                                      type};
        testCase test_case_out_one = {DVEC3_DVEC4, 0, false, false, (Utils::Shader::STAGES)m_stage,
                                      type};

        m_test_cases.push_back(test_case_in_arr);
        m_test_cases.push_back(test_case_in_one);

        if (Utils::Shader::FRAGMENT != m_stage)
        {
            m_test_cases.push_back(test_case_out_arr);
            m_test_cases.push_back(test_case_out_one);
        }
    }
    else
    {
        if (valid_components.empty())
        {
            TCU_FAIL("Unhandled type");
        }

        for (GLuint c = BLOCK; c < MAX_CASES; ++c)
        {
            testCase test_case_in_arr  = {(CASES)c, valid_components.back(),        true,
                                          true,     (Utils::Shader::STAGES)m_stage, type};
            testCase test_case_in_one  = {(CASES)c, valid_components.back(),        false,
                                          true,     (Utils::Shader::STAGES)m_stage, type};
            testCase test_case_out_arr = {(CASES)c, valid_components.back(),        true,
                                          false,    (Utils::Shader::STAGES)m_stage, type};
            testCase test_case_out_one = {(CASES)c, valid_components.back(),        false,
                                          false,    (Utils::Shader::STAGES)m_stage, type};

            if (Utils::Shader::VERTEX != m_stage)
            {
                m_test_cases.push_back(test_case_in_arr);
                m_test_cases.push_back(test_case_in_one);
            }

            if (Utils::Shader::FRAGMENT != m_stage)
            {
                m_test_cases.push_back(test_case_out_arr);
                m_test_cases.push_back(test_case_out_one);
            }
        }
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
InputComponentAliasingTest::InputComponentAliasingTest(deqp::Context &context, GLuint type)
    : NegativeTestBase(context,
                       "input_component_aliasing",
                       "Test verifies that compiler reports component aliasing as error"),
      m_type(type)
{
    std::string name = ("input_component_aliasing");
    name.append(getTypeName(type));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string InputComponentAliasingTest::getShaderSource(GLuint test_case_index,
                                                        Utils::Shader::STAGES stage)
{
    static const GLchar *var_definition =
        "layout (location = 1, component = COMPONENT) FLAT in TYPE gohanARRAY;\n"
#if DEBUG_NEG_REMOVE_ERROR
        "/* layout (location = 1, component = COMPONENT) */ FLAT in TYPE gotenARRAY;\n";
#else
        "layout (location = 1, component = COMPONENT) FLAT in TYPE gotenARRAY;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *test_one =
        "    if (TYPE(0) == gohanINDEX)\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    fs_out += result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "layout (location = 1, component = COMPONENT) FLAT out TYPE gohan;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gohan = TYPE(1);\n"
        "\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "layout (location = 1, component = COMPONENT) FLAT out TYPE gohan[];\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gohan[gl_InvocationID] = TYPE(1);\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "layout (location = 1, component = COMPONENT) FLAT out TYPE gohan;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gohan = TYPE(1);\n"
        "\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout (location = 1, component = COMPONENT) FLAT out TYPE gohan;\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gohan = TYPE(1);\n"
        "\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs += result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];
    GLchar buffer_gohan[16];
    const GLchar *type_name = test_case.m_type.GetGLSLTypeName();

    sprintf(buffer_gohan, "%d", test_case.m_component_gohan);

    if (test_case.m_stage == stage)
    {
        const GLchar *array = "";
        GLchar buffer_goten[16];
        const GLchar *flat    = "";
        const GLchar *index   = "";
        size_t position       = 0;
        const GLchar *var_use = test_one;

        if (isFlatRequired(stage, test_case.m_type, Utils::Variable::VARYING_INPUT, true))
        {
            flat = "flat";
        }

        sprintf(buffer_goten, "%d", test_case.m_component_goten);

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = "[]";
                index  = "[0]";
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                array  = "[]";
                index  = "[gl_InvocationID]";
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = "[]";
                index  = "[0]";
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("COMPONENT", position, buffer_gohan, source);
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("COMPONENT", position, buffer_goten, source);
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("FLAT", flat, source);
        Utils::replaceAllTokens("TYPE", type_name, source);
        Utils::replaceAllTokens("INDEX", index, source);
    }
    else
    {
        const GLchar *flat = "";

        if (isFlatRequired(stage, test_case.m_type, Utils::Variable::VARYING_OUTPUT, true))
        {
            flat = "flat";
        }

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceAllTokens("FLAT", flat, source);
        Utils::replaceAllTokens("COMPONENT", buffer_gohan, source);
        Utils::replaceAllTokens("TYPE", type_name, source);
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string InputComponentAliasingTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage)
           << " type: " << test_case.m_type.GetGLSLTypeName()
           << ", components: " << test_case.m_component_gohan << " & "
           << test_case.m_component_goten;

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint InputComponentAliasingTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool InputComponentAliasingTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Selects if compilation failure is expected result
 *
 * @param test_case_index Index of test case
 *
 * @return false for VS that use only single variable, true otherwise
 **/
bool InputComponentAliasingTest::isFailureExpected(GLuint test_case_index)
{
    testCase &test_case = m_test_cases[test_case_index];

    return (Utils::Shader::VERTEX != test_case.m_stage);
}

/** Prepare all test cases
 *
 **/
void InputComponentAliasingTest::testInit()
{
    const Utils::Type &type                     = getType(m_type);
    const std::vector<GLuint> &valid_components = type.GetValidComponents();

    for (std::vector<GLuint>::const_iterator it_gohan = valid_components.begin();
         it_gohan != valid_components.end(); ++it_gohan)
    {
        const GLuint max_component = *it_gohan + type.GetNumComponents();
        for (std::vector<GLuint>::const_iterator it_goten = it_gohan;
             it_goten != valid_components.end() && max_component > *it_goten; ++it_goten)
        {
            for (GLuint stage = 0; stage < Utils::Shader::STAGE_MAX; ++stage)
            {
                /* Skip compute shader */
                if (Utils::Shader::COMPUTE == stage)
                {
                    continue;
                }

                testCase test_case = {*it_gohan, *it_goten, (Utils::Shader::STAGES)stage, type};

                m_test_cases.push_back(test_case);
            }
        }
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
OutputComponentAliasingTest::OutputComponentAliasingTest(deqp::Context &context, GLuint type)
    : NegativeTestBase(context,
                       "output_component_aliasing",
                       "Test verifies that compiler reports component aliasing as error"),
      m_type(type)
{
    std::string name = ("output_component_aliasing_");
    name.append(getTypeName(type));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string OutputComponentAliasingTest::getShaderSource(GLuint test_case_index,
                                                         Utils::Shader::STAGES stage)
{
    static const GLchar *var_definition =
        "layout (location = 1, component = COMPONENT) FLAT out TYPE gohanARRAY;\n"
#if DEBUG_NEG_REMOVE_ERROR
        "/* layout (location = 1, component = COMPONENT) */ FLAT out TYPE gotenARRAY;\n";
#else
        "layout (location = 1, component = COMPONENT) FLAT out TYPE gotenARRAY;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *l_test =
        "    gohanINDEX = TYPE(1);\n"
        "    gotenINDEX = TYPE(0);\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    fs_out += result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs += result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        const GLchar *array = "";
        GLchar buffer_gohan[16];
        GLchar buffer_goten[16];
        const GLchar *flat      = "";
        const GLchar *index     = "";
        size_t position         = 0;
        const GLchar *type_name = test_case.m_type.GetGLSLTypeName();

        if (isFlatRequired(stage, test_case.m_type, Utils::Variable::VARYING_OUTPUT))
        {
            flat = "flat";
        }

        sprintf(buffer_gohan, "%d", test_case.m_component_gohan);
        sprintf(buffer_goten, "%d", test_case.m_component_goten);

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                array  = "[]";
                index  = "[gl_InvocationID]";
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("COMPONENT", position, buffer_gohan, source);
        Utils::replaceToken("FLAT", position, flat, source);
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("COMPONENT", position, buffer_goten, source);
        Utils::replaceToken("FLAT", position, flat, source);
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("VARIABLE_USE", position, l_test, source);

        Utils::replaceAllTokens("TYPE", type_name, source);
        Utils::replaceAllTokens("INDEX", index, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string OutputComponentAliasingTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage)
           << " type: " << test_case.m_type.GetGLSLTypeName()
           << ", components: " << test_case.m_component_gohan << " & "
           << test_case.m_component_goten;

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint OutputComponentAliasingTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool OutputComponentAliasingTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void OutputComponentAliasingTest::testInit()
{
    const Utils::Type &type                     = getType(m_type);
    const std::vector<GLuint> &valid_components = type.GetValidComponents();

    for (std::vector<GLuint>::const_iterator it_gohan = valid_components.begin();
         it_gohan != valid_components.end(); ++it_gohan)
    {
        const GLuint max_component = *it_gohan + type.GetNumComponents();
        for (std::vector<GLuint>::const_iterator it_goten = it_gohan;
             it_goten != valid_components.end() && max_component > *it_goten; ++it_goten)
        {
            for (GLuint stage = 0; stage < Utils::Shader::STAGE_MAX; ++stage)
            {
                /* Skip compute shader */
                if (Utils::Shader::COMPUTE == stage)
                {
                    continue;
                }

                if ((Utils::Shader::FRAGMENT == stage) &&
                    (Utils::Type::Double == type.m_basic_type))
                {
                    continue;
                }

                testCase test_case = {*it_gohan, *it_goten, (Utils::Shader::STAGES)stage, type};

                m_test_cases.push_back(test_case);
            }
        }
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingLocationAliasingWithMixedTypesTest::VaryingLocationAliasingWithMixedTypesTest(
    deqp::Context &context,
    GLuint type_gohan,
    GLuint type_goten)
    : NegativeTestBase(context,
                       "varying_location_aliasing_with_mixed_types",
                       "Test verifies that compiler reports error when float/int types are mixed "
                       "at one location"),
      m_type_gohan(type_gohan),
      m_type_goten(type_goten)
{
    std::string name = ("varying_location_aliasing_with_mixed_types_");
    name.append(getTypeName(type_gohan));
    name.append("_");
    name.append(getTypeName(type_goten));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string VaryingLocationAliasingWithMixedTypesTest::getShaderSource(GLuint test_case_index,
                                                                       Utils::Shader::STAGES stage)
{
    static const GLchar *var_definition =
        "layout (location = 1, component = COMPONENT) FLAT DIRECTION TYPE gohanARRAY;\n"
        "layout (location = 1, component = COMPONENT) FLAT DIRECTION TYPE gotenARRAY;\n";
    static const GLchar *input_use =
        "    if ((TYPE(0) == gohanINDEX) &&\n"
        "        (TYPE(1) == gotenINDEX) )\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *output_use =
        "    gohanINDEX = TYPE(0);\n"
        "    gotenINDEX = TYPE(1);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gohanINDEX = TYPE(1);\n"
        "        gotenINDEX = TYPE(0);\n"
        "    }\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    fs_out += result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs += result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        const GLchar *array = "";
        GLchar buffer_gohan[16];
        GLchar buffer_goten[16];
        const GLchar *direction  = "in ";
        const GLchar *flat_gohan = "";
        const GLchar *flat_goten = "";
        const GLchar *index      = "";
        size_t position          = 0;
        size_t temp;
        const GLchar *type_gohan_name    = test_case.m_type_gohan.GetGLSLTypeName();
        const GLchar *type_goten_name    = test_case.m_type_goten.GetGLSLTypeName();
        Utils::Variable::STORAGE storage = Utils::Variable::VARYING_INPUT;
        const GLchar *var_use            = Utils::Shader::VERTEX == stage ? input_use : "\n";

        if (false == test_case.m_is_input)
        {
            direction = "out";
            storage   = Utils::Variable::VARYING_OUTPUT;
            var_use   = output_use;
        }

        /* If the interpolation qualifier would be different, the test
         * would fail and we are testing here mixed types, not mixed
         * interpolation qualifiers.
         */
        if (isFlatRequired(stage, test_case.m_type_gohan, storage) ||
            isFlatRequired(stage, test_case.m_type_goten, storage))
        {
            flat_gohan = "flat";
            flat_goten = "flat";
        }

        sprintf(buffer_gohan, "%d", test_case.m_component_gohan);
        sprintf(buffer_goten, "%d", test_case.m_component_goten);

#if DEBUG_NEG_REMOVE_ERROR
        type_goten_name =
            Utils::Type::GetType(test_case.m_type_gohan.m_basic_type, 1, 1).GetGLSLTypeName();
        if (Utils::Type::Double == test_case.m_type_gohan.m_basic_type)
        {
            sprintf(buffer_goten, "%d", 0 == test_case.m_component_gohan ? 2 : 0);
        }
#endif /* DEBUG_NEG_REMOVE_ERROR */

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                array  = "[]";
                index  = "[gl_InvocationID]";
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("COMPONENT", position, buffer_gohan, source);
        Utils::replaceToken("FLAT", position, flat_gohan, source);
        Utils::replaceToken("DIRECTION", position, direction, source);
        Utils::replaceToken("TYPE", position, type_gohan_name, source);
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("COMPONENT", position, buffer_goten, source);
        Utils::replaceToken("FLAT", position, flat_goten, source);
        Utils::replaceToken("DIRECTION", position, direction, source);
        Utils::replaceToken("TYPE", position, type_goten_name, source);
        Utils::replaceToken("ARRAY", position, array, source);

        temp = position;
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);
        position = temp;
        if (!test_case.m_is_input)
        {
            Utils::replaceToken("TYPE", position, type_gohan_name, source);
            Utils::replaceToken("TYPE", position, type_goten_name, source);
            Utils::replaceToken("TYPE", position, type_gohan_name, source);
            Utils::replaceToken("TYPE", position, type_goten_name, source);
        }
        else if (Utils::Shader::VERTEX == stage)
        {
            Utils::replaceToken("TYPE", position, type_gohan_name, source);
            Utils::replaceToken("TYPE", position, type_goten_name, source);
        }

        Utils::replaceAllTokens("INDEX", index, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string VaryingLocationAliasingWithMixedTypesTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage) << ", "
           << test_case.m_type_gohan.GetGLSLTypeName() << " at " << test_case.m_component_gohan
           << ", " << test_case.m_type_goten.GetGLSLTypeName() << " at "
           << test_case.m_component_goten << ". Direction: ";

    if (true == test_case.m_is_input)
    {
        stream << "input";
    }
    else
    {
        stream << "output";
    }

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint VaryingLocationAliasingWithMixedTypesTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingLocationAliasingWithMixedTypesTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void VaryingLocationAliasingWithMixedTypesTest::testInit()
{
    const Utils::Type &type_gohan                     = getType(m_type_gohan);
    const std::vector<GLuint> &valid_components_gohan = type_gohan.GetValidComponents();

    const Utils::Type &type_goten                     = getType(m_type_goten);
    const std::vector<GLuint> &valid_components_goten = type_goten.GetValidComponents();

    for (std::vector<GLuint>::const_iterator it_gohan = valid_components_gohan.begin();
         it_gohan != valid_components_gohan.end(); ++it_gohan)
    {
        const GLuint min_component = *it_gohan + type_gohan.GetNumComponents();
        for (std::vector<GLuint>::const_iterator it_goten = valid_components_goten.begin();
             it_goten != valid_components_goten.end(); ++it_goten)
        {

            if (min_component > *it_goten)
            {
                continue;
            }

            for (GLuint stage = 0; stage < Utils::Shader::STAGE_MAX; ++stage)
            {
                /* Skip compute shader */
                if (Utils::Shader::COMPUTE == stage)
                {
                    continue;
                }

                if (Utils::Shader::VERTEX != stage)
                {
                    testCase test_case_in = {*it_gohan,  *it_goten,
                                             true,       (Utils::Shader::STAGES)stage,
                                             type_gohan, type_goten};

                    m_test_cases.push_back(test_case_in);
                }

                /* Skip double outputs in fragment shader */
                if ((Utils::Shader::FRAGMENT != stage) ||
                    ((Utils::Type::Double != type_gohan.m_basic_type) &&
                     (Utils::Type::Double != type_goten.m_basic_type)))
                {
                    testCase test_case_out = {*it_gohan,  *it_goten,
                                              false,      (Utils::Shader::STAGES)stage,
                                              type_gohan, type_goten};

                    m_test_cases.push_back(test_case_out);
                }
            }
        }
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingLocationAliasingWithMixedInterpolationTest::
    VaryingLocationAliasingWithMixedInterpolationTest(deqp::Context &context,
                                                      GLuint type_gohan,
                                                      GLuint type_goten)
    : NegativeTestBase(context,
                       "varying_location_aliasing_with_mixed_interpolation",
                       "Test verifies that compiler reports error when interpolation qualifiers "
                       "are mixed at one location"),
      m_type_gohan(type_gohan),
      m_type_goten(type_goten)
{
    std::string name = ("varying_location_aliasing_with_mixed_interpolation_");
    name.append(getTypeName(m_type_gohan));
    name.append("_");
    name.append(getTypeName(m_type_goten));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string VaryingLocationAliasingWithMixedInterpolationTest::getShaderSource(
    GLuint test_case_index,
    Utils::Shader::STAGES stage)
{
    static const GLchar *var_definition =
        "layout (location = 1, component = COMPONENT) INTERPOLATION DIRECTION TYPE gohanARRAY;\n"
        "layout (location = 1, component = COMPONENT) INTERPOLATION DIRECTION TYPE gotenARRAY;\n";
    static const GLchar *input_use =
        "    if ((TYPE(0) == gohanINDEX) &&\n"
        "        (TYPE(1) == gotenINDEX) )\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *output_use =
        "    gohanINDEX = TYPE(0);\n"
        "    gotenINDEX = TYPE(1);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gohanINDEX = TYPE(1);\n"
        "        gotenINDEX = TYPE(0);\n"
        "    }\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    fs_out = result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs += result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        const GLchar *array = "";
        GLchar buffer_gohan[16];
        GLchar buffer_goten[16];
        const GLchar *direction = "in ";
        const GLchar *index     = "";
        const GLchar *int_gohan = getInterpolationQualifier(test_case.m_interpolation_gohan);
        const GLchar *int_goten = getInterpolationQualifier(test_case.m_interpolation_goten);
#if DEBUG_NEG_REMOVE_ERROR
        if (FLAT == test_case.m_interpolation_goten)
        {
            int_gohan = int_goten;
        }
        else
        {
            int_goten = int_gohan;
        }
#endif /* DEBUG_NEG_REMOVE_ERROR */
        size_t position = 0;
        size_t temp;
        const GLchar *type_gohan_name = test_case.m_type_gohan.GetGLSLTypeName();
        const GLchar *type_goten_name = test_case.m_type_goten.GetGLSLTypeName();
        const GLchar *var_use         = Utils::Shader::VERTEX == stage ? input_use : "\n";

        if (false == test_case.m_is_input)
        {
            direction = "out";

            var_use = output_use;
        }

        sprintf(buffer_gohan, "%d", test_case.m_component_gohan);
        sprintf(buffer_goten, "%d", test_case.m_component_goten);

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                array  = "[]";
                index  = "[gl_InvocationID]";
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = test_case.m_is_input ? "[]" : "";
                index  = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("COMPONENT", position, buffer_gohan, source);
        Utils::replaceToken("INTERPOLATION", position, int_gohan, source);
        Utils::replaceToken("DIRECTION", position, direction, source);
        Utils::replaceToken("TYPE", position, type_gohan_name, source);
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("COMPONENT", position, buffer_goten, source);
        Utils::replaceToken("INTERPOLATION", position, int_goten, source);
        Utils::replaceToken("DIRECTION", position, direction, source);
        Utils::replaceToken("TYPE", position, type_goten_name, source);
        Utils::replaceToken("ARRAY", position, array, source);

        temp = position;
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);
        position = temp;
        if (!test_case.m_is_input)
        {
            Utils::replaceToken("TYPE", position, type_gohan_name, source);
            Utils::replaceToken("TYPE", position, type_goten_name, source);
            Utils::replaceToken("TYPE", position, type_gohan_name, source);
            Utils::replaceToken("TYPE", position, type_goten_name, source);
        }
        else if (Utils::Shader::VERTEX == stage)
        {
            Utils::replaceToken("TYPE", position, type_gohan_name, source);
            Utils::replaceToken("TYPE", position, type_goten_name, source);
        }

        Utils::replaceAllTokens("INDEX", index, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string VaryingLocationAliasingWithMixedInterpolationTest::getTestCaseName(
    GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage) << ", "
           << getInterpolationQualifier(test_case.m_interpolation_gohan) << " "
           << test_case.m_type_gohan.GetGLSLTypeName() << " at " << test_case.m_component_gohan
           << ", " << getInterpolationQualifier(test_case.m_interpolation_goten) << " "
           << test_case.m_type_goten.GetGLSLTypeName() << " at " << test_case.m_component_goten
           << ". Direction: ";

    if (true == test_case.m_is_input)
    {
        stream << "input";
    }
    else
    {
        stream << "output";
    }

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint VaryingLocationAliasingWithMixedInterpolationTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingLocationAliasingWithMixedInterpolationTest::isComputeRelevant(
    GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void VaryingLocationAliasingWithMixedInterpolationTest::testInit()
{
    const Utils::Type &type_gohan                     = getType(m_type_gohan);
    const std::vector<GLuint> &valid_components_gohan = type_gohan.GetValidComponents();

    const GLuint gohan = valid_components_gohan.front();

    const Utils::Type &type_goten                     = getType(m_type_goten);
    const std::vector<GLuint> &valid_components_goten = type_goten.GetValidComponents();

    const GLuint goten = valid_components_goten.back();

    for (GLuint stage = 0; stage < Utils::Shader::STAGE_MAX; ++stage)
    {
        /* Skip compute shader */
        if (Utils::Shader::COMPUTE == stage)
        {
            continue;
        }

        for (GLuint int_gohan = 0; int_gohan < INTERPOLATION_MAX; ++int_gohan)
        {
            for (GLuint int_goten = 0; int_goten < INTERPOLATION_MAX; ++int_goten)
            {
                /* Skip when both are the same */
                if (int_gohan == int_goten)
                {
                    continue;
                }

                /* Skip inputs in: vertex shader and whenever
                 * flat is mandatory and is not the chosen
                 * one.
                 */
                bool skip_inputs = Utils::Shader::VERTEX == stage;
                skip_inputs |= (FLAT != int_gohan &&
                                isFlatRequired(static_cast<Utils::Shader::STAGES>(stage),
                                               type_gohan, Utils::Variable::VARYING_INPUT));
                skip_inputs |= (FLAT != int_goten &&
                                isFlatRequired(static_cast<Utils::Shader::STAGES>(stage),
                                               type_goten, Utils::Variable::VARYING_INPUT));

                if (!skip_inputs)
                {
                    testCase test_case_in = {gohan,
                                             goten,
                                             static_cast<INTERPOLATIONS>(int_gohan),
                                             static_cast<INTERPOLATIONS>(int_goten),
                                             true,
                                             static_cast<Utils::Shader::STAGES>(stage),
                                             type_gohan,
                                             type_goten};
                    m_test_cases.push_back(test_case_in);
                }

                /* Skip outputs in fragment shader and
                 * whenever flat is mandatory and is not the
                 * chosen one.
                 */
                bool skip_outputs = Utils::Shader::FRAGMENT == stage;
                skip_outputs |= (FLAT != int_gohan &&
                                 isFlatRequired(static_cast<Utils::Shader::STAGES>(stage),
                                                type_gohan, Utils::Variable::VARYING_OUTPUT));
                skip_outputs |= (FLAT != int_goten &&
                                 isFlatRequired(static_cast<Utils::Shader::STAGES>(stage),
                                                type_goten, Utils::Variable::VARYING_OUTPUT));

                if (!skip_outputs)
                {
                    testCase test_case_out = {gohan,
                                              goten,
                                              static_cast<INTERPOLATIONS>(int_gohan),
                                              static_cast<INTERPOLATIONS>(int_goten),
                                              false,
                                              static_cast<Utils::Shader::STAGES>(stage),
                                              type_gohan,
                                              type_goten};
                    m_test_cases.push_back(test_case_out);
                }
            }
        }
    }
}

/** Get interpolation qualifier
 *
 * @param interpolation Enumeration
 *
 * @return GLSL qualifier
 **/
const GLchar *VaryingLocationAliasingWithMixedInterpolationTest::getInterpolationQualifier(
    INTERPOLATIONS interpolation)
{
    const GLchar *result = 0;

    switch (interpolation)
    {
        case SMOOTH:
            result = "smooth";
            break;
        case FLAT:
            result = "flat";
            break;
        case NO_PERSPECTIVE:
            result = "noperspective";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Constructor
 *
 * @param context Test framework context
 **/
VaryingLocationAliasingWithMixedAuxiliaryStorageTest::
    VaryingLocationAliasingWithMixedAuxiliaryStorageTest(deqp::Context &context,
                                                         GLuint type_gohan,
                                                         GLuint type_goten)
    : NegativeTestBase(context,
                       "varying_location_aliasing_with_mixed_auxiliary_storage",
                       "Test verifies that compiler reports error when auxiliary storage "
                       "qualifiers are mixed at one location"),
      m_type_gohan(type_gohan),
      m_type_goten(type_goten)
{
    std::string name = ("varying_location_aliasing_with_mixed_auxiliary_storage_");
    name.append(getTypeName(m_type_gohan));
    name.append("_");
    name.append(getTypeName(m_type_goten));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string VaryingLocationAliasingWithMixedAuxiliaryStorageTest::getShaderSource(
    GLuint test_case_index,
    Utils::Shader::STAGES stage)
{
    static const GLchar *var_definition =
        "layout (location = 1, component = COMPONENT) AUX INTERPOLATION DIRECTION TYPE "
        "gohanARRAY;\n"
        "layout (location = 1, component = COMPONENT) AUX INTERPOLATION DIRECTION TYPE "
        "gotenARRAY;\n";
    static const GLchar *input_use =
        "    if ((TYPE(0) == gohanINDEX_GOHAN) &&\n"
        "        (TYPE(1) == gotenINDEX_GOTEN) )\n"
        "    {\n"
        "        result += vec4(1, 0.5, 0.25, 0.125);\n"
        "    }\n";
    static const GLchar *output_use =
        "    gohanINDEX_GOHAN = TYPE(0);\n"
        "    gotenINDEX_GOTEN = TYPE(1);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gohanINDEX_GOHAN = TYPE(1);\n"
        "        gotenINDEX_GOTEN = TYPE(0);\n"
        "    }\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    fs_out = result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs += result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        const GLchar *array_gohan = "";
        const GLchar *array_goten = "";
        const GLchar *aux_gohan   = getAuxiliaryQualifier(test_case.m_aux_gohan);
#if DEBUG_NEG_REMOVE_ERROR
        const GLchar *aux_goten = aux_gohan;
#else
        const GLchar *aux_goten = getAuxiliaryQualifier(test_case.m_aux_goten);
#endif /* DEBUG_NEG_REMOVE_ERROR */
        GLchar buffer_gohan[16];
        GLchar buffer_goten[16];
        const GLchar *direction          = "in";
        const GLchar *index_gohan        = "";
        const GLchar *index_goten        = "";
        Utils::Variable::STORAGE storage = Utils::Variable::VARYING_INPUT;
        const GLchar *interpolation      = "";
        size_t position                  = 0;
        size_t temp;
        const GLchar *type_gohan_name = test_case.m_type_gohan.GetGLSLTypeName();
        const GLchar *type_goten_name = test_case.m_type_goten.GetGLSLTypeName();
        const GLchar *var_use         = Utils::Shader::VERTEX == stage ? input_use : "\n";

        if (false == test_case.m_is_input)
        {
            direction = "out";
            storage   = Utils::Variable::VARYING_OUTPUT;
            var_use   = output_use;
        }

        if (isFlatRequired(stage, test_case.m_type_gohan, storage) ||
            isFlatRequired(stage, test_case.m_type_goten, storage))
        {
            interpolation = "flat";
        }

        sprintf(buffer_gohan, "%d", test_case.m_component_gohan);
        sprintf(buffer_goten, "%d", test_case.m_component_goten);

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source      = gs_tested;
                array_gohan = test_case.m_is_input ? "[]" : "";
                index_gohan = test_case.m_is_input ? "[0]" : "";
                array_goten = test_case.m_is_input ? "[]" : "";
                index_goten = test_case.m_is_input ? "[0]" : "";
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                if (PATCH != test_case.m_aux_gohan)
                {
                    array_gohan = "[]";
                    index_gohan = "[gl_InvocationID]";
                }
#if DEBUG_NEG_REMOVE_ERROR
                array_goten = array_gohan;
                index_goten = index_gohan;
#else
                if (PATCH != test_case.m_aux_goten)
                {
                    array_goten = "[]";
                    index_goten = "[gl_InvocationID]";
                }
#endif /* DEBUG_NEG_REMOVE_ERROR */
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                if (PATCH != test_case.m_aux_gohan)
                {
                    array_gohan = test_case.m_is_input ? "[]" : "";
                    index_gohan = test_case.m_is_input ? "[0]" : "";
                }
#if DEBUG_NEG_REMOVE_ERROR
                array_goten = array_gohan;
                index_goten = index_gohan;
#else
                if (PATCH != test_case.m_aux_goten)
                {
                    array_goten = test_case.m_is_input ? "[]" : "";
                    index_goten = test_case.m_is_input ? "[0]" : "";
                }
#endif /* DEBUG_NEG_REMOVE_ERROR */
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("COMPONENT", position, buffer_gohan, source);
        Utils::replaceToken("AUX", position, aux_gohan, source);
        Utils::replaceToken("INTERPOLATION", position, interpolation, source);
        Utils::replaceToken("DIRECTION", position, direction, source);
        Utils::replaceToken("TYPE", position, type_gohan_name, source);
        Utils::replaceToken("ARRAY", position, array_gohan, source);
        Utils::replaceToken("COMPONENT", position, buffer_goten, source);
        Utils::replaceToken("AUX", position, aux_goten, source);
        Utils::replaceToken("INTERPOLATION", position, interpolation, source);
        Utils::replaceToken("DIRECTION", position, direction, source);
        Utils::replaceToken("TYPE", position, type_goten_name, source);
        Utils::replaceToken("ARRAY", position, array_goten, source);

        temp = position;
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);
        position = temp;
        if (!test_case.m_is_input)
        {
            Utils::replaceToken("TYPE", position, type_gohan_name, source);
            Utils::replaceToken("TYPE", position, type_goten_name, source);
            Utils::replaceToken("TYPE", position, type_gohan_name, source);
            Utils::replaceToken("TYPE", position, type_goten_name, source);
        }
        else if (Utils::Shader::VERTEX == stage)
        {
            Utils::replaceToken("TYPE", position, type_gohan_name, source);
            Utils::replaceToken("TYPE", position, type_goten_name, source);
        }

        Utils::replaceAllTokens("INDEX_GOHAN", index_gohan, source);
        Utils::replaceAllTokens("INDEX_GOTEN", index_goten, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string VaryingLocationAliasingWithMixedAuxiliaryStorageTest::getTestCaseName(
    GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage) << ", "
           << getAuxiliaryQualifier(test_case.m_aux_gohan) << " "
           << test_case.m_type_gohan.GetGLSLTypeName() << " at " << test_case.m_component_gohan
           << ", " << getAuxiliaryQualifier(test_case.m_aux_goten) << " "
           << test_case.m_type_goten.GetGLSLTypeName() << " at " << test_case.m_component_goten
           << ". Direction: ";

    if (true == test_case.m_is_input)
    {
        stream << "input";
    }
    else
    {
        stream << "output";
    }

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint VaryingLocationAliasingWithMixedAuxiliaryStorageTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VaryingLocationAliasingWithMixedAuxiliaryStorageTest::isComputeRelevant(
    GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void VaryingLocationAliasingWithMixedAuxiliaryStorageTest::testInit()
{
    const Utils::Type &type_gohan                     = getType(m_type_gohan);
    const std::vector<GLuint> &valid_components_gohan = type_gohan.GetValidComponents();

    const GLuint gohan = valid_components_gohan.front();

    const Utils::Type &type_goten                     = getType(m_type_goten);
    const std::vector<GLuint> &valid_components_goten = type_goten.GetValidComponents();

    const GLuint goten = valid_components_goten.back();

    for (GLuint stage = 0; stage < Utils::Shader::STAGE_MAX; ++stage)
    {
        /* Skip compute shader */
        if (Utils::Shader::COMPUTE == stage)
        {
            continue;
        }

        for (GLuint aux = 0; aux < AUXILIARY_MAX; ++aux)
        {
            Utils::Shader::STAGES const shader_stage = static_cast<Utils::Shader::STAGES>(stage);
            AUXILIARIES const auxiliary              = static_cast<AUXILIARIES>(aux);

            if (PATCH == auxiliary)
            {
                if (Utils::Shader::TESS_CTRL == shader_stage ||
                    Utils::Shader::TESS_EVAL == shader_stage)
                {
                    bool direction                 = Utils::Shader::TESS_EVAL == shader_stage;
                    testCase test_case_patch_gohan = {gohan,      goten,     auxiliary,
                                                      NONE,       direction, shader_stage,
                                                      type_gohan, type_goten};
                    testCase test_case_patch_goten = {gohan,      goten,     NONE,
                                                      auxiliary,  direction, shader_stage,
                                                      type_gohan, type_goten};

                    m_test_cases.push_back(test_case_patch_gohan);
                    m_test_cases.push_back(test_case_patch_goten);
                }
                continue;
            }

            for (GLuint second_aux = 0; second_aux < AUXILIARY_MAX; ++second_aux)
            {
                AUXILIARIES const second_auxiliary = static_cast<AUXILIARIES>(second_aux);

                if (PATCH == second_auxiliary || auxiliary == second_auxiliary)
                {
                    continue;
                }

                if (Utils::Shader::FRAGMENT != shader_stage)
                {
                    testCase test_case_out = {gohan, goten,        auxiliary,  second_auxiliary,
                                              false, shader_stage, type_gohan, type_goten};

                    m_test_cases.push_back(test_case_out);
                }

                if (Utils::Shader::VERTEX != shader_stage)
                {
                    testCase test_case_in = {gohan, goten,        auxiliary,  second_auxiliary,
                                             true,  shader_stage, type_gohan, type_goten};

                    m_test_cases.push_back(test_case_in);
                }
            }
        }
    }
}

/** Get auxiliary storage qualifier
 *
 * @param aux Enumeration
 *
 * @return GLSL qualifier
 **/
const GLchar *VaryingLocationAliasingWithMixedAuxiliaryStorageTest::getAuxiliaryQualifier(
    AUXILIARIES aux)
{
    const GLchar *result = 0;

    switch (aux)
    {
        case NONE:
            result = "";
            break;
        case PATCH:
            result = "patch";
            break;
        case CENTROID:
            result = "centroid";
            break;
        case SAMPLE:
            result = "sample";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/* Constants used by VertexAttribLocationAPITest */
const GLuint VertexAttribLocationAPITest::m_goten_location = 6;

/** Constructor
 *
 * @param context Test framework context
 **/
VertexAttribLocationAPITest::VertexAttribLocationAPITest(deqp::Context &context)
    : TextureTestBase(context,
                      "vertex_attrib_location_api",
                      "Test verifies that attribute locations API works as expected")
{}

/** Does BindAttribLocation for "goten" and relink program
 *
 * @param program           Program object
 * @param program_interface Interface of program
 **/
void VertexAttribLocationAPITest::prepareAttribLocation(Utils::Program &program,
                                                        Utils::ProgramInterface &program_interface)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();

    gl.bindAttribLocation(program.m_id, m_goten_location, "goten");
    GLU_EXPECT_NO_ERROR(gl.getError(), "BindAttribLocation");

    program.Link(gl, program.m_id);

    /* We still need to get locations for gohan and chichi */
    TextureTestBase::prepareAttribLocation(program, program_interface);
}

/** Get interface of program
 *
 * @param ignored
 * @param program_interface   Interface of program
 * @param ignored
 **/
void VertexAttribLocationAPITest::getProgramInterface(
    GLuint /* test_case_index */,
    Utils::ProgramInterface &program_interface,
    Utils::VaryingPassthrough & /* varying_passthrough */)
{
    Utils::ShaderInterface &si = program_interface.GetShaderInterface(Utils::Shader::VERTEX);
    const Utils::Type &type    = Utils::Type::vec4;
    const GLuint type_size     = type.GetSize();

    /* Offsets */
    const GLuint chichi_offset = 0;
    const GLuint goten_offset  = chichi_offset + type_size;
    const GLuint gohan_offset  = goten_offset + type_size;
    const GLuint goku_offset   = gohan_offset + type_size;

    /* Locations */
    const GLuint goku_location  = 2;
    const GLuint goten_location = m_goten_location;

    /* Generate data */
    m_goku_data   = type.GenerateDataPacked();
    m_gohan_data  = type.GenerateDataPacked();
    m_goten_data  = type.GenerateDataPacked();
    m_chichi_data = type.GenerateDataPacked();

    /* Globals */
    si.m_globals = "const uint GOKU_LOCATION = 2;\n";

    /* Attributes */
    si.Input("goku" /* name */, "layout (location = GOKU_LOCATION)" /* qualifiers */,
             0 /* expected_componenet */, goku_location /* expected_location */, type /* type */,
             GL_FALSE /* normalized */, 0u /* n_array_elements */, 0u /* stride */,
             goku_offset /* offset */, (GLvoid *)&m_goku_data[0] /* data */,
             m_goku_data.size() /* data_size */);

    si.Input("gohan" /* name */, "" /* qualifiers */, 0 /* expected_componenet */,
             Utils::Variable::m_automatic_location /* expected_location */, type /* type */,
             GL_FALSE /* normalized */, 0u /* n_array_elements */, 0u /* stride */,
             gohan_offset /* offset */, (GLvoid *)&m_gohan_data[0] /* data */,
             m_gohan_data.size() /* data_size */);

    si.Input("goten" /* name */, "" /* qualifiers */, 0 /* expected_componenet */,
             goten_location /* expected_location */, type /* type */, GL_FALSE /* normalized */,
             0u /* n_array_elements */, 0u /* stride */, goten_offset /* offset */,
             (GLvoid *)&m_goten_data[0] /* data */, m_goten_data.size() /* data_size */);

    si.Input("chichi" /* name */, "" /* qualifiers */, 0 /* expected_componenet */,
             Utils::Variable::m_automatic_location /* expected_location */, type /* type */,
             GL_FALSE /* normalized */, 0u /* n_array_elements */, 0u /* stride */,
             chichi_offset /* offset */, (GLvoid *)&m_chichi_data[0] /* data */,
             m_chichi_data.size() /* data_size */);
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool VertexAttribLocationAPITest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/* Constants used by FragmentDataLocationAPITest */
const GLuint FragmentDataLocationAPITest::m_goten_location = 6;

/** Constructor
 *
 * @param context Test framework context
 **/
FragmentDataLocationAPITest::FragmentDataLocationAPITest(deqp::Context &context)
    : TextureTestBase(context,
                      "fragment_data_location_api",
                      "Test verifies that fragment data locations API works as expected"),
      m_goku(context),
      m_gohan(context),
      m_goten(context),
      m_chichi(context),
      m_goku_location(0),
      m_gohan_location(0),
      m_chichi_location(0)
{}

/** Verifies contents of drawn images
 *
 * @param ignored
 * @param ignored
 *
 * @return true if images are filled with expected values, false otherwise
 **/
bool FragmentDataLocationAPITest::checkResults(glw::GLuint /* test_case_index */,
                                               Utils::Texture & /* color_0 */)
{
    static const GLuint size            = m_width * m_height;
    static const GLuint expected_goku   = 0xff000000;
    static const GLuint expected_gohan  = 0xff0000ff;
    static const GLuint expected_goten  = 0xff00ff00;
    static const GLuint expected_chichi = 0xffff0000;

    std::vector<GLuint> data;
    data.resize(size);

    m_goku.Get(GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

    for (GLuint i = 0; i < size; ++i)
    {
        const GLuint color = data[i];

        if (expected_goku != color)
        {
            m_context.getTestContext().getLog() << tcu::TestLog::Message << "RGBA8[" << i
                                                << "]:" << color << tcu::TestLog::EndMessage;
            return false;
        }
    }

    m_gohan.Get(GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

    for (GLuint i = 0; i < size; ++i)
    {
        const GLuint color = data[i];

        if (expected_gohan != color)
        {
            m_context.getTestContext().getLog() << tcu::TestLog::Message << "RGBA8[" << i
                                                << "]:" << color << tcu::TestLog::EndMessage;
            return false;
        }
    }

    m_goten.Get(GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

    for (GLuint i = 0; i < size; ++i)
    {
        const GLuint color = data[i];

        if (expected_goten != color)
        {
            m_context.getTestContext().getLog() << tcu::TestLog::Message << "RGBA8[" << i
                                                << "]:" << color << tcu::TestLog::EndMessage;
            return false;
        }
    }

    m_chichi.Get(GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

    for (GLuint i = 0; i < size; ++i)
    {
        const GLuint color = data[i];

        if (expected_chichi != color)
        {
            m_context.getTestContext().getLog() << tcu::TestLog::Message << "RGBA8[" << i
                                                << "]:" << color << tcu::TestLog::EndMessage;
            return false;
        }
    }

    return true;
}

/** Prepare code snippet that will set out variables
 *
 * @param ignored
 * @param ignored
 * @param stage               Shader stage
 *
 * @return Code that pass in variables to next stage
 **/
std::string FragmentDataLocationAPITest::getPassSnippet(
    GLuint /* test_case_index */,
    Utils::VaryingPassthrough & /* varying_passthrough */,
    Utils::Shader::STAGES stage)
{
    std::string result;

    /* Skip for compute shader */
    if (Utils::Shader::FRAGMENT != stage)
    {
        result = "";
    }
    else
    {
        result =
            "chichi = vec4(0, 0, 1, 1);\n"
            "    goku   = vec4(0, 0, 0, 1);\n"
            "    goten  = vec4(0, 1, 0, 1);\n"
            "    gohan  = vec4(1, 0, 0, 1);\n";
    }

    return result;
}

/** Get interface of program
 *
 * @param ignored
 * @param program_interface Interface of program
 * @param ignored
 **/
void FragmentDataLocationAPITest::getProgramInterface(
    GLuint /* test_case_index */,
    Utils::ProgramInterface &program_interface,
    Utils::VaryingPassthrough & /* varying_passthrough */)
{
    Utils::ShaderInterface &si = program_interface.GetShaderInterface(Utils::Shader::FRAGMENT);
    const Utils::Type &type    = Utils::Type::vec4;

    /* Locations */
    m_goku_location = 2;

    /* Globals */
    si.m_globals = "const uint GOKU_LOCATION = 2;\n";

    /* Attributes */
    si.Output("goku" /* name */, "layout (location = GOKU_LOCATION)" /* qualifiers */,
              0 /* expected_componenet */, m_goku_location /* expected_location */, type /* type */,
              GL_FALSE /* normalized */, 0u /* n_array_elements */, 0u /* stride */,
              0u /* offset */, (GLvoid *)0 /* data */, 0u /* data_size */);

    si.Output("gohan" /* name */, "" /* qualifiers */, 0 /* expected_componenet */,
              Utils::Variable::m_automatic_location /* expected_location */, type /* type */,
              GL_FALSE /* normalized */, 0u /* n_array_elements */, 0u /* stride */,
              0u /* offset */, (GLvoid *)0 /* data */, 0u /* data_size */);

    si.Output("goten" /* name */, "" /* qualifiers */, 0 /* expected_componenet */,
              m_goten_location /* expected_location */, type /* type */, GL_FALSE /* normalized */,
              0u /* n_array_elements */, 0u /* stride */, 0u /* offset */, (GLvoid *)0 /* data */,
              0u /* data_size */);

    si.Output("chichi" /* name */, "" /* qualifiers */, 0 /* expected_componenet */,
              Utils::Variable::m_automatic_location /* expected_location */, type /* type */,
              GL_FALSE /* normalized */, 0u /* n_array_elements */, 0u /* stride */,
              0u /* offset */, (GLvoid *)0 /* data */, 0u /* data_size */);
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool FragmentDataLocationAPITest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Get locations for all outputs with automatic_location
 *
 * @param program           Program object
 * @param program_interface Interface of program
 **/
void FragmentDataLocationAPITest::prepareFragmentDataLoc(Utils::Program &program,
                                                         Utils::ProgramInterface &program_interface)
{
    /* Bind location of goten */
    const Functions &gl = m_context.getRenderContext().getFunctions();

    gl.bindFragDataLocation(program.m_id, m_goten_location, "goten");
    GLU_EXPECT_NO_ERROR(gl.getError(), "BindFragDataLocation");

    program.Link(gl, program.m_id);

    /* Prepare locations for gohan and chichi */
    TextureTestBase::prepareFragmentDataLoc(program, program_interface);

    /* Get all locations */
    Utils::ShaderInterface &si = program_interface.GetShaderInterface(Utils::Shader::FRAGMENT);

    Utils::Variable::PtrVector &outputs = si.m_outputs;

    for (Utils::Variable::PtrVector::iterator it = outputs.begin(); outputs.end() != it; ++it)
    {
        const Utils::Variable::Descriptor &desc = (*it)->m_descriptor;

        if (0 == desc.m_name.compare("gohan"))
        {
            m_gohan_location = desc.m_expected_location;
        }
        else if (0 == desc.m_name.compare("chichi"))
        {
            m_chichi_location = desc.m_expected_location;
        }

        /* Locations of goku and goten are fixed */
    }
}

/** Prepare framebuffer with single texture as color attachment
 *
 * @param framebuffer     Framebuffer
 * @param color_0_texture Texture that will used as color attachment
 **/
void FragmentDataLocationAPITest::prepareFramebuffer(Utils::Framebuffer &framebuffer,
                                                     Utils::Texture &color_0_texture)
{
    /* Let parent prepare its stuff */
    TextureTestBase::prepareFramebuffer(framebuffer, color_0_texture);

    /* Prepare data */
    std::vector<GLuint> texture_data;
    texture_data.resize(m_width * m_height);

    for (GLuint i = 0; i < texture_data.size(); ++i)
    {
        texture_data[i] = 0x20406080;
    }

    /* Prepare textures */
    m_goku.Init(Utils::Texture::TEX_2D, m_width, m_height, 0, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
                &texture_data[0]);

    m_gohan.Init(Utils::Texture::TEX_2D, m_width, m_height, 0, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
                 &texture_data[0]);

    m_goten.Init(Utils::Texture::TEX_2D, m_width, m_height, 0, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
                 &texture_data[0]);

    m_chichi.Init(Utils::Texture::TEX_2D, m_width, m_height, 0, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
                  &texture_data[0]);

    /* Attach textures to framebuffer */
    framebuffer.Bind();
    framebuffer.AttachTexture(GL_COLOR_ATTACHMENT0 + m_goku_location, m_goku.m_id, m_width,
                              m_height);
    framebuffer.AttachTexture(GL_COLOR_ATTACHMENT0 + m_gohan_location, m_gohan.m_id, m_width,
                              m_height);
    framebuffer.AttachTexture(GL_COLOR_ATTACHMENT0 + m_goten_location, m_goten.m_id, m_width,
                              m_height);
    framebuffer.AttachTexture(GL_COLOR_ATTACHMENT0 + m_chichi_location, m_chichi.m_id, m_width,
                              m_height);

    /* Set up drawbuffers */
    const Functions &gl = m_context.getRenderContext().getFunctions();
    // The fragment shader can have more than 4 color outputs, but it only care about 4 (goku,
    // gohan, goten, chichi). We will first initialize all draw buffers to NONE and then set the
    // real value for the 4 outputs we care about
    GLint maxDrawBuffers = 0;
    gl.getIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

    std::vector<GLenum> buffers(maxDrawBuffers, GL_NONE);
    buffers[m_chichi_location] = GLenum(GL_COLOR_ATTACHMENT0 + m_chichi_location);
    buffers[m_goten_location]  = GLenum(GL_COLOR_ATTACHMENT0 + m_goten_location);
    buffers[m_goku_location]   = GLenum(GL_COLOR_ATTACHMENT0 + m_goku_location);
    buffers[m_gohan_location]  = GLenum(GL_COLOR_ATTACHMENT0 + m_gohan_location);

    gl.drawBuffers(maxDrawBuffers, buffers.data());
    GLU_EXPECT_NO_ERROR(gl.getError(), "DrawBuffers");
}

/** Constructor
 *
 * @param context Test framework context
 **/
XFBInputTest::XFBInputTest(deqp::Context &context, GLuint qualifier, GLuint stage)
    : NegativeTestBase(
          context,
          "xfb_input",
          "Test verifies that compiler reports error when xfb qualifiers are used with input"),
      m_qualifier(qualifier),
      m_stage(stage)
{
    std::string name = ("xfb_input_");
    name.append(EnhancedLayouts::XFBInputTest::getQualifierName(
        (EnhancedLayouts::XFBInputTest::QUALIFIERS)qualifier));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBInputTest::getShaderSource(GLuint test_case_index, Utils::Shader::STAGES stage)
{
#if DEBUG_NEG_REMOVE_ERROR
    static const GLchar *buffer_var_definition =
        "/* layout (xfb_buffer = 2) */ in vec4 gohanARRAY;\n";
    static const GLchar *offset_var_definition =
        "/* layout (xfb_offset = 16) */ in vec4 gohanARRAY;\n";
    static const GLchar *stride_var_definition =
        "/* layout (xfb_stride = 32) */ in vec4 gohanARRAY;\n";
#else
    static const GLchar *buffer_var_definition = "layout (xfb_buffer = 2) in vec4 gohanARRAY;\n";
    static const GLchar *offset_var_definition = "layout (xfb_offset = 16) in vec4 gohanARRAY;\n";
    static const GLchar *stride_var_definition = "layout (xfb_stride = 32) in vec4 gohanARRAY;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs;\n"
        "}\n"
        "\n";
    static const GLchar *fs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = gs_fs;\n"
        "\n"
        "    fs_out = result;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = tes_gs[0];\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    tes_gs = tcs_tes[0];\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "    tes_gs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "    vs_tcs += result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        const GLchar *array          = "";
        size_t position              = 0;
        const GLchar *var_definition = 0;

        switch (test_case.m_qualifier)
        {
            case BUFFER:
                var_definition = buffer_var_definition;
                break;
            case OFFSET:
                var_definition = offset_var_definition;
                break;
            case STRIDE:
                var_definition = stride_var_definition;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs_tested;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = "[]";
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs_tested;
                array  = "[]";
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = "[]";
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("ARRAY", position, array, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_CTRL:
                source = tcs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string XFBInputTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage) << ", qualifier: ";

    switch (test_case.m_qualifier)
    {
        case BUFFER:
            stream << "xfb_buffer";
            break;
        case OFFSET:
            stream << "xfb_offset";
            break;
        case STRIDE:
            stream << "xfb_stride";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint XFBInputTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool XFBInputTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void XFBInputTest::testInit()
{
    testCase test_case = {(QUALIFIERS)m_qualifier, (Utils::Shader::STAGES)m_stage};
    m_test_cases.push_back(test_case);
}

/** Get name of glsl constant
 *
 * @param Constant id
 *
 * @return Name of constant used in GLSL
 **/
const GLchar *XFBInputTest::getQualifierName(QUALIFIERS qualifier)
{
    const GLchar *name = "";

    switch (qualifier)
    {
        case BUFFER:
            name = "xtb_buffer";
            break;
        case STRIDE:
            name = "xtb_stride";
            break;
        case OFFSET:
            name = "xtb_offset";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return name;
}

/* Constants used by XFBAllStagesTest */
const GLuint XFBAllStagesTest::m_gs_index = 3;

/** Constructor
 *
 * @param context Test context
 **/
XFBAllStagesTest::XFBAllStagesTest(deqp::Context &context)
    : BufferTestBase(context,
                     "xfb_all_stages",
                     "Test verifies that only last stage in vertex processing can output to "
                     "transform feedback")
{
    /* Nothing to be done here */
}

/** Get descriptors of buffers necessary for test
 *
 * @param ignored
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBAllStagesTest::getBufferDescriptors(glw::GLuint /* test_case_index */,
                                            bufferDescriptor::Vector &out_descriptors)
{
    static const GLuint n_stages = 4;
    const Utils::Type &vec4      = Utils::Type::vec4;

    /* Data */
    tcu::Vec4 sum;

    /* Test uses single uniform and xfb per stage + uniform for fragment shader */
    out_descriptors.resize(n_stages * 2 + 1);

    /* */
    for (GLuint i = 0; i < n_stages; ++i)
    {
        /* Get references */
        bufferDescriptor &uniform = out_descriptors[i + 0];
        bufferDescriptor &xfb     = out_descriptors[i + n_stages];

        /* Index */
        uniform.m_index = i;
        xfb.m_index     = i;

        /* Target */
        uniform.m_target = Utils::Buffer::Uniform;
        xfb.m_target     = Utils::Buffer::Transform_feedback;

        /* Data */
        const tcu::Vec4 var(Utils::GetRandFloat(), Utils::GetRandFloat(), Utils::GetRandFloat(),
                            Utils::GetRandFloat());

        sum += var;

        uniform.m_initial_data.resize(vec4.GetSize());
        memcpy(&uniform.m_initial_data[0], var.getPtr(), vec4.GetSize());

        xfb.m_initial_data = vec4.GenerateDataPacked();

        if (m_gs_index != i)
        {
            xfb.m_expected_data = xfb.m_initial_data;
        }
        else
        {
            xfb.m_expected_data.resize(vec4.GetSize());
            memcpy(&xfb.m_expected_data[0], sum.getPtr(), vec4.GetSize());
        }
    }

    /* FS */
    {
        /* Get reference */
        bufferDescriptor &uniform = out_descriptors[n_stages * 2];

        /* Index */
        uniform.m_index = n_stages;

        /* Target */
        uniform.m_target = Utils::Buffer::Uniform;

        /* Data */
        const tcu::Vec4 var(Utils::GetRandFloat(), Utils::GetRandFloat(), Utils::GetRandFloat(),
                            Utils::GetRandFloat());

        uniform.m_initial_data.resize(vec4.GetSize());
        memcpy(&uniform.m_initial_data[0], var.getPtr(), vec4.GetSize());
    }
}

/** Get body of main function for given shader stage
 *
 * @param ignored
 * @param stage            Shader stage
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void XFBAllStagesTest::getShaderBody(glw::GLuint /* test_case_index */,
                                     Utils::Shader::STAGES stage,
                                     std::string &out_assignments,
                                     std::string &out_calculations)
{
    out_calculations = "";

    static const GLchar *vs = "    vs_tcs  = uni_vs;\n";
    static const GLchar *tcs =
        "    tcs_tes[gl_InvocationID] = uni_tcs + vs_tcs[gl_InvocationID];\n";
    static const GLchar *tes = "    tes_gs  = uni_tes + tcs_tes[0];\n";
    static const GLchar *gs  = "    gs_fs   = uni_gs  + tes_gs[0];\n";
    static const GLchar *fs  = "    fs_out  = uni_fs  + gs_fs;\n";

    const GLchar *assignments = 0;
    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            assignments = fs;
            break;
        case Utils::Shader::GEOMETRY:
            assignments = gs;
            break;
        case Utils::Shader::TESS_CTRL:
            assignments = tcs;
            break;
        case Utils::Shader::TESS_EVAL:
            assignments = tes;
            break;
        case Utils::Shader::VERTEX:
            assignments = vs;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    out_assignments = assignments;
}

/** Get interface of shader
 *
 * @param ignored
 * @param stage         Shader stage
 * @param out_interface Set to ""
 **/
void XFBAllStagesTest::getShaderInterface(glw::GLuint /* test_case_index */,
                                          Utils::Shader::STAGES stage,
                                          std::string &out_interface)
{
    static const GLchar *vs =
        "layout(xfb_buffer = 0, xfb_offset = 0) out     vec4 vs_tcs;\n"
        "layout(binding    = 0)                 uniform vs_block {\n"
        "    vec4 uni_vs;\n"
        "};\n";
    static const GLchar *tcs =
        "                                       in      vec4 vs_tcs[];\n"
        "layout(xfb_buffer = 1, xfb_offset = 0) out     vec4 tcs_tes[1];\n"
        "layout(binding    = 1)                 uniform tcs_block {\n"
        "    vec4 uni_tcs;\n"
        "};\n";
    static const GLchar *tes =
        "                                       in      vec4 tcs_tes[];\n"
        "layout(xfb_buffer = 2, xfb_offset = 0) out     vec4 tes_gs;\n"
        "layout(binding    = 2)                 uniform tes_block {\n"
        "    vec4 uni_tes;\n"
        "};\n";
    static const GLchar *gs =
        "                                       in      vec4 tes_gs[];\n"
        "layout(xfb_buffer = 3, xfb_offset = 0) out     vec4 gs_fs;\n"
        "layout(binding    = 3)                 uniform gs_block {\n"
        "    vec4 uni_gs;\n"
        "};\n";
    static const GLchar *fs =
        "                       in      vec4 gs_fs;\n"
        "                       out     vec4 fs_out;\n"
        "layout(binding    = 4) uniform fs_block {\n"
        "    vec4 uni_fs;\n"
        "};\n";

    const GLchar *interface = 0;
    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            interface = fs;
            break;
        case Utils::Shader::GEOMETRY:
            interface = gs;
            break;
        case Utils::Shader::TESS_CTRL:
            interface = tcs;
            break;
        case Utils::Shader::TESS_EVAL:
            interface = tes;
            break;
        case Utils::Shader::VERTEX:
            interface = vs;
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    out_interface = interface;
}

/* Constants used by XFBStrideOfEmptyListTest */
const GLuint XFBStrideOfEmptyListTest::m_stride = 64;

/** Constructor
 *
 * @param context Test context
 **/
XFBStrideOfEmptyListTest::XFBStrideOfEmptyListTest(deqp::Context &context)
    : BufferTestBase(context,
                     "xfb_stride_of_empty_list",
                     "Test verifies correct behavior when xfb_stride qualifier is specified but no "
                     "xfb_offset is specified")
{
    /* Nothing to be done here */
}

/** Execute drawArrays for single vertex
 *
 * @param test_case_index Index of test case
 *
 * @return true if proper error is reported
 **/
bool XFBStrideOfEmptyListTest::executeDrawCall(bool /* tesEnabled */, GLuint test_case_index)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();
    bool result         = true;

    /* Draw */
    gl.disable(GL_RASTERIZER_DISCARD);
    GLU_EXPECT_NO_ERROR(gl.getError(), "Disable");

    gl.beginTransformFeedback(GL_POINTS);
    GLenum error = gl.getError();
    switch (test_case_index)
    {
        case VALID:
            if (GL_NO_ERROR != error)
            {
                gl.endTransformFeedback();
                GLU_EXPECT_NO_ERROR(error, "BeginTransformFeedback");
            }

            gl.drawArrays(GL_PATCHES, 0 /* first */, 1 /* count */);
            error = gl.getError();

            gl.endTransformFeedback();
            GLU_EXPECT_NO_ERROR(error, "DrawArrays");
            GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

            break;

        case FIRST_MISSING:
            if (GL_NO_ERROR == error)
            {
                gl.endTransformFeedback();
            }

            if (GL_INVALID_OPERATION != error)
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message
                    << "XFB at index 0, that is written by GS, is missing. It was expected that "
                       "INVALID_OPERATION will generated by BeginTransformFeedback. Got: "
                    << glu::getErrorStr(error) << tcu::TestLog::EndMessage;

                result = false;
            }

            break;

        case SECOND_MISSING:
            if (GL_NO_ERROR != error)
            {
                gl.endTransformFeedback();
                GLU_EXPECT_NO_ERROR(error, "BeginTransformFeedback");
            }

            gl.drawArrays(GL_PATCHES, 0 /* first */, 1 /* count */);
            error = gl.getError();

            gl.endTransformFeedback();
            GLU_EXPECT_NO_ERROR(error, "DrawArrays");
            GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

            break;
    }

    /* Done */
    return result;
}

/** Get descriptors of buffers necessary for test
 *
 * @param test_case_index Index of test case
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBStrideOfEmptyListTest::getBufferDescriptors(glw::GLuint test_case_index,
                                                    bufferDescriptor::Vector &out_descriptors)
{
    switch (test_case_index)
    {
        case VALID:
        {
            /* Test needs single uniform and two xfbs */
            out_descriptors.resize(3);

            /* Get references */
            bufferDescriptor &uniform = out_descriptors[0];
            bufferDescriptor &xfb_0   = out_descriptors[1];
            bufferDescriptor &xfb_1   = out_descriptors[2];

            /* Index */
            uniform.m_index = 0;
            xfb_0.m_index   = 0;
            xfb_1.m_index   = 1;

            /* Target */
            uniform.m_target = Utils::Buffer::Uniform;
            xfb_0.m_target   = Utils::Buffer::Transform_feedback;
            xfb_1.m_target   = Utils::Buffer::Transform_feedback;

            /* Data */
            uniform.m_initial_data = Utils::Type::vec4.GenerateDataPacked();

            xfb_0.m_initial_data  = Utils::Type::vec4.GenerateDataPacked();
            xfb_0.m_expected_data = uniform.m_initial_data;

            /* Data, contents are the same as no modification is expected */
            xfb_1.m_initial_data.resize(m_stride);
            xfb_1.m_expected_data.resize(m_stride);

            for (GLuint i = 0; i < m_stride; ++i)
            {
                xfb_1.m_initial_data[0]  = (glw::GLubyte)i;
                xfb_1.m_expected_data[0] = (glw::GLubyte)i;
            }
        }

        break;

        case FIRST_MISSING:
        {
            /* Test needs single uniform and two xfbs */
            out_descriptors.resize(2);

            /* Get references */
            bufferDescriptor &uniform = out_descriptors[0];
            bufferDescriptor &xfb_1   = out_descriptors[1];

            /* Index */
            uniform.m_index = 0;
            xfb_1.m_index   = 1;

            /* Target */
            uniform.m_target = Utils::Buffer::Uniform;
            xfb_1.m_target   = Utils::Buffer::Transform_feedback;

            /* Data */
            uniform.m_initial_data = Utils::Type::vec4.GenerateDataPacked();

            /* Draw call will not be executed, contents does not matter */
            xfb_1.m_initial_data.resize(m_stride);
        }

        break;

        case SECOND_MISSING:
        {
            /* Test needs single uniform and two xfbs */
            out_descriptors.resize(2);

            /* Get references */
            bufferDescriptor &uniform = out_descriptors[0];
            bufferDescriptor &xfb_0   = out_descriptors[1];

            /* Index */
            uniform.m_index = 0;
            xfb_0.m_index   = 0;

            /* Target */
            uniform.m_target = Utils::Buffer::Uniform;
            xfb_0.m_target   = Utils::Buffer::Transform_feedback;

            /* Data */
            uniform.m_initial_data = Utils::Type::vec4.GenerateDataPacked();

            xfb_0.m_initial_data  = Utils::Type::vec4.GenerateDataPacked();
            xfb_0.m_expected_data = uniform.m_initial_data;
        }

        break;
    }
}

/** Get body of main function for given shader stage
 *
 * @param ignored
 * @param stage            Shader stage
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void XFBStrideOfEmptyListTest::getShaderBody(GLuint /* test_case_index */,
                                             Utils::Shader::STAGES stage,
                                             std::string &out_assignments,
                                             std::string &out_calculations)
{
    out_calculations = "";

    static const GLchar *gs = "    gs_fs  = uni_gs;\n";
    static const GLchar *fs = "    fs_out = vec4(gs_fs);\n";

    const GLchar *assignments = "";
    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            assignments = fs;
            break;
        case Utils::Shader::GEOMETRY:
            assignments = gs;
            break;
        default:
            break;
    }

    out_assignments = assignments;
}

/** Get interface of shader
 *
 * @param ignored
 * @param stage            Shader stage
 * @param out_interface    Set to ""
 **/
void XFBStrideOfEmptyListTest::getShaderInterface(GLuint /* test_case_index */,
                                                  Utils::Shader::STAGES stage,
                                                  std::string &out_interface)
{
    static const GLchar *gs =
        "layout (xfb_buffer = 0, xfb_offset = 0)  out     vec4 gs_fs;\n"
        "layout (xfb_buffer = 1, xfb_stride = 64) out;\n"
        "\n"
        "layout (binding    = 0)                  uniform gs_block {\n"
        "    vec4 uni_gs;\n"
        "};\n";
    static const GLchar *fs =
        "in  vec4 gs_fs;\n"
        "out vec4 fs_out;\n";

    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            out_interface = fs;
            break;
        case Utils::Shader::GEOMETRY:
            out_interface = gs;
            break;
        default:
            out_interface = "";
            return;
    }
}

/** Returns buffer details in human readable form.
 *
 * @param test_case_index Index of test case
 *
 * @return Case description
 **/
std::string XFBStrideOfEmptyListTest::getTestCaseName(GLuint test_case_index)
{
    std::string result;

    switch (test_case_index)
    {
        case VALID:
            result = "Valid case";
            break;
        case FIRST_MISSING:
            result = "Missing xfb at index 0";
            break;
        case SECOND_MISSING:
            result = "Missing xfb at index 1";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Get number of test cases
 *
 * @return 3
 **/
GLuint XFBStrideOfEmptyListTest::getTestCaseNumber()
{
    return 3;
}

/* Constants used by XFBStrideOfEmptyListTest */
const GLuint XFBStrideOfEmptyListAndAPITest::m_stride = 64;

/** Constructor
 *
 * @param context Test context
 **/
XFBStrideOfEmptyListAndAPITest::XFBStrideOfEmptyListAndAPITest(deqp::Context &context)
    : BufferTestBase(context,
                     "xfb_stride_of_empty_list_and_api",
                     "Test verifies that xfb_stride qualifier is not overriden by API")
{
    /* Nothing to be done here */
}

/** Execute drawArrays for single vertex
 *
 * @param test_case_index Index of test case
 *
 * @return true if proper error is reported
 **/
bool XFBStrideOfEmptyListAndAPITest::executeDrawCall(bool /* tesEnabled */, GLuint test_case_index)
{
    const Functions &gl = m_context.getRenderContext().getFunctions();
    bool result         = true;

    /* Draw */
    gl.disable(GL_RASTERIZER_DISCARD);
    GLU_EXPECT_NO_ERROR(gl.getError(), "Disable");

    gl.beginTransformFeedback(GL_POINTS);
    GLenum error = gl.getError();
    switch (test_case_index)
    {
        case VALID:
            if (GL_NO_ERROR != error)
            {
                gl.endTransformFeedback();
                GLU_EXPECT_NO_ERROR(error, "BeginTransformFeedback");
            }

            gl.drawArrays(GL_PATCHES, 0 /* first */, 1 /* count */);
            error = gl.getError();

            gl.endTransformFeedback();
            GLU_EXPECT_NO_ERROR(error, "DrawArrays");
            GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

            break;

        case FIRST_MISSING:
            if (GL_NO_ERROR != error)
            {
                gl.endTransformFeedback();
                GLU_EXPECT_NO_ERROR(error, "BeginTransformFeedback");
            }

            gl.drawArrays(GL_PATCHES, 0 /* first */, 1 /* count */);
            error = gl.getError();

            gl.endTransformFeedback();
            GLU_EXPECT_NO_ERROR(error, "DrawArrays");
            GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

            break;

        case SECOND_MISSING:
            if (GL_NO_ERROR == error)
            {
                gl.endTransformFeedback();
            }

            if (GL_INVALID_OPERATION != error)
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message
                    << "XFB at index 1, that is declared as empty, is missing. It was expected "
                       "that INVALID_OPERATION will generated by BeginTransformFeedback. Got: "
                    << glu::getErrorStr(error) << tcu::TestLog::EndMessage;

                result = false;
            }

            break;
    }

    /* Done */
    return result;
}

/** Get descriptors of buffers necessary for test
 *
 * @param test_case_index Index of test case
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBStrideOfEmptyListAndAPITest::getBufferDescriptors(glw::GLuint test_case_index,
                                                          bufferDescriptor::Vector &out_descriptors)
{
    switch (test_case_index)
    {
        case VALID:
        {
            /* Test needs single uniform and two xfbs */
            out_descriptors.resize(3);

            /* Get references */
            bufferDescriptor &uniform = out_descriptors[0];
            bufferDescriptor &xfb_0   = out_descriptors[1];
            bufferDescriptor &xfb_1   = out_descriptors[2];

            /* Index */
            uniform.m_index = 0;
            xfb_0.m_index   = 0;
            xfb_1.m_index   = 1;

            /* Target */
            uniform.m_target = Utils::Buffer::Uniform;
            xfb_0.m_target   = Utils::Buffer::Transform_feedback;
            xfb_1.m_target   = Utils::Buffer::Transform_feedback;

            /* Data */
            uniform.m_initial_data = Utils::Type::vec4.GenerateDataPacked();

            /* Data, contents are the same as no modification is expected */
            xfb_0.m_initial_data.resize(m_stride);
            xfb_0.m_expected_data.resize(m_stride);

            for (GLuint i = 0; i < m_stride; ++i)
            {
                xfb_0.m_initial_data[0]  = (glw::GLubyte)i;
                xfb_0.m_expected_data[0] = (glw::GLubyte)i;
            }

            xfb_1.m_initial_data  = Utils::Type::vec4.GenerateDataPacked();
            xfb_1.m_expected_data = uniform.m_initial_data;
        }

        break;

        case FIRST_MISSING:
        {
            /* Test needs single uniform and two xfbs */
            out_descriptors.resize(2);

            /* Get references */
            bufferDescriptor &uniform = out_descriptors[0];
            bufferDescriptor &xfb_1   = out_descriptors[1];

            /* Index */
            uniform.m_index = 0;
            xfb_1.m_index   = 1;

            /* Target */
            uniform.m_target = Utils::Buffer::Uniform;
            xfb_1.m_target   = Utils::Buffer::Transform_feedback;

            /* Data */
            uniform.m_initial_data = Utils::Type::vec4.GenerateDataPacked();

            /* Data, contents are the same as no modification is expected */
            xfb_1.m_initial_data  = Utils::Type::vec4.GenerateDataPacked();
            xfb_1.m_expected_data = uniform.m_initial_data;
        }

        break;

        case SECOND_MISSING:
        {
            /* Test needs single uniform and two xfbs */
            out_descriptors.resize(2);

            /* Get references */
            bufferDescriptor &uniform = out_descriptors[0];
            bufferDescriptor &xfb_0   = out_descriptors[1];

            /* Index */
            uniform.m_index = 0;
            xfb_0.m_index   = 0;

            /* Target */
            uniform.m_target = Utils::Buffer::Uniform;
            xfb_0.m_target   = Utils::Buffer::Transform_feedback;

            /* Data */
            uniform.m_initial_data = Utils::Type::vec4.GenerateDataPacked();

            /* Draw call will not be executed, contents does not matter */
            xfb_0.m_initial_data = Utils::Type::vec4.GenerateDataPacked();
        }

        break;
    }
}

/** Get list of names of varyings that will be registered with TransformFeedbackVaryings
 *
 * @param ignored
 * @param captured_varyings Vector of varying names to be captured
 **/
void XFBStrideOfEmptyListAndAPITest::getCapturedVaryings(
    glw::GLuint /* test_case_index */,
    Utils::Program::NameVector &captured_varyings,
    GLint *xfb_components)
{
    captured_varyings.push_back("gs_fs1");
    captured_varyings.push_back("gs_fs2");
    *xfb_components = 4;
}

/** Get body of main function for given shader stage
 *
 * @param ignored
 * @param stage            Shader stage
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void XFBStrideOfEmptyListAndAPITest::getShaderBody(GLuint /* test_case_index */,
                                                   Utils::Shader::STAGES stage,
                                                   std::string &out_assignments,
                                                   std::string &out_calculations)
{
    out_calculations = "";

    static const GLchar *gs =
        "    gs_fs1 = -uni_gs;\n"
        "    gs_fs2 = uni_gs;\n";
    static const GLchar *fs = "    fs_out = vec4(gs_fs2);\n";

    const GLchar *assignments = "";
    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            assignments = fs;
            break;
        case Utils::Shader::GEOMETRY:
            assignments = gs;
            break;
        default:
            break;
    }

    out_assignments = assignments;
}

/** Get interface of shader
 *
 * @param ignored
 * @param stage            Shader stage
 * @param out_interface    Set to ""
 **/
void XFBStrideOfEmptyListAndAPITest::getShaderInterface(GLuint /* test_case_index */,
                                                        Utils::Shader::STAGES stage,
                                                        std::string &out_interface)
{
    static const GLchar *gs =
        "layout (xfb_buffer = 0, xfb_stride = 64) out vec4 gs_fs1;\n"
        "layout (xfb_buffer = 1, xfb_offset = 0)  out vec4 gs_fs2;\n"
        "\n"
        "layout(binding    = 0) uniform gs_block {\n"
        "    vec4 uni_gs;\n"
        "};\n";
    static const GLchar *fs =
        "in  vec4 gs_fs2;\n"
        "out vec4 fs_out;\n";

    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            out_interface = fs;
            break;
        case Utils::Shader::GEOMETRY:
            out_interface = gs;
            break;
        default:
            out_interface = "";
            return;
    }
}

/** Returns buffer details in human readable form.
 *
 * @param test_case_index Index of test case
 *
 * @return Case description
 **/
std::string XFBStrideOfEmptyListAndAPITest::getTestCaseName(GLuint test_case_index)
{
    std::string result;

    switch (test_case_index)
    {
        case VALID:
            result = "Valid case";
            break;
        case FIRST_MISSING:
            result = "Missing xfb at index 0";
            break;
        case SECOND_MISSING:
            result = "Missing xfb at index 1";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return result;
}

/** Get number of test cases
 *
 * @return 2
 **/
GLuint XFBStrideOfEmptyListAndAPITest::getTestCaseNumber()
{
    return 3;
}

/** Constructor
 *
 * @param context Test framework context
 **/
XFBTooSmallStrideTest::XFBTooSmallStrideTest(deqp::Context &context, GLuint constant, GLuint stage)
    : NegativeTestBase(
          context,
          "xfb_too_small_stride",
          "Test verifies that compiler reports error when xfb_stride sets not enough space"),
      m_constant(constant),
      m_stage(stage)
{
    std::string name = ("xfb_too_small_stride_");
    name.append(getCaseEnumName(m_constant));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBTooSmallStrideTest::getShaderSource(GLuint test_case_index,
                                                   Utils::Shader::STAGES stage)
{
#if DEBUG_NEG_REMOVE_ERROR
    static const GLchar *array_var_definition =
        "layout (xfb_buffer = 0 /*, xfb_stride = 32 */ ) out;\n"
#else
    static const GLchar *array_var_definition =
        "layout (xfb_buffer = 0, xfb_stride = 32) out;\n"
#endif /* DEBUG_NEG_REMOVE_ERROR */
        "\n"
        "layout (xfb_offset = 16) out vec4 gohan[4];\n";
#if DEBUG_NEG_REMOVE_ERROR
    static const GLchar *block_var_definition =
        "layout (xfb_buffer = 0 /*, xfb_stride = 32 */ ) out;\n"
#else
    static const GLchar *block_var_definition =
        "layout (xfb_buffer = 0, xfb_stride = 32) out;\n"
#endif /* DEBUG_NEG_REMOVE_ERROR */
        "\n"
        "layout (xfb_offset = 0) out Goku {\n"
        "    vec4 gohan;\n"
        "    vec4 goten;\n"
        "    vec4 chichi;\n"
        "} goku;\n";
#if DEBUG_NEG_REMOVE_ERROR
    static const GLchar *offset_var_definition =
        "layout (xfb_buffer = 0 /*, xfb_stride = 40 */ ) out;\n"
#else
    static const GLchar *offset_var_definition =
        "layout (xfb_buffer = 0, xfb_stride = 40) out;\n"
#endif /* DEBUG_NEG_REMOVE_ERROR */
        "\n"
        "layout (xfb_offset = 32) out vec4 gohan;\n";
// The test considers gohan overflows the buffer 0, but according to spec, it is valid to declare
// the variable with qualifier "layout (xfb_offset = 16, xfb_stride = 32) out vec4 gohan;" To make
// the shader failed to compile, change xfb_stride to a value that is smaller than 32
#if DEBUG_NEG_REMOVE_ERROR
    static const GLchar *stride_var_definition =
        "layout (xfb_buffer = 0 /*, xfb_stride = 28 */ ) out;\n"
        "\n"
        "layout (xfb_offset = 16 /*, xfb_stride = 28 */ ) out vec4 gohan;\n";
#else
    static const GLchar *stride_var_definition =
        "layout (xfb_buffer = 0, xfb_stride = 28) out;\n"
        "\n"
        "layout (xfb_offset = 16, xfb_stride = 28) out vec4 gohan;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *array_use =
        "    gohan[0] = result / 2;\n"
        "    gohan[1] = result / 4;\n"
        "    gohan[2] = result / 6;\n"
        "    gohan[3] = result / 8;\n";
    static const GLchar *block_use =
        "    goku.gohan  = result / 2;\n"
        "    goku.goten  = result / 4;\n"
        "    goku.chichi = result / 6;\n";
    static const GLchar *output_use = "gohan = result / 4;\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 any_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = any_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_any[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_any[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_any;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_any = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs += result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        size_t position              = 0;
        const GLchar *var_definition = 0;
        const GLchar *var_use        = 0;

        switch (test_case.m_case)
        {
            case OFFSET:
                var_definition = offset_var_definition;
                var_use        = output_use;
                break;
            case STRIDE:
                var_definition = stride_var_definition;
                var_use        = output_use;
                break;
            case BLOCK:
                var_definition = block_var_definition;
                var_use        = block_use;
                break;
            case ARRAY:
                var_definition = array_var_definition;
                var_use        = array_use;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);
    }
    else
    {
        switch (test_case.m_stage)
        {
            case Utils::Shader::GEOMETRY:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::TESS_EVAL:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::TESS_CTRL:
                        source = tcs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::VERTEX:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    default:
                        source = "";
                }
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string XFBTooSmallStrideTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage) << ", case: ";

    switch (test_case.m_case)
    {
        case OFFSET:
            stream << "buffer stride: 40, vec4 offset: 32";
            break;
        case STRIDE:
            stream << "buffer stride: 32, vec4 off 16 stride: 32";
            break;
        case BLOCK:
            stream << "buffer stride: 32, block 3xvec4 offset 0";
            break;
        case ARRAY:
            stream << "buffer stride: 32, vec4[4] offset 16";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return stream.str();
}

std::string XFBTooSmallStrideTest::getCaseEnumName(glw::GLuint case_index)
{
    std::string case_name("case_max");
    switch (case_index)
    {
        case 0:
            case_name = "offset";
            break;
        case 1:
            case_name = "stride";
            break;
        case 2:
            case_name = "block";
            break;
        case 3:
            case_name = "array";
            break;
        default:
            case_name = "case_max";
            break;
    }
    return case_name;
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint XFBTooSmallStrideTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool XFBTooSmallStrideTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void XFBTooSmallStrideTest::testInit()
{
    testCase test_case = {(CASES)m_constant, (Utils::Shader::STAGES)m_stage};

    m_test_cases.push_back(test_case);
}

/** Constructor
 *
 * @param context Test framework context
 **/
XFBVariableStrideTest::XFBVariableStrideTest(deqp::Context &context,
                                             glw::GLuint type,
                                             glw::GLuint stage,
                                             glw::GLuint constant)
    : NegativeTestBase(context,
                       "xfb_variable_stride",
                       "Test verifies that stride qualifier is respected"),
      m_type(type),
      m_stage(stage),
      m_constant(constant)
{
    std::string name = ("xfb_variable_stride_");
    name.append(getTypeName(m_type));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)m_stage));
    name.append("_");
    name.append(getCaseEnumName(m_constant));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBVariableStrideTest::getShaderSource(GLuint test_case_index,
                                                   Utils::Shader::STAGES stage)
{
    static const GLchar *invalid_var_definition =
        "const uint type_size = SIZE;\n"
        "\n"
#if DEBUG_NEG_REMOVE_ERROR
        "layout (xfb_stride = 2 * type_size) out;\n"
#else
        "layout (xfb_stride = type_size) out;\n"
#endif /* DEBUG_NEG_REMOVE_ERROR */
        "\n"
        "layout (xfb_offset = 0)         out TYPE goku;\n"
        "layout (xfb_offset = type_size) out TYPE vegeta;\n";
    static const GLchar *valid_var_definition =
        "const uint type_size = SIZE;\n"
        "\n"
        "layout (xfb_stride = type_size) out;\n"
        "\n"
        "layout (xfb_offset = 0) out TYPE goku;\n";
    static const GLchar *invalid_use =
        "    goku   = TYPE(1);\n"
        "    vegeta = TYPE(0);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        goku   = TYPE(0);\n"
        "        vegeta = TYPE(1);\n"
        "    }\n";
    static const GLchar *valid_use =
        "    goku   = TYPE(1);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        goku   = TYPE(0);\n"
        "    }\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 any_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = any_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_any[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_any[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_any;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_any = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        GLchar buffer[16];
        size_t position              = 0;
        const GLchar *type_name      = test_case.m_type.GetGLSLTypeName();
        const GLchar *var_definition = 0;
        const GLchar *var_use        = 0;

        sprintf(buffer, "%d", test_case.m_type.GetSize());

        switch (test_case.m_case)
        {
            case VALID:
                var_definition = valid_var_definition;
                var_use        = valid_use;
                break;
            case INVALID:
                var_definition = invalid_var_definition;
                var_use        = invalid_use;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("SIZE", position, buffer, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("TYPE", type_name, source);
    }
    else
    {
        switch (test_case.m_stage)
        {
            case Utils::Shader::GEOMETRY:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::TESS_EVAL:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::TESS_CTRL:
                        source = tcs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::VERTEX:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    default:
                        source = "";
                }
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string XFBVariableStrideTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage)
           << ", type: " << test_case.m_type.GetGLSLTypeName() << ", case: ";

    switch (test_case.m_case)
    {
        case VALID:
            stream << "valid";
            break;
        case INVALID:
            stream << "invalid";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return stream.str();
}

std::string XFBVariableStrideTest::getCaseEnumName(glw::GLuint case_index)
{
    std::string case_name("case_max");
    switch (case_index)
    {
        case 0:
            case_name = "valid";
            break;
        case 1:
            case_name = "invalid";
            break;
        default:
            case_name = "case_max";
            break;
    }
    return case_name;
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint XFBVariableStrideTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool XFBVariableStrideTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Selects if compilation failure is expected result
 *
 * @param test_case_index Index of test case
 *
 * @return true
 **/
bool XFBVariableStrideTest::isFailureExpected(GLuint test_case_index)
{
    testCase &test_case = m_test_cases[test_case_index];

    return (INVALID == test_case.m_case);
}

/** Prepare all test cases
 *
 **/
void XFBVariableStrideTest::testInit()
{
    const Utils::Type &type = getType(m_type);

    testCase test_case = {static_cast<CASES>(m_constant),
                          static_cast<Utils::Shader::STAGES>(m_stage), type};

    m_test_cases.push_back(test_case);
}

/** Constructor
 *
 * @param context Test framework context
 **/
XFBBlockStrideTest::XFBBlockStrideTest(deqp::Context &context, GLuint stage)
    : TestBase(context,
               "xfb_block_stride",
               "Test verifies that stride qualifier is respected for blocks"),
      m_stage(stage)
{
    std::string name = ("xfb_block_stride_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)m_stage));

    TestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBBlockStrideTest::getShaderSource(GLuint test_case_index, Utils::Shader::STAGES stage)
{
    static const GLchar *var_definition =
        "layout (xfb_offset = 0, xfb_stride = 128) out Goku {\n"
        "    vec4 gohan;\n"
        "    vec4 goten;\n"
        "    vec4 chichi;\n"
        "} gokuARRAY;\n";
    static const GLchar *var_use =
        "    gokuINDEX.gohan  = vec4(1, 0, 0, 0);\n"
        "    gokuINDEX.goten  = vec4(0, 0, 1, 0);\n"
        "    gokuINDEX.chichi = vec4(0, 1, 0, 0);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gokuINDEX.gohan  = vec4(0, 1, 1, 1);\n"
        "        gokuINDEX.goten  = vec4(1, 1, 0, 1);\n"
        "        gokuINDEX.chichi = vec4(1, 0, 1, 1);\n"
        "    }\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "out gl_PerVertex \n"
        "{ \n"
        "   vec4  gl_Position; \n"  // gl_Position must be redeclared in separable program mode
        "}; \n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_tcs[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
#if 0
    static const GLchar* tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
#endif
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_tcs = tes_gs = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs += result;\n"
        "}\n"
        "\n";

    std::string source;
    Utils::Shader::STAGES test_case = m_test_cases[test_case_index];

    if (test_case == stage)
    {
        const GLchar *array = "";
        const GLchar *index = "";
        size_t position     = 0;
        size_t temp;
        // It is a compile time error to apply xfb_offset to the declaration of an unsized
        // array(GLSL4.5 spec: Page73) change array = "[]" to "[1]"
        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = "[1]";
                index  = "[0]";
                break;
/*
             It is invalid to define transform feedback output in HS
             */
#if 0
            case Utils::Shader::TESS_CTRL:
            source = tcs_tested;
            array = "[]";
            index = "[gl_InvocationID]";
            break;
#endif
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = "[1]";
                index  = "[0]";
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        temp = position;
        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = temp;
        Utils::replaceToken("ARRAY", position, array, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("INDEX", index, source);
    }
    else
    {
        switch (test_case)
        {
            case Utils::Shader::GEOMETRY:
                switch (stage)
                {
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::TESS_CTRL:
                switch (stage)
                {
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::TESS_EVAL:
                switch (stage)
                {
                    case Utils::Shader::TESS_CTRL:
                        source = tcs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::VERTEX:
                source = "";
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string XFBBlockStrideTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;

    stream << "Stage: " << Utils::Shader::GetStageName(m_test_cases[test_case_index]);

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint XFBBlockStrideTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Inspects program for xfb stride
 *
 * @param program Program to query
 *
 * @return true if query results match expected values, false otherwise
 **/
bool XFBBlockStrideTest::inspectProgram(Utils::Program &program)
{
    GLint stride = 0;

    program.GetResource(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */,
                        GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE, 1 /* buf_size */, &stride);

    return (128 == stride);
}

/** Runs test case
 *
 * @param test_case_index Id of test case
 *
 * @return true if test case pass, false otherwise
 **/
bool XFBBlockStrideTest::testCase(GLuint test_case_index)
{
    const std::string &gs_source = getShaderSource(test_case_index, Utils::Shader::GEOMETRY);
    Utils::Program program(m_context);
    const std::string &tcs_source = getShaderSource(test_case_index, Utils::Shader::TESS_CTRL);
    const std::string &tes_source = getShaderSource(test_case_index, Utils::Shader::TESS_EVAL);
    bool test_case_result         = true;
    const std::string &vs_source  = getShaderSource(test_case_index, Utils::Shader::VERTEX);

    program.Init("" /* cs */, "", gs_source, tcs_source, tes_source, vs_source,
                 true /* separable */);

    test_case_result = inspectProgram(program);

    return test_case_result;
}

/** Prepare all test cases
 *
 **/
void XFBBlockStrideTest::testInit()
{
    m_test_cases.push_back((Utils::Shader::STAGES)m_stage);
}

/** Constructor
 *
 * @param context Test context
 **/
XFBBlockMemberStrideTest::XFBBlockMemberStrideTest(deqp::Context &context)
    : BufferTestBase(context,
                     "xfb_block_member_stride",
                     "Test verifies that xfb_stride qualifier is respected for block member")
{
    /* Nothing to be done here */
}

/** Get descriptors of buffers necessary for test
 *
 * @param ignored
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBBlockMemberStrideTest::getBufferDescriptors(glw::GLuint /* test_case_index */,
                                                    bufferDescriptor::Vector &out_descriptors)
{
    const Utils::Type &vec4 = Utils::Type::vec4;

    /* Test needs single uniform and xfb */
    out_descriptors.resize(2);

    /* Get references */
    bufferDescriptor &uniform = out_descriptors[0];
    bufferDescriptor &xfb     = out_descriptors[1];

    /* Index */
    uniform.m_index = 0;
    xfb.m_index     = 0;

    /* Target */
    uniform.m_target = Utils::Buffer::Uniform;
    xfb.m_target     = Utils::Buffer::Transform_feedback;

    /* Data */
    static const GLuint vec4_size           = 16;
    const std::vector<GLubyte> &gohan_data  = vec4.GenerateDataPacked();
    const std::vector<GLubyte> &goten_data  = vec4.GenerateDataPacked();
    const std::vector<GLubyte> &chichi_data = vec4.GenerateDataPacked();

    /* Uniform data */
    uniform.m_initial_data.resize(3 * vec4_size);
    memcpy(&uniform.m_initial_data[0] + 0, &gohan_data[0], vec4_size);
    memcpy(&uniform.m_initial_data[0] + vec4_size, &goten_data[0], vec4_size);
    memcpy(&uniform.m_initial_data[0] + 2 * vec4_size, &chichi_data[0], vec4_size);

    /* XFB data */
    xfb.m_initial_data.resize(4 * vec4_size);
    xfb.m_expected_data.resize(4 * vec4_size);

    for (GLuint i = 0; i < 4 * vec4_size; ++i)
    {
        xfb.m_initial_data[i]  = (glw::GLubyte)i;
        xfb.m_expected_data[i] = (glw::GLubyte)i;
    }

    // the xfb_offset of "chichi" should be 32
    memcpy(&xfb.m_expected_data[0] + 0, &gohan_data[0], vec4_size);
    memcpy(&xfb.m_expected_data[0] + vec4_size, &goten_data[0], vec4_size);
    memcpy(&xfb.m_expected_data[0] + 2 * vec4_size, &chichi_data[0], vec4_size);
}

/** Get body of main function for given shader stage
 *
 * @param ignored
 * @param stage            Shader stage
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void XFBBlockMemberStrideTest::getShaderBody(GLuint /* test_case_index */,
                                             Utils::Shader::STAGES stage,
                                             std::string &out_assignments,
                                             std::string &out_calculations)
{
    out_calculations = "";

    static const GLchar *gs =
        "    gohan  = uni_gohan;\n"
        "    goten  = uni_goten;\n"
        "    chichi = uni_chichi;\n";
    static const GLchar *fs = "    fs_out = gohan + goten + chichi;\n";

    const GLchar *assignments = "";
    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            assignments = fs;
            break;
        case Utils::Shader::GEOMETRY:
            assignments = gs;
            break;
        default:
            break;
    }

    out_assignments = assignments;
}

/** Get interface of shader
 *
 * @param ignored
 * @param stage            Shader stage
 * @param out_interface    Set to ""
 **/
void XFBBlockMemberStrideTest::getShaderInterface(GLuint /* test_case_index */,
                                                  Utils::Shader::STAGES stage,
                                                  std::string &out_interface)
{
    static const GLchar *gs =
        "layout (xfb_buffer = 0, xfb_offset = 0) out Goku {\n"
        "                             vec4 gohan;\n"
        "    layout (xfb_stride = 48) vec4 goten;\n"
        "                             vec4 chichi;\n"
        "};\n"
        "layout(binding = 0) uniform gs_block {\n"
        "    vec4 uni_gohan;\n"
        "    vec4 uni_goten;\n"
        "    vec4 uni_chichi;\n"
        "};\n";
    static const GLchar *fs =
        "in Goku {\n"
        "    vec4 gohan;\n"
        "    vec4 goten;\n"
        "    vec4 chichi;\n"
        "};\n"
        "out vec4 fs_out;\n";

    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            out_interface = fs;
            break;
        case Utils::Shader::GEOMETRY:
            out_interface = gs;
            break;
        default:
            out_interface = "";
            return;
    }
}

/** Inspects program to check if all resources are as expected
 *
 * @param ignored
 * @param program    Program instance
 * @param out_stream Error message
 *
 * @return true if everything is ok, false otherwise
 **/
bool XFBBlockMemberStrideTest::inspectProgram(GLuint /* test_case_index*/,
                                              Utils::Program &program,
                                              std::stringstream &out_stream)
{
    const GLuint gohan_id  = program.GetResourceIndex("gohan", GL_TRANSFORM_FEEDBACK_VARYING);
    const GLuint goten_id  = program.GetResourceIndex("goten", GL_TRANSFORM_FEEDBACK_VARYING);
    const GLuint chichi_id = program.GetResourceIndex("chichi", GL_TRANSFORM_FEEDBACK_VARYING);

    GLint gohan_offset  = 0;
    GLint goten_offset  = 0;
    GLint chichi_offset = 0;

    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, gohan_id, GL_OFFSET, 1, &gohan_offset);
    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, goten_id, GL_OFFSET, 1, &goten_offset);
    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, chichi_id, GL_OFFSET, 1, &chichi_offset);

    // the xfb_offset of "chichi" should be 32
    if ((0 != gohan_offset) || (16 != goten_offset) || (32 != chichi_offset))
    {
        out_stream << "Got wrong offset: [" << gohan_offset << ", " << goten_offset << ", "
                   << chichi_offset << "] expected: [0, 16, 32]";
        return false;
    }

    return true;
}

/** Constructor
 *
 * @param context Test framework context
 **/
XFBDuplicatedStrideTest::XFBDuplicatedStrideTest(deqp::Context &context,
                                                 GLuint constant,
                                                 GLuint stage)
    : NegativeTestBase(
          context,
          "xfb_duplicated_stride",
          "Test verifies that compiler reports error when conflicting stride qualifiers are used"),
      m_constant(constant),
      m_stage(stage)
{
    std::string name = ("xfb_duplicated_stride_");
    name.append(getCaseEnumName(m_constant));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBDuplicatedStrideTest::getShaderSource(GLuint test_case_index,
                                                     Utils::Shader::STAGES stage)
{
    static const GLchar *invalid_var_definition =
        "const uint valid_stride = 64;\n"
#if DEBUG_NEG_REMOVE_ERROR
        "const uint conflicting_stride = 64;\n"
#else
        "const uint conflicting_stride = 128;\n"
#endif /* DEBUG_NEG_REMOVE_ERROR */
        "\n"
        "layout (xfb_buffer = 0, xfb_stride = valid_stride)       out;\n"
        "layout (xfb_buffer = 0, xfb_stride = conflicting_stride) out;\n";
    static const GLchar *valid_var_definition =
        "const uint valid_stride = 64;\n"
        "\n"
        "layout (xfb_buffer = 0, xfb_stride = valid_stride) out;\n"
        "layout (xfb_buffer = 0, xfb_stride = valid_stride) out;\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 any_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = any_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_any[0];\n"
        "\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_any[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "    any_fs = result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_any;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_any = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "    any_fs += result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        size_t position              = 0;
        const GLchar *var_definition = 0;

        switch (test_case.m_case)
        {
            case VALID:
                var_definition = valid_var_definition;
                break;
            case INVALID:
                var_definition = invalid_var_definition;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
    }
    else
    {
        switch (test_case.m_stage)
        {
            case Utils::Shader::GEOMETRY:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::TESS_EVAL:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::TESS_CTRL:
                        source = tcs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::VERTEX:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    default:
                        source = "";
                }
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string XFBDuplicatedStrideTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage) << ", case: ";

    switch (test_case.m_case)
    {
        case VALID:
            stream << "valid";
            break;
        case INVALID:
            stream << "invalid";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return stream.str();
}

std::string XFBDuplicatedStrideTest::getCaseEnumName(GLuint test_case_index)
{
    std::string name = "case_max";

    switch (test_case_index)
    {
        case 0:
            name = "valid";
            break;
        case 1:
            name = "invalid";
            break;
        default:
            name = "case_max";
            break;
    }

    return name;
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint XFBDuplicatedStrideTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool XFBDuplicatedStrideTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Selects if compilation failure is expected result
 *
 * @param test_case_index Index of test case
 *
 * @return true
 **/
bool XFBDuplicatedStrideTest::isFailureExpected(GLuint test_case_index)
{
    testCase &test_case = m_test_cases[test_case_index];

    return (INVALID == test_case.m_case);
}

/** Prepare all test cases
 *
 **/
void XFBDuplicatedStrideTest::testInit()
{
    testCase test_case = {(CASES)m_constant, (Utils::Shader::STAGES)m_stage};

    m_test_cases.push_back(test_case);
}

/** Constructor
 *
 * @param context Test framework context
 **/
XFBGetProgramResourceAPITest::XFBGetProgramResourceAPITest(deqp::Context &context, GLuint type)
    : TestBase(context,
               "xfb_get_program_resource_api",
               "Test verifies that get program resource reports correct results for XFB"),
      m_type(type)
{
    std::string name = ("xfb_get_program_resource_api_");
    name.append(getTypeName(type));

    TestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBGetProgramResourceAPITest::getShaderSource(GLuint test_case_index,
                                                          Utils::Shader::STAGES stage)
{
    static const GLchar *api_var_definition =
        "out TYPE b0_v1ARRAY;\n"
        "out TYPE b1_v1ARRAY;\n"
        "out TYPE b0_v3ARRAY;\n"
        "out TYPE b0_v0ARRAY;\n";
    static const GLchar *xfb_var_definition =
        "const uint type_size = SIZE;\n"
        "\n"
        "layout (xfb_buffer = 1, xfb_stride = 4 * type_size) out;\n"
        "\n"
        "layout (xfb_buffer = 0, xfb_offset = 1 * type_size) out TYPE b0_v1ARRAY;\n"
        "layout (xfb_buffer = 1, xfb_offset = 1 * type_size) out TYPE b1_v1ARRAY;\n"
        "layout (xfb_buffer = 0, xfb_offset = 3 * type_size) out TYPE b0_v3ARRAY;\n"
        "layout (xfb_buffer = 0, xfb_offset = 0 * type_size) out TYPE b0_v0ARRAY;\n";
    static const GLchar *var_use =
        "    b0_v1INDEX = TYPE(0);\n"
        "    b1_v1INDEX = TYPE(1);\n"
        "    b0_v3INDEX = TYPE(0);\n"
        "    b0_v0INDEX = TYPE(1);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        b0_v1INDEX = TYPE(1);\n"
        "        b1_v1INDEX = TYPE(0);\n"
        "        b0_v3INDEX = TYPE(1);\n"
        "        b0_v0INDEX = TYPE(0);\n"
        "    }\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "out gl_PerVertex \n"
        "{ \n"
        "   vec4  gl_Position; \n"  // gl_Position must be redeclared in separable program mode
        "}; \n"
        "in  vec4 tes_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tes_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
#if 0
    static const GLchar* tcs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_tcs[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_tcs[gl_InvocationID];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tcs_tes[gl_InvocationID] = result;\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
#endif
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 tes_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    tes_gs = result;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_tcs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    vs_tcs = result;\n"
        "}\n"
        "\n";

    std::string source;
    const test_Case &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        const GLchar *array = "";
        GLchar buffer[16];
        const GLchar *index = "";
        size_t position     = 0;
        size_t temp;
        const GLchar *type_name      = test_case.m_type.GetGLSLTypeName();
        const GLchar *var_definition = 0;

        sprintf(buffer, "%d", test_case.m_type.GetSize());

        if (XFB == test_case.m_case)
        {
            var_definition = xfb_var_definition;
        }
        else
        {
            var_definition = api_var_definition;
        }

        // It is a compile time error to apply xfb_offset to the declaration of an unsized
        // array(GLSL4.5 spec: Page73) change array = "[]" to "[1]"
        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                array  = "[1]";
                index  = "[0]";
                break;
// It is invalid to output transform feedback varyings in tessellation control shader
#if 0
        case Utils::Shader::TESS_CTRL:
            source = tcs_tested;
            array = "[]";
            index = "[gl_InvocationID]";
            break;
#endif
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                array  = "[1]";
                index  = "[0]";
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        temp = position;
        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        if (XFB == test_case.m_case)
        {
            position = temp;
            Utils::replaceToken("SIZE", position, buffer, source);
        }
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("ARRAY", array, source);
        Utils::replaceAllTokens("INDEX", index, source);
        Utils::replaceAllTokens("TYPE", type_name, source);
    }
    else
    {
        source = "";
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string XFBGetProgramResourceAPITest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    const test_Case &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage)
           << ", type: " << test_case.m_type.GetGLSLTypeName() << ", case: ";

    switch (test_case.m_case)
    {
        case INTERLEAVED:
            stream << "interleaved";
            break;
        case SEPARATED:
            stream << "separated";
            break;
        case XFB:
            stream << "xfb";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint XFBGetProgramResourceAPITest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Inspects program for offset, buffer index, buffer stride and type
 *
 * @param test_case_index Index of test case
 * @param program         Program to query
 *
 * @return true if query results match expected values, false otherwise
 **/
bool XFBGetProgramResourceAPITest::inspectProgram(glw::GLuint test_case_index,
                                                  Utils::Program &program)
{
    GLint b0_stride            = 0;
    GLint b1_stride            = 0;
    GLint b0_v0_buf            = 0;
    GLint b0_v0_offset         = 0;
    GLint b0_v0_type           = 0;
    GLint b0_v1_buf            = 0;
    GLint b0_v1_offset         = 0;
    GLint b0_v1_type           = 0;
    GLint b0_v3_buf            = 0;
    GLint b0_v3_offset         = 0;
    GLint b0_v3_type           = 0;
    GLint b1_v1_buf            = 0;
    GLint b1_v1_offset         = 0;
    GLint b1_v1_type           = 0;
    const test_Case &test_case = m_test_cases[test_case_index];
    const GLenum type_enum     = test_case.m_type.GetTypeGLenum();
    const GLint type_size      = test_case.m_type.GetSize();

    GLuint b0_v0_index = program.GetResourceIndex("b0_v0", GL_TRANSFORM_FEEDBACK_VARYING);
    GLuint b0_v1_index = program.GetResourceIndex("b0_v1", GL_TRANSFORM_FEEDBACK_VARYING);
    GLuint b0_v3_index = program.GetResourceIndex("b0_v3", GL_TRANSFORM_FEEDBACK_VARYING);
    GLuint b1_v1_index = program.GetResourceIndex("b1_v1", GL_TRANSFORM_FEEDBACK_VARYING);

    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, b0_v0_index, GL_OFFSET, 1 /* buf_size */,
                        &b0_v0_offset);
    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, b0_v1_index, GL_OFFSET, 1 /* buf_size */,
                        &b0_v1_offset);
    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, b0_v3_index, GL_OFFSET, 1 /* buf_size */,
                        &b0_v3_offset);
    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, b1_v1_index, GL_OFFSET, 1 /* buf_size */,
                        &b1_v1_offset);

    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, b0_v0_index, GL_TYPE, 1 /* buf_size */,
                        &b0_v0_type);
    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, b0_v1_index, GL_TYPE, 1 /* buf_size */,
                        &b0_v1_type);
    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, b0_v3_index, GL_TYPE, 1 /* buf_size */,
                        &b0_v3_type);
    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, b1_v1_index, GL_TYPE, 1 /* buf_size */,
                        &b1_v1_type);

    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, b0_v0_index,
                        GL_TRANSFORM_FEEDBACK_BUFFER_INDEX, 1 /* buf_size */, &b0_v0_buf);
    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, b0_v1_index,
                        GL_TRANSFORM_FEEDBACK_BUFFER_INDEX, 1 /* buf_size */, &b0_v1_buf);
    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, b0_v3_index,
                        GL_TRANSFORM_FEEDBACK_BUFFER_INDEX, 1 /* buf_size */, &b0_v3_buf);
    program.GetResource(GL_TRANSFORM_FEEDBACK_VARYING, b1_v1_index,
                        GL_TRANSFORM_FEEDBACK_BUFFER_INDEX, 1 /* buf_size */, &b1_v1_buf);

    program.GetResource(GL_TRANSFORM_FEEDBACK_BUFFER, b0_v0_buf,
                        GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE, 1 /* buf_size */, &b0_stride);
    program.GetResource(GL_TRANSFORM_FEEDBACK_BUFFER, b1_v1_buf,
                        GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE, 1 /* buf_size */, &b1_stride);

    if (SEPARATED != test_case.m_case)
    {
        return (((GLint)(4 * type_size) == b0_stride) && ((GLint)(4 * type_size) == b1_stride) &&
                ((GLint)(0) == b0_v0_buf) && ((GLint)(0 * type_size) == b0_v0_offset) &&
                ((GLint)(type_enum) == b0_v0_type) && ((GLint)(0) == b0_v1_buf) &&
                ((GLint)(1 * type_size) == b0_v1_offset) && ((GLint)(type_enum) == b0_v1_type) &&
                ((GLint)(0) == b0_v3_buf) && ((GLint)(3 * type_size) == b0_v3_offset) &&
                ((GLint)(type_enum) == b0_v3_type) && ((GLint)(1) == b1_v1_buf) &&
                ((GLint)(1 * type_size) == b1_v1_offset) && ((GLint)(type_enum) == b1_v1_type));
    }
    else
    {
        return (((GLint)(1 * type_size) == b0_stride) && ((GLint)(1 * type_size) == b1_stride) &&
                ((GLint)(0) == b0_v0_buf) && ((GLint)(0) == b0_v0_offset) &&
                ((GLint)(type_enum) == b0_v0_type) && ((GLint)(1) == b0_v1_buf) &&
                ((GLint)(0) == b0_v1_offset) && ((GLint)(type_enum) == b0_v1_type) &&
                ((GLint)(2) == b0_v3_buf) && ((GLint)(0) == b0_v3_offset) &&
                ((GLint)(type_enum) == b0_v3_type) && ((GLint)(3) == b1_v1_buf) &&
                ((GLint)(0) == b1_v1_offset) && ((GLint)(type_enum) == b1_v1_type));
    }
}

/** Insert gl_SkipComponents
 *
 * @param num_components How many gl_SkipComponents1 need to be inserted
 * @param varyings The transform feedback varyings string vector
 *
 **/
void XFBGetProgramResourceAPITest::insertSkipComponents(int num_components,
                                                        Utils::Program::NameVector &varyings)
{
    int num_component_4 = num_components / 4;
    int num_component_1 = num_components % 4;
    for (int i = 0; i < num_component_4; i++)
    {
        varyings.push_back("gl_SkipComponents4");
    }
    switch (num_component_1)
    {
        case 1:
            varyings.push_back("gl_SkipComponents1");
            break;
        case 2:
            varyings.push_back("gl_SkipComponents2");
            break;
        case 3:
            varyings.push_back("gl_SkipComponents3");
            break;
        default:
            break;
    }
}

/** Runs test case
 *
 * @param test_case_index Id of test case
 *
 * @return true if test case pass, false otherwise
 **/
bool XFBGetProgramResourceAPITest::testCase(GLuint test_case_index)
{
    const std::string &gs_source = getShaderSource(test_case_index, Utils::Shader::GEOMETRY);
    Utils::Program program(m_context);
    const std::string &tcs_source = getShaderSource(test_case_index, Utils::Shader::TESS_CTRL);
    const std::string &tes_source = getShaderSource(test_case_index, Utils::Shader::TESS_EVAL);
    const test_Case &test_case    = m_test_cases[test_case_index];
    bool test_case_result         = true;
    const std::string &vs_source  = getShaderSource(test_case_index, Utils::Shader::VERTEX);

    // According to spec: gl_SkipComponents1 ~ gl_SkipComponents4 is treated as specifying a one- to
    // four-component floating point output variables with undefined values. No data will be
    // recorded for such strings, but the offset assigned to the next variable in varyings and the
    // stride of the assigned bingding point will be affected.

    if (INTERLEAVED == test_case.m_case)
    {
        /*
         layout (xfb_buffer = 0, xfb_offset = 1 * type_size) out type b0_v1;
         layout (xfb_buffer = 1, xfb_offset = 1 * type_size) out type b1_v1;
         layout (xfb_buffer = 0, xfb_offset = 3 * type_size) out type b0_v3;
         layout (xfb_buffer = 0, xfb_offset = 0 * type_size) out type b0_v0;

         Note: the type can be float, double, mat2, mat3x2, dmat2, dmat3x2..., so to make the each
         variable of "captured_varyings" has the same xfb_offset with the above shaders, we need to
         calculate how many "gl_SkipComponents" need to be inserted.
         */
        Utils::Program::NameVector captured_varyings;
        captured_varyings.push_back("b0_v0");
        captured_varyings.push_back("b0_v1");
        // Compute how many gl_SkipComponents to be inserted
        int numComponents = test_case.m_type.GetSize() / 4;
        insertSkipComponents(numComponents, captured_varyings);
        captured_varyings.push_back("b0_v3");
        captured_varyings.push_back("gl_NextBuffer");
        insertSkipComponents(numComponents, captured_varyings);
        captured_varyings.push_back("b1_v1");
        insertSkipComponents(numComponents * 2, captured_varyings);

        program.Init("" /* cs */, "", gs_source, tcs_source, tes_source, vs_source,
                     captured_varyings, true, true /* separable */);
    }
    else if (SEPARATED == test_case.m_case)
    {
        Utils::Program::NameVector captured_varyings;

        captured_varyings.push_back("b0_v0");
        captured_varyings.push_back("b0_v1");
        captured_varyings.push_back("b0_v3");
        captured_varyings.push_back("b1_v1");

        program.Init("" /* cs */, "", gs_source, tcs_source, tes_source, vs_source,
                     captured_varyings, false, true /* separable */);
    }
    else
    {

        program.Init("" /* cs */, "", gs_source, tcs_source, tes_source, vs_source,
                     true /* separable */);
    }

    test_case_result = inspectProgram(test_case_index, program);

    return test_case_result;
}

/** Prepare all test cases
 *
 **/
void XFBGetProgramResourceAPITest::testInit()
{
    const Functions &gl = m_context.getRenderContext().getFunctions();
    GLint max_xfb_int;
    GLint max_xfb_sep;

    gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &max_xfb_int);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS, &max_xfb_sep);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    GLint max_varyings;
    gl.getIntegerv(GL_MAX_VARYING_COMPONENTS, &max_varyings);

    const Utils::Type &type = getType(m_type);
    bool requirment         = 4 * type.GetNumComponents() + 4 > (GLuint)max_varyings;

    // the MAX_VARYING_COMPONENTS is 32 in our driver, but when the variable type is dmat4 or
    // dmat4x3, the number of output component is 33, to make the shader valid, we can either skip
    // the dmat4, dmat4x3 or query the implementation-dependent value MAX_VARYING_COMPONENTS before
    // generating the shader to guarantee the number of varying not exceeded.
    /*
     layout (xfb_buffer = 1, xfb_stride = 4 * type_size) out;
     layout (xfb_buffer = 0, xfb_offset = 1 * type_size) out type b0_v1;
     layout (xfb_buffer = 1, xfb_offset = 1 * type_size) out type b1_v1;
     layout (xfb_buffer = 0, xfb_offset = 3 * type_size) out type b0_v3;
     layout (xfb_buffer = 0, xfb_offset = 0 * type_size) out type b0_v0;
     in  vec4 in_vs;
     out vec4 vs_tcs;
     */
    for (GLuint stage = 0; stage < Utils::Shader::STAGE_MAX; ++stage)
    {
        if (requirment)
        {
            continue;
        }
        /*
         It is invalid to define transform feedback output in HS
         */
        if ((Utils::Shader::COMPUTE == stage) || (Utils::Shader::TESS_CTRL == stage) ||
            (Utils::Shader::FRAGMENT == stage))
        {
            continue;
        }
        test_Case test_case_int = {INTERLEAVED, (Utils::Shader::STAGES)stage, type};
        test_Case test_case_sep = {SEPARATED, (Utils::Shader::STAGES)stage, type};
        test_Case test_case_xfb = {XFB, (Utils::Shader::STAGES)stage, type};

        if ((int)type.GetSize() <= max_xfb_int)
        {
            m_test_cases.push_back(test_case_xfb);
            m_test_cases.push_back(test_case_int);
        }

        if ((int)type.GetSize() <= max_xfb_sep)
        {
            m_test_cases.push_back(test_case_sep);
        }
    }
}

/** Constructor
 *
 * @param context Test context
 **/
XFBOverrideQualifiersWithAPITest::XFBOverrideQualifiersWithAPITest(deqp::Context &context)
    : BufferTestBase(context,
                     "xfb_override_qualifiers_with_api",
                     "Test verifies that xfb_offset qualifier is not overriden with API")
{
    /* Nothing to be done here */
}

/** Get descriptors of buffers necessary for test
 *
 * @param test_case_index Index of test case
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBOverrideQualifiersWithAPITest::getBufferDescriptors(
    glw::GLuint test_case_index,
    bufferDescriptor::Vector &out_descriptors)
{
    const Utils::Type &type = getType(test_case_index);

    /* Test needs single uniform and xfb */
    out_descriptors.resize(2);

    /* Get references */
    bufferDescriptor &uniform = out_descriptors[0];
    bufferDescriptor &xfb     = out_descriptors[1];

    /* Index */
    uniform.m_index = 0;
    xfb.m_index     = 0;

    /* Target */
    uniform.m_target = Utils::Buffer::Uniform;
    xfb.m_target     = Utils::Buffer::Transform_feedback;

    /* Data */
    const GLuint gen_start                  = Utils::s_rand;
    const std::vector<GLubyte> &vegeta_data = type.GenerateData();
    const std::vector<GLubyte> &trunks_data = type.GenerateData();
    const std::vector<GLubyte> &goku_data   = type.GenerateData();

    Utils::s_rand                               = gen_start;
    const std::vector<GLubyte> &vegeta_data_pck = type.GenerateDataPacked();
    type.GenerateDataPacked();  // generate the data for trunks
    const std::vector<GLubyte> &goku_data_pck = type.GenerateDataPacked();

    const GLuint type_size        = static_cast<GLuint>(vegeta_data.size());
    const GLuint padded_type_size = type.GetBaseAlignment(false) * type.m_n_columns;
    const GLuint type_size_pck    = static_cast<GLuint>(vegeta_data_pck.size());

    /* Uniform data */
    uniform.m_initial_data.resize(3 * padded_type_size);
    memcpy(&uniform.m_initial_data[0] + 0, &vegeta_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + padded_type_size, &trunks_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 2 * padded_type_size, &goku_data[0], type_size);

    /* XFB data */
    xfb.m_initial_data.resize(3 * type_size_pck);
    xfb.m_expected_data.resize(3 * type_size_pck);

    for (GLuint i = 0; i < 3 * type_size_pck; ++i)
    {
        xfb.m_initial_data[i]  = (glw::GLubyte)i;
        xfb.m_expected_data[i] = (glw::GLubyte)i;
    }

    memcpy(&xfb.m_expected_data[0] + 0, &goku_data_pck[0], type_size_pck);
    memcpy(&xfb.m_expected_data[0] + 2 * type_size_pck, &vegeta_data_pck[0], type_size_pck);
}

/** Get list of names of varyings that will be registered with TransformFeedbackVaryings
 *
 * @param ignored
 * @param captured_varyings List of names
 **/
void XFBOverrideQualifiersWithAPITest::getCapturedVaryings(
    glw::GLuint test_case_index,
    Utils::Program::NameVector &captured_varyings,
    GLint *xfb_components)
{
    captured_varyings.resize(1);

    captured_varyings[0] = "trunks";

    /* The test captures 3 varyings of type 'type' */
    Utils::Type type = getType(test_case_index);
    GLint type_size  = type.GetSize(false);
    *xfb_components  = 3 * type_size / 4;
}

/** Get body of main function for given shader stage
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void XFBOverrideQualifiersWithAPITest::getShaderBody(GLuint test_case_index,
                                                     Utils::Shader::STAGES stage,
                                                     std::string &out_assignments,
                                                     std::string &out_calculations)
{
    out_calculations = "";

    static const GLchar *gs =
        "    vegeta = uni_vegeta;\n"
        "    trunks = uni_trunks;\n"
        "    goku   = uni_goku;\n";
    static const GLchar *fs =
        "    fs_out = vec4(0);\n"
        "    if (TYPE(1) == goku + trunks + vegeta)\n"
        "    {\n"
        "        fs_out = vec4(1);\n"
        "    }\n";

    const GLchar *assignments = "";
    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            assignments = fs;
            break;
        case Utils::Shader::GEOMETRY:
            assignments = gs;
            break;
        default:
            break;
    }

    out_assignments = assignments;

    if (Utils::Shader::FRAGMENT == stage)
    {
        const Utils::Type &type = getType(test_case_index);

        Utils::replaceAllTokens("TYPE", type.GetGLSLTypeName(), out_assignments);
    }
}

/** Get interface of shader
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_interface    Set to ""
 **/
void XFBOverrideQualifiersWithAPITest::getShaderInterface(GLuint test_case_index,
                                                          Utils::Shader::STAGES stage,
                                                          std::string &out_interface)
{
    static const GLchar *gs =
        "const uint sizeof_type = SIZE;\n"
        "\n"
        "layout (xfb_offset = 2 * sizeof_type) flat out TYPE vegeta;\n"
        "                                      flat out TYPE trunks;\n"
        "layout (xfb_offset = 0)               flat out TYPE goku;\n"
        "\n"
        /*
There is no packing qualifier for uniform block gs_block, according to spec, it should be "shared"
by default, the definition equals to "layout(binding=0, shared)", if the block is declared as
shared, each block member will not be packed, and each block member's layout in memory is
implementation dependent, so we can't use the API glBufferData() to update the UBO directly, we need
to query each block member's offset first, then upload the data to the corresponding offset,
otherwise we can't get the correct data from UBO; to make the test passed, we need to add the
qualifier std140,  and change the declaration as layout(binding=0, std140), which can make sure all
the block members are packed and the application can upload the data by glBufferData() directly.
*/
        "layout(binding = 0, std140) uniform gs_block {\n"
        "    TYPE uni_vegeta;\n"
        "    TYPE uni_trunks;\n"
        "    TYPE uni_goku;\n"
        "};\n";
    static const GLchar *fs =
        "flat in TYPE vegeta;\n"
        "flat in TYPE trunks;\n"
        "flat in TYPE goku;\n"
        "\n"
        "out vec4 fs_out;\n";

    const Utils::Type &type = getType(test_case_index);

    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            out_interface = fs;
            break;
        case Utils::Shader::GEOMETRY:
            out_interface = gs;
            break;
        default:
            out_interface = "";
            return;
    }

    if (Utils::Shader::GEOMETRY == stage)
    {
        GLchar buffer[16];
        size_t position        = 0;
        const GLuint type_size = type.GetSize();

        sprintf(buffer, "%d", type_size);

        Utils::replaceToken("SIZE", position, buffer, out_interface);
    }

    Utils::replaceAllTokens("TYPE", type.GetGLSLTypeName(), out_interface);
}

/** Get type name
 *
 * @param test_case_index Index of test case
 *
 * @return Name of type test in test_case_index
 **/
std::string XFBOverrideQualifiersWithAPITest::getTestCaseName(glw::GLuint test_case_index)
{
    return getTypeName(test_case_index);
}

/** Returns number of types to test
 *
 * @return Number of types, 34
 **/
glw::GLuint XFBOverrideQualifiersWithAPITest::getTestCaseNumber()
{
    return getTypesNumber();
}

/** Inspects program to check if all resources are as expected
 *
 * @param test_case_index Index of test case
 * @param program         Program instance
 * @param out_stream      Error message
 *
 * @return true if everything is ok, false otherwise
 **/
bool XFBOverrideQualifiersWithAPITest::inspectProgram(GLuint test_case_index,
                                                      Utils::Program &program,
                                                      std::stringstream &out_stream)
{
    GLint stride            = 0;
    const Utils::Type &type = getType(test_case_index);
    const GLuint type_size  = type.GetSize(false);

    program.GetResource(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */,
                        GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE, 1 /* buf_size */, &stride);

    if ((GLint)(3 * type_size) != stride)
    {
        out_stream << "Stride is: " << stride << " expected: " << (3 * type_size);

        return false;
    }

    return true;
}

/** Constructor
 *
 * @param context Test context
 **/
XFBVertexStreamsTest::XFBVertexStreamsTest(deqp::Context &context)
    : BufferTestBase(context,
                     "xfb_vertex_streams",
                     "Test verifies that xfb qualifier works with multiple output streams")
{
    /* Nothing to be done here */
}

/** Get descriptors of buffers necessary for test
 *
 * @param ignored
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBVertexStreamsTest::getBufferDescriptors(glw::GLuint /* test_case_index */,
                                                bufferDescriptor::Vector &out_descriptors)
{
    const Utils::Type &type = Utils::Type::vec4;

    /* Test needs single uniform and three xfbs */
    out_descriptors.resize(4);

    /* Get references */
    bufferDescriptor &uniform = out_descriptors[0];
    bufferDescriptor &xfb_1   = out_descriptors[1];
    bufferDescriptor &xfb_2   = out_descriptors[2];
    bufferDescriptor &xfb_3   = out_descriptors[3];

    /* Index */
    uniform.m_index = 0;
    xfb_1.m_index   = 1;
    xfb_2.m_index   = 2;
    xfb_3.m_index   = 3;

    /* Target */
    uniform.m_target = Utils::Buffer::Uniform;
    xfb_1.m_target   = Utils::Buffer::Transform_feedback;
    xfb_2.m_target   = Utils::Buffer::Transform_feedback;
    xfb_3.m_target   = Utils::Buffer::Transform_feedback;

    /* Data */
    const std::vector<GLubyte> &goku_data   = type.GenerateData();
    const std::vector<GLubyte> &gohan_data  = type.GenerateData();
    const std::vector<GLubyte> &goten_data  = type.GenerateData();
    const std::vector<GLubyte> &picolo_data = type.GenerateData();
    const std::vector<GLubyte> &vegeta_data = type.GenerateData();
    const std::vector<GLubyte> &bulma_data  = type.GenerateData();

    const GLuint type_size = static_cast<GLuint>(vegeta_data.size());

    /* Uniform data */
    uniform.m_initial_data.resize(6 * type_size);
    memcpy(&uniform.m_initial_data[0] + 0, &goku_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + type_size, &gohan_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 2 * type_size, &goten_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 3 * type_size, &picolo_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 4 * type_size, &vegeta_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 5 * type_size, &bulma_data[0], type_size);

    /* XFB data */
    static const GLuint xfb_stride = 64;
    xfb_1.m_initial_data.resize(xfb_stride);
    xfb_1.m_expected_data.resize(xfb_stride);
    xfb_2.m_initial_data.resize(xfb_stride);
    xfb_2.m_expected_data.resize(xfb_stride);
    xfb_3.m_initial_data.resize(xfb_stride);
    xfb_3.m_expected_data.resize(xfb_stride);

    for (GLuint i = 0; i < xfb_stride; ++i)
    {
        xfb_1.m_initial_data[i]  = (glw::GLubyte)i;
        xfb_1.m_expected_data[i] = (glw::GLubyte)i;
        xfb_2.m_initial_data[i]  = (glw::GLubyte)i;
        xfb_2.m_expected_data[i] = (glw::GLubyte)i;
        xfb_3.m_initial_data[i]  = (glw::GLubyte)i;
        xfb_3.m_expected_data[i] = (glw::GLubyte)i;
    }

    memcpy(&xfb_1.m_expected_data[0] + 48, &goku_data[0], type_size);
    memcpy(&xfb_1.m_expected_data[0] + 32, &gohan_data[0], type_size);
    memcpy(&xfb_1.m_expected_data[0] + 16, &goten_data[0], type_size);
    memcpy(&xfb_3.m_expected_data[0] + 48, &picolo_data[0], type_size);
    memcpy(&xfb_3.m_expected_data[0] + 32, &vegeta_data[0], type_size);
    memcpy(&xfb_2.m_expected_data[0] + 32, &bulma_data[0], type_size);
}

/** Get body of main function for given shader stage
 *
 * @param ignored
 * @param stage            Shader stage
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void XFBVertexStreamsTest::getShaderBody(GLuint /* test_case_index */,
                                         Utils::Shader::STAGES stage,
                                         std::string &out_assignments,
                                         std::string &out_calculations)
{
    out_calculations = "";

    // the shader declares the output variables with different "stream" qualifier, to make the data
    // can export to each stream, we must call the function EmitStreamVertex() and
    // EndStreamPrimitive() to make each vertex emitted by the GS is assigned to specific stream.
    static const GLchar *gs =
        "    goku   = uni_goku;\n"
        "    gohan  = uni_gohan;\n"
        "    goten  = uni_goten;\n"
        "    EmitStreamVertex(0);\n"
        "    EndStreamPrimitive(0);\n"
        "    picolo = uni_picolo;\n"
        "    vegeta = uni_vegeta;\n"
        "    EmitStreamVertex(1);\n"
        "    EndStreamPrimitive(1);\n"
        "    bulma  = uni_bulma;\n"
        "    EmitStreamVertex(2);\n"
        "    EndStreamPrimitive(2);\n";

    static const GLchar *fs = "    fs_out = gohan + goku + goten;\n";

    const GLchar *assignments = "";
    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            assignments = fs;
            break;
        case Utils::Shader::GEOMETRY:
            assignments = gs;
            break;
        default:
            break;
    }

    out_assignments = assignments;
}

/** Get interface of shader
 *
 * @param ignored
 * @param stage            Shader stage
 * @param out_interface    Set to ""
 **/
void XFBVertexStreamsTest::getShaderInterface(GLuint /* test_case_index */,
                                              Utils::Shader::STAGES stage,
                                              std::string &out_interface)
{
    static const GLchar *gs =
        "layout (xfb_buffer = 1, xfb_stride = 64) out;\n"
        "layout (xfb_buffer = 2, xfb_stride = 64) out;\n"
        "layout (xfb_buffer = 3, xfb_stride = 64) out;\n"
        "\n"
        "layout (stream = 0, xfb_buffer = 1, xfb_offset = 48) out vec4 goku;\n"
        "layout (stream = 0, xfb_buffer = 1, xfb_offset = 32) out vec4 gohan;\n"
        "layout (stream = 0, xfb_buffer = 1, xfb_offset = 16) out vec4 goten;\n"
        "layout (stream = 1, xfb_buffer = 3, xfb_offset = 48) out vec4 picolo;\n"
        "layout (stream = 1, xfb_buffer = 3, xfb_offset = 32) out vec4 vegeta;\n"
        "layout (stream = 2, xfb_buffer = 2, xfb_offset = 32) out vec4 bulma;\n"
        "\n"
        "layout(binding = 0) uniform gs_block {\n"
        "    vec4 uni_goku;\n"
        "    vec4 uni_gohan;\n"
        "    vec4 uni_goten;\n"
        "    vec4 uni_picolo;\n"
        "    vec4 uni_vegeta;\n"
        "    vec4 uni_bulma;\n"
        "};\n";
    /*
     Fixed incorrect usage of in/out qualifier, the following variable should be input symbols for
     fragment shader
     */
    static const GLchar *fs =
        "in vec4 goku;\n"
        "in vec4 gohan;\n"
        "in vec4 goten;\n"
        "\n"
        "out vec4 fs_out;\n";

    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            out_interface = fs;
            break;
        case Utils::Shader::GEOMETRY:
            out_interface = gs;
            break;
        default:
            out_interface = "";
            return;
    }
}

/** Constructor
 *
 * @param context Test framework context
 **/
XFBMultipleVertexStreamsTest::XFBMultipleVertexStreamsTest(deqp::Context &context)
    : NegativeTestBase(context,
                       "xfb_multiple_vertex_streams",
                       "Test verifies that compiler reports error when multiple streams are "
                       "captured with same xfb_buffer")
{}

/** Source for given test case and stage
 *
 * @param ignored
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBMultipleVertexStreamsTest::getShaderSource(GLuint /* test_case_index */,
                                                          Utils::Shader::STAGES stage)
{
    static const GLchar *var_definition =
        "const uint valid_stride = 64;\n"
        "\n"
        "layout (xfb_buffer = 1, xfb_stride = valid_stride) out;\n"
        "layout (xfb_buffer = 3, xfb_stride = valid_stride) out;\n"
        "\n"
        "\n"
#if DEBUG_NEG_REMOVE_ERROR
        "layout (stream = 0, xfb_buffer = 1, xfb_offset = 48) out vec4 goku;\n"
        "layout (stream = 1, xfb_buffer = 3, xfb_offset = 32) out vec4 gohan;\n"
        "layout (stream = 2, xfb_buffer = 2, xfb_offset = 16) out vec4 goten;\n";
#else
        "layout (stream = 0, xfb_buffer = 1, xfb_offset = 48) out vec4 goku;\n"
        "layout (stream = 1, xfb_buffer = 1, xfb_offset = 32) out vec4 gohan;\n"
        "layout (stream = 2, xfb_buffer = 1, xfb_offset = 16) out vec4 goten;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *var_use =
        "    goku  = result / 2;\n"
        "    gohan = result / 4;\n"
        "    goten = result / 6;\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 gs_fs;\n"
        "in  vec4 goku;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = gs_fs + goku;\n"
        "}\n"
        "\n";
    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_gs[];\n"
        "out vec4 gs_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_gs[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    gs_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_gs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_gs = in_vs;\n"
        "}\n"
        "\n";

    std::string source;

    if (Utils::Shader::GEOMETRY == stage)
    {
        size_t position = 0;

        source = gs;

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                source = fs;
                break;
            case Utils::Shader::VERTEX:
                source = vs;
                break;
            default:
                source = "";
        }
    }

    return source;
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool XFBMultipleVertexStreamsTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Constructor
 *
 * @param context Test framework context
 **/
XFBExceedBufferLimitTest::XFBExceedBufferLimitTest(deqp::Context &context,
                                                   GLuint constant,
                                                   GLuint stage)
    : NegativeTestBase(
          context,
          "xfb_exceed_buffer_limit",
          "Test verifies that compiler reports error when xfb_buffer qualifier exceeds limit"),
      m_constant(constant),
      m_stage(stage)
{
    std::string name = ("xfb_exceed_buffer_limit_");
    name.append(getCaseEnumName(m_constant));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBExceedBufferLimitTest::getShaderSource(GLuint test_case_index,
                                                      Utils::Shader::STAGES stage)
{
    static const GLchar *block_var_definition =
        "const uint buffer_index = MAX_BUFFER;\n"
        "\n"
#if DEBUG_NEG_REMOVE_ERROR
        "layout (xfb_buffer = 0, xfb_offset = 0) out Goku {\n"
#else
        "layout (xfb_buffer = buffer_index, xfb_offset = 0) out Goku {\n"
#endif /* DEBUG_NEG_REMOVE_ERROR */
        "    vec4 member;\n"
        "} goku;\n";
    static const GLchar *global_var_definition =
        "const uint buffer_index = MAX_BUFFER;\n"
        "\n"
#if DEBUG_NEG_REMOVE_ERROR
        "layout (xfb_buffer = 0) out;\n";
#else
        "layout (xfb_buffer = buffer_index) out;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *vector_var_definition =
        "const uint buffer_index = MAX_BUFFER;\n"
        "\n"
#if DEBUG_NEG_REMOVE_ERROR
        "layout (xfb_buffer = 0) out vec4 goku;\n";
#else
        "layout (xfb_buffer = buffer_index) out vec4 goku;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *block_use  = "    goku.member = result / 2;\n";
    static const GLchar *global_use = "";
    static const GLchar *vector_use = "    goku = result / 2;\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 any_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = any_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_any[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_any[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_any;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_any = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        GLchar buffer[16];
        const Functions &gl          = m_context.getRenderContext().getFunctions();
        GLint max_n_xfb              = 0;
        size_t position              = 0;
        const GLchar *var_definition = 0;
        const GLchar *var_use        = 0;

        gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, &max_n_xfb);
        GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

        sprintf(buffer, "%d", max_n_xfb);

        switch (test_case.m_case)
        {
            case BLOCK:
                var_definition = block_var_definition;
                var_use        = block_use;
                break;
            case GLOBAL:
                var_definition = global_var_definition;
                var_use        = global_use;
                break;
            case VECTOR:
                var_definition = vector_var_definition;
                var_use        = vector_use;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("MAX_BUFFER", position, buffer, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);
    }
    else
    {
        switch (test_case.m_stage)
        {
            case Utils::Shader::GEOMETRY:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::TESS_EVAL:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::TESS_CTRL:
                        source = tcs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::VERTEX:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    default:
                        source = "";
                }
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string XFBExceedBufferLimitTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage) << ", case: ";

    switch (test_case.m_case)
    {
        case BLOCK:
            stream << "BLOCK";
            break;
        case GLOBAL:
            stream << "GLOBAL";
            break;
        case VECTOR:
            stream << "VECTOR";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return stream.str();
}

std::string XFBExceedBufferLimitTest::getCaseEnumName(glw::GLuint test_case_index)
{
    std::string name = "case_max";

    switch (test_case_index)
    {
        case 0:
            name = "block";
            break;
        case 1:
            name = "global";
            break;
        case 2:
            name = "vector";
            break;
        default:
            name = "case_max";
            break;
    }

    return name;
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint XFBExceedBufferLimitTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool XFBExceedBufferLimitTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void XFBExceedBufferLimitTest::testInit()
{
    testCase test_case = {(CASES)m_constant, (Utils::Shader::STAGES)m_stage};

    m_test_cases.push_back(test_case);
}

/** Constructor
 *
 * @param context Test framework context
 **/
XFBExceedOffsetLimitTest::XFBExceedOffsetLimitTest(deqp::Context &context,
                                                   GLuint constant,
                                                   GLuint stage)
    : NegativeTestBase(
          context,
          "xfb_exceed_offset_limit",
          "Test verifies that compiler reports error when xfb_offset qualifier exceeds limit"),
      m_constant(constant),
      m_stage(stage)
{
    std::string name = ("xfb_exceed_offset_limit_");
    name.append(getCaseEnumName(m_constant));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBExceedOffsetLimitTest::getShaderSource(GLuint test_case_index,
                                                      Utils::Shader::STAGES stage)
{
    static const GLchar *block_var_definition =
        "const uint overflow_offset = MAX_SIZE + 16;\n"
        "\n"
#if DEBUG_NEG_REMOVE_ERROR
        "layout (xfb_buffer = 0, xfb_offset = 0) out Goku {\n"
#else
        "layout (xfb_buffer = 0, xfb_offset = overflow_offset + 16) out Goku "
        "{\n"
#endif /* DEBUG_NEG_REMOVE_ERROR */
        "    vec4 member;\n"
        "} goku;\n";
    static const GLchar *global_var_definition =
        "const uint overflow_offset = MAX_SIZE + 16;\n"
        "\n"
#if DEBUG_NEG_REMOVE_ERROR
        "layout (xfb_buffer = 0, xfb_stride = 0) out;\n";
#else
        "layout (xfb_buffer = 0, xfb_stride = overflow_offset) out;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *vector_var_definition =
        "const uint overflow_offset = MAX_SIZE + 16;\n"
        "\n"
#if DEBUG_NEG_REMOVE_ERROR
        "layout (xfb_buffer = 0, xfb_offset = 0) out vec4 goku;\n";
#else
        "layout (xfb_buffer = 0, xfb_offset = overflow_offset) out vec4 "
        "goku;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *block_use  = "    goku.member = result / 2;\n";
    static const GLchar *global_use = "";
    static const GLchar *vector_use = "    goku = result / 2;\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 any_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = any_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_any[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_any[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_any;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_any = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        GLchar buffer[16];
        const Functions &gl          = m_context.getRenderContext().getFunctions();
        GLint max_n_xfb_comp         = 0;
        GLint max_n_xfb_bytes        = 0;
        size_t position              = 0;
        const GLchar *var_definition = 0;
        const GLchar *var_use        = 0;

        gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &max_n_xfb_comp);
        GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

        max_n_xfb_bytes = max_n_xfb_comp * 4;

        sprintf(buffer, "%d", max_n_xfb_bytes);

        switch (test_case.m_case)
        {
            case BLOCK:
                var_definition = block_var_definition;
                var_use        = block_use;
                break;
            case GLOBAL:
                var_definition = global_var_definition;
                var_use        = global_use;
                break;
            case VECTOR:
                var_definition = vector_var_definition;
                var_use        = vector_use;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("MAX_SIZE", position, buffer, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);
    }
    else
    {
        switch (test_case.m_stage)
        {
            case Utils::Shader::GEOMETRY:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::TESS_EVAL:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::TESS_CTRL:
                        source = tcs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::VERTEX:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    default:
                        source = "";
                }
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string XFBExceedOffsetLimitTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage) << ", case: ";

    switch (test_case.m_case)
    {
        case BLOCK:
            stream << "BLOCK";
            break;
        case GLOBAL:
            stream << "GLOBAL";
            break;
        case VECTOR:
            stream << "VECTOR";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return stream.str();
}

std::string XFBExceedOffsetLimitTest::getCaseEnumName(glw::GLuint test_case_index)
{
    std::string name = "case_max";

    switch (test_case_index)
    {
        case 0:
            name = "block";
            break;
        case 1:
            name = "global";
            break;
        case 2:
            name = "vector";
            break;
        default:
            name = "case_max";
            break;
    }

    return name;
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint XFBExceedOffsetLimitTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool XFBExceedOffsetLimitTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void XFBExceedOffsetLimitTest::testInit()
{
    testCase test_case = {(CASES)m_constant, (Utils::Shader::STAGES)m_stage};

    m_test_cases.push_back(test_case);
}

/** Constructor
 *
 * @param context Test context
 **/
XFBGlobalBufferTest::XFBGlobalBufferTest(deqp::Context &context, GLuint type)
    : BufferTestBase(context,
                     "xfb_global_buffer",
                     "Test verifies that global xfb_buffer qualifier is respected"),
      m_type(type)
{
    std::string name = ("xfb_global_buffer_");
    name.append(getTypeName(m_type));

    BufferTestBase::m_name = name.c_str();
}

/** Get descriptors of buffers necessary for test
 *
 * @param test_case_index Index of test case
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBGlobalBufferTest::getBufferDescriptors(glw::GLuint test_case_index,
                                               bufferDescriptor::Vector &out_descriptors)
{
    // the function "getType(test_case_index)" can't return correct data type, so change code as
    // following:
    const Utils::Type &type = m_test_cases[test_case_index].m_type;

    /* Test needs single uniform and two xfbs */
    out_descriptors.resize(3);

    /* Get references */
    bufferDescriptor &uniform = out_descriptors[0];
    bufferDescriptor &xfb_1   = out_descriptors[1];
    bufferDescriptor &xfb_3   = out_descriptors[2];

    /* Index */
    uniform.m_index = 0;
    xfb_1.m_index   = 1;
    xfb_3.m_index   = 3;

    /* Target */
    uniform.m_target = Utils::Buffer::Uniform;
    xfb_1.m_target   = Utils::Buffer::Transform_feedback;
    xfb_3.m_target   = Utils::Buffer::Transform_feedback;

    /* Data */
    const GLuint gen_start                  = Utils::s_rand;
    const std::vector<GLubyte> &chichi_data = type.GenerateData();
    const std::vector<GLubyte> &bulma_data  = type.GenerateData();
    const std::vector<GLubyte> &trunks_data = type.GenerateData();
    const std::vector<GLubyte> &bra_data    = type.GenerateData();
    const std::vector<GLubyte> &gohan_data  = type.GenerateData();
    const std::vector<GLubyte> &goten_data  = type.GenerateData();

    Utils::s_rand                               = gen_start;
    const std::vector<GLubyte> &chichi_data_pck = type.GenerateDataPacked();
    const std::vector<GLubyte> &bulma_data_pck  = type.GenerateDataPacked();
    const std::vector<GLubyte> &trunks_data_pck = type.GenerateDataPacked();
    const std::vector<GLubyte> &bra_data_pck    = type.GenerateDataPacked();
    const std::vector<GLubyte> &gohan_data_pck  = type.GenerateDataPacked();
    const std::vector<GLubyte> &goten_data_pck  = type.GenerateDataPacked();

    const GLuint type_size        = static_cast<GLuint>(chichi_data.size());
    const GLuint padded_type_size = type.GetBaseAlignment(false) * type.m_n_columns;
    const GLuint type_size_pck    = static_cast<GLuint>(chichi_data_pck.size());

    /* Uniform data */
    uniform.m_initial_data.resize(6 * padded_type_size);
    memcpy(&uniform.m_initial_data[0] + 0, &chichi_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + padded_type_size, &bulma_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 2 * padded_type_size, &trunks_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 3 * padded_type_size, &bra_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 4 * padded_type_size, &gohan_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 5 * padded_type_size, &goten_data[0], type_size);

    /* XFB data */
    xfb_1.m_initial_data.resize(3 * type_size_pck);
    xfb_1.m_expected_data.resize(3 * type_size_pck);
    xfb_3.m_initial_data.resize(3 * type_size_pck);
    xfb_3.m_expected_data.resize(3 * type_size_pck);

    for (GLuint i = 0; i < 3 * type_size_pck; ++i)
    {
        xfb_1.m_initial_data[i]  = (glw::GLubyte)i;
        xfb_1.m_expected_data[i] = (glw::GLubyte)i;
        xfb_3.m_initial_data[i]  = (glw::GLubyte)i;
        xfb_3.m_expected_data[i] = (glw::GLubyte)i;
    }

    memcpy(&xfb_3.m_expected_data[0] + 2 * type_size_pck, &chichi_data_pck[0], type_size_pck);
    memcpy(&xfb_1.m_expected_data[0] + 0 * type_size_pck, &bulma_data_pck[0], type_size_pck);
    memcpy(&xfb_1.m_expected_data[0] + 1 * type_size_pck, &trunks_data_pck[0], type_size_pck);
    memcpy(&xfb_1.m_expected_data[0] + 2 * type_size_pck, &bra_data_pck[0], type_size_pck);
    memcpy(&xfb_3.m_expected_data[0] + 0 * type_size_pck, &gohan_data_pck[0], type_size_pck);
    memcpy(&xfb_3.m_expected_data[0] + 1 * type_size_pck, &goten_data_pck[0], type_size_pck);
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBGlobalBufferTest::getShaderSource(GLuint test_case_index,
                                                 Utils::Shader::STAGES stage)
{
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "flat in TYPE chichi;\n"
        "flat in TYPE bulma;\n"
        "in Vegeta {\n"
        "    flat TYPE trunk;\n"
        "    flat TYPE bra;\n"
        "} vegeta;\n"
        "in Goku {\n"
        "    flat TYPE gohan;\n"
        "    flat TYPE goten;\n"
        "} goku;\n"
        "\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = vec4(1);\n"
        "    if (TYPE(1) != chichi + bulma + vegeta.trunk + vegeta.bra + goku.gohan + goku.goten)\n"
        "    {\n"
        "        fs_out = vec4(0);\n"
        "    }\n"
        "}\n"
        "\n";

    static const GLchar *gs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                   in;\n"
        "layout(points, max_vertices = 1) out;\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "ASSIGNMENTS"
        "    EmitVertex();\n"
        "}\n"
        "\n";

    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";

    static const GLchar *tes =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "ASSIGNMENTS"
        "}\n"
        "\n";

    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "void main()\n"
        "{\n"
        "}\n"
        "\n";

    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "INTERFACE"
        "\n"
        "void main()\n"
        "{\n"
        "ASSIGNMENTS"
        "}\n"
        "\n";

    std::string source;
    const _testCase &test_case = m_test_cases[test_case_index];
    const GLchar *type_name    = test_case.m_type.GetGLSLTypeName();

    if (test_case.m_stage == stage)
    {
        std::string assignments =
            "    chichi       = uni_chichi;\n"
            "    bulma        = uni_bulma;\n"
            "    vegeta.trunk = uni_trunk;\n"
            "    vegeta.bra   = uni_bra;\n"
            "    goku.gohan   = uni_gohan;\n"
            "    goku.goten   = uni_goten;\n";

        std::string interface =
            "layout (xfb_buffer = 3) out;\n"
            "\n"
            "const uint type_size = SIZE;\n"
            "\n"
            "layout (                xfb_offset = 2 * type_size) flat out TYPE chichi;\n"
            "layout (xfb_buffer = 1, xfb_offset = 0)             flat out TYPE bulma;\n"
            "layout (xfb_buffer = 1, xfb_offset = 1 * type_size) out Vegeta {\n"
            "    flat TYPE trunk;\n"
            "    flat TYPE bra;\n"
            "} vegeta;\n"
            "layout (                xfb_offset = 0)             out Goku {\n"
            "    flat TYPE gohan;\n"
            "    flat TYPE goten;\n"
            "} goku;\n"
            "\n"
            // Uniform block must be declared with std140, otherwise each block member is not packed
            "layout(binding = 0, std140) uniform block {\n"
            "    TYPE uni_chichi;\n"
            "    TYPE uni_bulma;\n"
            "    TYPE uni_trunk;\n"
            "    TYPE uni_bra;\n"
            "    TYPE uni_gohan;\n"
            "    TYPE uni_goten;\n"
            "};\n";

        /* Prepare interface string */
        {
            GLchar buffer[16];
            size_t position        = 0;
            const GLuint type_size = test_case.m_type.GetSize();

            sprintf(buffer, "%d", type_size);

            Utils::replaceToken("SIZE", position, buffer, interface);
            Utils::replaceAllTokens("TYPE", type_name, interface);
        }

        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                source = gs;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        /* Replace tokens */
        {
            size_t position = 0;

            Utils::replaceToken("INTERFACE", position, interface.c_str(), source);
            Utils::replaceToken("ASSIGNMENTS", position, assignments.c_str(), source);
        }
    }
    else
    {
        switch (test_case.m_stage)
        {
            case Utils::Shader::GEOMETRY:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        Utils::replaceAllTokens("TYPE", type_name, source);
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::TESS_EVAL:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        Utils::replaceAllTokens("TYPE", type_name, source);
                        break;
                    case Utils::Shader::TESS_CTRL:
                        source = tcs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::VERTEX:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        Utils::replaceAllTokens("TYPE", type_name, source);
                        break;
                    default:
                        source = "";
                }
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get name of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Name of case
 **/
std::string XFBGlobalBufferTest::getTestCaseName(GLuint test_case_index)
{
    std::string name;
    const _testCase &test_case = m_test_cases[test_case_index];

    name = "Tested stage: ";
    name.append(Utils::Shader::GetStageName(test_case.m_stage));
    name.append(". Tested type: ");
    name.append(test_case.m_type.GetGLSLTypeName());

    return name;
}

/** Get number of cases
 *
 * @return Number of test cases
 **/
GLuint XFBGlobalBufferTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Prepare set of test cases
 *
 **/
void XFBGlobalBufferTest::testInit()
{
    const Utils::Type &type      = getType(m_type);
    const _testCase test_cases[] = {{Utils::Shader::VERTEX, type},
                                    {Utils::Shader::GEOMETRY, type},
                                    {Utils::Shader::TESS_EVAL, type}};

    m_test_cases.push_back(test_cases[0]);
    m_test_cases.push_back(test_cases[1]);
    m_test_cases.push_back(test_cases[2]);
}

/** Constructor
 *
 * @param context Test context
 **/
XFBStrideTest::XFBStrideTest(deqp::Context &context, glw::GLuint type, glw::GLuint stage)
    : BufferTestBase(context,
                     "xfb_stride",
                     "Test verifies that correct stride is used for all types"),
      m_type(type),
      m_stage(stage)
{
    std::string name = ("xfb_stride_");
    name.append(getTypeName(type));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    BufferTestBase::m_name = name.c_str();
}

/** Execute drawArrays for single vertex
 *
 * @param test_case_index
 *
 * @return true
 **/
bool XFBStrideTest::executeDrawCall(bool /* tesEnabled */, GLuint test_case_index)
{
    const Functions &gl       = m_context.getRenderContext().getFunctions();
    GLenum primitive_type     = GL_PATCHES;
    const testCase &test_case = m_test_cases[test_case_index];

    if (Utils::Shader::VERTEX == test_case.m_stage)
    {
        primitive_type = GL_POINTS;
    }

    gl.disable(GL_RASTERIZER_DISCARD);
    GLU_EXPECT_NO_ERROR(gl.getError(), "Disable");

    gl.beginTransformFeedback(GL_POINTS);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

    gl.drawArrays(primitive_type, 0 /* first */, 2 /* count */);
    GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

    gl.endTransformFeedback();
    GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

    return true;
}

/** Get descriptors of buffers necessary for test
 *
 * @param test_case_index Index of test case
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBStrideTest::getBufferDescriptors(GLuint test_case_index,
                                         bufferDescriptor::Vector &out_descriptors)
{
    const testCase &test_case = m_test_cases[test_case_index];
    const Utils::Type &type   = test_case.m_type;

    /* Test needs single uniform and xfb */
    out_descriptors.resize(2);

    /* Get references */
    bufferDescriptor &uniform = out_descriptors[0];
    bufferDescriptor &xfb     = out_descriptors[1];

    /* Index */
    uniform.m_index = 0;
    xfb.m_index     = 0;

    /* Target */
    uniform.m_target = Utils::Buffer::Uniform;
    xfb.m_target     = Utils::Buffer::Transform_feedback;

    /* Data */
    const GLuint rand_start                  = Utils::s_rand;
    const std::vector<GLubyte> &uniform_data = type.GenerateData();

    Utils::s_rand                        = rand_start;
    const std::vector<GLubyte> &xfb_data = type.GenerateDataPacked();

    const GLuint uni_type_size = static_cast<GLuint>(uniform_data.size());
    const GLuint xfb_type_size = static_cast<GLuint>(xfb_data.size());
    /*
     Note: If xfb varying output from vertex shader, the variable "goku" will only output once to
     transform feedback buffer, if xfb varying output from TES or GS, because the input primitive
     type in TES is defined as "layout(isolines, point_mode) in;", the primitive type is line which
     make the variable "goku" will output twice to transform feedback buffer, so for vertex shader
     only one valid data should be initialized in xfb.m_expected_data
     */
    const GLuint xfb_data_size =
        (test_case.m_stage == Utils::Shader::VERTEX) ? xfb_type_size : xfb_type_size * 2;
    /* Uniform data */
    uniform.m_initial_data.resize(uni_type_size);
    memcpy(&uniform.m_initial_data[0] + 0 * uni_type_size, &uniform_data[0], uni_type_size);

    /* XFB data */
    xfb.m_initial_data.resize(xfb_data_size);
    xfb.m_expected_data.resize(xfb_data_size);

    for (GLuint i = 0; i < xfb_data_size; ++i)
    {
        xfb.m_initial_data[i]  = (glw::GLubyte)i;
        xfb.m_expected_data[i] = (glw::GLubyte)i;
    }

    if (test_case.m_stage == Utils::Shader::VERTEX)
    {
        memcpy(&xfb.m_expected_data[0] + 0 * xfb_type_size, &xfb_data[0], xfb_type_size);
    }
    else
    {
        memcpy(&xfb.m_expected_data[0] + 0 * xfb_type_size, &xfb_data[0], xfb_type_size);
        memcpy(&xfb.m_expected_data[0] + 1 * xfb_type_size, &xfb_data[0], xfb_type_size);
    }
}

/** Get body of main function for given shader stage
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void XFBStrideTest::getShaderBody(GLuint test_case_index,
                                  Utils::Shader::STAGES stage,
                                  std::string &out_assignments,
                                  std::string &out_calculations)
{
    const testCase &test_case = m_test_cases[test_case_index];

    out_calculations = "";

    static const GLchar *vs_tes_gs = "    goku = uni_goku;\n";
    static const GLchar *fs =
        "    fs_out = vec4(1, 0.25, 0.5, 0.75);\n"
        "    if (TYPE(0) == goku)\n"
        "    {\n"
        "         fs_out = vec4(1, 0.75, 0.5, 0.5);\n"
        "    }\n";

    const GLchar *assignments = "";

    if (test_case.m_stage == stage)
    {
        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                assignments = vs_tes_gs;
                break;
            case Utils::Shader::TESS_EVAL:
                assignments = vs_tes_gs;
                break;
            case Utils::Shader::VERTEX:
                assignments = vs_tes_gs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                assignments = fs;
                break;
            case Utils::Shader::GEOMETRY:
            case Utils::Shader::TESS_CTRL:
            case Utils::Shader::TESS_EVAL:
            case Utils::Shader::VERTEX:
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    out_assignments = assignments;

    if (Utils::Shader::FRAGMENT == stage)
    {
        Utils::replaceAllTokens("TYPE", test_case.m_type.GetGLSLTypeName(), out_assignments);
    }
}

/** Get interface of shader
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_interface    Set to ""
 **/
void XFBStrideTest::getShaderInterface(GLuint test_case_index,
                                       Utils::Shader::STAGES stage,
                                       std::string &out_interface)
{
    static const GLchar *vs_tes_gs =
        "layout (xfb_offset = 0) FLAT out TYPE goku;\n"
        "\n"
        "layout(std140, binding = 0) uniform Goku {\n"
        "    TYPE uni_goku;\n"
        "};\n";
    static const GLchar *fs =
        "FLAT in TYPE goku;\n"
        "\n"
        "out vec4 fs_out;\n";

    const testCase &test_case = m_test_cases[test_case_index];
    const GLchar *interface   = "";
    const GLchar *flat        = "";

    if (test_case.m_stage == stage)
    {
        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                interface = vs_tes_gs;
                break;
            case Utils::Shader::TESS_EVAL:
                interface = vs_tes_gs;
                break;
            case Utils::Shader::VERTEX:
                interface = vs_tes_gs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                interface = fs;
                break;
            case Utils::Shader::GEOMETRY:
            case Utils::Shader::TESS_CTRL:
            case Utils::Shader::TESS_EVAL:
            case Utils::Shader::VERTEX:
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    out_interface = interface;

    if (Utils::Type::Float != test_case.m_type.m_basic_type)
    {
        flat = "flat";
    }

    Utils::replaceAllTokens("FLAT", flat, out_interface);
    Utils::replaceAllTokens("TYPE", test_case.m_type.GetGLSLTypeName(), out_interface);
}

/** Get source code of shader
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Source
 **/
std::string XFBStrideTest::getShaderSource(GLuint test_case_index, Utils::Shader::STAGES stage)
{
    std::string source;
    const testCase &test_case = m_test_cases[test_case_index];

    switch (test_case.m_stage)
    {
        case Utils::Shader::VERTEX:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case Utils::Shader::TESS_EVAL:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::TESS_CTRL:
                case Utils::Shader::TESS_EVAL:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case Utils::Shader::GEOMETRY:
            source = BufferTestBase::getShaderSource(test_case_index, stage);
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    /* */
    return source;
}

/** Get name of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Name of tested stage
 **/
std::string XFBStrideTest::getTestCaseName(glw::GLuint test_case_index)
{
    std::stringstream stream;
    const testCase &test_case = m_test_cases[test_case_index];

    stream << "Type: " << test_case.m_type.GetGLSLTypeName()
           << ", stage: " << Utils::Shader::GetStageName(test_case.m_stage);

    return stream.str();
}

/** Returns number of test cases
 *
 * @return TEST_MAX
 **/
glw::GLuint XFBStrideTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Prepare all test cases
 *
 **/
void XFBStrideTest::testInit()
{
    const Utils::Type &type = getType(m_type);

    testCase test_case = {(Utils::Shader::STAGES)m_stage, type};

    m_test_cases.push_back(test_case);
}

/** Constructor
 *
 * @param context Test framework context
 **/
XFBBlockMemberBufferTest::XFBBlockMemberBufferTest(deqp::Context &context, GLuint stage)
    : NegativeTestBase(context,
                       "xfb_block_member_buffer",
                       "Test verifies that compiler reports error when block member has different "
                       "xfb_buffer qualifier than buffer"),
      m_stage(stage)
{
    std::string name = ("xfb_block_member_buffer_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBBlockMemberBufferTest::getShaderSource(GLuint test_case_index,
                                                      Utils::Shader::STAGES stage)
{
    static const GLchar *var_definition =
        "layout (xfb_offset = 0) out Goku {\n"
        "                            vec4 gohan;\n"
#if DEBUG_NEG_REMOVE_ERROR
        "    /* layout (xfb_buffer = 1) */ vec4 goten;\n"
#else
        "    layout (xfb_buffer = 1) vec4 goten;\n"
#endif /* DEBUG_NEG_REMOVE_ERROR */
        "} goku;\n";
    static const GLchar *var_use =
        "    goku.gohan = result / 2;\n"
        "    goku.goten = result / 4;\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 any_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = any_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_any[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_any[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_any;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_any = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        size_t position = 0;

        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);
    }
    else
    {
        switch (test_case.m_stage)
        {
            case Utils::Shader::GEOMETRY:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::TESS_EVAL:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::TESS_CTRL:
                        source = tcs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::VERTEX:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    default:
                        source = "";
                }
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string XFBBlockMemberBufferTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage);

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint XFBBlockMemberBufferTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool XFBBlockMemberBufferTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void XFBBlockMemberBufferTest::testInit()
{
    testCase test_case = {(Utils::Shader::STAGES)m_stage};

    m_test_cases.push_back(test_case);
}

/** Constructor
 *
 * @param context Test framework context
 **/
XFBOutputOverlappingTest::XFBOutputOverlappingTest(deqp::Context &context,
                                                   GLuint type,
                                                   GLuint stage)
    : NegativeTestBase(
          context,
          "xfb_output_overlapping",
          "Test verifies that compiler reports error when two xfb qualified outputs overlap"),
      m_type(type),
      m_stage(stage)
{
    std::string name = ("xfb_output_overlapping_");
    name.append(getTypeName(type));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBOutputOverlappingTest::getShaderSource(GLuint test_case_index,
                                                      Utils::Shader::STAGES stage)
{
    static const GLchar *var_definition =
        "layout (xfb_offset = 0) out TYPE gohan;\n"
#if DEBUG_NEG_REMOVE_ERROR
        "/* layout (xfb_offset = OFFSET) */ out TYPE goten;\n";
#else
        "layout (xfb_offset = OFFSET) out TYPE goten;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *var_use =
        "    gohan = TYPE(0);\n"
        "    goten = TYPE(1);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gohan = TYPE(1);\n"
        "        goten = TYPE(0);\n"
        "    }\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 any_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = any_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_any[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_any[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_any;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_any = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        GLchar offset[16];
        size_t position         = 0;
        const GLchar *type_name = test_case.m_type.GetGLSLTypeName();

        sprintf(offset, "%d", test_case.m_offset);

        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("OFFSET", position, offset, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("TYPE", type_name, source);
    }
    else
    {
        switch (test_case.m_stage)
        {
            case Utils::Shader::GEOMETRY:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::TESS_EVAL:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::TESS_CTRL:
                        source = tcs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::VERTEX:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    default:
                        source = "";
                }
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string XFBOutputOverlappingTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage)
           << ", type: " << test_case.m_type.GetGLSLTypeName()
           << ", offset: " << test_case.m_offset;

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint XFBOutputOverlappingTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool XFBOutputOverlappingTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void XFBOutputOverlappingTest::testInit()
{
    const Utils::Type &type      = getType(m_type);
    const GLuint basic_type_size = Utils::Type::GetTypeSize(type.m_basic_type);

    /* Skip scalars, not applicable as:
     *
     *     The offset must be a multiple of the size of the first component of the first
     *     qualified variable or block member, or a compile-time error results.
     */

    testCase test_case = {basic_type_size /* offset */, (Utils::Shader::STAGES)m_stage, type};

    m_test_cases.push_back(test_case);
}

/** Constructor
 *
 * @param context Test framework context
 **/
XFBInvalidOffsetAlignmentTest::XFBInvalidOffsetAlignmentTest(deqp::Context &context,
                                                             GLuint type,
                                                             GLuint stage)
    : NegativeTestBase(
          context,
          "xfb_invalid_offset_alignment",
          "Test verifies that compiler reports error when xfb_offset has invalid alignment"),
      m_type(type),
      m_stage(stage)
{
    std::string name = ("xfb_invalid_offset_alignment_");
    name.append(getTypeName(type));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBInvalidOffsetAlignmentTest::getShaderSource(GLuint test_case_index,
                                                           Utils::Shader::STAGES stage)
{
#if DEBUG_NEG_REMOVE_ERROR
    static const GLchar *var_definition = "/* layout (xfb_offset = OFFSET) */ out TYPE gohan;\n";
#else
    static const GLchar *var_definition = "layout (xfb_offset = OFFSET) out TYPE gohan;\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *var_use =
        "    gohan = TYPE(0);\n"
        "    if (vec4(0) == result)\n"
        "    {\n"
        "        gohan = TYPE(1);\n"
        "    }\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 any_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = any_fs;\n"
        "}\n"
        "\n";
    static const GLchar *gs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(points)                           in;\n"
        "layout(triangle_strip, max_vertices = 4) out;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = vs_any[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(-1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, -1, 0, 1);\n"
        "    EmitVertex();\n"
        "    any_fs = result;\n"
        "    gl_Position  = vec4(1, 1, 0, 1);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n";
    static const GLchar *tcs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(vertices = 1) out;\n"
        "\n"
        "in  vec4 vs_any[];\n"
        "out vec4 tcs_tes[];\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    tcs_tes[gl_InvocationID] = vs_any[gl_InvocationID];\n"
        "\n"
        "    gl_TessLevelOuter[0] = 1.0;\n"
        "    gl_TessLevelOuter[1] = 1.0;\n"
        "    gl_TessLevelOuter[2] = 1.0;\n"
        "    gl_TessLevelOuter[3] = 1.0;\n"
        "    gl_TessLevelInner[0] = 1.0;\n"
        "    gl_TessLevelInner[1] = 1.0;\n"
        "}\n"
        "\n";
    static const GLchar *tes_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "layout(isolines, point_mode) in;\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 tcs_tes[];\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = tcs_tes[0];\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs += result;\n"
        "}\n"
        "\n";
    static const GLchar *vs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 vs_any;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_any = in_vs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        GLchar offset[16];
        size_t position         = 0;
        const GLchar *type_name = test_case.m_type.GetGLSLTypeName();

        sprintf(offset, "%d", test_case.m_offset);

        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                source = gs_tested;
                break;
            case Utils::Shader::TESS_EVAL:
                source = tes_tested;
                break;
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("OFFSET", position, offset, source);
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);

        Utils::replaceAllTokens("TYPE", type_name, source);
    }
    else
    {
        switch (test_case.m_stage)
        {
            case Utils::Shader::GEOMETRY:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::TESS_EVAL:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    case Utils::Shader::TESS_CTRL:
                        source = tcs;
                        break;
                    case Utils::Shader::VERTEX:
                        source = vs;
                        break;
                    default:
                        source = "";
                }
                break;
            case Utils::Shader::VERTEX:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    default:
                        source = "";
                }
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string XFBInvalidOffsetAlignmentTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage)
           << ", type: " << test_case.m_type.GetGLSLTypeName()
           << ", offset: " << test_case.m_offset;

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint XFBInvalidOffsetAlignmentTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool XFBInvalidOffsetAlignmentTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void XFBInvalidOffsetAlignmentTest::testInit()
{
    const Utils::Type &type      = getType(m_type);
    const GLuint basic_type_size = Utils::Type::GetTypeSize(type.m_basic_type);

    for (GLuint offset = basic_type_size + 1; offset < 2 * basic_type_size; ++offset)
    {
        testCase test_case = {offset, (Utils::Shader::STAGES)m_stage, type};

        m_test_cases.push_back(test_case);
    }
}

/** Constructor
 *
 * @param context Test context
 **/
XFBCaptureInactiveOutputVariableTest::XFBCaptureInactiveOutputVariableTest(deqp::Context &context)
    : BufferTestBase(context,
                     "xfb_capture_inactive_output_variable",
                     "Test verifies that inactive variables are captured")
{
    /* Nothing to be done here */
}

/** Execute drawArrays for single vertex
 *
 * @param test_case_index
 *
 * @return true
 **/
bool XFBCaptureInactiveOutputVariableTest::executeDrawCall(bool /* tesEnabled */,
                                                           GLuint test_case_index)
{
    const Functions &gl   = m_context.getRenderContext().getFunctions();
    GLenum primitive_type = GL_PATCHES;

    if (TEST_VS == test_case_index)
    {
        primitive_type = GL_POINTS;
    }

    gl.disable(GL_RASTERIZER_DISCARD);
    GLU_EXPECT_NO_ERROR(gl.getError(), "Disable");

    gl.beginTransformFeedback(GL_POINTS);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

    gl.drawArrays(primitive_type, 0 /* first */, 1 /* count */);
    GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

    gl.endTransformFeedback();
    GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

    return true;
}

/** Get descriptors of buffers necessary for test
 *
 * @param ignored
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBCaptureInactiveOutputVariableTest::getBufferDescriptors(
    glw::GLuint /* test_case_index */,
    bufferDescriptor::Vector &out_descriptors)
{
    const Utils::Type &type = Utils::Type::vec4;

    /* Test needs single uniform and xfb */
    out_descriptors.resize(2);

    /* Get references */
    bufferDescriptor &uniform = out_descriptors[0];
    bufferDescriptor &xfb     = out_descriptors[1];

    /* Index */
    uniform.m_index = 0;
    xfb.m_index     = 0;

    /* Target */
    uniform.m_target = Utils::Buffer::Uniform;
    xfb.m_target     = Utils::Buffer::Transform_feedback;

    /* Data */
    const std::vector<GLubyte> &gohan_data = type.GenerateData();
    const std::vector<GLubyte> &goten_data = type.GenerateData();

    const GLuint type_size = static_cast<GLuint>(gohan_data.size());

    /* Uniform data */
    uniform.m_initial_data.resize(2 * type_size);
    memcpy(&uniform.m_initial_data[0] + 0, &gohan_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + type_size, &goten_data[0], type_size);

    /* XFB data */
    xfb.m_initial_data.resize(3 * type_size);
    xfb.m_expected_data.resize(3 * type_size);

    for (GLuint i = 0; i < 3 * type_size; ++i)
    {
        xfb.m_initial_data[i]  = (glw::GLubyte)i;
        xfb.m_expected_data[i] = (glw::GLubyte)i;
    }

    memcpy(&xfb.m_expected_data[0] + 2 * type_size, &gohan_data[0], type_size);
    memcpy(&xfb.m_expected_data[0] + 0 * type_size, &goten_data[0], type_size);
}

/** Get body of main function for given shader stage
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void XFBCaptureInactiveOutputVariableTest::getShaderBody(GLuint test_case_index,
                                                         Utils::Shader::STAGES stage,
                                                         std::string &out_assignments,
                                                         std::string &out_calculations)
{
    out_calculations = "";

    static const GLchar *vs_tes_gs =
        "    goten = uni_goten;\n"
        "    gohan = uni_gohan;\n";
    static const GLchar *fs = "    fs_out = goku + gohan + goten;\n";

    const GLchar *assignments = "";

    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            assignments = fs;
            break;

        case Utils::Shader::GEOMETRY:
            if (TEST_GS == test_case_index)
            {
                assignments = vs_tes_gs;
            }
            break;

        case Utils::Shader::TESS_CTRL:
            break;

        case Utils::Shader::TESS_EVAL:
            if (TEST_TES == test_case_index)
            {
                assignments = vs_tes_gs;
            }
            break;

        case Utils::Shader::VERTEX:
            if (TEST_VS == test_case_index)
            {
                assignments = vs_tes_gs;
            }
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    out_assignments = assignments;
}

/** Get interface of shader
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_interface    Set to ""
 **/
void XFBCaptureInactiveOutputVariableTest::getShaderInterface(GLuint test_case_index,
                                                              Utils::Shader::STAGES stage,
                                                              std::string &out_interface)
{
    static const GLchar *vs_tes_gs =
        "const uint sizeof_type = 16;\n"
        "\n"
        "layout (xfb_offset = 1 * sizeof_type) out vec4 goku;\n"
        "layout (xfb_offset = 2 * sizeof_type) out vec4 gohan;\n"
        "layout (xfb_offset = 0 * sizeof_type) out vec4 goten;\n"
        "\n"
        "layout(binding = 0) uniform block {\n"
        "    vec4 uni_gohan;\n"
        "    vec4 uni_goten;\n"
        "};\n";
    static const GLchar *fs =
        "in vec4 goku;\n"
        "in vec4 gohan;\n"
        "in vec4 goten;\n"
        "out vec4 fs_out;\n";

    const GLchar *interface = "";

    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            interface = fs;
            break;

        case Utils::Shader::GEOMETRY:
            if (TEST_GS == test_case_index)
            {
                interface = vs_tes_gs;
            }
            break;

        case Utils::Shader::TESS_CTRL:
            break;

        case Utils::Shader::TESS_EVAL:
            if (TEST_TES == test_case_index)
            {
                interface = vs_tes_gs;
            }
            break;

        case Utils::Shader::VERTEX:
            if (TEST_VS == test_case_index)
            {
                interface = vs_tes_gs;
            }
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    out_interface = interface;
}

/** Get source code of shader
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Source
 **/
std::string XFBCaptureInactiveOutputVariableTest::getShaderSource(GLuint test_case_index,
                                                                  Utils::Shader::STAGES stage)
{
    std::string source;

    switch (test_case_index)
    {
        case TEST_VS:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case TEST_TES:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::TESS_CTRL:
                case Utils::Shader::TESS_EVAL:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case TEST_GS:
            source = BufferTestBase::getShaderSource(test_case_index, stage);
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    /* */
    return source;
}

/** Get name of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Name of tested stage
 **/
std::string XFBCaptureInactiveOutputVariableTest::getTestCaseName(glw::GLuint test_case_index)
{
    const GLchar *name = 0;

    switch (test_case_index)
    {
        case TEST_VS:
            name = "vertex";
            break;
        case TEST_TES:
            name = "tessellation evaluation";
            break;
        case TEST_GS:
            name = "geometry";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return name;
}

/** Returns number of test cases
 *
 * @return TEST_MAX
 **/
glw::GLuint XFBCaptureInactiveOutputVariableTest::getTestCaseNumber()
{
    return TEST_MAX;
}

/** Inspects program to check if all resources are as expected
 *
 * @param ignored
 * @param program         Program instance
 * @param out_stream      Error message
 *
 * @return true if everything is ok, false otherwise
 **/
bool XFBCaptureInactiveOutputVariableTest::inspectProgram(GLuint /* test_case_index */,
                                                          Utils::Program &program,
                                                          std::stringstream &out_stream)
{
    GLint stride            = 0;
    const Utils::Type &type = Utils::Type::vec4;
    const GLuint type_size  = type.GetSize();

    program.GetResource(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */,
                        GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE, 1 /* buf_size */, &stride);

    if ((GLint)(3 * type_size) != stride)
    {
        out_stream << "Stride is: " << stride << " expected: " << (3 * type_size);

        return false;
    }

    return true;
}

/** Verify contents of buffers
 *
 * @param buffers Collection of buffers to be verified
 *
 * @return true if everything is as expected, false otherwise
 **/
bool XFBCaptureInactiveOutputVariableTest::verifyBuffers(bufferCollection &buffers)
{
    bool result = true;

    bufferCollection::pair &pair = buffers.m_vector[1] /* xfb */;
    Utils::Buffer *buffer        = pair.m_buffer;
    bufferDescriptor *descriptor = pair.m_descriptor;

    /* Get pointer to contents of buffer */
    buffer->Bind();
    GLubyte *buffer_data = (GLubyte *)buffer->Map(Utils::Buffer::ReadOnly);

    /* Get pointer to expected data */
    GLubyte *expected_data = (GLubyte *)&descriptor->m_expected_data[0];

    /* Compare */
    static const GLuint vec4_size = 16;

    int res_gohan = memcmp(buffer_data + 2 * vec4_size, expected_data + 2 * vec4_size, vec4_size);
    int res_goten = memcmp(buffer_data + 0 * vec4_size, expected_data + 0 * vec4_size, vec4_size);

    if ((0 != res_gohan) || (0 != res_goten))
    {
        m_context.getTestContext().getLog()
            << tcu::TestLog::Message
            << "Invalid result. Buffer: " << Utils::Buffer::GetBufferName(descriptor->m_target)
            << ". Index: " << descriptor->m_index << tcu::TestLog::EndMessage;

        result = false;
    }

    /* Release buffer mapping */
    buffer->UnMap();

    return result;
}

/** Constructor
 *
 * @param context Test context
 **/
XFBCaptureInactiveOutputComponentTest::XFBCaptureInactiveOutputComponentTest(deqp::Context &context)
    : BufferTestBase(context,
                     "xfb_capture_inactive_output_component",
                     "Test verifies that inactive components are not modified")
{
    /* Nothing to be done here */
}

/** Execute drawArrays for single vertex
 *
 * @param test_case_index
 *
 * @return true
 **/
bool XFBCaptureInactiveOutputComponentTest::executeDrawCall(bool /* tesEnabled */,
                                                            GLuint test_case_index)
{
    const Functions &gl   = m_context.getRenderContext().getFunctions();
    GLenum primitive_type = GL_PATCHES;

    if (TEST_VS == test_case_index)
    {
        primitive_type = GL_POINTS;
    }

    gl.disable(GL_RASTERIZER_DISCARD);
    GLU_EXPECT_NO_ERROR(gl.getError(), "Disable");

    gl.beginTransformFeedback(GL_POINTS);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

    gl.drawArrays(primitive_type, 0 /* first */, 1 /* count */);
    GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

    gl.endTransformFeedback();
    GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

    return true;
}

/** Get descriptors of buffers necessary for test
 *
 * @param ignored
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBCaptureInactiveOutputComponentTest::getBufferDescriptors(
    glw::GLuint /* test_case_index */,
    bufferDescriptor::Vector &out_descriptors)
{
    const Utils::Type &type = Utils::Type::vec4;

    /* Test needs single uniform and xfb */
    out_descriptors.resize(2);

    /* Get references */
    bufferDescriptor &uniform = out_descriptors[0];
    bufferDescriptor &xfb     = out_descriptors[1];

    /* Index */
    uniform.m_index = 0;
    xfb.m_index     = 0;

    /* Target */
    uniform.m_target = Utils::Buffer::Uniform;
    xfb.m_target     = Utils::Buffer::Transform_feedback;

    /* Data */
    const std::vector<GLubyte> &goku_data   = type.GenerateData();
    const std::vector<GLubyte> &gohan_data  = type.GenerateData();
    const std::vector<GLubyte> &goten_data  = type.GenerateData();
    const std::vector<GLubyte> &chichi_data = type.GenerateData();
    const std::vector<GLubyte> &vegeta_data = type.GenerateData();
    const std::vector<GLubyte> &trunks_data = type.GenerateData();
    const std::vector<GLubyte> &bra_data    = type.GenerateData();
    const std::vector<GLubyte> &bulma_data  = type.GenerateData();

    const GLuint comp_size = Utils::Type::GetTypeSize(type.m_basic_type);
    const GLuint type_size = static_cast<GLuint>(gohan_data.size());

    /* Uniform data */
    uniform.m_initial_data.resize(8 * type_size);
    memcpy(&uniform.m_initial_data[0] + 0 * type_size, &goku_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 1 * type_size, &gohan_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 2 * type_size, &goten_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 3 * type_size, &chichi_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 4 * type_size, &vegeta_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 5 * type_size, &trunks_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 6 * type_size, &bra_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + 7 * type_size, &bulma_data[0], type_size);

    /* XFB data */
    xfb.m_initial_data.resize(8 * type_size);
    xfb.m_expected_data.resize(8 * type_size);

    for (GLuint i = 0; i < 8 * type_size; ++i)
    {
        xfb.m_initial_data[i]  = (glw::GLubyte)i;
        xfb.m_expected_data[i] = (glw::GLubyte)i;
    }

    /* goku - x, z - 32 */
    memcpy(&xfb.m_expected_data[0] + 2 * type_size + 0 * comp_size, &goku_data[0] + 0 * comp_size,
           comp_size);
    memcpy(&xfb.m_expected_data[0] + 2 * type_size + 2 * comp_size, &goku_data[0] + 2 * comp_size,
           comp_size);

    /* gohan - y, w - 0 */
    memcpy(&xfb.m_expected_data[0] + 0 * type_size + 1 * comp_size, &gohan_data[0] + 1 * comp_size,
           comp_size);
    memcpy(&xfb.m_expected_data[0] + 0 * type_size + 3 * comp_size, &gohan_data[0] + 3 * comp_size,
           comp_size);

    /* goten - x, y - 16 */
    memcpy(&xfb.m_expected_data[0] + 1 * type_size + 0 * comp_size, &goten_data[0] + 0 * comp_size,
           comp_size);
    memcpy(&xfb.m_expected_data[0] + 1 * type_size + 1 * comp_size, &goten_data[0] + 1 * comp_size,
           comp_size);

    /* chichi - z, w - 48 */
    memcpy(&xfb.m_expected_data[0] + 3 * type_size + 2 * comp_size, &chichi_data[0] + 2 * comp_size,
           comp_size);
    memcpy(&xfb.m_expected_data[0] + 3 * type_size + 3 * comp_size, &chichi_data[0] + 3 * comp_size,
           comp_size);

    /* vegeta - x - 112 */
    memcpy(&xfb.m_expected_data[0] + 7 * type_size + 0 * comp_size, &vegeta_data[0] + 0 * comp_size,
           comp_size);

    /* trunks - y - 96 */
    memcpy(&xfb.m_expected_data[0] + 6 * type_size + 1 * comp_size, &trunks_data[0] + 1 * comp_size,
           comp_size);

    /* bra - z - 80 */
    memcpy(&xfb.m_expected_data[0] + 5 * type_size + 2 * comp_size, &bra_data[0] + 2 * comp_size,
           comp_size);

    /* bulma - w - 64 */
    memcpy(&xfb.m_expected_data[0] + 4 * type_size + 3 * comp_size, &bulma_data[0] + 3 * comp_size,
           comp_size);
}

/** Get body of main function for given shader stage
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void XFBCaptureInactiveOutputComponentTest::getShaderBody(GLuint test_case_index,
                                                          Utils::Shader::STAGES stage,
                                                          std::string &out_assignments,
                                                          std::string &out_calculations)
{
    out_calculations = "";

    static const GLchar *vs_tes_gs =
        "    goku.x    = uni_goku.x   ;\n"
        "    goku.z    = uni_goku.z   ;\n"
        "    gohan.y   = uni_gohan.y  ;\n"
        "    gohan.w   = uni_gohan.w  ;\n"
        "    goten.x   = uni_goten.x  ;\n"
        "    goten.y   = uni_goten.y  ;\n"
        "    chichi.z  = uni_chichi.z ;\n"
        "    chichi.w  = uni_chichi.w ;\n"
        "    vegeta.x  = uni_vegeta.x ;\n"
        "    trunks.y  = uni_trunks.y ;\n"
        "    bra.z     = uni_bra.z    ;\n"
        "    bulma.w   = uni_bulma.w  ;\n";
    static const GLchar *fs =
        "    fs_out = goku + gohan + goten + chichi + vegeta + trunks + bra + bulma;\n";

    const GLchar *assignments = "";

    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            assignments = fs;
            break;

        case Utils::Shader::GEOMETRY:
            if (TEST_GS == test_case_index)
            {
                assignments = vs_tes_gs;
            }
            break;

        case Utils::Shader::TESS_CTRL:
            break;

        case Utils::Shader::TESS_EVAL:
            if (TEST_TES == test_case_index)
            {
                assignments = vs_tes_gs;
            }
            break;

        case Utils::Shader::VERTEX:
            if (TEST_VS == test_case_index)
            {
                assignments = vs_tes_gs;
            }
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    out_assignments = assignments;
}

/** Get interface of shader
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_interface    Set to ""
 **/
void XFBCaptureInactiveOutputComponentTest::getShaderInterface(GLuint test_case_index,
                                                               Utils::Shader::STAGES stage,
                                                               std::string &out_interface)
{
    static const GLchar *vs_tes_gs =
        "const uint sizeof_type = 16;\n"
        "\n"
        "layout (xfb_offset = 2 * sizeof_type) out vec4 goku;\n"
        "layout (xfb_offset = 0 * sizeof_type) out vec4 gohan;\n"
        "layout (xfb_offset = 1 * sizeof_type) out vec4 goten;\n"
        "layout (xfb_offset = 3 * sizeof_type) out vec4 chichi;\n"
        "layout (xfb_offset = 7 * sizeof_type) out vec4 vegeta;\n"
        "layout (xfb_offset = 6 * sizeof_type) out vec4 trunks;\n"
        "layout (xfb_offset = 5 * sizeof_type) out vec4 bra;\n"
        "layout (xfb_offset = 4 * sizeof_type) out vec4 bulma;\n"
        "\n"
        "layout(binding = 0) uniform block {\n"
        "    vec4 uni_goku;\n"
        "    vec4 uni_gohan;\n"
        "    vec4 uni_goten;\n"
        "    vec4 uni_chichi;\n"
        "    vec4 uni_vegeta;\n"
        "    vec4 uni_trunks;\n"
        "    vec4 uni_bra;\n"
        "    vec4 uni_bulma;\n"
        "};\n";
    static const GLchar *fs =
        "in vec4 vegeta;\n"
        "in vec4 trunks;\n"
        "in vec4 bra;\n"
        "in vec4 bulma;\n"
        "in vec4 goku;\n"
        "in vec4 gohan;\n"
        "in vec4 goten;\n"
        "in vec4 chichi;\n"
        "\n"
        "out vec4 fs_out;\n";

    const GLchar *interface = "";

    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            interface = fs;
            break;

        case Utils::Shader::GEOMETRY:
            if (TEST_GS == test_case_index)
            {
                interface = vs_tes_gs;
            }
            break;

        case Utils::Shader::TESS_CTRL:
            break;

        case Utils::Shader::TESS_EVAL:
            if (TEST_TES == test_case_index)
            {
                interface = vs_tes_gs;
            }
            break;

        case Utils::Shader::VERTEX:
            if (TEST_VS == test_case_index)
            {
                interface = vs_tes_gs;
            }
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    out_interface = interface;
}

/** Get source code of shader
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Source
 **/
std::string XFBCaptureInactiveOutputComponentTest::getShaderSource(GLuint test_case_index,
                                                                   Utils::Shader::STAGES stage)
{
    std::string source;

    switch (test_case_index)
    {
        case TEST_VS:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case TEST_TES:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::TESS_CTRL:
                case Utils::Shader::TESS_EVAL:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case TEST_GS:
            source = BufferTestBase::getShaderSource(test_case_index, stage);
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    /* */
    return source;
}

/** Get name of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Name of tested stage
 **/
std::string XFBCaptureInactiveOutputComponentTest::getTestCaseName(glw::GLuint test_case_index)
{
    const GLchar *name = 0;

    switch (test_case_index)
    {
        case TEST_VS:
            name = "vertex";
            break;
        case TEST_TES:
            name = "tessellation evaluation";
            break;
        case TEST_GS:
            name = "geometry";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return name;
}

/** Returns number of test cases
 *
 * @return TEST_MAX
 **/
glw::GLuint XFBCaptureInactiveOutputComponentTest::getTestCaseNumber()
{
    return TEST_MAX;
}

/** Verify contents of buffers
 *
 * @param buffers Collection of buffers to be verified
 *
 * @return true if everything is as expected, false otherwise
 **/
bool XFBCaptureInactiveOutputComponentTest::verifyBuffers(bufferCollection &buffers)
{
    bool result = true;

    bufferCollection::pair &pair = buffers.m_vector[1] /* xfb */;
    Utils::Buffer *buffer        = pair.m_buffer;
    bufferDescriptor *descriptor = pair.m_descriptor;

    /* Get pointer to contents of buffer */
    buffer->Bind();
    GLubyte *buffer_data = (GLubyte *)buffer->Map(Utils::Buffer::ReadOnly);

    /* Get pointer to expected data */
    GLubyte *expected_data = (GLubyte *)&descriptor->m_expected_data[0];

    /* Compare */
    static const GLuint comp_size = 4;
    static const GLuint vec4_size = 16;

    int res_goku_x = memcmp(buffer_data + 2 * vec4_size + 0 * comp_size,
                            expected_data + 2 * vec4_size + 0 * comp_size, comp_size);
    int res_goku_z = memcmp(buffer_data + 2 * vec4_size + 2 * comp_size,
                            expected_data + 2 * vec4_size + 2 * comp_size, comp_size);

    int res_gohan_y = memcmp(buffer_data + 0 * vec4_size + 1 * comp_size,
                             expected_data + 0 * vec4_size + 1 * comp_size, comp_size);
    int res_gohan_w = memcmp(buffer_data + 0 * vec4_size + 3 * comp_size,
                             expected_data + 0 * vec4_size + 3 * comp_size, comp_size);

    int res_goten_x = memcmp(buffer_data + 1 * vec4_size + 0 * comp_size,
                             expected_data + 1 * vec4_size + 0 * comp_size, comp_size);
    int res_goten_y = memcmp(buffer_data + 1 * vec4_size + 1 * comp_size,
                             expected_data + 1 * vec4_size + 1 * comp_size, comp_size);

    int res_chichi_z = memcmp(buffer_data + 3 * vec4_size + 2 * comp_size,
                              expected_data + 3 * vec4_size + 2 * comp_size, comp_size);
    int res_chichi_w = memcmp(buffer_data + 3 * vec4_size + 3 * comp_size,
                              expected_data + 3 * vec4_size + 3 * comp_size, comp_size);

    int res_vegeta_x = memcmp(buffer_data + 7 * vec4_size + 0 * comp_size,
                              expected_data + 7 * vec4_size + 0 * comp_size, comp_size);

    int res_trunks_y = memcmp(buffer_data + 6 * vec4_size + 1 * comp_size,
                              expected_data + 6 * vec4_size + 1 * comp_size, comp_size);

    int res_bra_z = memcmp(buffer_data + 5 * vec4_size + 2 * comp_size,
                           expected_data + 5 * vec4_size + 2 * comp_size, comp_size);

    int res_bulma_w = memcmp(buffer_data + 4 * vec4_size + 3 * comp_size,
                             expected_data + 4 * vec4_size + 3 * comp_size, comp_size);

    if ((0 != res_goku_x) || (0 != res_goku_z) || (0 != res_gohan_y) || (0 != res_gohan_w) ||
        (0 != res_goten_x) || (0 != res_goten_y) || (0 != res_chichi_z) || (0 != res_chichi_w) ||
        (0 != res_vegeta_x) || (0 != res_trunks_y) || (0 != res_bra_z) || (0 != res_bulma_w))
    {
        m_context.getTestContext().getLog()
            << tcu::TestLog::Message
            << "Invalid result. Buffer: " << Utils::Buffer::GetBufferName(descriptor->m_target)
            << ". Index: " << descriptor->m_index << tcu::TestLog::EndMessage;

        result = false;
    }

    /* Release buffer mapping */
    buffer->UnMap();

    return result;
}

/** Constructor
 *
 * @param context Test context
 **/
XFBCaptureInactiveOutputBlockMemberTest::XFBCaptureInactiveOutputBlockMemberTest(
    deqp::Context &context)
    : BufferTestBase(context,
                     "xfb_capture_inactive_output_block_member",
                     "Test verifies that inactive block members are captured")
{
    /* Nothing to be done here */
}

/** Execute drawArrays for single vertex
 *
 * @param test_case_index
 *
 * @return true
 **/
bool XFBCaptureInactiveOutputBlockMemberTest::executeDrawCall(bool /* tesEnabled */,
                                                              GLuint test_case_index)
{
    const Functions &gl   = m_context.getRenderContext().getFunctions();
    GLenum primitive_type = GL_PATCHES;

    if (TEST_VS == test_case_index)
    {
        primitive_type = GL_POINTS;
    }

    gl.disable(GL_RASTERIZER_DISCARD);
    GLU_EXPECT_NO_ERROR(gl.getError(), "Disable");

    gl.beginTransformFeedback(GL_POINTS);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

    gl.drawArrays(primitive_type, 0 /* first */, 1 /* count */);
    GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

    gl.endTransformFeedback();
    GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

    return true;
}

/** Get descriptors of buffers necessary for test
 *
 * @param ignored
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBCaptureInactiveOutputBlockMemberTest::getBufferDescriptors(
    glw::GLuint /* test_case_index */,
    bufferDescriptor::Vector &out_descriptors)
{
    const Utils::Type &type = Utils::Type::vec4;

    /* Test needs single uniform and xfb */
    out_descriptors.resize(2);

    /* Get references */
    bufferDescriptor &uniform = out_descriptors[0];
    bufferDescriptor &xfb     = out_descriptors[1];

    /* Index */
    uniform.m_index = 0;
    xfb.m_index     = 0;

    /* Target */
    uniform.m_target = Utils::Buffer::Uniform;
    xfb.m_target     = Utils::Buffer::Transform_feedback;

    /* Data */
    const std::vector<GLubyte> &gohan_data  = type.GenerateData();
    const std::vector<GLubyte> &chichi_data = type.GenerateData();

    const GLuint type_size = static_cast<GLuint>(gohan_data.size());

    /* Uniform data */
    uniform.m_initial_data.resize(2 * type_size);
    memcpy(&uniform.m_initial_data[0] + 0, &gohan_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + type_size, &chichi_data[0], type_size);

    /* XFB data */
    xfb.m_initial_data.resize(4 * type_size);
    xfb.m_expected_data.resize(4 * type_size);

    for (GLuint i = 0; i < 4 * type_size; ++i)
    {
        xfb.m_initial_data[i]  = (glw::GLubyte)i;
        xfb.m_expected_data[i] = (glw::GLubyte)i;
    }

    memcpy(&xfb.m_expected_data[0] + 1 * type_size, &gohan_data[0], type_size);
    memcpy(&xfb.m_expected_data[0] + 3 * type_size, &chichi_data[0], type_size);
}

/** Get body of main function for given shader stage
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void XFBCaptureInactiveOutputBlockMemberTest::getShaderBody(GLuint test_case_index,
                                                            Utils::Shader::STAGES stage,
                                                            std::string &out_assignments,
                                                            std::string &out_calculations)
{
    out_calculations = "";

    static const GLchar *vs_tes_gs =
        "    chichi = uni_chichi;\n"
        "    gohan  = uni_gohan;\n";
    static const GLchar *fs = "    fs_out = goten + gohan + chichi;\n";

    const GLchar *assignments = "";

    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            assignments = fs;
            break;

        case Utils::Shader::GEOMETRY:
            if (TEST_GS == test_case_index)
            {
                assignments = vs_tes_gs;
            }
            break;

        case Utils::Shader::TESS_CTRL:
            break;

        case Utils::Shader::TESS_EVAL:
            if (TEST_TES == test_case_index)
            {
                assignments = vs_tes_gs;
            }
            break;

        case Utils::Shader::VERTEX:
            if (TEST_VS == test_case_index)
            {
                assignments = vs_tes_gs;
            }
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    out_assignments = assignments;
}

/** Get interface of shader
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_interface    Set to ""
 **/
void XFBCaptureInactiveOutputBlockMemberTest::getShaderInterface(GLuint test_case_index,
                                                                 Utils::Shader::STAGES stage,
                                                                 std::string &out_interface)
{
    static const GLchar *vs_tes_gs =
        "const uint sizeof_type = 16;\n"
        "\n"
        "layout (xfb_offset = 1 * sizeof_type) out Goku {\n"
        "    vec4 gohan;\n"
        "    vec4 goten;\n"
        "    vec4 chichi;\n"
        "};\n"
        "\n"
        "layout(binding = 0) uniform block {\n"
        "    vec4 uni_gohan;\n"
        "    vec4 uni_chichi;\n"
        "};\n";
    static const GLchar *fs =
        "in Goku {\n"
        "    vec4 gohan;\n"
        "    vec4 goten;\n"
        "    vec4 chichi;\n"
        "};\n"
        "out vec4 fs_out;\n";

    const GLchar *interface = "";

    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            interface = fs;
            break;

        case Utils::Shader::GEOMETRY:
            if (TEST_GS == test_case_index)
            {
                interface = vs_tes_gs;
            }
            break;

        case Utils::Shader::TESS_CTRL:
            break;

        case Utils::Shader::TESS_EVAL:
            if (TEST_TES == test_case_index)
            {
                interface = vs_tes_gs;
            }
            break;

        case Utils::Shader::VERTEX:
            if (TEST_VS == test_case_index)
            {
                interface = vs_tes_gs;
            }
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    out_interface = interface;
}

/** Get source code of shader
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Source
 **/
std::string XFBCaptureInactiveOutputBlockMemberTest::getShaderSource(GLuint test_case_index,
                                                                     Utils::Shader::STAGES stage)
{
    std::string source;

    switch (test_case_index)
    {
        case TEST_VS:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case TEST_TES:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::TESS_CTRL:
                case Utils::Shader::TESS_EVAL:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case TEST_GS:
            source = BufferTestBase::getShaderSource(test_case_index, stage);
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    /* */
    return source;
}

/** Get name of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Name of tested stage
 **/
std::string XFBCaptureInactiveOutputBlockMemberTest::getTestCaseName(glw::GLuint test_case_index)
{
    const GLchar *name = 0;

    switch (test_case_index)
    {
        case TEST_VS:
            name = "vertex";
            break;
        case TEST_TES:
            name = "tessellation evaluation";
            break;
        case TEST_GS:
            name = "geometry";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return name;
}

/** Returns number of test cases
 *
 * @return TEST_MAX
 **/
glw::GLuint XFBCaptureInactiveOutputBlockMemberTest::getTestCaseNumber()
{
    return TEST_MAX;
}

/** Verify contents of buffers
 *
 * @param buffers Collection of buffers to be verified
 *
 * @return true if everything is as expected, false otherwise
 **/
bool XFBCaptureInactiveOutputBlockMemberTest::verifyBuffers(bufferCollection &buffers)
{
    bool result = true;

    bufferCollection::pair &pair = buffers.m_vector[1] /* xfb */;
    Utils::Buffer *buffer        = pair.m_buffer;
    bufferDescriptor *descriptor = pair.m_descriptor;

    /* Get pointer to contents of buffer */
    buffer->Bind();
    GLubyte *buffer_data = (GLubyte *)buffer->Map(Utils::Buffer::ReadOnly);

    /* Get pointer to expected data */
    GLubyte *expected_data = (GLubyte *)&descriptor->m_expected_data[0];

    /* Compare */
    static const GLuint vec4_size = 16;

    int res_before = memcmp(buffer_data, expected_data, vec4_size);
    int res_gohan  = memcmp(buffer_data + 1 * vec4_size, expected_data + 1 * vec4_size, vec4_size);
    int res_chichi = memcmp(buffer_data + 3 * vec4_size, expected_data + 3 * vec4_size, vec4_size);

    if ((0 != res_before) || (0 != res_gohan) || (0 != res_chichi))
    {
        m_context.getTestContext().getLog()
            << tcu::TestLog::Message
            << "Invalid result. Buffer: " << Utils::Buffer::GetBufferName(descriptor->m_target)
            << ". Index: " << descriptor->m_index << tcu::TestLog::EndMessage;

        result = false;
    }

    /* Release buffer mapping */
    buffer->UnMap();

    return result;
}

/** Constructor
 *
 * @param context Test context
 **/
XFBCaptureStructTest::XFBCaptureStructTest(deqp::Context &context)
    : BufferTestBase(context,
                     "xfb_capture_struct",
                     "Test verifies that inactive structure members are captured")
{
    /* Nothing to be done here */
}

/** Execute drawArrays for single vertex
 *
 * @param test_case_index
 *
 * @return true
 **/
bool XFBCaptureStructTest::executeDrawCall(bool /* tesEnabled */, GLuint test_case_index)
{
    const Functions &gl   = m_context.getRenderContext().getFunctions();
    GLenum primitive_type = GL_PATCHES;

    if (TEST_VS == test_case_index)
    {
        primitive_type = GL_POINTS;
    }

    gl.disable(GL_RASTERIZER_DISCARD);
    GLU_EXPECT_NO_ERROR(gl.getError(), "Disable");

    gl.beginTransformFeedback(GL_POINTS);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

    gl.drawArrays(primitive_type, 0 /* first */, 1 /* count */);
    GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

    gl.endTransformFeedback();
    GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

    return true;
}

/** Get descriptors of buffers necessary for test
 *
 * @param ignored
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBCaptureStructTest::getBufferDescriptors(glw::GLuint /* test_case_index */,
                                                bufferDescriptor::Vector &out_descriptors)
{
    const Utils::Type &type = Utils::Type::vec4;

    /* Test needs single uniform and xfb */
    out_descriptors.resize(2);

    /* Get references */
    bufferDescriptor &uniform = out_descriptors[0];
    bufferDescriptor &xfb     = out_descriptors[1];

    /* Index */
    uniform.m_index = 0;
    xfb.m_index     = 0;

    /* Target */
    uniform.m_target = Utils::Buffer::Uniform;
    xfb.m_target     = Utils::Buffer::Transform_feedback;

    /* Data */
    const std::vector<GLubyte> &gohan_data  = type.GenerateData();
    const std::vector<GLubyte> &chichi_data = type.GenerateData();

    const GLuint type_size = static_cast<GLuint>(gohan_data.size());

    /* Uniform data */
    uniform.m_initial_data.resize(2 * type_size);
    memcpy(&uniform.m_initial_data[0] + 0, &gohan_data[0], type_size);
    memcpy(&uniform.m_initial_data[0] + type_size, &chichi_data[0], type_size);

    /* XFB data */
    xfb.m_initial_data.resize(4 * type_size);
    xfb.m_expected_data.resize(4 * type_size);

    for (GLuint i = 0; i < 4 * type_size; ++i)
    {
        xfb.m_initial_data[i]  = (glw::GLubyte)i;
        xfb.m_expected_data[i] = (glw::GLubyte)i;
    }

    memcpy(&xfb.m_expected_data[0] + 1 * type_size, &gohan_data[0], type_size);
    memcpy(&xfb.m_expected_data[0] + 3 * type_size, &chichi_data[0], type_size);
}

/** Get body of main function for given shader stage
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void XFBCaptureStructTest::getShaderBody(GLuint test_case_index,
                                         Utils::Shader::STAGES stage,
                                         std::string &out_assignments,
                                         std::string &out_calculations)
{
    out_calculations = "";

    static const GLchar *vs_tes_gs =
        "    goku.chichi = uni_chichi;\n"
        "    goku.gohan  = uni_gohan;\n";
    static const GLchar *fs = "    fs_out = goku.goten + goku.gohan + goku.chichi;\n";

    const GLchar *assignments = "";

    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            assignments = fs;
            break;

        case Utils::Shader::GEOMETRY:
            if (TEST_GS == test_case_index)
            {
                assignments = vs_tes_gs;
            }
            break;

        case Utils::Shader::TESS_CTRL:
            break;

        case Utils::Shader::TESS_EVAL:
            if (TEST_TES == test_case_index)
            {
                assignments = vs_tes_gs;
            }
            break;

        case Utils::Shader::VERTEX:
            if (TEST_VS == test_case_index)
            {
                assignments = vs_tes_gs;
            }
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    out_assignments = assignments;
}

/** Get interface of shader
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_interface    Set to ""
 **/
void XFBCaptureStructTest::getShaderInterface(GLuint test_case_index,
                                              Utils::Shader::STAGES stage,
                                              std::string &out_interface)
{
    static const GLchar *vs_tes_gs =
        "const uint sizeof_type = 16;\n"
        "\n"
        "struct Goku {\n"
        "    vec4 gohan;\n"
        "    vec4 goten;\n"
        "    vec4 chichi;\n"
        "};\n"
        "\n"
        "layout (xfb_offset = sizeof_type) out Goku goku;\n"
        "\n"
        "layout(binding = 0, std140) uniform block {\n"
        "    vec4 uni_gohan;\n"
        "    vec4 uni_chichi;\n"
        "};\n";
    static const GLchar *fs =
        "struct Goku {\n"
        "    vec4 gohan;\n"
        "    vec4 goten;\n"
        "    vec4 chichi;\n"
        "};\n"
        "\n"
        "in Goku goku;\n"
        "\n"
        "out vec4 fs_out;\n";

    const GLchar *interface = "";

    switch (stage)
    {
        case Utils::Shader::FRAGMENT:
            interface = fs;
            break;

        case Utils::Shader::GEOMETRY:
            if (TEST_GS == test_case_index)
            {
                interface = vs_tes_gs;
            }
            break;

        case Utils::Shader::TESS_CTRL:
            break;

        case Utils::Shader::TESS_EVAL:
            if (TEST_TES == test_case_index)
            {
                interface = vs_tes_gs;
            }
            break;

        case Utils::Shader::VERTEX:
            if (TEST_VS == test_case_index)
            {
                interface = vs_tes_gs;
            }
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    out_interface = interface;
}

/** Get source code of shader
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Source
 **/
std::string XFBCaptureStructTest::getShaderSource(GLuint test_case_index,
                                                  Utils::Shader::STAGES stage)
{
    std::string source;

    switch (test_case_index)
    {
        case TEST_VS:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case TEST_TES:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::TESS_CTRL:
                case Utils::Shader::TESS_EVAL:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case TEST_GS:
            source = BufferTestBase::getShaderSource(test_case_index, stage);
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    /* */
    return source;
}

/** Get name of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Name of tested stage
 **/
std::string XFBCaptureStructTest::getTestCaseName(glw::GLuint test_case_index)
{
    const GLchar *name = 0;

    switch (test_case_index)
    {
        case TEST_VS:
            name = "vertex";
            break;
        case TEST_TES:
            name = "tessellation evaluation";
            break;
        case TEST_GS:
            name = "geometry";
            break;
        default:
            TCU_FAIL("Invalid enum");
    }

    return name;
}

/** Returns number of test cases
 *
 * @return TEST_MAX
 **/
glw::GLuint XFBCaptureStructTest::getTestCaseNumber()
{
    return TEST_MAX;
}

/** Verify contents of buffers
 *
 * @param buffers Collection of buffers to be verified
 *
 * @return true if everything is as expected, false otherwise
 **/
bool XFBCaptureStructTest::verifyBuffers(bufferCollection &buffers)
{
    bool result = true;

    bufferCollection::pair &pair = buffers.m_vector[1] /* xfb */;
    Utils::Buffer *buffer        = pair.m_buffer;
    bufferDescriptor *descriptor = pair.m_descriptor;

    /* Get pointer to contents of buffer */
    buffer->Bind();
    GLubyte *buffer_data = (GLubyte *)buffer->Map(Utils::Buffer::ReadOnly);

    /* Get pointer to expected data */
    GLubyte *expected_data = (GLubyte *)&descriptor->m_expected_data[0];

    /* Compare */
    static const GLuint vec4_size = 16;

    int res_before = memcmp(buffer_data, expected_data, vec4_size);
    int res_gohan  = memcmp(buffer_data + 1 * vec4_size, expected_data + 1 * vec4_size, vec4_size);
    int res_chichi = memcmp(buffer_data + 3 * vec4_size, expected_data + 3 * vec4_size, vec4_size);

    if ((0 != res_before) || (0 != res_gohan) || (0 != res_chichi))
    {
        m_context.getTestContext().getLog()
            << tcu::TestLog::Message
            << "Invalid result. Buffer: " << Utils::Buffer::GetBufferName(descriptor->m_target)
            << ". Index: " << descriptor->m_index << tcu::TestLog::EndMessage;

        result = false;
    }

    /* Release buffer mapping */
    buffer->UnMap();

    return result;
}

/** Constructor
 *
 * @param context Test framework context
 **/
XFBCaptureUnsizedArrayTest::XFBCaptureUnsizedArrayTest(deqp::Context &context, GLuint stage)
    : NegativeTestBase(context,
                       "xfb_capture_unsized_array",
                       "Test verifies that compiler reports error when unsized array is qualified "
                       "with xfb_offset"),
      m_stage(stage)
{
    std::string name = ("xfb_capture_unsized_array_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    NegativeTestBase::m_name = name.c_str();
}

/** Source for given test case and stage
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Shader source
 **/
std::string XFBCaptureUnsizedArrayTest::getShaderSource(GLuint test_case_index,
                                                        Utils::Shader::STAGES stage)
{
#if DEBUG_NEG_REMOVE_ERROR
    static const GLchar *var_definition = "/* layout (xfb_offset = 0) */ out vec4 goku[];\n";
#else
    static const GLchar *var_definition = "layout (xfb_offset = 0) out vec4 goku[];\n";
#endif /* DEBUG_NEG_REMOVE_ERROR */
    static const GLchar *var_use = "    goku[0] = result / 2;\n";
    static const GLchar *fs =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "in  vec4 any_fs;\n"
        "out vec4 fs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fs_out = any_fs;\n"
        "}\n"
        "\n";
    static const GLchar *vs_tested =
        "#version 430 core\n"
        "#extension GL_ARB_enhanced_layouts : require\n"
        "\n"
        "VAR_DEFINITION"
        "\n"
        "in  vec4 in_vs;\n"
        "out vec4 any_fs;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 result = in_vs;\n"
        "\n"
        "VARIABLE_USE"
        "\n"
        "    any_fs = result;\n"
        "}\n"
        "\n";

    std::string source;
    testCase &test_case = m_test_cases[test_case_index];

    if (test_case.m_stage == stage)
    {
        size_t position = 0;

        switch (stage)
        {
            case Utils::Shader::VERTEX:
                source = vs_tested;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }

        Utils::replaceToken("VAR_DEFINITION", position, var_definition, source);
        position = 0;
        Utils::replaceToken("VARIABLE_USE", position, var_use, source);
    }
    else
    {
        switch (test_case.m_stage)
        {
            case Utils::Shader::VERTEX:
                switch (stage)
                {
                    case Utils::Shader::FRAGMENT:
                        source = fs;
                        break;
                    default:
                        source = "";
                }
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    return source;
}

/** Get description of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Test case description
 **/
std::string XFBCaptureUnsizedArrayTest::getTestCaseName(GLuint test_case_index)
{
    std::stringstream stream;
    testCase &test_case = m_test_cases[test_case_index];

    stream << "Stage: " << Utils::Shader::GetStageName(test_case.m_stage);

    return stream.str();
}

/** Get number of test cases
 *
 * @return Number of test cases
 **/
GLuint XFBCaptureUnsizedArrayTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Selects if "compute" stage is relevant for test
 *
 * @param ignored
 *
 * @return false
 **/
bool XFBCaptureUnsizedArrayTest::isComputeRelevant(GLuint /* test_case_index */)
{
    return false;
}

/** Prepare all test cases
 *
 **/
void XFBCaptureUnsizedArrayTest::testInit()
{
    testCase test_case = {(Utils::Shader::STAGES)m_stage};

    m_test_cases.push_back(test_case);
}

/** Constructor
 *
 * @param context Test context
 **/
XFBExplicitLocationTest::XFBExplicitLocationTest(deqp::Context &context, GLuint type, GLuint stage)
    : BufferTestBase(
          context,
          "xfb_explicit_location",
          "Test verifies that explicit location on matrices and arrays does not impact xfb output"),
      m_type(type),
      m_stage(stage)
{
    std::string name = ("xfb_explicit_location_");
    name.append(getTypeName(m_type));
    name.append("_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)stage));

    BufferTestBase::m_name = name.c_str();
}

/** Execute drawArrays for single vertex
 *
 * @param test_case_index
 *
 * @return true
 **/
bool XFBExplicitLocationTest::executeDrawCall(bool /* tesEnabled */, GLuint test_case_index)
{
    const Functions &gl       = m_context.getRenderContext().getFunctions();
    GLenum primitive_type     = GL_PATCHES;
    const testCase &test_case = m_test_cases[test_case_index];

    if (Utils::Shader::VERTEX == test_case.m_stage)
    {
        primitive_type = GL_POINTS;
    }

    gl.disable(GL_RASTERIZER_DISCARD);
    GLU_EXPECT_NO_ERROR(gl.getError(), "Disable");

    gl.beginTransformFeedback(GL_POINTS);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

    gl.drawArrays(primitive_type, 0 /* first */, 2 /* count */);
    GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

    gl.endTransformFeedback();
    GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

    return true;
}

/** Get descriptors of buffers necessary for test
 *
 * @param test_case_index Index of test case
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBExplicitLocationTest::getBufferDescriptors(GLuint test_case_index,
                                                   bufferDescriptor::Vector &out_descriptors)
{
    const testCase &test_case = m_test_cases[test_case_index];
    const Utils::Type &type   = test_case.m_type;

    /* Test needs single uniform and xfb */
    out_descriptors.resize(2);

    /* Get references */
    bufferDescriptor &uniform = out_descriptors[0];
    bufferDescriptor &xfb     = out_descriptors[1];

    /* Index */
    uniform.m_index = 0;
    xfb.m_index     = 0;

    /* Target */
    uniform.m_target = Utils::Buffer::Uniform;
    xfb.m_target     = Utils::Buffer::Transform_feedback;

    /* Data */
    const GLuint rand_start = Utils::s_rand;
    std::vector<GLubyte> uniform_data;

    for (GLuint i = 0; i < std::max(test_case.m_array_size, 1u); i++)
    {
        const std::vector<GLubyte> &type_uniform_data = type.GenerateData();
        /**
         * Rule 4 of Section 7.6.2.2:
         *
         * If the member is an array of scalars or vectors, the base alignment and array stride
         * are set to match the base alignment of a single array element, according to rules (1),
         * (2), and (3), and rounded up to the base alignment of a vec4.
         */
        uniform_data.resize(Utils::align((glw::GLuint)uniform_data.size(), 16));
        uniform_data.insert(uniform_data.end(), type_uniform_data.begin(), type_uniform_data.end());
    }

    Utils::s_rand = rand_start;
    std::vector<GLubyte> xfb_data;

    for (GLuint i = 0; i < std::max(test_case.m_array_size, 1u); i++)
    {
        const std::vector<GLubyte> &type_xfb_data = type.GenerateDataPacked();
        xfb_data.insert(xfb_data.end(), type_xfb_data.begin(), type_xfb_data.end());
    }

    const GLuint uni_type_size = static_cast<GLuint>(uniform_data.size());
    const GLuint xfb_type_size = static_cast<GLuint>(xfb_data.size());
    /*
     Note: If xfb varying output from vertex shader, the variable "goku" will only output once to
     transform feedback buffer, if xfb varying output from TES or GS, because the input primitive
     type in TES is defined as "layout(isolines, point_mode) in;", the primitive type is line which
     make the variable "goku" will output twice to transform feedback buffer, so for vertex shader
     only one valid data should be initialized in xfb.m_expected_data
     */
    const GLuint xfb_data_size =
        (test_case.m_stage == Utils::Shader::VERTEX) ? xfb_type_size : xfb_type_size * 2;
    /* Uniform data */
    uniform.m_initial_data.resize(uni_type_size);
    memcpy(&uniform.m_initial_data[0] + 0 * uni_type_size, &uniform_data[0], uni_type_size);

    /* XFB data */
    xfb.m_initial_data.resize(xfb_data_size, 0);
    xfb.m_expected_data.resize(xfb_data_size);

    if (test_case.m_stage == Utils::Shader::VERTEX)
    {
        memcpy(&xfb.m_expected_data[0] + 0 * xfb_type_size, &xfb_data[0], xfb_type_size);
    }
    else
    {
        memcpy(&xfb.m_expected_data[0] + 0 * xfb_type_size, &xfb_data[0], xfb_type_size);
        memcpy(&xfb.m_expected_data[0] + 1 * xfb_type_size, &xfb_data[0], xfb_type_size);
    }
}

/** Get body of main function for given shader stage
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void XFBExplicitLocationTest::getShaderBody(GLuint test_case_index,
                                            Utils::Shader::STAGES stage,
                                            std::string &out_assignments,
                                            std::string &out_calculations)
{
    const testCase &test_case = m_test_cases[test_case_index];

    out_calculations = "";

    static const GLchar *vs_tes_gs = "    goku = uni_goku;\n";

    const GLchar *assignments = "";

    if (test_case.m_stage == stage)
    {
        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                assignments = vs_tes_gs;
                break;
            case Utils::Shader::TESS_EVAL:
                assignments = vs_tes_gs;
                break;
            case Utils::Shader::VERTEX:
                assignments = vs_tes_gs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                assignments = "";
                break;
            case Utils::Shader::GEOMETRY:
            case Utils::Shader::TESS_CTRL:
            case Utils::Shader::TESS_EVAL:
            case Utils::Shader::VERTEX:
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    out_assignments = assignments;

    if (Utils::Shader::FRAGMENT == stage)
    {
        Utils::replaceAllTokens("TYPE", test_case.m_type.GetGLSLTypeName(), out_assignments);
    }
}

/** Get interface of shader
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_interface    Set to ""
 **/
void XFBExplicitLocationTest::getShaderInterface(GLuint test_case_index,
                                                 Utils::Shader::STAGES stage,
                                                 std::string &out_interface)
{
    static const GLchar *vs_tes_gs =
        "layout (location = 0, xfb_offset = 0) FLAT out TYPE gokuARRAY;\n"
        "\n"
        "layout(std140, binding = 0) uniform Goku {\n"
        "    TYPE uni_gokuARRAY;\n"
        "};\n";

    const testCase &test_case = m_test_cases[test_case_index];
    const GLchar *interface   = "";
    const GLchar *flat        = "";

    if (test_case.m_stage == stage)
    {
        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
                interface = vs_tes_gs;
                break;
            case Utils::Shader::TESS_EVAL:
                interface = vs_tes_gs;
                break;
            case Utils::Shader::VERTEX:
                interface = vs_tes_gs;
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                interface = "";
                break;
            case Utils::Shader::GEOMETRY:
            case Utils::Shader::TESS_CTRL:
            case Utils::Shader::TESS_EVAL:
            case Utils::Shader::VERTEX:
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    out_interface = interface;

    if (Utils::Type::Float != test_case.m_type.m_basic_type)
    {
        flat = "flat";
    }

    /* Array size */
    if (0 == test_case.m_array_size)
    {
        Utils::replaceAllTokens("ARRAY", "", out_interface);
    }
    else
    {
        char buffer[16];
        sprintf(buffer, "[%d]", test_case.m_array_size);

        Utils::replaceAllTokens("ARRAY", buffer, out_interface);
    }

    Utils::replaceAllTokens("FLAT", flat, out_interface);
    Utils::replaceAllTokens("TYPE", test_case.m_type.GetGLSLTypeName(), out_interface);
}

/** Get source code of shader
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Source
 **/
std::string XFBExplicitLocationTest::getShaderSource(GLuint test_case_index,
                                                     Utils::Shader::STAGES stage)
{
    std::string source;
    const testCase &test_case = m_test_cases[test_case_index];

    switch (test_case.m_stage)
    {
        case Utils::Shader::VERTEX:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case Utils::Shader::TESS_EVAL:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::TESS_CTRL:
                case Utils::Shader::TESS_EVAL:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case Utils::Shader::GEOMETRY:
            source = BufferTestBase::getShaderSource(test_case_index, stage);
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    /* */
    return source;
}

/** Get name of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Name of tested stage
 **/
std::string XFBExplicitLocationTest::getTestCaseName(glw::GLuint test_case_index)
{
    std::stringstream stream;
    const testCase &test_case = m_test_cases[test_case_index];

    stream << "Type: " << test_case.m_type.GetGLSLTypeName()
           << ", stage: " << Utils::Shader::GetStageName(test_case.m_stage);

    return stream.str();
}

/** Returns number of test cases
 *
 * @return TEST_MAX
 **/
glw::GLuint XFBExplicitLocationTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Prepare all test cases
 *
 **/
void XFBExplicitLocationTest::testInit()
{
    const Functions &gl = m_context.getRenderContext().getFunctions();
    GLint max_xfb_int;

    gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &max_xfb_int);
    GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

    const Utils::Type &type = getType(m_type);

    if (type.m_n_columns > 1)
    {
        testCase test_case = {(Utils::Shader::STAGES)m_stage, type, 0};

        m_test_cases.push_back(test_case);
    }

    for (GLuint array_size = 3; array_size > 1; array_size--)
    {
        if (type.GetNumComponents() * array_size <= GLuint(max_xfb_int))
        {
            testCase test_case = {(Utils::Shader::STAGES)m_stage, type, array_size};

            m_test_cases.push_back(test_case);

            break;
        }
    }
}

/** Constructor
 *
 * @param context Test context
 **/
XFBExplicitLocationStructTest::XFBExplicitLocationStructTest(deqp::Context &context, GLuint stage)
    : BufferTestBase(context,
                     "xfb_struct_explicit_location",
                     "Test verifies that explicit location on structs does not impact xfb output"),
      m_stage(stage)
{
    std::string name = ("xfb_struct_explicit_location_");
    name.append(EnhancedLayouts::Utils::Shader::GetStageName(
        (EnhancedLayouts::Utils::Shader::STAGES)m_stage));

    BufferTestBase::m_name = name.c_str();
}

/** Execute drawArrays for single vertex
 *
 * @param test_case_index
 *
 * @return true
 **/
bool XFBExplicitLocationStructTest::executeDrawCall(bool /* tesEnabled */, GLuint test_case_index)
{
    const Functions &gl       = m_context.getRenderContext().getFunctions();
    GLenum primitive_type     = GL_PATCHES;
    const testCase &test_case = m_test_cases[test_case_index];

    if (Utils::Shader::VERTEX == test_case.m_stage)
    {
        primitive_type = GL_POINTS;
    }

    gl.disable(GL_RASTERIZER_DISCARD);
    GLU_EXPECT_NO_ERROR(gl.getError(), "Disable");

    gl.beginTransformFeedback(GL_POINTS);
    GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

    gl.drawArrays(primitive_type, 0 /* first */, 2 /* count */);
    GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

    gl.endTransformFeedback();
    GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

    return true;
}

/** Get descriptors of buffers necessary for test
 *
 * @param test_case_index Index of test case
 * @param out_descriptors Descriptors of buffers used by test
 **/
void XFBExplicitLocationStructTest::getBufferDescriptors(GLuint test_case_index,
                                                         bufferDescriptor::Vector &out_descriptors)
{
    const testCase &test_case = m_test_cases[test_case_index];

    /* Test needs single uniform and xfb */
    out_descriptors.resize(2);

    /* Get references */
    bufferDescriptor &uniform = out_descriptors[0];
    bufferDescriptor &xfb     = out_descriptors[1];

    /* Index */
    uniform.m_index = 0;
    xfb.m_index     = 0;

    /* Target */
    uniform.m_target = Utils::Buffer::Uniform;
    xfb.m_target     = Utils::Buffer::Transform_feedback;

    /* Data */
    const GLuint rand_start = Utils::s_rand;
    std::vector<GLubyte> uniform_data;
    GLuint max_aligment = 1;

    for (const testType &type : test_case.m_types)
    {
        GLuint base_aligment = type.m_type.GetBaseAlignment(false);
        if (type.m_array_size > 0)
        {
            /**
             * Rule 4 of Section 7.6.2.2:
             *
             * If the member is an array of scalars or vectors, the base alignment and array stride
             * are set to match the base alignment of a single array element, according to rules
             * (1), (2), and (3), and rounded up to the base alignment of a vec4.
             */
            base_aligment = Utils::align(base_aligment, Utils::Type::vec4.GetBaseAlignment(false));
        }

        max_aligment = std::max(base_aligment, max_aligment);

        uniform_data.resize(Utils::align((glw::GLuint)uniform_data.size(), base_aligment), 0);

        for (GLuint i = 0; i < std::max(type.m_array_size, 1u); i++)
        {
            const std::vector<GLubyte> &type_uniform_data = type.m_type.GenerateData();
            uniform_data.insert(uniform_data.end(), type_uniform_data.begin(),
                                type_uniform_data.end());

            if (type.m_array_size > 0)
            {
                uniform_data.resize(Utils::align((glw::GLuint)uniform_data.size(), base_aligment),
                                    0);
            }
        }
    }

    const GLuint struct_aligment =
        Utils::align(max_aligment, Utils::Type::vec4.GetBaseAlignment(false));

    if (test_case.m_nested_struct)
    {
        uniform_data.resize(Utils::align((glw::GLuint)uniform_data.size(), struct_aligment), 0);

        const GLuint old_size = (glw::GLuint)uniform_data.size();
        uniform_data.resize(2 * old_size);
        std::copy_n(uniform_data.begin(), old_size, uniform_data.begin() + old_size);
    }

    uniform_data.resize(Utils::align((glw::GLuint)uniform_data.size(), struct_aligment), 0);

    Utils::s_rand = rand_start;
    std::vector<GLubyte> xfb_data;

    GLuint max_type_size = 1;
    for (const testType &type : test_case.m_types)
    {
        const GLuint basic_type_size = Utils::Type::GetTypeSize(type.m_type.m_basic_type);
        max_type_size                = std::max(max_type_size, basic_type_size);

        /* Align per current type's aligment requirements */
        xfb_data.resize(Utils::align((glw::GLuint)xfb_data.size(), basic_type_size), 0);

        for (GLuint i = 0; i < std::max(type.m_array_size, 1u); i++)
        {
            const std::vector<GLubyte> &type_xfb_data = type.m_type.GenerateDataPacked();
            xfb_data.insert(xfb_data.end(), type_xfb_data.begin(), type_xfb_data.end());
        }
    }

    if (test_case.m_nested_struct)
    {
        /* Struct has aligment requirement equal to largest requirement of its members */
        xfb_data.resize(Utils::align((glw::GLuint)xfb_data.size(), max_type_size), 0);

        const GLuint old_size = (glw::GLuint)xfb_data.size();
        xfb_data.resize(2 * old_size);
        std::copy_n(xfb_data.begin(), old_size, xfb_data.begin() + old_size);
    }

    xfb_data.resize(Utils::align((glw::GLuint)xfb_data.size(), max_type_size), 0);

    const GLuint uni_type_size = static_cast<GLuint>(uniform_data.size());
    const GLuint xfb_type_size = static_cast<GLuint>(xfb_data.size());

    /* Do not exceed the minimum value of MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS */
    DE_ASSERT(xfb_type_size <= 64 * sizeof(GLuint));

    /*
     Note: If xfb varying output from vertex shader, the variable "goku" will only output once to
     transform feedback buffer, if xfb varying output from TES or GS, because the input primitive
     type in TES is defined as "layout(isolines, point_mode) in;", the primitive type is line which
     make the variable "goku" will output twice to transform feedback buffer, so for vertex shader
     only one valid data should be initialized in xfb.m_expected_data
     */
    const GLuint xfb_data_size =
        (test_case.m_stage == Utils::Shader::VERTEX) ? xfb_type_size : xfb_type_size * 2;
    /* Uniform data */
    uniform.m_initial_data.resize(uni_type_size);
    memcpy(&uniform.m_initial_data[0] + 0 * uni_type_size, &uniform_data[0], uni_type_size);

    /* XFB data */
    xfb.m_initial_data.resize(xfb_data_size, 0);
    xfb.m_expected_data.resize(xfb_data_size);

    if (test_case.m_stage == Utils::Shader::VERTEX)
    {
        memcpy(&xfb.m_expected_data[0] + 0 * xfb_type_size, &xfb_data[0], xfb_type_size);
    }
    else
    {
        memcpy(&xfb.m_expected_data[0] + 0 * xfb_type_size, &xfb_data[0], xfb_type_size);
        memcpy(&xfb.m_expected_data[0] + 1 * xfb_type_size, &xfb_data[0], xfb_type_size);
    }
}

/** Get body of main function for given shader stage
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_assignments  Set to empty
 * @param out_calculations Set to empty
 **/
void XFBExplicitLocationStructTest::getShaderBody(GLuint test_case_index,
                                                  Utils::Shader::STAGES stage,
                                                  std::string &out_assignments,
                                                  std::string &out_calculations)
{
    const testCase &test_case = m_test_cases[test_case_index];

    out_calculations = "";

    static const GLchar *vs_tes_gs = "    goku = uni_goku;\n";
    static const GLchar *vs_tes_gs_nested =
        "    goku.inner_struct_a = uni_goku;\n"
        "    goku.inner_struct_b = uni_goku;\n";

    const GLchar *assignments = "";

    if (test_case.m_stage == stage)
    {
        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
            case Utils::Shader::TESS_EVAL:
            case Utils::Shader::VERTEX:
                if (test_case.m_nested_struct)
                {
                    assignments = vs_tes_gs_nested;
                }
                else
                {
                    assignments = vs_tes_gs;
                }
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                assignments = "";
                break;
            case Utils::Shader::GEOMETRY:
            case Utils::Shader::TESS_CTRL:
            case Utils::Shader::TESS_EVAL:
            case Utils::Shader::VERTEX:
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    out_assignments = assignments;
}

/** Get interface of shader
 *
 * @param test_case_index  Index of test case
 * @param stage            Shader stage
 * @param out_interface    Set to ""
 **/
void XFBExplicitLocationStructTest::getShaderInterface(GLuint test_case_index,
                                                       Utils::Shader::STAGES stage,
                                                       std::string &out_interface)
{
    static const GLchar *vs_tes_gs =
        "struct TestStruct {\n"
        "STRUCT_MEMBERS"
        "};\n"
        "layout (location = 0, xfb_offset = 0) flat out TestStruct goku;\n"
        "\n"
        "layout(std140, binding = 0) uniform Goku {\n"
        "    TestStruct uni_goku;\n"
        "};\n";

    static const GLchar *vs_tes_gs_nested =
        "struct TestStruct {\n"
        "STRUCT_MEMBERS"
        "};\n"
        "struct OuterStruct {\n"
        "    TestStruct inner_struct_a;\n"
        "    TestStruct inner_struct_b;\n"
        "};\n"
        "layout (location = 0, xfb_offset = 0) flat out OuterStruct goku;\n"
        "\n"
        "layout(std140, binding = 0) uniform Goku {\n"
        "    TestStruct uni_goku;\n"
        "};\n";

    const testCase &test_case = m_test_cases[test_case_index];
    const GLchar *interface   = "";

    if (test_case.m_stage == stage)
    {
        switch (stage)
        {
            case Utils::Shader::GEOMETRY:
            case Utils::Shader::TESS_EVAL:
            case Utils::Shader::VERTEX:
                if (test_case.m_nested_struct)
                {
                    interface = vs_tes_gs_nested;
                }
                else
                {
                    interface = vs_tes_gs;
                }
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }
    else
    {
        switch (stage)
        {
            case Utils::Shader::FRAGMENT:
                interface = "";
                break;
            case Utils::Shader::GEOMETRY:
            case Utils::Shader::TESS_CTRL:
            case Utils::Shader::TESS_EVAL:
            case Utils::Shader::VERTEX:
                break;
            default:
                TCU_FAIL("Invalid enum");
        }
    }

    out_interface = interface;

    std::stringstream stream;

    char member_name = 'a';
    for (const testType &type : test_case.m_types)
    {
        stream << "   " << type.m_type.GetGLSLTypeName() << " " << member_name++;
        if (type.m_array_size > 0)
        {
            stream << "[" << type.m_array_size << "]";
        }
        stream << ";\n";
    }

    Utils::replaceAllTokens("STRUCT_MEMBERS", stream.str().c_str(), out_interface);
}

/** Get source code of shader
 *
 * @param test_case_index Index of test case
 * @param stage           Shader stage
 *
 * @return Source
 **/
std::string XFBExplicitLocationStructTest::getShaderSource(GLuint test_case_index,
                                                           Utils::Shader::STAGES stage)
{
    std::string source;
    const testCase &test_case = m_test_cases[test_case_index];

    switch (test_case.m_stage)
    {
        case Utils::Shader::VERTEX:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case Utils::Shader::TESS_EVAL:
            switch (stage)
            {
                case Utils::Shader::FRAGMENT:
                case Utils::Shader::TESS_CTRL:
                case Utils::Shader::TESS_EVAL:
                case Utils::Shader::VERTEX:
                    source = BufferTestBase::getShaderSource(test_case_index, stage);
                    break;
                default:
                    break;
            }
            break;

        case Utils::Shader::GEOMETRY:
            source = BufferTestBase::getShaderSource(test_case_index, stage);
            break;

        default:
            TCU_FAIL("Invalid enum");
    }

    /* */
    return source;
}

/** Get name of test case
 *
 * @param test_case_index Index of test case
 *
 * @return Name of tested stage
 **/
std::string XFBExplicitLocationStructTest::getTestCaseName(glw::GLuint test_case_index)
{
    std::stringstream stream;
    const testCase &test_case = m_test_cases[test_case_index];

    stream << "Struct: { ";

    for (const testType &type : test_case.m_types)
    {
        stream << type.m_type.GetGLSLTypeName() << "@" << type.m_array_size << ", ";
    }

    stream << "}, stage: " << Utils::Shader::GetStageName(test_case.m_stage);

    return stream.str();
}

/** Returns number of test cases
 *
 * @return TEST_MAX
 **/
glw::GLuint XFBExplicitLocationStructTest::getTestCaseNumber()
{
    return static_cast<GLuint>(m_test_cases.size());
}

/** Prepare all test cases
 *
 **/
void XFBExplicitLocationStructTest::testInit()
{
    const GLuint n_types = getTypesNumber();

    for (GLuint i = 0; i < n_types; ++i)
    {
        const Utils::Type &type = getType(i);

        m_test_cases.push_back(
            testCase{(Utils::Shader::STAGES)m_stage, {{Utils::Type::_float, 0}, {type, 0}}, false});
    }

    for (bool is_nested_struct : {false, true})
    {
        m_test_cases.push_back(
            testCase{(Utils::Shader::STAGES)m_stage,
                     {{Utils::Type::_double, 0}, {Utils::Type::dvec2, 0}, {Utils::Type::dmat3, 0}},
                     is_nested_struct});
        m_test_cases.push_back(testCase{(Utils::Shader::STAGES)m_stage,
                                        {{Utils::Type::_double, 0}, {Utils::Type::vec3, 0}},
                                        is_nested_struct});
        m_test_cases.push_back(testCase{(Utils::Shader::STAGES)m_stage,
                                        {{Utils::Type::dvec3, 0}, {Utils::Type::mat4x3, 0}},
                                        is_nested_struct});
        m_test_cases.push_back(testCase{(Utils::Shader::STAGES)m_stage,
                                        {{Utils::Type::_float, 0},
                                         {Utils::Type::dvec3, 0},
                                         {Utils::Type::_float, 0},
                                         {Utils::Type::_double, 0}},
                                        is_nested_struct});
        m_test_cases.push_back(testCase{(Utils::Shader::STAGES)m_stage,
                                        {{Utils::Type::vec2, 0},
                                         {Utils::Type::dvec3, 0},
                                         {Utils::Type::_float, 0},
                                         {Utils::Type::_double, 0}},
                                        is_nested_struct});
        m_test_cases.push_back(testCase{(Utils::Shader::STAGES)m_stage,
                                        {{Utils::Type::_double, 0},
                                         {Utils::Type::_float, 0},
                                         {Utils::Type::dvec2, 0},
                                         {Utils::Type::vec3, 0}},
                                        is_nested_struct});
        m_test_cases.push_back(testCase{(Utils::Shader::STAGES)m_stage,
                                        {{Utils::Type::dmat3x4, 0},
                                         {Utils::Type::_double, 0},
                                         {Utils::Type::_float, 0},
                                         {Utils::Type::dvec2, 0}},
                                        is_nested_struct});

        m_test_cases.push_back(
            testCase{(Utils::Shader::STAGES)m_stage,
                     {{Utils::Type::_float, 3}, {Utils::Type::dvec3, 0}, {Utils::Type::_double, 2}},
                     is_nested_struct});
        m_test_cases.push_back(testCase{(Utils::Shader::STAGES)m_stage,
                                        {{Utils::Type::_int, 1},
                                         {Utils::Type::_double, 1},
                                         {Utils::Type::_float, 1},
                                         {Utils::Type::dmat2x4, 1}},
                                        is_nested_struct});
        m_test_cases.push_back(testCase{(Utils::Shader::STAGES)m_stage,
                                        {{Utils::Type::_int, 5},
                                         {Utils::Type::dvec3, 2},
                                         {Utils::Type::uvec3, 0},
                                         {Utils::Type::_double, 1}},
                                        is_nested_struct});
        m_test_cases.push_back(
            testCase{(Utils::Shader::STAGES)m_stage,
                     {{Utils::Type::mat2x3, 3}, {Utils::Type::uvec4, 1}, {Utils::Type::dvec4, 0}},
                     is_nested_struct});
    }

    m_test_cases.push_back(
        testCase{(Utils::Shader::STAGES)m_stage,
                 {{Utils::Type::dmat2x3, 2}, {Utils::Type::mat2x3, 2}, {Utils::Type::dvec2, 0}},
                 false});
}
}  // namespace EnhancedLayouts

/** Constructor.
 *
 *  @param context Rendering context.
 **/
EnhancedLayoutsTests::EnhancedLayoutsTests(deqp::Context &context)
    : TestCaseGroup(context, "enhanced_layouts", "Verifies \"enhanced layouts\" functionality")
{
    /* Left blank on purpose */
}

/** Initializes a texture_storage_multisample test group.
 *
 **/
void EnhancedLayoutsTests::init(void)
{
    addChild(new EnhancedLayouts::APIConstantValuesTest(m_context));
    addChild(new EnhancedLayouts::APIErrorsTest(m_context));
    addChild(new EnhancedLayouts::GLSLContantValuesTest(m_context));
    addGLSLContantImmutablityTest();
    addChild(new EnhancedLayouts::GLSLConstantIntegralExpressionTest(m_context));
    addUniformBlockLayoutQualifierConflictTest();
    addSSBMemberInvalidOffsetAlignmentTest();
    addSSBMemberOverlappingOffsetsTest();
    addVaryingInvalidValueComponentTest();
    addVaryingExceedingComponentsTest();
    addVaryingComponentOfInvalidTypeTest();
    addOutputComponentAliasingTest();
    addChild(new EnhancedLayouts::VertexAttribLocationAPITest(m_context));
    addXFBInputTest();
    addChild(new EnhancedLayouts::XFBAllStagesTest(m_context));
    addChild(new EnhancedLayouts::XFBCaptureInactiveOutputVariableTest(m_context));
    addChild(new EnhancedLayouts::XFBCaptureInactiveOutputComponentTest(m_context));
    addChild(new EnhancedLayouts::XFBCaptureInactiveOutputBlockMemberTest(m_context));
    addXFBStrideTest();
    addChild(new EnhancedLayouts::UniformBlockMemberOffsetAndAlignTest(m_context));
    addUniformBlockMemberInvalidOffsetAlignmentTest();
    addUniformBlockMemberOverlappingOffsetsTest();
    addUniformBlockMemberAlignNonPowerOf2Test();
    addSSBLayoutQualifierConflictTest();
    addSSBMemberAlignNonPowerOf2Test();
    addChild(new EnhancedLayouts::SSBAlignmentTest(m_context));
    addVaryingStructureMemberLocationTest();
    addVaryingBlockAutomaticMemberLocationsTest();
    addVaryingComponentWithoutLocationTest();
    addInputComponentAliasingTest();
    addVaryingLocationAliasingWithMixedTypesTest();
    addVaryingLocationAliasingWithMixedInterpolationTest();
    addVaryingLocationAliasingWithMixedAuxiliaryStorageTest();
    addChild(new EnhancedLayouts::XFBStrideOfEmptyListTest(m_context));
    addChild(new EnhancedLayouts::XFBStrideOfEmptyListAndAPITest(m_context));
    addXFBTooSmallStrideTest();
    addChild(new EnhancedLayouts::XFBBlockMemberStrideTest(m_context));
    addXFBDuplicatedStrideTest();
    addXFBGetProgramResourceAPITest();
    addChild(new EnhancedLayouts::XFBMultipleVertexStreamsTest(m_context));
    addXFBExceedBufferLimitTest();
    addXFBExceedOffsetLimitTest();
    addXFBBlockMemberBufferTest();
    addXFBOutputOverlappingTest();
    addXFBInvalidOffsetAlignmentTest();
    addChild(new EnhancedLayouts::XFBCaptureStructTest(m_context));
    addXFBCaptureUnsizedArrayTest();
    addChild(new EnhancedLayouts::UniformBlockAlignmentTest(m_context));
    addChild(new EnhancedLayouts::SSBMemberOffsetAndAlignTest(m_context));
    addChild(new EnhancedLayouts::VertexAttribLocationsTest(m_context));
    addChild(new EnhancedLayouts::VaryingLocationsTest(m_context));
    addChild(new EnhancedLayouts::VaryingArrayLocationsTest(m_context));
    addChild(new EnhancedLayouts::VaryingStructureLocationsTest(m_context));
    addChild(new EnhancedLayouts::VaryingBlockLocationsTest(m_context));
    addVaryingBlockMemberLocationsTest();
    addXFBVariableStrideTest();
    addXFBBlockStrideTest();
    addChild(new EnhancedLayouts::XFBOverrideQualifiersWithAPITest(m_context));
    addChild(new EnhancedLayouts::XFBVertexStreamsTest(m_context));
    addXFBGlobalBufferTest();
    addXFBExplicitLocationTest();
    addXFBExplicitLocationStructTest();
    addChild(new EnhancedLayouts::FragmentDataLocationAPITest(m_context));
    addVaryingLocationLimitTest();
    addVaryingComponentsTest();
    addVaryingArrayComponentsTest();
}

void EnhancedLayoutsTests::addGLSLContantImmutablityTest()
{
    for (GLuint constant = 0; constant < EnhancedLayouts::GLSLContantImmutablityTest::CONSTANTS_MAX;
         ++constant)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            addChild(new EnhancedLayouts::GLSLContantImmutablityTest(m_context, constant, stage));
        }
    }
}

void EnhancedLayoutsTests::addUniformBlockLayoutQualifierConflictTest()
{
    for (GLuint qualifier = 0;
         qualifier < EnhancedLayouts::UniformBlockLayoutQualifierConflictTest::QUALIFIERS_MAX;
         ++qualifier)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            addChild(new EnhancedLayouts::UniformBlockLayoutQualifierConflictTest(
                m_context, qualifier, stage));
        }
    }
}

void EnhancedLayoutsTests::addSSBMemberInvalidOffsetAlignmentTest()
{
    for (GLuint type = 0; type < EnhancedLayouts::TYPES_NUMBER; ++type)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            EnhancedLayouts::SSBMemberInvalidOffsetAlignmentTest *test =
                new EnhancedLayouts::SSBMemberInvalidOffsetAlignmentTest(m_context, type, stage);
            if (test->stageAllowed(stage))
            {
                addChild(test);
            }
            else
            {
                delete test;
            }
        }
    }
}

void EnhancedLayoutsTests::addSSBMemberOverlappingOffsetsTest()
{
    for (GLuint type_i = 0; type_i < EnhancedLayouts::TYPES_NUMBER; ++type_i)
    {
        for (GLuint type_j = 0; type_j < EnhancedLayouts::TYPES_NUMBER; ++type_j)
        {
            addChild(
                new EnhancedLayouts::SSBMemberOverlappingOffsetsTest(m_context, type_i, type_j));
        }
    }
}

void EnhancedLayoutsTests::addVaryingInvalidValueComponentTest()
{
    for (GLuint type_num = 0; type_num < EnhancedLayouts::TYPES_NUMBER; ++type_num)
    {
        EnhancedLayouts::VaryingInvalidValueComponentTest *test =
            new EnhancedLayouts::VaryingInvalidValueComponentTest(m_context, type_num);

        const EnhancedLayouts::Utils::Type &type    = test->getTypeHelper(type_num);
        const std::vector<GLuint> &valid_components = type.GetValidComponents();

        if (valid_components.empty())
        {
            delete test;
        }
        else
        {
            addChild(test);
        }
    }
}

void EnhancedLayoutsTests::addVaryingExceedingComponentsTest()
{
    for (GLuint type_num = 0; type_num < EnhancedLayouts::TYPES_NUMBER; ++type_num)
    {
        EnhancedLayouts::VaryingExceedingComponentsTest *test =
            new EnhancedLayouts::VaryingExceedingComponentsTest(m_context, type_num);

        const EnhancedLayouts::Utils::Type &type    = test->getTypeHelper(type_num);
        const std::vector<GLuint> &valid_components = type.GetValidComponents();

        if (valid_components.empty())
        {
            delete test;
        }
        else
        {
            addChild(test);
        }
    }
}

void EnhancedLayoutsTests::addVaryingComponentOfInvalidTypeTest()
{
    for (GLuint type = 0; type < EnhancedLayouts::TYPES_NUMBER; ++type)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            if (EnhancedLayouts::Utils::Shader::COMPUTE == stage)
            {
                continue;
            }
            addChild(
                new EnhancedLayouts::VaryingComponentOfInvalidTypeTest(m_context, type, stage));
        }
    }
}

void EnhancedLayoutsTests::addOutputComponentAliasingTest()
{
    for (GLuint type_num = 0; type_num < EnhancedLayouts::TYPES_NUMBER; ++type_num)
    {
        EnhancedLayouts::OutputComponentAliasingTest *test =
            new EnhancedLayouts::OutputComponentAliasingTest(m_context, type_num);

        const EnhancedLayouts::Utils::Type &type    = test->getTypeHelper(type_num);
        const std::vector<GLuint> &valid_components = type.GetValidComponents();

        if (valid_components.empty())
        {
            delete test;
        }
        else
        {
            addChild(test);
        }
    }
}

void EnhancedLayoutsTests::addXFBInputTest()
{
    for (GLuint qualifier = 0; qualifier < EnhancedLayouts::XFBInputTest::QUALIFIERS_MAX;
         ++qualifier)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            if (EnhancedLayouts::Utils::Shader::COMPUTE == stage)
            {
                continue;
            }
            addChild(new EnhancedLayouts::XFBInputTest(m_context, qualifier, stage));
        }
    }
}

void EnhancedLayoutsTests::addXFBStrideTest()
{
    for (GLuint type = 0; type < EnhancedLayouts::TYPES_NUMBER; ++type)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            if ((EnhancedLayouts::Utils::Shader::COMPUTE == stage) ||
                (EnhancedLayouts::Utils::Shader::FRAGMENT == stage) ||
                (EnhancedLayouts::Utils::Shader::TESS_CTRL == stage))
            {
                continue;
            }
            addChild(new EnhancedLayouts::XFBStrideTest(m_context, type, stage));
        }
    }
}

void EnhancedLayoutsTests::addUniformBlockMemberInvalidOffsetAlignmentTest()
{
    for (GLuint type = 0; type < EnhancedLayouts::TYPES_NUMBER; ++type)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            EnhancedLayouts::UniformBlockMemberInvalidOffsetAlignmentTest *test =
                new EnhancedLayouts::UniformBlockMemberInvalidOffsetAlignmentTest(m_context, type,
                                                                                  stage);
            if (test->stageAllowed(stage))
            {
                addChild(test);
            }
            else
            {
                delete test;
            }
        }
    }
}

void EnhancedLayoutsTests::addUniformBlockMemberOverlappingOffsetsTest()
{
    for (GLuint type_i = 0; type_i < EnhancedLayouts::TYPES_NUMBER; ++type_i)
    {
        for (GLuint type_j = 0; type_j < EnhancedLayouts::TYPES_NUMBER; ++type_j)
        {
            addChild(new EnhancedLayouts::UniformBlockMemberOverlappingOffsetsTest(m_context,
                                                                                   type_i, type_j));
        }
    }
}

void EnhancedLayoutsTests::addUniformBlockMemberAlignNonPowerOf2Test()
{
    for (GLuint type = 0; type < EnhancedLayouts::TYPES_NUMBER; ++type)
    {
        addChild(new EnhancedLayouts::UniformBlockMemberAlignNonPowerOf2Test(m_context, type));
    }
}

void EnhancedLayoutsTests::addSSBMemberAlignNonPowerOf2Test()
{
    for (GLuint type = 0; type < EnhancedLayouts::TYPES_NUMBER; ++type)
    {
        addChild(new EnhancedLayouts::SSBMemberAlignNonPowerOf2Test(m_context, type));
    }
}

void EnhancedLayoutsTests::addSSBLayoutQualifierConflictTest()
{
    for (GLuint qualifier = 0;
         qualifier < EnhancedLayouts::SSBLayoutQualifierConflictTest::QUALIFIERS_MAX; ++qualifier)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            EnhancedLayouts::SSBLayoutQualifierConflictTest *test =
                new EnhancedLayouts::SSBLayoutQualifierConflictTest(m_context, qualifier, stage);
            if (test->stageAllowed(stage))
            {
                addChild(test);
            }
            else
            {
                delete test;
            }
        }
    }
}

void EnhancedLayoutsTests::addVaryingStructureMemberLocationTest()
{

    for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
    {
        if (EnhancedLayouts::Utils::Shader::COMPUTE == stage)
        {
            continue;
        }
        addChild(new EnhancedLayouts::VaryingStructureMemberLocationTest(m_context, stage));
    }
}

void EnhancedLayoutsTests::addVaryingBlockAutomaticMemberLocationsTest()
{

    for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
    {
        if (EnhancedLayouts::Utils::Shader::COMPUTE == stage)
        {
            continue;
        }
        addChild(new EnhancedLayouts::VaryingBlockAutomaticMemberLocationsTest(m_context, stage));
    }
}

void EnhancedLayoutsTests::addVaryingComponentWithoutLocationTest()
{
    for (GLuint type_num = 0; type_num < EnhancedLayouts::TYPES_NUMBER; ++type_num)
    {
        EnhancedLayouts::VaryingComponentWithoutLocationTest *test =
            new EnhancedLayouts::VaryingComponentWithoutLocationTest(m_context, type_num);

        const EnhancedLayouts::Utils::Type &type    = test->getTypeHelper(type_num);
        const std::vector<GLuint> &valid_components = type.GetValidComponents();

        if (valid_components.empty())
        {
            delete test;
        }
        else
        {
            addChild(test);
        }
    }
}

void EnhancedLayoutsTests::addInputComponentAliasingTest()
{
    for (GLuint type_num = 0; type_num < EnhancedLayouts::TYPES_NUMBER; ++type_num)
    {
        EnhancedLayouts::InputComponentAliasingTest *test =
            new EnhancedLayouts::InputComponentAliasingTest(m_context, type_num);

        const EnhancedLayouts::Utils::Type &type    = test->getTypeHelper(type_num);
        const std::vector<GLuint> &valid_components = type.GetValidComponents();

        if (valid_components.empty())
        {
            delete test;
        }
        else
        {
            addChild(test);
        }
    }
}

void EnhancedLayoutsTests::addVaryingLocationAliasingWithMixedTypesTest()
{
    for (GLuint type_gohan = 0; type_gohan < EnhancedLayouts::TYPES_NUMBER; ++type_gohan)
    {
        for (GLuint type_goten = 0; type_goten < EnhancedLayouts::TYPES_NUMBER; ++type_goten)
        {
            EnhancedLayouts::VaryingLocationAliasingWithMixedTypesTest *test =
                new EnhancedLayouts::VaryingLocationAliasingWithMixedTypesTest(
                    m_context, type_gohan, type_goten);

            const EnhancedLayouts::Utils::Type &tp_gohan      = test->getTypeHelper(type_gohan);
            const std::vector<GLuint> &valid_components_gohan = tp_gohan.GetValidComponents();
            const EnhancedLayouts::Utils::Type &tp_goten      = test->getTypeHelper(type_goten);
            const std::vector<GLuint> &valid_components_goten = tp_goten.GetValidComponents();

            if (valid_components_gohan.empty() || valid_components_goten.empty() ||
                EnhancedLayouts::Utils::Type::CanTypesShareLocation(tp_gohan.m_basic_type,
                                                                    tp_goten.m_basic_type))
            {
                delete test;
            }
            else
            {
                addChild(test);
            }
        }
    }
}

void EnhancedLayoutsTests::addVaryingLocationAliasingWithMixedInterpolationTest()
{
    for (GLuint type_gohan = 0; type_gohan < EnhancedLayouts::TYPES_NUMBER; ++type_gohan)
    {
        for (GLuint type_goten = 0; type_goten < EnhancedLayouts::TYPES_NUMBER; ++type_goten)
        {
            EnhancedLayouts::VaryingLocationAliasingWithMixedInterpolationTest *test =
                new EnhancedLayouts::VaryingLocationAliasingWithMixedInterpolationTest(
                    m_context, type_gohan, type_goten);

            const EnhancedLayouts::Utils::Type &tp_gohan      = test->getTypeHelper(type_gohan);
            const std::vector<GLuint> &valid_components_gohan = tp_gohan.GetValidComponents();
            const EnhancedLayouts::Utils::Type &tp_goten      = test->getTypeHelper(type_goten);
            const std::vector<GLuint> &valid_components_goten = tp_goten.GetValidComponents();

            if (valid_components_gohan.empty() || valid_components_goten.empty())
            {
                delete test;
            }
            else
            {
                const GLuint gohan         = valid_components_gohan.front();
                const GLuint min_component = gohan + tp_gohan.GetNumComponents();
                const GLuint goten         = valid_components_goten.back();
                if ((min_component > goten) ||
                    (!EnhancedLayouts::Utils::Type::CanTypesShareLocation(tp_gohan.m_basic_type,
                                                                          tp_goten.m_basic_type)))
                {
                    delete test;
                }
                else
                {
                    addChild(test);
                }
            }
        }
    }
}

void EnhancedLayoutsTests::addVaryingLocationAliasingWithMixedAuxiliaryStorageTest()
{
    for (GLuint type_gohan = 0; type_gohan < EnhancedLayouts::TYPES_NUMBER; ++type_gohan)
    {
        for (GLuint type_goten = 0; type_goten < EnhancedLayouts::TYPES_NUMBER; ++type_goten)
        {
            EnhancedLayouts::VaryingLocationAliasingWithMixedAuxiliaryStorageTest *test =
                new EnhancedLayouts::VaryingLocationAliasingWithMixedAuxiliaryStorageTest(
                    m_context, type_gohan, type_goten);

            const EnhancedLayouts::Utils::Type &tp_gohan      = test->getTypeHelper(type_gohan);
            const std::vector<GLuint> &valid_components_gohan = tp_gohan.GetValidComponents();
            const EnhancedLayouts::Utils::Type &tp_goten      = test->getTypeHelper(type_goten);
            const std::vector<GLuint> &valid_components_goten = tp_goten.GetValidComponents();

            if (valid_components_gohan.empty() || valid_components_goten.empty())
            {
                delete test;
            }
            else
            {
                const GLuint gohan         = valid_components_gohan.front();
                const GLuint min_component = gohan + tp_gohan.GetNumComponents();
                const GLuint goten         = valid_components_goten.back();
                if ((min_component > goten) ||
                    (!EnhancedLayouts::Utils::Type::CanTypesShareLocation(tp_gohan.m_basic_type,
                                                                          tp_goten.m_basic_type)))
                {
                    delete test;
                }
                else
                {
                    addChild(test);
                }
            }
        }
    }
}

void EnhancedLayoutsTests::addXFBTooSmallStrideTest()
{
    for (GLuint constant = 0; constant < EnhancedLayouts::XFBTooSmallStrideTest::CASES::CASE_MAX;
         ++constant)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            /*
             It is invalid to define transform feedback output in TCS, according to spec:
             The data captured in transform feedback mode depends on the active programs on each of
             the shader stages. If a program is active for the geometry shader stage, transform
             feedback captures the vertices of each primitive emitted by the geometry shader.
             Otherwise, if a program is active for the tessellation evaluation shader stage,
             transform feedback captures each primitive produced by the tessellation primitive
             generator, whose vertices are processed by the tessellation evaluation shader.
             Otherwise, transform feedback captures each primitive processed by the vertex shader.
             */
            if ((EnhancedLayouts::Utils::Shader::COMPUTE == stage) ||
                (EnhancedLayouts::Utils::Shader::TESS_CTRL == stage) ||
                (EnhancedLayouts::Utils::Shader::FRAGMENT == stage))
            {
                continue;
            }
            addChild(new EnhancedLayouts::XFBTooSmallStrideTest(m_context, constant, stage));
        }
    }
}

void EnhancedLayoutsTests::addXFBDuplicatedStrideTest()
{
    for (GLuint constant = 0; constant < EnhancedLayouts::XFBDuplicatedStrideTest::CASES::CASE_MAX;
         ++constant)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            if ((EnhancedLayouts::Utils::Shader::COMPUTE == stage) ||
                (EnhancedLayouts::Utils::Shader::TESS_CTRL == stage) ||
                (EnhancedLayouts::Utils::Shader::FRAGMENT == stage))
            {
                continue;
            }
            addChild(new EnhancedLayouts::XFBDuplicatedStrideTest(m_context, constant, stage));
        }
    }
}

void EnhancedLayoutsTests::addXFBGetProgramResourceAPITest()
{
    for (GLuint type = 0; type < EnhancedLayouts::TYPES_NUMBER; ++type)
    {
        // When i == 7, the type is dmat4, i == 9 the type is dmat4x3, the number of output
        // components exceeds the maximum value that AMD's driver supported
        if ((type == 7 || type == 9))
        {
            continue;
        }
        addChild(new EnhancedLayouts::XFBGetProgramResourceAPITest(m_context, type));
    }
}

void EnhancedLayoutsTests::addXFBExceedBufferLimitTest()
{
    for (GLuint constant = 0; constant < EnhancedLayouts::XFBExceedBufferLimitTest::CASES::CASE_MAX;
         ++constant)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            if ((EnhancedLayouts::Utils::Shader::COMPUTE == stage) ||
                (EnhancedLayouts::Utils::Shader::TESS_CTRL == stage) ||
                (EnhancedLayouts::Utils::Shader::FRAGMENT == stage))
            {
                continue;
            }
            addChild(new EnhancedLayouts::XFBExceedBufferLimitTest(m_context, constant, stage));
        }
    }
}

void EnhancedLayoutsTests::addXFBExceedOffsetLimitTest()
{
    for (GLuint constant = 0; constant < EnhancedLayouts::XFBExceedOffsetLimitTest::CASES::CASE_MAX;
         ++constant)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            if ((EnhancedLayouts::Utils::Shader::COMPUTE == stage) ||
                (EnhancedLayouts::Utils::Shader::TESS_CTRL == stage) ||
                (EnhancedLayouts::Utils::Shader::FRAGMENT == stage))
            {
                continue;
            }
            addChild(new EnhancedLayouts::XFBExceedOffsetLimitTest(m_context, constant, stage));
        }
    }
}

void EnhancedLayoutsTests::addXFBBlockMemberBufferTest()
{
    for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
    {
        if ((EnhancedLayouts::Utils::Shader::COMPUTE == stage) ||
            (EnhancedLayouts::Utils::Shader::TESS_CTRL == stage) ||
            (EnhancedLayouts::Utils::Shader::FRAGMENT == stage))
        {
            continue;
        }
        addChild(new EnhancedLayouts::XFBBlockMemberBufferTest(m_context, stage));
    }
}

void EnhancedLayoutsTests::addXFBOutputOverlappingTest()
{
    for (GLuint type = 0; type < EnhancedLayouts::TYPES_NUMBER; ++type)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            EnhancedLayouts::XFBOutputOverlappingTest *test =
                new EnhancedLayouts::XFBOutputOverlappingTest(m_context, type, stage);

            const EnhancedLayouts::Utils::Type &utils_type = test->getTypeHelper(type);

            if (((1 == utils_type.m_n_columns) && (1 == utils_type.m_n_rows)) ||
                ((EnhancedLayouts::Utils::Shader::COMPUTE == stage) ||
                 (EnhancedLayouts::Utils::Shader::TESS_CTRL == stage) ||
                 (EnhancedLayouts::Utils::Shader::FRAGMENT == stage)))
            {
                delete test;
            }
            else
            {
                addChild(test);
            }
        }
    }
}

void EnhancedLayoutsTests::addXFBInvalidOffsetAlignmentTest()
{
    for (GLuint type = 0; type < EnhancedLayouts::TYPES_NUMBER; ++type)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            if (((EnhancedLayouts::Utils::Shader::COMPUTE == stage) ||
                 (EnhancedLayouts::Utils::Shader::TESS_CTRL == stage) ||
                 (EnhancedLayouts::Utils::Shader::FRAGMENT == stage)))
            {
                continue;
            }
            else
            {
                addChild(
                    new EnhancedLayouts::XFBInvalidOffsetAlignmentTest(m_context, type, stage));
            }
        }
    }
}

void EnhancedLayoutsTests::addXFBCaptureUnsizedArrayTest()
{
    for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
    {
        if ((EnhancedLayouts::Utils::Shader::COMPUTE == stage) ||
            (EnhancedLayouts::Utils::Shader::TESS_CTRL == stage) ||
            (EnhancedLayouts::Utils::Shader::FRAGMENT == stage) ||
            (EnhancedLayouts::Utils::Shader::GEOMETRY == stage) ||
            (EnhancedLayouts::Utils::Shader::TESS_EVAL == stage))
        {
            continue;
        }
        addChild(new EnhancedLayouts::XFBCaptureUnsizedArrayTest(m_context, stage));
    }
}

void EnhancedLayoutsTests::addVaryingBlockMemberLocationsTest()
{
    for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
    {
        if (EnhancedLayouts::Utils::Shader::COMPUTE == stage)
        {
            continue;
        }
        addChild(new EnhancedLayouts::VaryingBlockMemberLocationsTest(m_context, stage));
    }
}

void EnhancedLayoutsTests::addXFBVariableStrideTest()
{
    for (GLuint type = 0; type < EnhancedLayouts::TYPES_NUMBER; ++type)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            if ((EnhancedLayouts::Utils::Shader::COMPUTE == stage) ||
                (EnhancedLayouts::Utils::Shader::TESS_CTRL == stage) ||
                (EnhancedLayouts::Utils::Shader::FRAGMENT == stage))
            {
                continue;
            }
            for (GLuint constant = 0;
                 constant < EnhancedLayouts::XFBVariableStrideTest::CASES::CASE_MAX; ++constant)
            {
                addChild(
                    new EnhancedLayouts::XFBVariableStrideTest(m_context, type, stage, constant));
            }
        }
    }
}

void EnhancedLayoutsTests::addXFBBlockStrideTest()
{
    for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
    {
        if ((EnhancedLayouts::Utils::Shader::COMPUTE == stage) ||
            (EnhancedLayouts::Utils::Shader::TESS_CTRL == stage) ||
            (EnhancedLayouts::Utils::Shader::FRAGMENT == stage))
        {
            continue;
        }
        addChild(new EnhancedLayouts::XFBBlockStrideTest(m_context, stage));
    }
}

void EnhancedLayoutsTests::addXFBGlobalBufferTest()
{
    for (GLuint type = 0; type < EnhancedLayouts::TYPES_NUMBER; ++type)
    {
        EnhancedLayouts::XFBGlobalBufferTest *test =
            new EnhancedLayouts::XFBGlobalBufferTest(m_context, type);
        const EnhancedLayouts::Utils::Type &utils_type = test->getTypeHelper(type);
        /*
         When the tfx varying is the following type, the number of output exceeds the
         gl_MaxVaryingComponents, which will cause a link time error.
         */
        if (strcmp(utils_type.GetGLSLTypeName(), "dmat3") == 0 ||
            strcmp(utils_type.GetGLSLTypeName(), "dmat4") == 0 ||
            strcmp(utils_type.GetGLSLTypeName(), "dmat3x4") == 0 ||
            strcmp(utils_type.GetGLSLTypeName(), "dmat4x3") == 0)
        {
            delete test;
        }
        else
        {
            addChild(test);
        }
    }
}

void EnhancedLayoutsTests::addXFBExplicitLocationTest()
{
    for (GLuint type = 0; type < EnhancedLayouts::TYPES_NUMBER; ++type)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            if ((EnhancedLayouts::Utils::Shader::COMPUTE == stage) ||
                (EnhancedLayouts::Utils::Shader::FRAGMENT == stage) ||
                (EnhancedLayouts::Utils::Shader::TESS_CTRL == stage))
            {
                continue;
            }
            else
            {
                addChild(new EnhancedLayouts::XFBExplicitLocationTest(m_context, type, stage));
            }
        }
    }
}

void EnhancedLayoutsTests::addXFBExplicitLocationStructTest()
{
    for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
    {
        if ((EnhancedLayouts::Utils::Shader::COMPUTE == stage) ||
            (EnhancedLayouts::Utils::Shader::FRAGMENT == stage) ||
            (EnhancedLayouts::Utils::Shader::TESS_CTRL == stage))
        {
            continue;
        }
        else
        {
            addChild(new EnhancedLayouts::XFBExplicitLocationStructTest(m_context, stage));
        }
    }
}

void EnhancedLayoutsTests::addVaryingLocationLimitTest()
{
    for (GLuint type = 0; type < EnhancedLayouts::TYPES_NUMBER; ++type)
    {
        for (GLuint stage = 0; stage < EnhancedLayouts::Utils::Shader::STAGE_MAX; ++stage)
        {
            if (EnhancedLayouts::Utils::Shader::COMPUTE == stage)
            {
                continue;
            }
            else
            {
                addChild(new EnhancedLayouts::VaryingLocationLimitTest(m_context, type, stage));
            }
        }
    }
}

template <class varyingClass>
void EnhancedLayoutsTests::addVaryingTest()
{
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::G64VEC2,
                              EnhancedLayouts::Utils::Type::Double));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::G64SCALAR_G64SCALAR,
                              EnhancedLayouts::Utils::Type::Double));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::GVEC4,
                              EnhancedLayouts::Utils::Type::Float));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::SCALAR_GVEC3,
                              EnhancedLayouts::Utils::Type::Float));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::GVEC3_SCALAR,
                              EnhancedLayouts::Utils::Type::Float));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::GVEC2_GVEC2,
                              EnhancedLayouts::Utils::Type::Float));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::GVEC2_SCALAR_SCALAR,
                              EnhancedLayouts::Utils::Type::Float));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::SCALAR_GVEC2_SCALAR,
                              EnhancedLayouts::Utils::Type::Float));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::SCALAR_SCALAR_GVEC2,
                              EnhancedLayouts::Utils::Type::Float));
    addChild(new varyingClass(m_context,
                              varyingClass::COMPONENTS_LAYOUT::SCALAR_SCALAR_SCALAR_SCALAR,
                              EnhancedLayouts::Utils::Type::Float));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::GVEC4,
                              EnhancedLayouts::Utils::Type::Int));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::SCALAR_GVEC3,
                              EnhancedLayouts::Utils::Type::Int));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::GVEC3_SCALAR,
                              EnhancedLayouts::Utils::Type::Int));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::GVEC2_GVEC2,
                              EnhancedLayouts::Utils::Type::Int));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::GVEC2_SCALAR_SCALAR,
                              EnhancedLayouts::Utils::Type::Int));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::SCALAR_GVEC2_SCALAR,
                              EnhancedLayouts::Utils::Type::Int));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::SCALAR_SCALAR_GVEC2,
                              EnhancedLayouts::Utils::Type::Int));
    addChild(new varyingClass(m_context,
                              varyingClass::COMPONENTS_LAYOUT::SCALAR_SCALAR_SCALAR_SCALAR,
                              EnhancedLayouts::Utils::Type::Int));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::GVEC4,
                              EnhancedLayouts::Utils::Type::Uint));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::SCALAR_GVEC3,
                              EnhancedLayouts::Utils::Type::Uint));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::GVEC3_SCALAR,
                              EnhancedLayouts::Utils::Type::Uint));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::GVEC2_GVEC2,
                              EnhancedLayouts::Utils::Type::Uint));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::GVEC2_SCALAR_SCALAR,
                              EnhancedLayouts::Utils::Type::Uint));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::SCALAR_GVEC2_SCALAR,
                              EnhancedLayouts::Utils::Type::Uint));
    addChild(new varyingClass(m_context, varyingClass::COMPONENTS_LAYOUT::SCALAR_SCALAR_GVEC2,
                              EnhancedLayouts::Utils::Type::Uint));
    addChild(new varyingClass(m_context,
                              varyingClass::COMPONENTS_LAYOUT::SCALAR_SCALAR_SCALAR_SCALAR,
                              EnhancedLayouts::Utils::Type::Uint));
}

void EnhancedLayoutsTests::addVaryingComponentsTest()
{
    addVaryingTest<EnhancedLayouts::VaryingComponentsTest>();
}

void EnhancedLayoutsTests::addVaryingArrayComponentsTest()
{
    addVaryingTest<EnhancedLayouts::VaryingArrayComponentsTest>();
}

}  // namespace gl4cts
