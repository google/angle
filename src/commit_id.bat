@echo off
echo|set /p=#define COMMIT_HASH > %~dp0\common\commit.h
(git rev-parse --short=12 HEAD >> %~dp0\common\commit.h) || (echo badf00dbad00 >> %~dp0\common\commit.h) > NUL