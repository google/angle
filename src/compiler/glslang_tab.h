/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
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
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */


#define YYLTYPE TSourceLoc
#define YYLTYPE_IS_DECLARED 1




/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INVARIANT = 258,
     HIGH_PRECISION = 259,
     MEDIUM_PRECISION = 260,
     LOW_PRECISION = 261,
     PRECISION = 262,
     ATTRIBUTE = 263,
     CONST_QUAL = 264,
     BOOL_TYPE = 265,
     FLOAT_TYPE = 266,
     INT_TYPE = 267,
     UINT_TYPE = 268,
     BREAK = 269,
     CONTINUE = 270,
     DO = 271,
     ELSE = 272,
     FOR = 273,
     IF = 274,
     DISCARD = 275,
     RETURN = 276,
     SWITCH = 277,
     CASE = 278,
     DEFAULT = 279,
     BVEC2 = 280,
     BVEC3 = 281,
     BVEC4 = 282,
     IVEC2 = 283,
     IVEC3 = 284,
     IVEC4 = 285,
     VEC2 = 286,
     VEC3 = 287,
     VEC4 = 288,
     UVEC2 = 289,
     UVEC3 = 290,
     UVEC4 = 291,
     MATRIX2 = 292,
     MATRIX3 = 293,
     MATRIX4 = 294,
     IN_QUAL = 295,
     OUT_QUAL = 296,
     INOUT_QUAL = 297,
     UNIFORM = 298,
     VARYING = 299,
     MATRIX2x3 = 300,
     MATRIX3x2 = 301,
     MATRIX2x4 = 302,
     MATRIX4x2 = 303,
     MATRIX3x4 = 304,
     MATRIX4x3 = 305,
     CENTROID = 306,
     FLAT = 307,
     SMOOTH = 308,
     STRUCT = 309,
     VOID_TYPE = 310,
     WHILE = 311,
     SAMPLER2D = 312,
     SAMPLERCUBE = 313,
     SAMPLER_EXTERNAL_OES = 314,
     SAMPLER2DRECT = 315,
     SAMPLER2DARRAY = 316,
     ISAMPLER2D = 317,
     ISAMPLER3D = 318,
     ISAMPLERCUBE = 319,
     ISAMPLER2DARRAY = 320,
     USAMPLER2D = 321,
     USAMPLER3D = 322,
     USAMPLERCUBE = 323,
     USAMPLER2DARRAY = 324,
     SAMPLER3D = 325,
     SAMPLER3DRECT = 326,
     SAMPLER2DSHADOW = 327,
     LAYOUT = 328,
     IDENTIFIER = 329,
     TYPE_NAME = 330,
     FLOATCONSTANT = 331,
     INTCONSTANT = 332,
     UINTCONSTANT = 333,
     BOOLCONSTANT = 334,
     FIELD_SELECTION = 335,
     LEFT_OP = 336,
     RIGHT_OP = 337,
     INC_OP = 338,
     DEC_OP = 339,
     LE_OP = 340,
     GE_OP = 341,
     EQ_OP = 342,
     NE_OP = 343,
     AND_OP = 344,
     OR_OP = 345,
     XOR_OP = 346,
     MUL_ASSIGN = 347,
     DIV_ASSIGN = 348,
     ADD_ASSIGN = 349,
     MOD_ASSIGN = 350,
     LEFT_ASSIGN = 351,
     RIGHT_ASSIGN = 352,
     AND_ASSIGN = 353,
     XOR_ASSIGN = 354,
     OR_ASSIGN = 355,
     SUB_ASSIGN = 356,
     LEFT_PAREN = 357,
     RIGHT_PAREN = 358,
     LEFT_BRACKET = 359,
     RIGHT_BRACKET = 360,
     LEFT_BRACE = 361,
     RIGHT_BRACE = 362,
     DOT = 363,
     COMMA = 364,
     COLON = 365,
     EQUAL = 366,
     SEMICOLON = 367,
     BANG = 368,
     DASH = 369,
     TILDE = 370,
     PLUS = 371,
     STAR = 372,
     SLASH = 373,
     PERCENT = 374,
     LEFT_ANGLE = 375,
     RIGHT_ANGLE = 376,
     VERTICAL_BAR = 377,
     CARET = 378,
     AMPERSAND = 379,
     QUESTION = 380
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
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
        };
        union {
            TPublicType type;
            TPrecision precision;
            TLayoutQualifier layoutQualifier;
            TQualifier qualifier;
            TFunction* function;
            TParameter param;
            TTypeLine typeLine;
            TTypeList* typeList;
        };
    } interm;



} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (TParseContext* context);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_GLSLANG_TAB_H_INCLUDED  */
