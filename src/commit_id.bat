@echo off

REM commit hash
(FOR /F "delims=" %%i IN ('call git rev-parse --short^=12 HEAD') DO set _Str=%%i) || (set _Str=badf00dbad00)
set _Str=#define COMMIT_HASH %_Str%
echo %_Str% > %~dp0\common\commit.h

REM commit date
(FOR /F "delims=" %%i IN ('git show -s --format^="%%ci" HEAD') DO set _Str=%%i) || (set _Str=Unknown Date)
set _Str=#define COMMIT_DATE "%_Str%"
echo %_Str% >> %~dp0\common\commit.h
