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
     MATRIX2 = 289,
     MATRIX3 = 290,
     MATRIX4 = 291,
     IN_QUAL = 292,
     OUT_QUAL = 293,
     INOUT_QUAL = 294,
     UNIFORM = 295,
     VARYING = 296,
     MATRIX2x3 = 297,
     MATRIX3x2 = 298,
     MATRIX2x4 = 299,
     MATRIX4x2 = 300,
     MATRIX3x4 = 301,
     MATRIX4x3 = 302,
     CENTROID = 303,
     FLAT = 304,
     SMOOTH = 305,
     STRUCT = 306,
     VOID_TYPE = 307,
     WHILE = 308,
     SAMPLER2D = 309,
     SAMPLERCUBE = 310,
     SAMPLER_EXTERNAL_OES = 311,
     SAMPLER2DRECT = 312,
     SAMPLER3D = 313,
     SAMPLER3DRECT = 314,
     SAMPLER2DSHADOW = 315,
     IDENTIFIER = 316,
     TYPE_NAME = 317,
     FLOATCONSTANT = 318,
     INTCONSTANT = 319,
     BOOLCONSTANT = 320,
     FIELD_SELECTION = 321,
     LEFT_OP = 322,
     RIGHT_OP = 323,
     INC_OP = 324,
     DEC_OP = 325,
     LE_OP = 326,
     GE_OP = 327,
     EQ_OP = 328,
     NE_OP = 329,
     AND_OP = 330,
     OR_OP = 331,
     XOR_OP = 332,
     MUL_ASSIGN = 333,
     DIV_ASSIGN = 334,
     ADD_ASSIGN = 335,
     MOD_ASSIGN = 336,
     LEFT_ASSIGN = 337,
     RIGHT_ASSIGN = 338,
     AND_ASSIGN = 339,
     XOR_ASSIGN = 340,
     OR_ASSIGN = 341,
     SUB_ASSIGN = 342,
     LEFT_PAREN = 343,
     RIGHT_PAREN = 344,
     LEFT_BRACKET = 345,
     RIGHT_BRACKET = 346,
     LEFT_BRACE = 347,
     RIGHT_BRACE = 348,
     DOT = 349,
     COMMA = 350,
     COLON = 351,
     EQUAL = 352,
     SEMICOLON = 353,
     BANG = 354,
     DASH = 355,
     TILDE = 356,
     PLUS = 357,
     STAR = 358,
     SLASH = 359,
     PERCENT = 360,
     LEFT_ANGLE = 361,
     RIGHT_ANGLE = 362,
     VERTICAL_BAR = 363,
     CARET = 364,
     AMPERSAND = 365,
     QUESTION = 366
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
