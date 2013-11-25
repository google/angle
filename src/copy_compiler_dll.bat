@echo off
set _arch=%1
set _arch=%_arch:Win32=x86%
set _arch=%_arch:"=%
set _sdkdir=%2
set _sdkdir=%_sdkdir:"=%
copy "%_sdkdir%\Redist\D3D\%_arch%\d3dcompiler_46.dll" %3
