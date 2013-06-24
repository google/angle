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
     ISAMPLERCUBE = 317,
     USAMPLER2D = 318,
     USAMPLERCUBE = 319,
     SAMPLER3D = 320,
     SAMPLER3DRECT = 321,
     SAMPLER2DSHADOW = 322,
     LAYOUT = 323,
     IDENTIFIER = 324,
     TYPE_NAME = 325,
     FLOATCONSTANT = 326,
     INTCONSTANT = 327,
     UINTCONSTANT = 328,
     BOOLCONSTANT = 329,
     FIELD_SELECTION = 330,
     LEFT_OP = 331,
     RIGHT_OP = 332,
     INC_OP = 333,
     DEC_OP = 334,
     LE_OP = 335,
     GE_OP = 336,
     EQ_OP = 337,
     NE_OP = 338,
     AND_OP = 339,
     OR_OP = 340,
     XOR_OP = 341,
     MUL_ASSIGN = 342,
     DIV_ASSIGN = 343,
     ADD_ASSIGN = 344,
     MOD_ASSIGN = 345,
     LEFT_ASSIGN = 346,
     RIGHT_ASSIGN = 347,
     AND_ASSIGN = 348,
     XOR_ASSIGN = 349,
     OR_ASSIGN = 350,
     SUB_ASSIGN = 351,
     LEFT_PAREN = 352,
     RIGHT_PAREN = 353,
     LEFT_BRACKET = 354,
     RIGHT_BRACKET = 355,
     LEFT_BRACE = 356,
     RIGHT_BRACE = 357,
     DOT = 358,
     COMMA = 359,
     COLON = 360,
     EQUAL = 361,
     SEMICOLON = 362,
     BANG = 363,
     DASH = 364,
     TILDE = 365,
     PLUS = 366,
     STAR = 367,
     SLASH = 368,
     PERCENT = 369,
     LEFT_ANGLE = 370,
     RIGHT_ANGLE = 371,
     VERTICAL_BAR = 372,
     CARET = 373,
     AMPERSAND = 374,
     QUESTION = 375
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
