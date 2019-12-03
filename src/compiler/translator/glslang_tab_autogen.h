/* A Bison parser, made by GNU Bison 3.3.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_GLSLANG_TAB_AUTOGEN_H_INCLUDED
#define YY_YY_GLSLANG_TAB_AUTOGEN_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
#    define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */

#define YYLTYPE TSourceLoc
#define YYLTYPE_IS_DECLARED 1
#define YYLTYPE_IS_TRIVIAL 1

/* Token type.  */
#ifndef YYTOKENTYPE
#    define YYTOKENTYPE
enum yytokentype
{
    INVARIANT                 = 258,
    PRECISE                   = 259,
    HIGH_PRECISION            = 260,
    MEDIUM_PRECISION          = 261,
    LOW_PRECISION             = 262,
    PRECISION                 = 263,
    ATTRIBUTE                 = 264,
    CONST_QUAL                = 265,
    BOOL_TYPE                 = 266,
    FLOAT_TYPE                = 267,
    INT_TYPE                  = 268,
    UINT_TYPE                 = 269,
    BREAK                     = 270,
    CONTINUE                  = 271,
    DO                        = 272,
    ELSE                      = 273,
    FOR                       = 274,
    IF                        = 275,
    DISCARD                   = 276,
    RETURN                    = 277,
    SWITCH                    = 278,
    CASE                      = 279,
    DEFAULT                   = 280,
    BVEC2                     = 281,
    BVEC3                     = 282,
    BVEC4                     = 283,
    IVEC2                     = 284,
    IVEC3                     = 285,
    IVEC4                     = 286,
    VEC2                      = 287,
    VEC3                      = 288,
    VEC4                      = 289,
    UVEC2                     = 290,
    UVEC3                     = 291,
    UVEC4                     = 292,
    MATRIX2                   = 293,
    MATRIX3                   = 294,
    MATRIX4                   = 295,
    IN_QUAL                   = 296,
    OUT_QUAL                  = 297,
    INOUT_QUAL                = 298,
    UNIFORM                   = 299,
    BUFFER                    = 300,
    VARYING                   = 301,
    MATRIX2x3                 = 302,
    MATRIX3x2                 = 303,
    MATRIX2x4                 = 304,
    MATRIX4x2                 = 305,
    MATRIX3x4                 = 306,
    MATRIX4x3                 = 307,
    CENTROID                  = 308,
    FLAT                      = 309,
    SMOOTH                    = 310,
    READONLY                  = 311,
    WRITEONLY                 = 312,
    COHERENT                  = 313,
    RESTRICT                  = 314,
    VOLATILE                  = 315,
    SHARED                    = 316,
    STRUCT                    = 317,
    VOID_TYPE                 = 318,
    WHILE                     = 319,
    SAMPLER2D                 = 320,
    SAMPLERCUBE               = 321,
    SAMPLER_EXTERNAL_OES      = 322,
    SAMPLER2DRECT             = 323,
    SAMPLER2DARRAY            = 324,
    ISAMPLER2D                = 325,
    ISAMPLER3D                = 326,
    ISAMPLERCUBE              = 327,
    ISAMPLER2DARRAY           = 328,
    USAMPLER2D                = 329,
    USAMPLER3D                = 330,
    USAMPLERCUBE              = 331,
    USAMPLER2DARRAY           = 332,
    SAMPLER2DMS               = 333,
    ISAMPLER2DMS              = 334,
    USAMPLER2DMS              = 335,
    SAMPLER2DMSARRAY          = 336,
    ISAMPLER2DMSARRAY         = 337,
    USAMPLER2DMSARRAY         = 338,
    SAMPLER3D                 = 339,
    SAMPLER3DRECT             = 340,
    SAMPLER2DSHADOW           = 341,
    SAMPLERCUBESHADOW         = 342,
    SAMPLER2DARRAYSHADOW      = 343,
    SAMPLEREXTERNAL2DY2YEXT   = 344,
    IMAGE2D                   = 345,
    IIMAGE2D                  = 346,
    UIMAGE2D                  = 347,
    IMAGE3D                   = 348,
    IIMAGE3D                  = 349,
    UIMAGE3D                  = 350,
    IMAGE2DARRAY              = 351,
    IIMAGE2DARRAY             = 352,
    UIMAGE2DARRAY             = 353,
    IMAGECUBE                 = 354,
    IIMAGECUBE                = 355,
    UIMAGECUBE                = 356,
    ATOMICUINT                = 357,
    LAYOUT                    = 358,
    YUVCSCSTANDARDEXT         = 359,
    YUVCSCSTANDARDEXTCONSTANT = 360,
    IDENTIFIER                = 361,
    TYPE_NAME                 = 362,
    FLOATCONSTANT             = 363,
    INTCONSTANT               = 364,
    UINTCONSTANT              = 365,
    BOOLCONSTANT              = 366,
    FIELD_SELECTION           = 367,
    LEFT_OP                   = 368,
    RIGHT_OP                  = 369,
    INC_OP                    = 370,
    DEC_OP                    = 371,
    LE_OP                     = 372,
    GE_OP                     = 373,
    EQ_OP                     = 374,
    NE_OP                     = 375,
    AND_OP                    = 376,
    OR_OP                     = 377,
    XOR_OP                    = 378,
    MUL_ASSIGN                = 379,
    DIV_ASSIGN                = 380,
    ADD_ASSIGN                = 381,
    MOD_ASSIGN                = 382,
    LEFT_ASSIGN               = 383,
    RIGHT_ASSIGN              = 384,
    AND_ASSIGN                = 385,
    XOR_ASSIGN                = 386,
    OR_ASSIGN                 = 387,
    SUB_ASSIGN                = 388,
    LEFT_PAREN                = 389,
    RIGHT_PAREN               = 390,
    LEFT_BRACKET              = 391,
    RIGHT_BRACKET             = 392,
    LEFT_BRACE                = 393,
    RIGHT_BRACE               = 394,
    DOT                       = 395,
    COMMA                     = 396,
    COLON                     = 397,
    EQUAL                     = 398,
    SEMICOLON                 = 399,
    BANG                      = 400,
    DASH                      = 401,
    TILDE                     = 402,
    PLUS                      = 403,
    STAR                      = 404,
    SLASH                     = 405,
    PERCENT                   = 406,
    LEFT_ANGLE                = 407,
    RIGHT_ANGLE               = 408,
    VERTICAL_BAR              = 409,
    CARET                     = 410,
    AMPERSAND                 = 411,
    QUESTION                  = 412
};
#endif

