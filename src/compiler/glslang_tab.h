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
     BREAK = 268,
     CONTINUE = 269,
     DO = 270,
     ELSE = 271,
     FOR = 272,
     IF = 273,
     DISCARD = 274,
     RETURN = 275,
     SWITCH = 276,
     CASE = 277,
     DEFAULT = 278,
     BVEC2 = 279,
     BVEC3 = 280,
     BVEC4 = 281,
     IVEC2 = 282,
     IVEC3 = 283,
     IVEC4 = 284,
     VEC2 = 285,
     VEC3 = 286,
     VEC4 = 287,
     MATRIX2 = 288,
     MATRIX3 = 289,
     MATRIX4 = 290,
     IN_QUAL = 291,
     OUT_QUAL = 292,
     INOUT_QUAL = 293,
     UNIFORM = 294,
     VARYING = 295,
     CENTROID = 296,
     FLAT = 297,
     SMOOTH = 298,
     STRUCT = 299,
     VOID_TYPE = 300,
     WHILE = 301,
     SAMPLER2D = 302,
     SAMPLERCUBE = 303,
     SAMPLER_EXTERNAL_OES = 304,
     SAMPLER2DRECT = 305,
     SAMPLER3D = 306,
     SAMPLER3DRECT = 307,
     SAMPLER2DSHADOW = 308,
     IDENTIFIER = 309,
     TYPE_NAME = 310,
     FLOATCONSTANT = 311,
     INTCONSTANT = 312,
     BOOLCONSTANT = 313,
     FIELD_SELECTION = 314,
     LEFT_OP = 315,
     RIGHT_OP = 316,
     INC_OP = 317,
     DEC_OP = 318,
     LE_OP = 319,
     GE_OP = 320,
     EQ_OP = 321,
     NE_OP = 322,
     AND_OP = 323,
     OR_OP = 324,
     XOR_OP = 325,
     MUL_ASSIGN = 326,
     DIV_ASSIGN = 327,
     ADD_ASSIGN = 328,
     MOD_ASSIGN = 329,
     LEFT_ASSIGN = 330,
     RIGHT_ASSIGN = 331,
     AND_ASSIGN = 332,
     XOR_ASSIGN = 333,
     OR_ASSIGN = 334,
     SUB_ASSIGN = 335,
     LEFT_PAREN = 336,
     RIGHT_PAREN = 337,
     LEFT_BRACKET = 338,
     RIGHT_BRACKET = 339,
     LEFT_BRACE = 340,
     RIGHT_BRACE = 341,
     DOT = 342,
     COMMA = 343,
     COLON = 344,
     EQUAL = 345,
     SEMICOLON = 346,
     BANG = 347,
     DASH = 348,
     TILDE = 349,
     PLUS = 350,
     STAR = 351,
     SLASH = 352,
     PERCENT = 353,
     LEFT_ANGLE = 354,
     RIGHT_ANGLE = 355,
     VERTICAL_BAR = 356,
     CARET = 357,
     AMPERSAND = 358,
     QUESTION = 359
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
