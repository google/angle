@ECHO OFF
REM
REM Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
REM Use of this source code is governed by a BSD-style license that can be
REM found in the LICENSE file.
REM

PATH %PATH%;%ProgramFiles(x86)%\Windows Kits\8.0\bin\x86;%DXSDK_DIR%\Utilities\bin\x86

fxc /E standardvs /T vs_2_0 /Fh compiled/standardvs.h Blit.vs
fxc /E flipyvs /T vs_2_0 /Fh compiled/flipyvs.h Blit.vs
fxc /E passthroughps /T ps_2_0 /Fh compiled/passthroughps.h Blit.ps
fxc /E luminanceps /T ps_2_0 /Fh compiled/luminanceps.h Blit.ps
fxc /E componentmaskps /T ps_2_0 /Fh compiled/componentmaskps.h Blit.ps

fxc /E VS_Passthrough2D /T vs_4_0 /Fh compiled/passthrough2d11vs.h Passthrough2D11.hlsl
fxc /E PS_PassthroughDepth2D /T ps_4_0 /Fh compiled/passthroughdepth2d11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughRGBA2D /T ps_4_0 /Fh compiled/passthroughrgba2d11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughRGBA2DUI /T ps_4_0 /Fh compiled/passthroughrgba2dui11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughRGBA2DI /T ps_4_0 /Fh compiled/passthroughrgba2di11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughRGB2D /T ps_4_0 /Fh compiled/passthroughrgb2d11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughRGB2DUI /T ps_4_0 /Fh compiled/passthroughrgb2dui11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughRGB2DI /T ps_4_0 /Fh compiled/passthroughrgb2di11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughRG2D /T ps_4_0 /Fh compiled/passthroughrg2d11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughRG2DUI /T ps_4_0 /Fh compiled/passthroughrg2dui11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughRG2DI /T ps_4_0 /Fh compiled/passthroughrg2di11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughR2D /T ps_4_0 /Fh compiled/passthroughr2d11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughR2DUI /T ps_4_0 /Fh compiled/passthroughr2dui11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughR2DI /T ps_4_0 /Fh compiled/passthroughr2di11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughLum2D /T ps_4_0 /Fh compiled/passthroughlum2d11ps.h Passthrough2D11.hlsl
fxc /E PS_PassthroughLumAlpha2D /T ps_4_0 /Fh compiled/passthroughlumalpha2d11ps.h Passthrough2D11.hlsl

fxc /E VS_Passthrough3D /T vs_4_0 /Fh compiled/passthrough3d11vs.h Passthrough3D11.hlsl
fxc /E GS_Passthrough3D /T gs_4_0 /Fh compiled/passthrough3d11gs.h Passthrough3D11.hlsl
fxc /E PS_PassthroughRGBA3D /T ps_4_0 /Fh compiled/passthroughrgba3d11ps.h Passthrough3D11.hlsl
fxc /E PS_PassthroughRGBA3DUI /T ps_4_0 /Fh compiled/passthroughrgba3dui11ps.h Passthrough3D11.hlsl
fxc /E PS_PassthroughRGBA3DI /T ps_4_0 /Fh compiled/passthroughrgba3di11ps.h Passthrough3D11.hlsl
fxc /E PS_PassthroughRGB3D /T ps_4_0 /Fh compiled/passthroughrgb3d11ps.h Passthrough3D11.hlsl
fxc /E PS_PassthroughRGB3DUI /T ps_4_0 /Fh compiled/passthroughrgb3dui11ps.h Passthrough3D11.hlsl
fxc /E PS_PassthroughRGB3DI /T ps_4_0 /Fh compiled/passthroughrgb3di11ps.h Passthrough3D11.hlsl
fxc /E PS_PassthroughRG3D /T ps_4_0 /Fh compiled/passthroughrg3d11ps.h Passthrough3D11.hlsl
fxc /E PS_PassthroughRG3DUI /T ps_4_0 /Fh compiled/passthroughrg3dui11ps.h Passthrough3D11.hlsl
fxc /E PS_PassthroughRG3DI /T ps_4_0 /Fh compiled/passthroughrg3di11ps.h Passthrough3D11.hlsl
fxc /E PS_PassthroughR3D /T ps_4_0 /Fh compiled/passthroughr3d11ps.h Passthrough3D11.hlsl
fxc /E PS_PassthroughR3DUI /T ps_4_0 /Fh compiled/passthroughr3dui11ps.h Passthrough3D11.hlsl
fxc /E PS_PassthroughR3DI /T ps_4_0 /Fh compiled/passthroughr3di11ps.h Passthrough3D11.hlsl
fxc /E PS_PassthroughLum3D /T ps_4_0 /Fh compiled/passthroughlum3d11ps.h Passthrough3D11.hlsl
fxc /E PS_PassthroughLumAlpha3D /T ps_4_0 /Fh compiled/passthroughlumalpha3d11ps.h Passthrough3D11.hlsl

fxc /E VS_ClearFloat /T vs_4_0 /Fh compiled/clearfloat11vs.h Clear11.hlsl
fxc /E PS_ClearFloat /T ps_4_0 /Fh compiled/clearfloat11ps.h Clear11.hlsl

fxc /E VS_ClearUint /T vs_4_0 /Fh compiled/clearuint11vs.h Clear11.hlsl
fxc /E PS_ClearUint /T ps_4_0 /Fh compiled/clearuint11ps.h Clear11.hlsl

fxc /E VS_ClearSint /T vs_4_0 /Fh compiled/clearsint11vs.h Clear11.hlsl
fxc /E PS_ClearSint /T ps_4_0 /Fh compiled/clearsint11ps.h Clear11.hlsl