/* Value type.  */
#if !defined YYSTYPE && !defined YYSTYPE_IS_DECLARED

union YYSTYPE
{

    struct
    {
        union
        {
            const char *string;  // pool allocated.
            float f;
            int i;
            unsigned int u;
            bool b;
        };
        const TSymbol *symbol;
    } lex;
    struct
    {
        TOperator op;
        union
        {
            TIntermNode *intermNode;
            TIntermNodePair nodePair;
            TIntermTyped *intermTypedNode;
            TIntermAggregate *intermAggregate;
            TIntermBlock *intermBlock;
            TIntermDeclaration *intermDeclaration;
            TIntermFunctionPrototype *intermFunctionPrototype;
            TIntermSwitch *intermSwitch;
            TIntermCase *intermCase;
        };
        union
        {
            TVector<unsigned int> *arraySizes;
            TTypeSpecifierNonArray typeSpecifierNonArray;
            TPublicType type;
            TPrecision precision;
            TLayoutQualifier layoutQualifier;
            TQualifier qualifier;
            TFunction *function;
            TFunctionLookup *functionLookup;
            TParameter param;
            TDeclarator *declarator;
            TDeclaratorList *declaratorList;
            TFieldList *fieldList;
            TQualifierWrapperBase *qualifierWrapper;
            TTypeQualifierBuilder *typeQualifierBuilder;
        };
    } interm;
};

typedef union YYSTYPE YYSTYPE;
#    define YYSTYPE_IS_TRIVIAL 1
#    define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if !defined YYLTYPE && !defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
    int first_line;
    int first_column;
    int last_line;
    int last_column;
};
#    define YYLTYPE_IS_DECLARED 1
#    define YYLTYPE_IS_TRIVIAL 1
#endif

int yyparse(TParseContext *context, void *scanner);

#endif /* !YY_YY_GLSLANG_TAB_AUTOGEN_H_INCLUDED  */
