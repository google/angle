/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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

#ifndef YY_YY_GLSLANG_TAB_H_INCLUDED
# define YY_YY_GLSLANG_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */


#define YYLTYPE TSourceLoc
#define YYLTYPE_IS_DECLARED 1



/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
enum yytokentype
{
    INVARIANT            = 258,
    HIGH_PRECISION       = 259,
    MEDIUM_PRECISION     = 260,
    LOW_PRECISION        = 261,
    PRECISION            = 262,
    ATTRIBUTE            = 263,
    CONST_QUAL           = 264,
    BOOL_TYPE            = 265,
    FLOAT_TYPE           = 266,
    INT_TYPE             = 267,
    UINT_TYPE            = 268,
    BREAK                = 269,
    CONTINUE             = 270,
    DO                   = 271,
    ELSE                 = 272,
    FOR                  = 273,
    IF                   = 274,
    DISCARD              = 275,
    RETURN               = 276,
    SWITCH               = 277,
    CASE                 = 278,
    DEFAULT              = 279,
    BVEC2                = 280,
    BVEC3                = 281,
    BVEC4                = 282,
    IVEC2                = 283,
    IVEC3                = 284,
    IVEC4                = 285,
    VEC2                 = 286,
    VEC3                 = 287,
    VEC4                 = 288,
    UVEC2                = 289,
    UVEC3                = 290,
    UVEC4                = 291,
    MATRIX2              = 292,
    MATRIX3              = 293,
    MATRIX4              = 294,
    IN_QUAL              = 295,
    OUT_QUAL             = 296,
    INOUT_QUAL           = 297,
    UNIFORM              = 298,
    VARYING              = 299,
    MATRIX2x3            = 300,
    MATRIX3x2            = 301,
    MATRIX2x4            = 302,
    MATRIX4x2            = 303,
    MATRIX3x4            = 304,
    MATRIX4x3            = 305,
    CENTROID             = 306,
    FLAT                 = 307,
    SMOOTH               = 308,
    READONLY             = 309,
    WRITEONLY            = 310,
    STRUCT               = 311,
    VOID_TYPE            = 312,
    WHILE                = 313,
    SAMPLER2D            = 314,
    SAMPLERCUBE          = 315,
    SAMPLER_EXTERNAL_OES = 316,
    SAMPLER2DRECT        = 317,
    SAMPLER2DARRAY       = 318,
    ISAMPLER2D           = 319,
    ISAMPLER3D           = 320,
    ISAMPLERCUBE         = 321,
    ISAMPLER2DARRAY      = 322,
    USAMPLER2D           = 323,
    USAMPLER3D           = 324,
    USAMPLERCUBE         = 325,
    USAMPLER2DARRAY      = 326,
    SAMPLER3D            = 327,
    SAMPLER3DRECT        = 328,
    SAMPLER2DSHADOW      = 329,
    SAMPLERCUBESHADOW    = 330,
    SAMPLER2DARRAYSHADOW = 331,
    IMAGE2D              = 332,
    IIMAGE2D             = 333,
    UIMAGE2D             = 334,
    IMAGE3D              = 335,
    IIMAGE3D             = 336,
    UIMAGE3D             = 337,
    IMAGE2DARRAY         = 338,
    IIMAGE2DARRAY        = 339,
    UIMAGE2DARRAY        = 340,
    IMAGECUBE            = 341,
    IIMAGECUBE           = 342,
    UIMAGECUBE           = 343,
    LAYOUT               = 344,
    IDENTIFIER           = 345,
    TYPE_NAME            = 346,
    FLOATCONSTANT        = 347,
    INTCONSTANT          = 348,
    UINTCONSTANT         = 349,
    BOOLCONSTANT         = 350,
    FIELD_SELECTION      = 351,
    LEFT_OP              = 352,
    RIGHT_OP             = 353,
    INC_OP               = 354,
    DEC_OP               = 355,
    LE_OP                = 356,
    GE_OP                = 357,
    EQ_OP                = 358,
    NE_OP                = 359,
    AND_OP               = 360,
    OR_OP                = 361,
    XOR_OP               = 362,
    MUL_ASSIGN           = 363,
    DIV_ASSIGN           = 364,
    ADD_ASSIGN           = 365,
    MOD_ASSIGN           = 366,
    LEFT_ASSIGN          = 367,
    RIGHT_ASSIGN         = 368,
    AND_ASSIGN           = 369,
    XOR_ASSIGN           = 370,
    OR_ASSIGN            = 371,
    SUB_ASSIGN           = 372,
    LEFT_PAREN           = 373,
    RIGHT_PAREN          = 374,
    LEFT_BRACKET         = 375,
    RIGHT_BRACKET        = 376,
    LEFT_BRACE           = 377,
    RIGHT_BRACE          = 378,
    DOT                  = 379,
    COMMA                = 380,
    COLON                = 381,
    EQUAL                = 382,
    SEMICOLON            = 383,
    BANG                 = 384,
    DASH                 = 385,
    TILDE                = 386,
    PLUS                 = 387,
    STAR                 = 388,
    SLASH                = 389,
    PERCENT              = 390,
    LEFT_ANGLE           = 391,
    RIGHT_ANGLE          = 392,
    VERTICAL_BAR         = 393,
    CARET                = 394,
    AMPERSAND            = 395,
    QUESTION             = 396
};
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{


    struct {
        union {
            TString *string;
            float f;
            int i;
            unsigned int u;
            bool b;
        };
        TSymbol* symbol;
    } lex;
    struct {
        TOperator op;
        union {
            TIntermNode* intermNode;
            TIntermNodePair nodePair;
            TIntermTyped* intermTypedNode;
            TIntermAggregate* intermAggregate;
            TIntermBlock *intermBlock;
            TIntermSwitch* intermSwitch;
            TIntermCase* intermCase;
        };
        union {
            TTypeSpecifierNonArray typeSpecifierNonArray;
            TPublicType type;
            TPrecision precision;
            TLayoutQualifier layoutQualifier;
            TQualifier qualifier;
            TFunction* function;
            TParameter param;
            TField* field;
            TFieldList* fieldList;
            TQualifierWrapperBase *qualifierWrapper;
            TTypeQualifierBuilder *typeQualifierBuilder;
        };
    } interm;


};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



int yyparse (TParseContext* context, void *scanner);

#endif /* !YY_YY_GLSLANG_TAB_H_INCLUDED  */
