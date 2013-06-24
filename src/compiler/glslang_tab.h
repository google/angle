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
     ISAMPLER2D = 316,
     ISAMPLER3D = 317,
     ISAMPLERCUBE = 318,
     USAMPLER2D = 319,
     USAMPLER3D = 320,
     USAMPLERCUBE = 321,
     SAMPLER3D = 322,
     SAMPLER3DRECT = 323,
     SAMPLER2DSHADOW = 324,
     LAYOUT = 325,
     IDENTIFIER = 326,
     TYPE_NAME = 327,
     FLOATCONSTANT = 328,
     INTCONSTANT = 329,
     UINTCONSTANT = 330,
     BOOLCONSTANT = 331,
     FIELD_SELECTION = 332,
     LEFT_OP = 333,
     RIGHT_OP = 334,
     INC_OP = 335,
     DEC_OP = 336,
     LE_OP = 337,
     GE_OP = 338,
     EQ_OP = 339,
     NE_OP = 340,
     AND_OP = 341,
     OR_OP = 342,
     XOR_OP = 343,
     MUL_ASSIGN = 344,
     DIV_ASSIGN = 345,
     ADD_ASSIGN = 346,
     MOD_ASSIGN = 347,
     LEFT_ASSIGN = 348,
     RIGHT_ASSIGN = 349,
     AND_ASSIGN = 350,
     XOR_ASSIGN = 351,
     OR_ASSIGN = 352,
     SUB_ASSIGN = 353,
     LEFT_PAREN = 354,
     RIGHT_PAREN = 355,
     LEFT_BRACKET = 356,
     RIGHT_BRACKET = 357,
     LEFT_BRACE = 358,
     RIGHT_BRACE = 359,
     DOT = 360,
     COMMA = 361,
     COLON = 362,
     EQUAL = 363,
     SEMICOLON = 364,
     BANG = 365,
     DASH = 366,
     TILDE = 367,
     PLUS = 368,
     STAR = 369,
     SLASH = 370,
     PERCENT = 371,
     LEFT_ANGLE = 372,
     RIGHT_ANGLE = 373,
     VERTICAL_BAR = 374,
     CARET = 375,
     AMPERSAND = 376,
     QUESTION = 377
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{


    struct {
        TSourceLoc line;
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
        TSourceLoc line;
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
