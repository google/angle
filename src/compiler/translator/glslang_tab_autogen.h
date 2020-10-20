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
    SAMPLE                    = 308,
    CENTROID                  = 309,
    FLAT                      = 310,
    SMOOTH                    = 311,
    NOPERSPECTIVE             = 312,
    READONLY                  = 313,
    WRITEONLY                 = 314,
    COHERENT                  = 315,
    RESTRICT                  = 316,
    VOLATILE                  = 317,
    SHARED                    = 318,
    STRUCT                    = 319,
    VOID_TYPE                 = 320,
    WHILE                     = 321,
    SAMPLER2D                 = 322,
    SAMPLERCUBE               = 323,
    SAMPLER_EXTERNAL_OES      = 324,
    SAMPLER2DRECT             = 325,
    SAMPLER2DARRAY            = 326,
    ISAMPLER2D                = 327,
    ISAMPLER3D                = 328,
    ISAMPLERCUBE              = 329,
    ISAMPLER2DARRAY           = 330,
    USAMPLER2D                = 331,
    USAMPLER3D                = 332,
    USAMPLERCUBE              = 333,
    USAMPLER2DARRAY           = 334,
    SAMPLER2DMS               = 335,
    ISAMPLER2DMS              = 336,
    USAMPLER2DMS              = 337,
    SAMPLER2DMSARRAY          = 338,
    ISAMPLER2DMSARRAY         = 339,
    USAMPLER2DMSARRAY         = 340,
    SAMPLER3D                 = 341,
    SAMPLER3DRECT             = 342,
    SAMPLER2DSHADOW           = 343,
    SAMPLERCUBESHADOW         = 344,
    SAMPLER2DARRAYSHADOW      = 345,
    SAMPLERVIDEOWEBGL         = 346,
    SAMPLERCUBEARRAYOES       = 347,
    SAMPLERCUBEARRAYSHADOWOES = 348,
    ISAMPLERCUBEARRAYOES      = 349,
    USAMPLERCUBEARRAYOES      = 350,
    SAMPLERCUBEARRAYEXT       = 351,
    SAMPLERCUBEARRAYSHADOWEXT = 352,
    ISAMPLERCUBEARRAYEXT      = 353,
    USAMPLERCUBEARRAYEXT      = 354,
    SAMPLEREXTERNAL2DY2YEXT   = 355,
    IMAGE2D                   = 356,
    IIMAGE2D                  = 357,
    UIMAGE2D                  = 358,
    IMAGE3D                   = 359,
    IIMAGE3D                  = 360,
    UIMAGE3D                  = 361,
    IMAGE2DARRAY              = 362,
    IIMAGE2DARRAY             = 363,
    UIMAGE2DARRAY             = 364,
    IMAGECUBE                 = 365,
    IIMAGECUBE                = 366,
    UIMAGECUBE                = 367,
    IMAGECUBEARRAYOES         = 368,
    IIMAGECUBEARRAYOES        = 369,
    UIMAGECUBEARRAYOES        = 370,
    IMAGECUBEARRAYEXT         = 371,
    IIMAGECUBEARRAYEXT        = 372,
    UIMAGECUBEARRAYEXT        = 373,
    ATOMICUINT                = 374,
    LAYOUT                    = 375,
    YUVCSCSTANDARDEXT         = 376,
    YUVCSCSTANDARDEXTCONSTANT = 377,
    IDENTIFIER                = 378,
    TYPE_NAME                 = 379,
    FLOATCONSTANT             = 380,
    INTCONSTANT               = 381,
    UINTCONSTANT              = 382,
    BOOLCONSTANT              = 383,
    FIELD_SELECTION           = 384,
    LEFT_OP                   = 385,
    RIGHT_OP                  = 386,
    INC_OP                    = 387,
    DEC_OP                    = 388,
    LE_OP                     = 389,
    GE_OP                     = 390,
    EQ_OP                     = 391,
    NE_OP                     = 392,
    AND_OP                    = 393,
    OR_OP                     = 394,
    XOR_OP                    = 395,
    MUL_ASSIGN                = 396,
    DIV_ASSIGN                = 397,
    ADD_ASSIGN                = 398,
    MOD_ASSIGN                = 399,
    LEFT_ASSIGN               = 400,
    RIGHT_ASSIGN              = 401,
    AND_ASSIGN                = 402,
    XOR_ASSIGN                = 403,
    OR_ASSIGN                 = 404,
    SUB_ASSIGN                = 405,
    LEFT_PAREN                = 406,
    RIGHT_PAREN               = 407,
    LEFT_BRACKET              = 408,
    RIGHT_BRACKET             = 409,
    LEFT_BRACE                = 410,
    RIGHT_BRACE               = 411,
    DOT                       = 412,
    COMMA                     = 413,
    COLON                     = 414,
    EQUAL                     = 415,
    SEMICOLON                 = 416,
    BANG                      = 417,
    DASH                      = 418,
    TILDE                     = 419,
    PLUS                      = 420,
    STAR                      = 421,
    SLASH                     = 422,
    PERCENT                   = 423,
    LEFT_ANGLE                = 424,
    RIGHT_ANGLE               = 425,
    VERTICAL_BAR              = 426,
    CARET                     = 427,
    AMPERSAND                 = 428,
    QUESTION                  = 429
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
